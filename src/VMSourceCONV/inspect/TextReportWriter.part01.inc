               << (resourceGraph.includeAssets
                       ? "all referenced resources"
                       : "structural map resources")
               << '\n'
               << "Maximum depth: " << resourceGraph.maximumDepth << '\n'
               << "Maximum resources: " << resourceGraph.maximumResources << '\n'
               << "Search roots:\n";
        for (const auto& root : resourceGraph.searchRoots) {
            output << "  " << root.string() << '\n';
        }
        output << "Mounted VPKs:\n";
        if (resourceGraph.mountedVpks.empty()) {
            output << "  (none)\n";
        } else {
            for (const auto& path : resourceGraph.mountedVpks) {
                output << "  " << path.string() << '\n';
            }
        }

        output << "\nResolved resources:\n";
        if (resourceGraph.nodes.empty()) {
            output << "(none)\n";
        } else {
            for (const auto& node : resourceGraph.nodes) {
                output << '[' << node.index << "] depth=" << node.depth
                       << " status=" << graph::ToString(node.status)
                       << " source=" << graph::ToString(node.source)
                       << ' ' << node.logicalName;
                if (node.source == graph::ResourceNodeSource::LooseFile) {
                    output << "\n    file=" << node.resolvedPath.string();
                } else if (node.source == graph::ResourceNodeSource::VpkArchive) {
                    output << "\n    archive=" << node.resolvedPath.string()
                           << "\n    entry=" << node.vpkEntryPath;
                }
                if (node.status == graph::ResourceNodeStatus::Loaded) {
                    output << "\n    size=" << node.actualSize
                           << " blocks=" << node.blocks.size()
                           << " references=" << node.externalReferenceCount
                           << '\n';
                    WriteDataSummary(output, node.dataInspection, "    ");
                    WriteKv3Summary(output, node.kv3Decode, "    ");
                    if (!node.kv3JsonPath.empty()) {
                        output << "    kv3-json="
                               << node.kv3JsonPath.string() << '\n';
                    }
                }
                if (node.parentIndex.has_value()) {
                    output << "    parent=" << *node.parentIndex << '\n';
                }
                if (!node.message.empty()) {
                    output << "    message=" << node.message << '\n';
                }
            }
        }

        output << "\nGraph summary:\n"
               << "  references seen: " << statistics.referencesSeen << '\n'
               << "  references skipped: " << statistics.referencesSkipped << '\n'
               << "  duplicate references: " << statistics.duplicateReferences << '\n'
               << "  depth-limited references: " << statistics.depthLimitedReferences << '\n'
               << "  resources loaded: " << statistics.resourcesLoaded << '\n'
               << "  loaded from loose files: " << statistics.resourcesLoadedLoose << '\n'
               << "  loaded from VPKs: " << statistics.resourcesLoadedFromVpk << '\n'
               << "  resources missing: " << statistics.resourcesMissing << '\n'
               << "  resources failed: " << statistics.resourcesFailed << '\n'
               << "  KV3 documents decoded: " << statistics.kv3DocumentsDecoded << '\n'
               << "  KV3 documents failed: " << statistics.kv3DocumentsFailed << '\n'
               << "  KV3 documents exported: " << statistics.kv3DocumentsExported << '\n'
               << "  entity documents retained: " << statistics.entityDocumentsRetained << '\n'
               << "  scene documents retained: " << statistics.sceneDocumentsRetained << '\n'
               << "  resource limit reached: "
               << (statistics.resourceLimitReached ? "yes" : "no") << '\n';
    }

    if (report.worldScene.enabled) {
        const auto& statistics = report.worldScene.statistics;
        output << "\nWorld scene reconstruction\n--------------------------\n"
               << "Worlds: " << statistics.worldCount << '\n'
               << "World nodes: " << statistics.worldNodeCount << '\n'
               << "Layers: " << statistics.layerCount << '\n'
               << "Scene objects: " << statistics.sceneObjectCount << '\n'
               << "Model instances: " << statistics.modelInstanceCount << '\n'
               << "Mesh instances: " << statistics.meshInstanceCount << '\n'
               << "Aggregates: " << statistics.aggregateCount << '\n'
               << "Aggregate fragments: "
               << statistics.aggregateFragmentCount << '\n'
               << "Clutter objects: " << statistics.clutterObjectCount << '\n'
               << "Objects with transforms: "
               << statistics.transformedObjectCount << '\n'
               << "Objects with bounds: "
               << statistics.boundedObjectCount << '\n'
               << "Unique renderables: "
               << statistics.uniqueRenderableCount << '\n';
    }

    if (!report.entityLumps.empty()) {
        std::size_t entityCount = 0;
        std::size_t binaryCount = 0;
        for (const auto& lump : report.entityLumps) {
            entityCount += lump.entities.size();
            binaryCount += lump.preservedBinaryEntities;
        }
        output << "\nEntity export\n-------------\n"
               << "Lumps: " << report.entityLumps.size() << '\n'
               << "Entities: " << entityCount << '\n'
               << "Binary entity payloads preserved: " << binaryCount << '\n';
    }

    output << "\nDiagnostics\n-----------\n";
    if (report.diagnostics.empty()) {
        output << "No diagnostics.\n";
    } else {
        for (const auto& diagnostic : report.diagnostics) {
            output << core::ToString(diagnostic.severity)
                   << " [" << diagnostic.code << "] ";
            if (diagnostic.offset.has_value()) {
                WriteHex(output, *diagnostic.offset);
                output << ": ";
            }
            output << diagnostic.message << '\n';
        }
    }

    output << "\nSummary: " << report.WarningCount() << " warning(s), "
           << report.ErrorCount() << " error(s)\n";
}

} // namespace vmsourceconv::inspect
