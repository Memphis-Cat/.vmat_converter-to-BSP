ResourceGraph ResourceGraphBuilder::Build(
    const std::filesystem::path& input,
    const std::vector<inspect::ExternalReference>& rootReferences,
    const std::vector<std::filesystem::path>& resourceRoots,
    const std::vector<std::filesystem::path>& vpkPaths,
    const bool includeAssets,
    const bool inspectData,
    const bool decodeKv3,
    const bool decodeEntityLumps,
    const bool reconstructScene,
    const std::filesystem::path& kv3JsonDirectory,
    const std::vector<std::string>& kv3Filters,
    const std::size_t maximumDepth,
    const std::size_t maximumResources,
    std::vector<core::Diagnostic>& diagnostics) const {
    ResourceGraph graph;
    graph.enabled = true;
    graph.includeAssets = includeAssets;
    graph.maximumDepth = maximumDepth;
    graph.maximumResources = maximumResources;

    const discovery::ResourceLocator locator(input, resourceRoots, vpkPaths);
    graph.searchRoots = locator.SearchRoots();
    graph.mountedVpks = locator.MountedVpks();
    for (const auto& warning : locator.VpkWarnings()) {
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
    rootNode.depth = 0;
    rootNode.status = ResourceNodeStatus::Loaded;
    rootNode.source = ResourceNodeSource::LooseFile;
    rootNode.externalReferenceCount = rootReferences.size();
    std::error_code rootSizeError;
    rootNode.actualSize = std::filesystem::file_size(input, rootSizeError);
    if (rootSizeError) {
        rootNode.actualSize = 0;
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
        includeAssets,
        pending,
        graph,
        idToKey,
        keyToId,
        diagnostics);

    bool resourceLimitReported = false;
    while (!pending.empty()) {
        auto current = std::move(pending.front());
        pending.pop_front();
        auto& edge = graph.edges.at(current.edgeIndex);

        if (current.depth > maximumDepth) {
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

        if (graph.nodes.size() >= maximumResources) {
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

        const auto location = locator.Locate(current.identity.logicalName);
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
            auto file = locator.Read(location);
            auto document = resource::ResourceParser{}.Parse(
                std::move(file), localDiagnostics);
            childReferences = inspect::RerlInspector{}.Inspect(
                document, localDiagnostics);

            if (inspectData || decodeKv3 || decodeEntityLumps
                || reconstructScene || !kv3JsonDirectory.empty()) {
                node.dataInspection =
                    inspect::data::DataBlockInspector{}.Inspect(document);
            }
            DecodeKv3IfRequested(
                node.logicalName,
                decodeKv3,
                decodeEntityLumps,
                reconstructScene,
                kv3JsonDirectory,
                kv3Filters,
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
                includeAssets,
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
