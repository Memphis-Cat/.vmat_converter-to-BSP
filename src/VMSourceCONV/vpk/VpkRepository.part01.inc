std::vector<VpkVerificationReport> VpkRepository::VerifyAll(
    const VpkVerificationOptions& options) const {
    std::vector<VpkVerificationReport> reports;
    reports.reserve(archives_.size());
    for (const auto& archive : archives_) {
        reports.push_back(archive.Verify(options));
    }
    return reports;
}

std::vector<std::filesystem::path> VpkRepository::MountedPaths() const {
    std::vector<std::filesystem::path> paths;
    paths.reserve(archives_.size());
    for (const auto& archive : archives_) {
        paths.push_back(archive.Path());
    }
    return paths;
}

const std::vector<VpkArchive>& VpkRepository::Archives() const noexcept {
    return archives_;
}

const std::vector<std::string>& VpkRepository::Warnings() const noexcept {
    return warnings_;
}

bool VpkRepository::AlreadyMounted(
    const std::filesystem::path& path) const {
    return std::any_of(
        archives_.begin(), archives_.end(),
        [&path](const VpkArchive& archive) {
            return archive.Path() == path;
        });
}

bool VpkRepository::TryMount(const std::filesystem::path& path) {
    const auto normalized = NormalizeAbsolute(path);
    if (AlreadyMounted(normalized)) {
        return true;
    }
    if (!HasVpkMagic(normalized)) {
        return false;
    }
    try {
        archives_.emplace_back(normalized);
        return true;
    } catch (const std::exception& exception) {
        warnings_.push_back(exception.what());
        return false;
    }
}

} // namespace vmsourceconv::vpk
