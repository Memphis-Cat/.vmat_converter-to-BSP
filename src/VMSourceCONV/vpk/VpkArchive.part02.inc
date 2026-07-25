VpkVerificationReport VpkArchive::Verify(
    const VpkVerificationOptions& options) const {
    VpkVerificationReport report;
    report.directoryFile = directoryFile_;
    report.version = version_;
    report.directoryEntries = entries_.size();
    report.duplicatePaths = duplicatePaths_.size();

    for (const auto& path : duplicatePaths_) {
        AddIssue(
            report,
            VpkVerificationSeverity::Error,
            "DUPLICATE_PATH",
            "duplicate internal path: " + path);
    }

    std::set<std::uint16_t> referencedChunks;
    std::set<std::filesystem::path> countedFiles;
    countedFiles.insert(directoryFile_);
    report.totalPackageBytes = GetFileSize(directoryFile_);

    for (const auto& item : entries_) {
        const auto& entry = item.second;
        const auto totalLength = CheckedAdd(
            static_cast<std::uint64_t>(entry.preloadBytes.size()),
            entry.length,
            "VPK entry size overflow");
        if (totalLength > options.maximumEntrySize) {
            ++report.outOfRangeEntries;
            AddIssue(
                report,
                VpkVerificationSeverity::Error,
                "ENTRY_SIZE_LIMIT",
                entry.path + " exceeds the configured entry-size limit");
            continue;
        }

        try {
            std::filesystem::path sourcePath;
            std::uint64_t baseOffset = 0U;
            std::uint64_t dataLimit = 0U;
            if (entry.archiveIndex == DirectoryArchiveIndex) {
                ++report.embeddedEntries;
                sourcePath = directoryFile_;
                const auto sourceSize = GetFileSize(sourcePath);
                baseOffset = EmbeddedDataOffset();
                dataLimit = EmbeddedDataLimit(sourceSize);
            } else {
                ++report.splitEntries;
                referencedChunks.insert(entry.archiveIndex);
                sourcePath = ChunkPath(entry.archiveIndex);
                std::error_code existsError;
                if (!std::filesystem::is_regular_file(sourcePath, existsError)
                    || existsError) {
                    continue;
                }
                dataLimit = GetFileSize(sourcePath);
                if (countedFiles.insert(sourcePath).second) {
                    report.totalPackageBytes = CheckedAdd(
                        report.totalPackageBytes,
                        dataLimit,
                        "VPK package size overflow");
                }
            }

            const auto absoluteOffset = CheckedAdd(
                baseOffset, entry.offset, "VPK entry offset overflow");
            const auto endOffset = CheckedAdd(
                absoluteOffset, entry.length, "VPK entry range overflow");
            if (absoluteOffset > dataLimit || endOffset > dataLimit) {
                ++report.outOfRangeEntries;
                AddIssue(
                    report,
                    VpkVerificationSeverity::Error,
                    "ENTRY_OUT_OF_RANGE",
                    entry.path + " is outside " + sourcePath.string());
                continue;
            }

            if (options.verifyCrc) {
                ++report.crcChecked;
                try {
                    (void)Read(entry, options);
                    ++report.crcPassed;
                } catch (const std::exception& error) {
                    ++report.crcFailed;
                    AddIssue(
                        report,
                        VpkVerificationSeverity::Error,
                        "ENTRY_CRC_FAILED",
                        error.what());
                }
            }
        } catch (const std::exception& error) {
            ++report.outOfRangeEntries;
            AddIssue(
                report,
                VpkVerificationSeverity::Error,
                "ENTRY_VERIFY_FAILED",
                entry.path + ": " + error.what());
        }
    }

    report.referencedChunks = referencedChunks.size();
    for (const auto archiveIndex : referencedChunks) {
        const auto path = ChunkPath(archiveIndex);
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error) && !error) {
            ++report.chunksPresent;
        } else {
            ++report.missingChunks;
            AddIssue(
                report,
                VpkVerificationSeverity::Error,
                "MISSING_CHUNK",
                "referenced archive chunk is missing: " + path.string());
        }
    }

    if (!referencedChunks.empty()) {
        const auto first = *referencedChunks.begin();
        const auto last = *referencedChunks.rbegin();
        for (std::uint32_t index = first; index <= last; ++index) {
            if (referencedChunks.find(static_cast<std::uint16_t>(index))
                == referencedChunks.end()) {
                AddIssue(
                    report,
                    VpkVerificationSeverity::Info,
                    "CHUNK_NUMBERING_GAP",
                    "archive index " + std::to_string(index)
                        + " is unused between referenced chunks");
            }
        }
    }

    if (options.scanOrphanChunks) {
        const auto stem = PackageStem(directoryFile_);
        std::error_code iteratorError;
        std::filesystem::directory_iterator iterator(
            directoryFile_.parent_path(),
            std::filesystem::directory_options::skip_permission_denied,
            iteratorError);
        const std::filesystem::directory_iterator end;
        while (iterator != end) {
            if (iteratorError) {
                iteratorError.clear();
                iterator.increment(iteratorError);
                continue;
            }
            std::error_code entryError;
            if (iterator->is_regular_file(entryError) && !entryError) {
                const auto index = ParseChunkIndex(iterator->path(), stem);
                if (index.has_value()
                    && referencedChunks.find(*index) == referencedChunks.end()) {
                    ++report.orphanChunks;
                    AddIssue(
                        report,
                        VpkVerificationSeverity::Warning,
                        "ORPHAN_CHUNK",
                        "unreferenced package chunk: "
                            + iterator->path().string());
                    const auto orphanPath = std::filesystem::absolute(
                        iterator->path()).lexically_normal();
                    if (countedFiles.insert(orphanPath).second) {
                        report.totalPackageBytes = CheckedAdd(
                            report.totalPackageBytes,
                            GetFileSize(orphanPath),
                            "VPK package size overflow");
                    }
                }
            }
            iterator.increment(iteratorError);
        }
    }

    if (version_ == 2U && sections_.archiveMd5Size != 0U) {
        const auto sectionOffset = CheckedAdd(
            EmbeddedDataOffset(),
            sections_.fileDataSize,
            "VPK archive-MD5 offset overflow");
        std::ifstream stream(directoryFile_, std::ios::binary);
        stream.seekg(static_cast<std::streamoff>(sectionOffset));
        const auto recordCount =
            sections_.archiveMd5Size / ArchiveMd5RecordSize;
        for (std::uint32_t record = 0U; record < recordCount; ++record) {
            const auto archiveIndex32 = ReadU32(stream);
            const auto offset = ReadU32(stream);
            const auto length = ReadU32(stream);
            std::array<std::uint8_t, 16> digest{};
            ReadExact(stream, digest.data(), digest.size());

            if (archiveIndex32 > std::numeric_limits<std::uint16_t>::max()
                && archiveIndex32 != DirectoryArchiveIndex) {
                AddIssue(
                    report,
                    VpkVerificationSeverity::Error,
                    "MD5_ARCHIVE_INDEX_INVALID",
                    "archive-MD5 record contains an invalid archive index");
                continue;
            }
            const auto sourcePath = archiveIndex32 == DirectoryArchiveIndex
                ? directoryFile_
                : ChunkPath(static_cast<std::uint16_t>(archiveIndex32));
            std::error_code error;
            if (!std::filesystem::is_regular_file(sourcePath, error) || error) {
                AddIssue(
                    report,
                    VpkVerificationSeverity::Error,
                    "MD5_SOURCE_MISSING",
                    "archive-MD5 record references missing data: "
                        + sourcePath.string());
                continue;
            }
            const auto sourceSize = GetFileSize(sourcePath);
            const auto endOffset = CheckedAdd(
                offset, length, "VPK archive-MD5 range overflow");
            if (endOffset > sourceSize) {
                AddIssue(
                    report,
                    VpkVerificationSeverity::Error,
                    "MD5_RANGE_INVALID",
                    "archive-MD5 record is outside " + sourcePath.string());
            }
        }
    }

    return report;
}

