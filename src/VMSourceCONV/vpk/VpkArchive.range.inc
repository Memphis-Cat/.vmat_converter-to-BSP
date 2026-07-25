io::FileData VpkArchive::ReadRange(
    const VpkEntry& entry,
    const std::uint64_t offset,
    const std::uint64_t length,
    const VpkReadOptions& options) const {
    const auto totalLength = CheckedAdd(
        static_cast<std::uint64_t>(entry.preloadBytes.size()),
        entry.length,
        "VPK entry size overflow");
    if (offset > totalLength || length > totalLength - offset) {
        throw std::runtime_error(
            "requested VPK range is outside the entry: " + entry.path);
    }
    if (length > options.maximumEntrySize
        || length > static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error(
            "requested VPK range exceeds the configured size limit: "
            + entry.path);
    }

    if (options.verifyCrc) {
        auto complete = Read(entry, options);
        io::FileData result;
        result.path = complete.path;
        const auto begin = complete.bytes.begin()
            + static_cast<std::ptrdiff_t>(offset);
        result.bytes.assign(
            begin,
            begin + static_cast<std::ptrdiff_t>(length));
        return result;
    }

    io::FileData result;
    result.path = std::filesystem::path(
        directoryFile_.string() + "::" + entry.path);
    result.bytes.resize(static_cast<std::size_t>(length));
    if (length == 0U) {
        return result;
    }

    const auto preloadSize =
        static_cast<std::uint64_t>(entry.preloadBytes.size());
    std::uint64_t written = 0;
    if (offset < preloadSize) {
        const auto preloadLength = std::min(length, preloadSize - offset);
        std::copy_n(
            entry.preloadBytes.begin() + static_cast<std::ptrdiff_t>(offset),
            static_cast<std::size_t>(preloadLength),
            result.bytes.begin());
        written = preloadLength;
    }

    if (written < length) {
        const auto logicalDataOffset = offset + written - preloadSize;
        const auto sourcePath = entry.archiveIndex == DirectoryArchiveIndex
            ? directoryFile_
            : ChunkPath(entry.archiveIndex);
        const auto sourceSize = GetFileSize(sourcePath);
        const auto baseOffset = entry.archiveIndex == DirectoryArchiveIndex
            ? EmbeddedDataOffset()
            : 0U;
        auto absoluteOffset = CheckedAdd(
            baseOffset, entry.offset, "VPK range offset overflow");
        absoluteOffset = CheckedAdd(
            absoluteOffset, logicalDataOffset, "VPK range offset overflow");
        const auto dataLength = length - written;
        const auto endOffset = CheckedAdd(
            absoluteOffset, dataLength, "VPK range overflow");
        const auto dataLimit = entry.archiveIndex == DirectoryArchiveIndex
            ? EmbeddedDataLimit(sourceSize)
            : sourceSize;
        if (endOffset > dataLimit) {
            throw std::runtime_error(
                "requested VPK range is outside its data section: "
                + entry.path);
        }

        std::ifstream stream(sourcePath, std::ios::binary);
        if (!stream) {
            throw std::runtime_error(
                "unable to open VPK data file: " + sourcePath.string());
        }
        stream.seekg(
            static_cast<std::streamoff>(absoluteOffset), std::ios::beg);
        if (!stream) {
            throw std::runtime_error("unable to seek VPK range");
        }
        ReadExact(
            stream,
            result.bytes.data() + static_cast<std::size_t>(written),
            static_cast<std::size_t>(dataLength));
    }

    return result;
}
