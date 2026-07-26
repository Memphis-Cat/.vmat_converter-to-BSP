std::uint64_t VpkRepository::EntrySize(const VpkMatch& match) const {
    if (match.archiveIndex >= archives_.size()) {
        throw std::runtime_error("invalid VPK repository index");
    }
    const auto* entry = archives_[match.archiveIndex].Find(match.entryPath);
    if (entry == nullptr) {
        throw std::runtime_error("VPK entry disappeared: " + match.entryPath);
    }
    return static_cast<std::uint64_t>(entry->preloadBytes.size())
        + static_cast<std::uint64_t>(entry->length);
}