std::filesystem::path VpkArchive::ChunkPath(
    const std::uint16_t archiveIndex) const {
    std::ostringstream filename;
    filename << PackageStem(directoryFile_) << '_'
             << std::setw(3) << std::setfill('0') << archiveIndex
             << ".vpk";
    return directoryFile_.parent_path() / filename.str();
}

std::uint64_t VpkArchive::EmbeddedDataOffset() const noexcept {
    return static_cast<std::uint64_t>(headerSize_) + treeSize_;
}

std::uint64_t VpkArchive::EmbeddedDataLimit(
    const std::uint64_t fileSize) const noexcept {
    return version_ == 2U
        ? EmbeddedDataOffset() + sections_.fileDataSize
        : fileSize;
}

std::string VpkArchive::NormalizePath(const std::string_view path) {
    std::string normalized;
    normalized.reserve(path.size());
    bool leading = true;
    bool previousSeparator = false;
    for (const char rawCharacter : path) {
        const auto character = static_cast<unsigned char>(rawCharacter);
        const auto separator = character == static_cast<unsigned char>('\\')
            || character == static_cast<unsigned char>('/');
        if (separator) {
            if (leading || previousSeparator) {
                continue;
            }
            normalized.push_back('/');
            previousSeparator = true;
            continue;
        }
        leading = false;
        previousSeparator = false;
        normalized.push_back(static_cast<char>(std::tolower(character)));
    }
    while (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized;
}

} // namespace vmsourceconv::vpk
