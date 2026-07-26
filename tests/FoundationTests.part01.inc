struct VpkFixtureOptions {
    std::uint32_t version = 1U;
    bool embedded = true;
    std::size_t preloadBytes = 0U;
    bool writeChunk = true;
    bool wrongCrc = false;
    bool duplicatePath = false;
    bool source2SignatureDescriptor = false;
};

std::filesystem::path ChunkPathFor(
    const std::filesystem::path& directoryFile) {
    auto stem = directoryFile.stem().string();
    constexpr std::string_view suffix = "_dir";
    if (stem.size() >= suffix.size()
        && stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) == 0) {
        stem.erase(stem.size() - suffix.size());
    }
    return directoryFile.parent_path() / (stem + "_000.vpk");
}

void WriteSingleVpk(
    const std::filesystem::path& directoryFile,
    const std::string& entryPath,
    const std::vector<std::uint8_t>& data,
    const VpkFixtureOptions& options = {}) {
    CHECK(options.version == 1U || options.version == 2U);
    CHECK(options.preloadBytes <= data.size());
    CHECK(!options.source2SignatureDescriptor || options.version == 2U);

    const auto slash = entryPath.find_last_of('/');
    const auto dot = entryPath.find_last_of('.');
    CHECK(dot != std::string::npos);
    CHECK(slash == std::string::npos || dot > slash);
    const auto directory = slash == std::string::npos
        ? std::string(" ")
        : entryPath.substr(0, slash);
    const auto filenameBegin = slash == std::string::npos ? 0U : slash + 1U;
    const auto filename = entryPath.substr(filenameBegin, dot - filenameBegin);
    const auto extension = entryPath.substr(dot + 1U);

    const auto calculatedCrc = vmsourceconv::vpk::ComputeCrc32(data);
    const auto storedCrc = options.wrongCrc
        ? calculatedCrc ^ 0xFFFFFFFFU
        : calculatedCrc;
    const auto archiveLength = data.size() - options.preloadBytes;

    std::vector<std::uint8_t> tree;
    AppendCString(tree, extension.empty() ? " " : extension);
    AppendCString(tree, directory.empty() ? " " : directory);
    const auto appendRecord = [&]() {
        AppendCString(tree, filename);
        AppendU32(tree, storedCrc);
        AppendU16(tree, static_cast<std::uint16_t>(options.preloadBytes));
        AppendU16(tree, options.embedded ? 0x7FFFU : 0U);
        AppendU32(tree, 0U);
        AppendU32(tree, static_cast<std::uint32_t>(archiveLength));
        AppendU16(tree, 0xFFFFU);
        tree.insert(
            tree.end(),
            data.begin(),
            data.begin() + static_cast<std::ptrdiff_t>(options.preloadBytes));
    };
    appendRecord();
    if (options.duplicatePath) {
        appendRecord();
    }
    AppendCString(tree, "");
    AppendCString(tree, "");
    AppendCString(tree, "");

    std::vector<std::uint8_t> directoryBytes;
    AppendU32(directoryBytes, 0x55AA1234U);
    AppendU32(directoryBytes, options.version);
    AppendU32(directoryBytes, static_cast<std::uint32_t>(tree.size()));
    if (options.version == 2U) {
        AppendU32(
            directoryBytes,
            options.embedded ? static_cast<std::uint32_t>(archiveLength) : 0U);
        AppendU32(directoryBytes, 0U);
        AppendU32(directoryBytes, 0U);
        AppendU32(
            directoryBytes,
            options.source2SignatureDescriptor ? 20U : 0U);
    }
    directoryBytes.insert(directoryBytes.end(), tree.begin(), tree.end());
    if (options.embedded) {
        directoryBytes.insert(
            directoryBytes.end(),
            data.begin() + static_cast<std::ptrdiff_t>(options.preloadBytes),
            data.end());
    }
    if (options.source2SignatureDescriptor) {
        constexpr std::uint32_t PublicKeySize = 8U;
        constexpr std::uint32_t SignatureSize = 16U;
        AppendU32(directoryBytes, 0x55AA1234U);
        AppendU32(directoryBytes, 1U);
        AppendU32(directoryBytes, PublicKeySize);
        AppendU32(directoryBytes, SignatureSize);
        AppendU32(directoryBytes, 0U);
        directoryBytes.insert(directoryBytes.end(), PublicKeySize, 0xA5U);
        directoryBytes.insert(directoryBytes.end(), SignatureSize, 0x5AU);
    }
    WriteBytes(directoryFile, directoryBytes);

    if (!options.embedded && options.writeChunk) {
        std::vector<std::uint8_t> archiveData(
            data.begin() + static_cast<std::ptrdiff_t>(options.preloadBytes),
            data.end());
        WriteBytes(ChunkPathFor(directoryFile), archiveData);
    }
}

