io::FileData VpkArchive::Read(
    const VpkEntry& entry,
    const VpkReadOptions& options) const {
    io::FileData data;
    data.path = std::filesystem::path(
        directoryFile_.string() + "::" + entry.path);

    const auto totalLength = CheckedAdd(
        static_cast<std::uint64_t>(entry.preloadBytes.size()),
        static_cast<std::uint64_t>(entry.length),
        "VPK entry size overflow");
    if (totalLength > options.maximumEntrySize) {
        throw std::runtime_error(
            "VPK entry exceeds the configured size limit: " + entry.path);
    }
    if (totalLength > static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("VPK entry is too large");
    }

    data.bytes.resize(static_cast<std::size_t>(totalLength));
    std::copy(
        entry.preloadBytes.begin(),
        entry.preloadBytes.end(),
        data.bytes.begin());

    if (entry.length != 0U) {
        const auto sourcePath = entry.archiveIndex == DirectoryArchiveIndex
            ? directoryFile_
            : ChunkPath(entry.archiveIndex);
        const auto sourceSize = GetFileSize(sourcePath);
        const auto baseOffset = entry.archiveIndex == DirectoryArchiveIndex
            ? EmbeddedDataOffset()
            : 0U;
        const auto absoluteOffset = CheckedAdd(
            baseOffset, entry.offset, "VPK entry offset overflow");
        const auto endOffset = CheckedAdd(
            absoluteOffset, entry.length, "VPK entry range overflow");
        const auto dataLimit = entry.archiveIndex == DirectoryArchiveIndex
            ? EmbeddedDataLimit(sourceSize)
            : sourceSize;
        if (absoluteOffset > dataLimit || endOffset > dataLimit) {
            throw std::runtime_error(
                "VPK entry range is outside its data section: " + entry.path);
        }

        std::ifstream stream(sourcePath, std::ios::binary);
        if (!stream) {
            throw std::runtime_error(
                "unable to open VPK data file: " + sourcePath.string());
        }
        stream.seekg(
            static_cast<std::streamoff>(absoluteOffset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("unable to seek VPK entry data");
        }
        ReadExact(
            stream,
            data.bytes.data() + entry.preloadBytes.size(),
            entry.length);
    }

    if (options.verifyCrc) {
        const auto calculated = ComputeCrc32(data.bytes);
        if (calculated != entry.crc32) {
            throw std::runtime_error(
                "VPK CRC32 mismatch for " + entry.path
                + ": expected " + Hex32(entry.crc32)
                + ", calculated " + Hex32(calculated));
        }
    }
    return data;
}

void VpkArchive::Parse() {
    std::ifstream stream(directoryFile_, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error(
            "unable to open VPK: " + directoryFile_.string());
    }
    const auto fileSizePosition = stream.tellg();
    if (fileSizePosition == std::streampos(-1)
        || fileSizePosition < std::streampos(12)) {
        throw std::runtime_error(
            "file is too small to be a VPK: " + directoryFile_.string());
    }
    const auto fileSize = static_cast<std::uint64_t>(fileSizePosition);
    stream.seekg(0, std::ios::beg);

    if (ReadU32(stream) != VpkMagic) {
        throw std::runtime_error(
            "file is not a Valve VPK: " + directoryFile_.string());
    }
    version_ = ReadU32(stream);
    treeSize_ = ReadU32(stream);

    if (version_ == 1U) {
        headerSize_ = 12U;
    } else if (version_ == 2U) {
        sections_.fileDataSize = ReadU32(stream);
        sections_.archiveMd5Size = ReadU32(stream);
        sections_.otherMd5Size = ReadU32(stream);
        sections_.signatureSize = ReadU32(stream);
        headerSize_ = 28U;
        ValidateV2Sections(fileSize);
    } else {
        throw std::runtime_error(
            "unsupported VPK version " + std::to_string(version_)
            + ": " + directoryFile_.string());
    }

    const auto treeEndValue = CheckedAdd(
        headerSize_, treeSize_, "VPK directory tree overflow");
    if (treeEndValue > fileSize
        || treeEndValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::streamoff>::max())) {
        throw std::runtime_error("VPK directory tree is outside the file");
    }
    const auto treeEnd = std::streampos(
        static_cast<std::streamoff>(treeEndValue));

    while (true) {
        const auto extension = ReadCString(stream, treeEnd);
        if (extension.empty()) {
            break;
        }
        while (true) {
            const auto directory = ReadCString(stream, treeEnd);
            if (directory.empty()) {
                break;
            }
            while (true) {
                const auto filename = ReadCString(stream, treeEnd);
                if (filename.empty()) {
                    break;
                }

                VpkEntry entry;
                entry.crc32 = ReadU32(stream);
                const auto preloadSize = ReadU16(stream);
                entry.archiveIndex = ReadU16(stream);
                entry.offset = ReadU32(stream);
                entry.length = ReadU32(stream);
                if (ReadU16(stream) != 0xFFFFU) {
                    throw std::runtime_error("invalid VPK entry terminator");
                }
                entry.preloadBytes.resize(preloadSize);
                ReadExact(
                    stream,
                    entry.preloadBytes.data(),
                    entry.preloadBytes.size());

                std::string fullPath;
                if (directory != " ") {
                    fullPath = directory;
                    fullPath.push_back('/');
                }
                fullPath += filename;
                if (extension != " ") {
                    fullPath.push_back('.');
                    fullPath += extension;
                }
                entry.path = NormalizePath(fullPath);
                const auto inserted = entries_.emplace(entry.path, entry);
                if (!inserted.second) {
                    duplicatePaths_.push_back(entry.path);
                }
            }
        }
    }

    const auto position = stream.tellg();
    if (position == std::streampos(-1) || position > treeEnd) {
        throw std::runtime_error("VPK tree parser exceeded the declared tree size");
    }
    while (stream.tellg() < treeEnd) {
        char padding = 0;
        stream.get(padding);
        if (!stream || padding != '\0') {
            throw std::runtime_error(
                "VPK directory tree contains non-zero trailing data");
        }
    }
}

