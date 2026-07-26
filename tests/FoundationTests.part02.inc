void TestVpkReadAndVerification() {
    TemporaryDirectory temporary;
    const auto payload = std::vector<std::uint8_t>{
        'a', 'b', 'c', 'd', 'e', 'f',
    };

    {
        const auto path = temporary.Path() / "embedded_dir.vpk";
        WriteSingleVpk(path, "maps/test/file.bin", payload);
        const vmsourceconv::vpk::VpkArchive archive(path);
        CHECK(archive.Version() == 1U);
        const auto* entry = archive.Find("MAPS\\TEST\\FILE.BIN");
        CHECK(entry != nullptr);
        CHECK(archive.Read(*entry).bytes == payload);
        const auto range = archive.ReadRange(
            *entry, 2U, 3U,
            vmsourceconv::vpk::VpkReadOptions{false, 1024U});
        CHECK(range.bytes == std::vector<std::uint8_t>({'c', 'd', 'e'}));
        const auto report = archive.Verify();
        CHECK(report.Success());
        CHECK(report.crcChecked == 1U);
        CHECK(report.crcPassed == 1U);
    }

    {
        const auto path = temporary.Path() / "split_dir.vpk";
        VpkFixtureOptions options;
        options.embedded = false;
        options.preloadBytes = 2U;
        WriteSingleVpk(path, "maps/test/split.bin", payload, options);
        const vmsourceconv::vpk::VpkArchive archive(path);
        const auto* entry = archive.Find("maps/test/split.bin");
        CHECK(entry != nullptr);
        CHECK(archive.Read(*entry).bytes == payload);
        const auto report = archive.Verify();
        CHECK(report.Success());
        CHECK(report.referencedChunks == 1U);
        CHECK(report.chunksPresent == 1U);
    }

    {
        const auto path = temporary.Path() / "version2_dir.vpk";
        VpkFixtureOptions options;
        options.version = 2U;
        WriteSingleVpk(path, "maps/test/v2.bin", payload, options);
        const vmsourceconv::vpk::VpkArchive archive(path);
        CHECK(archive.Version() == 2U);
        CHECK(archive.Sections().fileDataSize == payload.size());
        CHECK(archive.Verify().Success());
    }

    {
        const auto path = temporary.Path() / "preload_dir.vpk";
        VpkFixtureOptions options;
        options.preloadBytes = payload.size();
        WriteSingleVpk(path, "maps/test/preload.bin", payload, options);
        const vmsourceconv::vpk::VpkArchive archive(path);
        const auto* entry = archive.Find("maps/test/preload.bin");
        CHECK(entry != nullptr);
        CHECK(entry->length == 0U);
        CHECK(archive.Read(*entry).bytes == payload);
    }

    {
        const auto path = temporary.Path() / "bad-crc_dir.vpk";
        VpkFixtureOptions options;
        options.wrongCrc = true;
        WriteSingleVpk(path, "maps/test/bad.bin", payload, options);
        const vmsourceconv::vpk::VpkArchive archive(path);
        const auto* entry = archive.Find("maps/test/bad.bin");
        CHECK(entry != nullptr);
        bool threw = false;
        try {
            (void)archive.Read(*entry);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        CHECK(threw);
        const auto report = archive.Verify();
        CHECK(!report.Success());
        CHECK(report.crcFailed == 1U);
    }

    {
        const auto path = temporary.Path() / "missing_dir.vpk";
        VpkFixtureOptions options;
        options.embedded = false;
        options.writeChunk = false;
        WriteSingleVpk(path, "maps/test/missing.bin", payload, options);
        const vmsourceconv::vpk::VpkArchive archive(path);
        const auto report = archive.Verify();
        CHECK(!report.Success());
        CHECK(report.missingChunks == 1U);
    }

    {
        const auto path = temporary.Path() / "duplicate_dir.vpk";
        VpkFixtureOptions options;
        options.duplicatePath = true;
        WriteSingleVpk(path, "maps/test/duplicate.bin", payload, options);
        const vmsourceconv::vpk::VpkArchive archive(path);
        CHECK(archive.DuplicatePaths().size() == 1U);
        const auto report = archive.Verify();
        CHECK(!report.Success());
        CHECK(report.duplicatePaths == 1U);
    }

    {
        const auto path = temporary.Path() / "orphan_dir.vpk";
        WriteSingleVpk(path, "maps/test/orphan.bin", payload);
        WriteBytes(temporary.Path() / "orphan_000.vpk", {1U, 2U, 3U});
        const vmsourceconv::vpk::VpkArchive archive(path);
        const auto report = archive.Verify();
        CHECK(report.orphanChunks == 1U);
        CHECK(report.WarningCount() >= 1U);
    }
}

void TestResourceLocatorPriorityAndCollisions() {
    TemporaryDirectory temporary;
    const auto input = temporary.Path() / "test.vmap_c";
    WriteBytes(input, BuildResource({}));

    const auto internalPath = std::string("models/item.vmdl_c");
    const auto overrideRoot = temporary.Path() / "override";
    const auto normalRoot = temporary.Path() / "content";
    WriteBytes(overrideRoot / internalPath, {'o', 'v', 'r'});
    WriteBytes(normalRoot / internalPath, {'l', 'o', 'o', 's', 'e'});

    const auto explicitVpk = temporary.Path() / "explicit_dir.vpk";
    WriteSingleVpk(
        explicitVpk, internalPath,
        std::vector<std::uint8_t>{'e', 'x', 'p'});
    const auto mapVpk = temporary.Path() / "test.vpk";
    WriteSingleVpk(
        mapVpk, internalPath,
        std::vector<std::uint8_t>{'m', 'a', 'p'});
    const auto fallbackVpk = temporary.Path() / "vpk" / "pak01_dir.vpk";
    WriteSingleVpk(
        fallbackVpk, internalPath,
        std::vector<std::uint8_t>{'f', 'a', 'l', 'l'});

    {
        const vmsourceconv::discovery::ResourceLocator locator(
            input, {normalRoot}, {explicitVpk}, {overrideRoot});
        const auto location = locator.Locate("MODELS\\ITEM.VMDL");
        CHECK(location.Found());
        CHECK(location.tier
            == vmsourceconv::discovery::ResourceLookupTier::OverrideLoose);
        CHECK(locator.Read(location).bytes
            == std::vector<std::uint8_t>({'o', 'v', 'r'}));
        CHECK(location.shadowedVpks.size() == 3U);
    }

    std::filesystem::remove(overrideRoot / internalPath);
    {
        const vmsourceconv::discovery::ResourceLocator locator(
            input, {normalRoot}, {explicitVpk});
        const auto location = locator.Locate(internalPath);
        CHECK(location.tier
            == vmsourceconv::discovery::ResourceLookupTier::ExplicitVpk);
        CHECK(locator.Read(location).bytes
            == std::vector<std::uint8_t>({'e', 'x', 'p'}));
    }

    {
        const vmsourceconv::discovery::ResourceLocator locator(
            input, {normalRoot}, {});
        const auto location = locator.Locate(internalPath);
        CHECK(location.tier
            == vmsourceconv::discovery::ResourceLookupTier::MapVpk);
        CHECK(locator.Read(location).bytes
            == std::vector<std::uint8_t>({'m', 'a', 'p'}));
    }

    std::filesystem::remove(mapVpk);
    {
        const vmsourceconv::discovery::ResourceLocator locator(
            input, {normalRoot}, {});
        const auto location = locator.Locate(internalPath);
        CHECK(location.tier
            == vmsourceconv::discovery::ResourceLookupTier::LooseRoot);
        CHECK(locator.Read(location).bytes
            == std::vector<std::uint8_t>({'l', 'o', 'o', 's', 'e'}));
    }

    std::filesystem::remove(normalRoot / internalPath);
    {
        const vmsourceconv::discovery::ResourceLocator locator(
            input, {normalRoot}, {});
        const auto location = locator.Locate(internalPath);
        CHECK(location.tier
            == vmsourceconv::discovery::ResourceLookupTier::FallbackVpk);
        CHECK(locator.Read(location).bytes
            == std::vector<std::uint8_t>({'f', 'a', 'l', 'l'}));
    }
}

void WriteGraphFixtures(
    const std::filesystem::path& input,
    const std::filesystem::path& contentRoot) {
    WriteBytes(input, BuildResource({}));
    WriteBytes(
        contentRoot / "maps/a.vdata_c",
        BuildResourceWithRerl({{2U, "maps/b.vdata", 0U, 0U}}));
    WriteBytes(
        contentRoot / "maps/b.vdata_c",
        BuildResourceWithRerl({{1U, "maps/a.vdata", 0U, 0U}}));
}

void TestDependencyGraph() {
    TemporaryDirectory temporary;
    const auto input = temporary.Path() / "root.vmap_c";
    const auto contentRoot = temporary.Path() / "content";
    WriteGraphFixtures(input, contentRoot);

    const std::vector<vmsourceconv::inspect::ExternalReference> rootReferences{
        {1U, "maps/a.vdata", 0U, 0U},
        {1U, "MAPS\\A.VDATA_C", 0U, 0U},
        {3U, "root.vmap", 0U, 0U},
    };

    vmsourceconv::graph::ResourceGraphBuildOptions options;
    options.resourceRoots = {contentRoot};
    options.maximumDepth = 8U;
    options.maximumResources = 32U;
    std::vector<vmsourceconv::core::Diagnostic> diagnostics;
    const auto graph = vmsourceconv::graph::ResourceGraphBuilder{}.Build(
        input, rootReferences, options, diagnostics);

    CHECK(graph.nodes.size() == 3U);
    CHECK(graph.nodes[0].logicalName == "root.vmap");
    CHECK(graph.edges.size() == 5U);
    CHECK(graph.statistics.repeatedEdges == 1U);
    CHECK(graph.statistics.selfReferences == 1U);
    CHECK(graph.statistics.cycleEdges == 1U);
    CHECK(graph.statistics.resourcesLoaded == 2U);
    CHECK(graph.statistics.resourcesMissing == 0U);

    {
        auto limited = options;
        limited.maximumDepth = 1U;
        std::vector<vmsourceconv::core::Diagnostic> localDiagnostics;
        const auto depthGraph =
            vmsourceconv::graph::ResourceGraphBuilder{}.Build(
                input, rootReferences, limited, localDiagnostics);
        CHECK(depthGraph.statistics.depthLimitedReferences >= 1U);
        CHECK(depthGraph.nodes.size() == 2U);
    }

    {
        auto limited = options;
        limited.maximumResources = 2U;
        std::vector<vmsourceconv::core::Diagnostic> localDiagnostics;
        const auto countGraph =
            vmsourceconv::graph::ResourceGraphBuilder{}.Build(
                input, rootReferences, limited, localDiagnostics);
        CHECK(countGraph.statistics.resourceLimitReached);
        CHECK(countGraph.nodes.size() == 2U);
    }

    {
        auto limited = options;
        const auto rootSize = std::filesystem::file_size(input);
        const auto childSize = std::filesystem::file_size(
            contentRoot / "maps/a.vdata_c");
        limited.readLimits.maximumGraphBytes = rootSize + childSize - 1U;
        std::vector<vmsourceconv::core::Diagnostic> localDiagnostics;
        const auto byteGraph =
            vmsourceconv::graph::ResourceGraphBuilder{}.Build(
                input, rootReferences, limited, localDiagnostics);
        CHECK(byteGraph.statistics.graphByteLimitReached);
        CHECK(byteGraph.nodes.size() >= 2U);
        CHECK(byteGraph.nodes[1].status
            == vmsourceconv::graph::ResourceNodeStatus::Limited);
    }

    {
        const std::vector<vmsourceconv::inspect::ExternalReference> conflicts{
            {7U, "maps/a.vdata", 0U, 0U},
            {7U, "maps/b.vdata", 0U, 0U},
            {8U, "maps/a.vdata", 0U, 0U},
        };
        std::vector<vmsourceconv::core::Diagnostic> localDiagnostics;
        const auto conflictGraph =
            vmsourceconv::graph::ResourceGraphBuilder{}.Build(
                input, conflicts, options, localDiagnostics);
        CHECK(conflictGraph.statistics.referenceIdentityConflicts >= 2U);
        CHECK(HasDiagnostic(localDiagnostics, "RERL_ID_NAME_CONFLICT"));
        CHECK(HasDiagnostic(localDiagnostics, "RERL_NAME_ID_CONFLICT"));
    }
}