vmsourceconv::resource::ResourceDocument ParseResourceBytes(
    std::vector<std::uint8_t> bytes,
    std::vector<vmsourceconv::core::Diagnostic>& diagnostics) {
    vmsourceconv::io::FileData file;
    file.path = "synthetic.vdata_c";
    file.bytes = std::move(bytes);
    return vmsourceconv::resource::ResourceParser{}.Parse(
        std::move(file), diagnostics);
}

void TestResourceIdentity() {
    using vmsourceconv::discovery::ResourceIdentity;
    const auto identity = ResourceIdentity::TryCreate(
        ".\\Models\\Props//Crate.VMDL_C");
    CHECK(identity.has_value());
    CHECK(identity->logicalName == "models/props/crate.vmdl");
    CHECK(identity->key == "models/props/crate.vmdl_c");
    CHECK(identity->compiledRelativePath.generic_string()
        == "models/props/crate.vmdl_c");

    CHECK(!ResourceIdentity::TryCreate("../secret.vdata").has_value());
    CHECK(!ResourceIdentity::TryCreate("C:/game/file.vdata").has_value());
    CHECK(!ResourceIdentity::TryCreate("/rooted/file.vdata").has_value());
    CHECK(!ResourceIdentity::TryCreate("_c").has_value());
}

void TestResourceContainerValidation() {
    {
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        const auto document = ParseResourceBytes(
            BuildResource({{"DATA", {1U, 2U, 3U, 4U}}}), diagnostics);
        CHECK(document.blocks.size() == 1U);
        CHECK(!HasDiagnostic(diagnostics, "BLOCK_OUT_OF_RANGE"));
    }

    {
        bool threw = false;
        try {
            std::vector<vmsourceconv::core::Diagnostic> diagnostics;
            (void)ParseResourceBytes({0U, 1U, 2U}, diagnostics);
        } catch (const vmsourceconv::core::ParseError&) {
            threw = true;
        }
        CHECK(threw);
    }

    {
        auto bytes = BuildResource({
            {"DATA", {1U, 2U, 3U, 4U}},
            {"DATA", {5U, 6U, 7U, 8U}},
        });
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        (void)ParseResourceBytes(std::move(bytes), diagnostics);
        CHECK(HasDiagnostic(diagnostics, "DUPLICATE_SINGLETON_BLOCK"));
    }

    {
        auto bytes = BuildResource({
            {"DATA", {1U, 2U, 3U, 4U}},
            {"RED2", {5U, 6U, 7U, 8U}},
        });
        PatchU32(bytes, 20U, 4U);
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        (void)ParseResourceBytes(std::move(bytes), diagnostics);
        CHECK(HasDiagnostic(diagnostics, "BLOCK_OVERLAPS_DIRECTORY"));
    }

    {
        auto bytes = BuildResource({
            {"DATA", std::vector<std::uint8_t>(8U, 1U)},
            {"RED2", std::vector<std::uint8_t>(8U, 2U)},
        });
        PatchU32(bytes, 20U, 24U);
        PatchU32(bytes, 32U, 8U);
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        (void)ParseResourceBytes(std::move(bytes), diagnostics);
        CHECK(HasDiagnostic(diagnostics, "OVERLAPPING_BLOCKS"));
    }

    {
        auto bytes = BuildResource({{"DATA", {1U, 2U, 3U, 4U}}});
        bytes.insert(bytes.end(), {9U, 9U, 9U, 9U});
        PatchU32(bytes, 0U, static_cast<std::uint32_t>(bytes.size()));
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        (void)ParseResourceBytes(std::move(bytes), diagnostics);
        CHECK(HasDiagnostic(diagnostics, "TRAILING_RESOURCE_DATA"));
    }

    {
        auto bytes = BuildResource({{"DATA", {1U, 2U, 3U, 4U}}});
        PatchU32(bytes, 24U, 0xFFFFFFF0U);
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        (void)ParseResourceBytes(std::move(bytes), diagnostics);
        CHECK(HasDiagnostic(diagnostics, "BLOCK_OUT_OF_RANGE"));
    }

    {
        auto bytes = BuildResource({{"DATA", {1U, 2U, 3U, 4U}}});
        PatchU32(bytes, 8U, 0U);
        std::vector<vmsourceconv::core::Diagnostic> diagnostics;
        (void)ParseResourceBytes(std::move(bytes), diagnostics);
        CHECK(HasDiagnostic(diagnostics, "BLOCK_DIRECTORY_OVERLAPS_HEADER"));
    }
}

void TestFileReaderLimits() {
    TemporaryDirectory temporary;
    const auto file = temporary.Path() / "range.bin";
    WriteBytes(file, {0U, 1U, 2U, 3U, 4U, 5U});

    const auto range = vmsourceconv::io::FileReader::ReadRange(
        file, 2U, 3U, 3U);
    CHECK(range.bytes == std::vector<std::uint8_t>({2U, 3U, 4U}));

    bool threw = false;
    try {
        (void)vmsourceconv::io::FileReader::ReadAll(file, 5U);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}