void VpkArchive::ValidateV2Sections(const std::uint64_t fileSize) const {
    auto declaredEnd = CheckedAdd(
        headerSize_, treeSize_, "VPK section overflow");
    declaredEnd = CheckedAdd(
        declaredEnd, sections_.fileDataSize, "VPK section overflow");
    declaredEnd = CheckedAdd(
        declaredEnd, sections_.archiveMd5Size, "VPK section overflow");
    declaredEnd = CheckedAdd(
        declaredEnd, sections_.otherMd5Size, "VPK section overflow");
    declaredEnd = CheckedAdd(
        declaredEnd, sections_.signatureSize, "VPK section overflow");
    if (declaredEnd > fileSize) {
        throw std::runtime_error("VPK v2 sections extend beyond the file");
    }
    if (sections_.archiveMd5Size % ArchiveMd5RecordSize != 0U) {
        throw std::runtime_error(
            "VPK archive-MD5 section is not a whole number of records");
    }
    if (sections_.otherMd5Size != 0U && sections_.otherMd5Size != 48U) {
        throw std::runtime_error(
            "VPK other-MD5 section must be empty or 48 bytes");
    }

    if (sections_.signatureSize == 0U) {
        return;
    }
    if (sections_.signatureSize < 4U) {
        throw std::runtime_error("VPK signature section is too small");
    }

    const auto signatureOffset = declaredEnd - sections_.signatureSize;
    if (signatureOffset > static_cast<std::uint64_t>(
            std::numeric_limits<std::streamoff>::max())) {
        throw std::runtime_error("VPK signature offset is too large");
    }

    std::ifstream stream(directoryFile_, std::ios::binary);
    if (!stream) {
        throw std::runtime_error(
            "unable to open VPK signature section: " + directoryFile_.string());
    }
    stream.seekg(static_cast<std::streamoff>(signatureOffset));
    if (!stream) {
        throw std::runtime_error("unable to seek VPK signature section");
    }

    const auto firstValue = ReadU32(stream);

    // Newer Source 2 packages use a fixed 20-byte descriptor. The descriptor
    // stores the VPK magic, signature type, key length, signature length, and a
    // reserved field. The key and signature payloads immediately follow the
    // declared descriptor and are therefore outside SignatureSectionSize.
    if (sections_.signatureSize == 20U && firstValue == VpkMagic) {
        (void)ReadU32(stream); // signature type
        const auto publicKeySize = ReadU32(stream);
        const auto signatureSize = ReadU32(stream);
        (void)ReadU32(stream); // reserved

        auto payloadEnd = CheckedAdd(
            declaredEnd, publicKeySize, "VPK signature public-key overflow");
        payloadEnd = CheckedAdd(
            payloadEnd, signatureSize, "VPK signature payload overflow");
        if (payloadEnd > fileSize) {
            throw std::runtime_error(
                "VPK Source 2 signature payload is out of range");
        }
        return;
    }

    // Legacy VPK v2 signature framing keeps both payloads inside the declared
    // signature section: [key length][key][signature length][signature].
    const auto publicKeySize = firstValue;
    const auto afterKey = CheckedAdd(
        4U, publicKeySize, "VPK signature overflow");
    if (afterKey > sections_.signatureSize
        || sections_.signatureSize - afterKey < 4U) {
        throw std::runtime_error("VPK signature public key is out of range");
    }
    stream.seekg(static_cast<std::streamoff>(publicKeySize), std::ios::cur);
    if (!stream) {
        throw std::runtime_error("unable to seek past VPK signature public key");
    }
    const auto signatureSize = ReadU32(stream);
    const auto consumed = CheckedAdd(
        afterKey + 4U, signatureSize, "VPK signature overflow");
    if (consumed != sections_.signatureSize) {
        throw std::runtime_error(
            "VPK signature section sizes do not consume the section exactly");
    }
}
