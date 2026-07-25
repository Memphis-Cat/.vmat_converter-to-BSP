ResourceGraph ResourceGraphBuilder::Build(
    const std::filesystem::path& input,
    const std::vector<inspect::ExternalReference>& rootReferences,
    const ResourceGraphBuildOptions& options,
    std::vector<core::Diagnostic>& diagnostics) const {
    ResourceGraph graph;
    graph.enabled = true;
    graph.includeAssets = options.includeAssets;
    graph.maximumDepth = options.maximumDepth;
    graph.maximumResources = options.maximumResources;
    graph.maximumGraphBytes = options.readLimits.maximumGraphBytes;

    const discovery::ResourceLocator locator(
        input,
        options.resourceRoots,
        options.vpkPaths,
        options.overrideRoots,
        options.vpkReadOptions,
        options.readLimits);
    const discovery::IResourceProvider& provider = locator;
    graph.overrideRoots = provider.OverrideRoots();
    graph.searchRoots = provider.SearchRoots();
    graph.mountedVpks = provider.MountedVpks();
    for (const auto& warning : provider.VpkWarnings()) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "VPK_MOUNT_WARNING",
            warning,
            std::nullopt,
        });
    }

    std::string rootError;
    const auto rootIdentity = discovery::ResourceIdentity::TryCreate(
        input.filename().generic_string(), &rootError);
    if (!rootIdentity.has_value()) {
        throw std::runtime_error("invalid root resource identity: " + rootError);
    }

    ResourceGraphNode rootNode;
    rootNode.index = 0;
    rootNode.logicalName = rootIdentity->logicalName;
    rootNode.identityKey = rootIdentity->key;
    rootNode.compiledRelativePath = rootIdentity->compiledRelativePath;
    rootNode.resolvedPath = std::filesystem::absolute(input).lexically_normal();
    rootNode.lookupTier = "input";
    rootNode.depth = 0;
    rootNode.status = ResourceNodeStatus::Loaded;
    rootNode.source = ResourceNodeSource::LooseFile;
    rootNode.externalReferenceCount = rootReferences.size();
    std::error_code rootSizeError;
    rootNode.actualSize = std::filesystem::file_size(input, rootSizeError);
    if (rootSizeError) {
        rootNode.actualSize = 0;
    }
    graph.statistics.totalBytesLoaded = rootNode.actualSize;
    if (graph.statistics.totalBytesLoaded > graph.maximumGraphBytes) {
        graph.statistics.graphByteLimitReached = true;
        diagnostics.push_back({
            core::DiagnosticSeverity::Error,
            "GRAPH_BYTE_LIMIT_REACHED",
            "root resource alone exceeds the total graph byte limit",
            std::nullopt,
        });
    }
    graph.nodes.push_back(std::move(rootNode));

    std::unordered_map<std::string, std::size_t> visited;
    visited.emplace(rootIdentity->key, 0U);
    std::unordered_map<std::uint64_t, std::string> idToKey;
    std::unordered_map<std::string, std::uint64_t> keyToId;
    std::deque<PendingReference> pending;
    EnqueueReferences(
        rootReferences,
        0U,
        1U,
        {rootIdentity->key},
        options.includeAssets,
        pending,
        graph,
        idToKey,
        keyToId,
        diagnostics);

    bool resourceLimitReported = false;
    bool byteLimitReported = graph.statistics.graphByteLimitReached;
    while (!pending.empty()) {
        auto current = std::move(pending.front());
        pending.pop_front();
        auto& edge = graph.edges.at(current.edgeIndex);

        if (current.depth > options.maximumDepth) {
            edge.disposition = DependencyDisposition::DepthLimited;
            edge.message = "maximum dependency depth reached";
            ++graph.statistics.depthLimitedReferences;
            continue;
        }

        const auto visitedMatch = visited.find(current.identity.key);
        if (visitedMatch != visited.end()) {
            edge.targetNode = visitedMatch->second;
            const auto selfReference = visitedMatch->second == current.sourceNode;
            const auto cycle = std::find(
                current.ancestry.begin(),
                current.ancestry.end(),
                current.identity.key) != current.ancestry.end();
            if (selfReference) {
                edge.disposition = DependencyDisposition::SelfReference;
                edge.message = "resource references itself";
                ++graph.statistics.selfReferences;
            } else if (cycle) {
                edge.disposition = DependencyDisposition::Cycle;
                edge.message = "reference closes an ancestry cycle";
                ++graph.statistics.cycleEdges;
            } else {
                edge.disposition = DependencyDisposition::Repeated;
                edge.message = "resource was already loaded through another edge";
                ++graph.statistics.repeatedEdges;
                ++graph.statistics.duplicateReferences;
            }
            continue;
        }

        if (graph.nodes.size() >= options.maximumResources) {
            edge.disposition = DependencyDisposition::ResourceLimited;
            edge.message = "maximum resource count reached";
            graph.statistics.resourceLimitReached = true;
            if (!resourceLimitReported) {
                diagnostics.push_back({
                    core::DiagnosticSeverity::Warning,
                    "RESOURCE_LIMIT_REACHED",
                    "dependency traversal reached the resource limit",
                    std::nullopt,
                });
                resourceLimitReported = true;
            }
            continue;
        }

        ResourceGraphNode node;
        node.index = graph.nodes.size();
        node.parentIndex = current.sourceNode;
        node.referenceId = current.reference.id;
        node.logicalName = current.identity.logicalName;
        node.identityKey = current.identity.key;
        node.compiledRelativePath = current.identity.compiledRelativePath;
        node.depth = current.depth;
        edge.targetNode = node.index;
        visited.emplace(current.identity.key, node.index);

        const auto location = provider.Locate(current.identity.logicalName);
        node.lookupTier = discovery::ToString(location.tier);
        node.shadowedVpks = location.shadowedVpks;
        if (!location.shadowedVpks.empty()) {
            ++graph.statistics.vpkCollisionCount;
            graph.vpkCollisions.push_back({
                node.logicalName,
                location.resolvedPath,
                node.lookupTier,
                location.shadowedVpks,
            });
            if (options.reportVpkCollisions || options.strictVpkCollisions) {
                diagnostics.push_back({
                    options.strictVpkCollisions
                        ? core::DiagnosticSeverity::Error
                        : core::DiagnosticSeverity::Warning,
                    "VPK_RESOURCE_COLLISION",
                    node.logicalName + " selected "
                        + location.resolvedPath.string() + " and shadowed "
                        + std::to_string(location.shadowedVpks.size())
                        + " exact match(es)",
                    std::nullopt,
                });
            }
        }

        if (!location.Found()) {
            node.status = ResourceNodeStatus::Missing;
            node.message = location.error;
            edge.disposition = DependencyDisposition::Missing;
            edge.message = location.error;
            ++graph.statistics.resourcesMissing;
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "DEPENDENCY_MISSING",
                node.logicalName + ": " + location.error,
                std::nullopt,
            });
            graph.nodes.push_back(std::move(node));
            continue;
        }

        node.resolvedPath = location.resolvedPath;
        if (location.source == discovery::ResourceSource::LooseFile) {
            node.source = ResourceNodeSource::LooseFile;
        } else if (location.source == discovery::ResourceSource::VpkArchive) {
            node.source = ResourceNodeSource::VpkArchive;
            node.vpkEntryPath = location.vpkMatch.entryPath;
        }

        std::vector<inspect::ExternalReference> childReferences;
        try {
            std::vector<core::Diagnostic> localDiagnostics;
            auto file = provider.Read(location);
            const auto fileSize = static_cast<std::uint64_t>(file.bytes.size());
            if (fileSize > graph.maximumGraphBytes
                || graph.statistics.totalBytesLoaded
                    > graph.maximumGraphBytes - fileSize) {
                graph.statistics.graphByteLimitReached = true;
                node.status = ResourceNodeStatus::Limited;
                node.message = "total graph byte limit reached before parsing";
                edge.disposition = DependencyDisposition::ResourceLimited;
                edge.message = node.message;
                if (!byteLimitReported) {
                    diagnostics.push_back({
                        core::DiagnosticSeverity::Warning,
                        "GRAPH_BYTE_LIMIT_REACHED",
                        "dependency traversal reached the total graph byte limit",
                        std::nullopt,
                    });
                    byteLimitReported = true;
                }
            } else {
                graph.statistics.totalBytesLoaded += fileSize;
                auto document = resource::ResourceParser{}.Parse(
                    std::move(file), localDiagnostics);
                childReferences = inspect::RerlInspector{}.Inspect(
                    document, localDiagnostics);

                if (options.inspectData || options.decodeKv3
                    || options.decodeEntityLumps || options.reconstructScene
                    || !options.kv3JsonDirectory.empty()) {
                    node.dataInspection =
                        inspect::data::DataBlockInspector{}.Inspect(document);
                }
                DecodeKv3IfRequested(
                    node.logicalName,
                    options.decodeKv3,
                    options.decodeEntityLumps,
                    options.reconstructScene,
                    options.kv3JsonDirectory,
                    options.kv3Filters,
                    document,
                    node,
                    graph.statistics,
                    diagnostics);

                node.status = ResourceNodeStatus::Loaded;
                node.actualSize = document.file.bytes.size();
                node.declaredFileSize = document.header.declaredFileSize;
                node.headerVersion = document.header.headerVersion;
                node.resourceVersion = document.header.resourceVersion;
                node.externalReferenceCount = childReferences.size();
                node.warningCount = CountSeverity(
                    localDiagnostics, core::DiagnosticSeverity::Warning);
                node.errorCount = CountSeverity(
                    localDiagnostics, core::DiagnosticSeverity::Error);
                node.blocks.reserve(document.blocks.size());
                for (const auto& block : document.blocks) {
                    node.blocks.push_back({
                        block.type.ToString(), block.offset, block.size,
                    });
                }

                AppendDiagnostics(node.logicalName, localDiagnostics, diagnostics);
                ++graph.statistics.resourcesLoaded;
                if (node.source == ResourceNodeSource::VpkArchive) {
                    ++graph.statistics.resourcesLoadedFromVpk;
                } else {
                    ++graph.statistics.resourcesLoadedLoose;
                }
                edge.disposition = DependencyDisposition::Resolved;
            }
        } catch (const core::ParseError& error) {
            node.status = ResourceNodeStatus::ParseError;
            node.message = error.what();
            node.errorCount = 1;
            edge.disposition = DependencyDisposition::Failed;
            edge.message = error.what();
            ++graph.statistics.resourcesFailed;
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "DEPENDENCY_PARSE_ERROR",
                node.logicalName + ": " + error.what(),
                error.Offset(),
            });
        } catch (const std::runtime_error& error) {
            node.status = ResourceNodeStatus::IoError;
            node.message = error.what();
            node.errorCount = 1;
            edge.disposition = DependencyDisposition::Failed;
            edge.message = error.what();
            ++graph.statistics.resourcesFailed;
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "DEPENDENCY_IO_ERROR",
                node.logicalName + ": " + error.what(),
                std::nullopt,
            });
        }

        const auto nodeIndex = node.index;
        const auto loaded = node.status == ResourceNodeStatus::Loaded;
        graph.nodes.push_back(std::move(node));
        if (loaded) {
            auto ancestry = current.ancestry;
            ancestry.push_back(current.identity.key);
            EnqueueReferences(
                childReferences,
                nodeIndex,
                current.depth + 1U,
                ancestry,
                options.includeAssets,
                pending,
                graph,
                idToKey,
                keyToId,
                diagnostics);
        }
    }

    return graph;
}

} // namespace vmsourceconv::graph
