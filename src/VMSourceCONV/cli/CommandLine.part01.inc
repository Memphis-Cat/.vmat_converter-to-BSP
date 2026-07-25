ParsedCommandLine CommandLine::Parse(const int argc, char** argv) {
    ParsedCommandLine result;
    if (argc <= 1) {
        result.showHelp = true;
        return result;
    }

    const std::string_view first{argv[1]};
    if (IsHelp(first)) {
        result.showHelp = true;
        return result;
    }
    if (first == "--version" || first == "-v") {
        result.showVersion = true;
        return result;
    }
    if (first == "inspect") {
        result.command = CommandKind::Inspect;
    } else if (first == "verify-vpk") {
        result.command = CommandKind::VerifyVpk;
    } else {
        result.error = "unknown command: " + std::string(first);
        return result;
    }

    for (int index = 2; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (IsHelp(argument)) {
            result.showHelp = true;
            return result;
        }

        const auto global = ParseGlobalOption(argc, argv, index, result);
        if (global == GlobalOptionResult::Error) {
            return result;
        }
        if (global == GlobalOptionResult::Handled) {
            continue;
        }

        if (result.command == CommandKind::VerifyVpk) {
            if (argument == "--json") {
                if (!RequireValue(argc, index, result, "--json")) return result;
                result.verifyVpkOptions.jsonOutput = argv[++index];
                continue;
            }
            if (argument == "--no-vpk-crc") {
                result.verifyVpkOptions.verification.verifyCrc = false;
                continue;
            }
            if (argument == "--no-orphan-scan") {
                result.verifyVpkOptions.verification.scanOrphanChunks = false;
                continue;
            }
            if (argument == "--max-vpk-entry-bytes") {
                if (!ParseByteLimit(
                        argc, argv, index, result,
                        "--max-vpk-entry-bytes",
                        result.verifyVpkOptions.verification.maximumEntrySize)) {
                    return result;
                }
                continue;
            }
            if (!argument.empty() && argument.front() == '-') {
                result.error = "unknown verify-vpk option: " + std::string(argument);
                return result;
            }
            if (!result.verifyVpkOptions.input.empty()) {
                result.error = "verify-vpk accepts exactly one VPK directory file";
                return result;
            }
            result.verifyVpkOptions.input = argv[index];
            continue;
        }

        auto& options = result.inspectOptions;
        if (argument == "--json") {
            if (!RequireValue(argc, index, result, "--json")) return result;
            options.jsonOutput = argv[++index];
            continue;
        }
        if (argument == "--entities-json") {
            if (!RequireValue(argc, index, result, "--entities-json")) return result;
            options.entitiesJsonOutput = argv[++index];
            options.inspectData = true;
            options.decodeEntityLumps = true;
            continue;
        }
        if (argument == "--kv3-json-dir") {
            if (!RequireValue(argc, index, result, "--kv3-json-dir")) return result;
            options.kv3JsonDirectory = argv[++index];
            options.inspectData = true;
            options.decodeKv3 = true;
            continue;
        }
        if (argument == "--kv3-filter") {
            if (!RequireValue(argc, index, result, "--kv3-filter")) return result;
            options.kv3Filters.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--scene-json") {
            if (!RequireValue(argc, index, result, "--scene-json")) return result;
            options.sceneJsonOutput = argv[++index];
            options.inspectData = true;
            options.decodeKv3 = true;
            options.reconstructScene = true;
            continue;
        }
        if (argument == "--dump-blocks") {
            if (!RequireValue(argc, index, result, "--dump-blocks")) return result;
            options.dumpDirectory = argv[++index];
            continue;
        }
        if (argument == "--resource-root") {
            if (!RequireValue(argc, index, result, "--resource-root")) return result;
            options.resourceRoots.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--override-root") {
            if (!RequireValue(argc, index, result, "--override-root")) return result;
            options.overrideRoots.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--vpk") {
            if (!RequireValue(argc, index, result, "--vpk")) return result;
            options.vpkPaths.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--max-depth") {
            if (!RequireValue(argc, index, result, "--max-depth")) return result;
            const std::string_view value{argv[++index]};
            if (!ParseBoundedSize(value, 1U, 64U, options.maximumDepth)) {
                result.error = "--max-depth must be an integer from 1 to 64";
                return result;
            }
            continue;
        }
        if (argument == "--max-resources") {
            if (!RequireValue(argc, index, result, "--max-resources")) return result;
            const std::string_view value{argv[++index]};
            if (!ParseBoundedSize(value, 1U, 1'000'000U, options.maximumResources)) {
                result.error = "--max-resources must be an integer from 1 to 1000000";
                return result;
            }
            continue;
        }
        if (argument == "--max-input-bytes") {
            if (!ParseByteLimit(
                    argc, argv, index, result, "--max-input-bytes",
                    options.readLimits.maximumInputBytes)) return result;
            continue;
        }
        if (argument == "--max-vpk-entry-bytes") {
            if (!ParseByteLimit(
                    argc, argv, index, result, "--max-vpk-entry-bytes",
                    options.readLimits.maximumVpkEntryBytes)) return result;
            continue;
        }
        if (argument == "--max-graph-bytes") {
            if (!ParseByteLimit(
                    argc, argv, index, result, "--max-graph-bytes",
                    options.readLimits.maximumGraphBytes)) return result;
            continue;
        }
        if (argument == "--follow-references") {
            options.followReferences = true;
            continue;
        }
        if (argument == "--include-assets") {
            options.followReferences = true;
            options.includeAssets = true;
            continue;
        }
        if (argument == "--inspect-data") {
            options.inspectData = true;
            continue;
        }
        if (argument == "--decode-kv3") {
            options.inspectData = true;
            options.decodeKv3 = true;
            continue;
        }
        if (argument == "--no-rerl") {
            options.inspectExternalReferences = false;
            continue;
        }
        if (argument == "--no-vpk-crc") {
            options.verifyVpkCrc = false;
            continue;
        }
        if (argument == "--report-vpk-collisions") {
            options.reportVpkCollisions = true;
            continue;
        }
        if (argument == "--strict-vpk-collisions") {
            options.reportVpkCollisions = true;
            options.strictVpkCollisions = true;
            continue;
        }
        if (argument == "--strict") {
            options.strict = true;
            continue;
        }
        if (!argument.empty() && argument.front() == '-') {
            result.error = "unknown inspect option: " + std::string(argument);
            return result;
        }
        if (!options.input.empty()) {
            result.error = "inspect accepts exactly one input file";
            return result;
        }
        options.input = argv[index];
    }

    if (result.command == CommandKind::VerifyVpk) {
        if (result.verifyVpkOptions.input.empty()) {
            result.error = "verify-vpk requires a VPK directory file";
        }
        return result;
    }

    auto& options = result.inspectOptions;
    if (options.input.empty()) {
        result.error = "inspect requires a Source 2 compiled resource";
        return result;
    }
    if (!options.kv3Filters.empty() && options.kv3JsonDirectory.empty()) {
        result.error = "--kv3-filter requires --kv3-json-dir";
        return result;
    }

    const auto inputFileName = options.input.filename().string();
    if (options.decodeEntityLumps
        && !EndsWithIgnoreCase(inputFileName, ".vents_c")) {
        options.followReferences = true;
    }
    if (options.reconstructScene
        && !EndsWithIgnoreCase(inputFileName, ".vwrld_c")
        && !EndsWithIgnoreCase(inputFileName, ".vwnod_c")) {
        options.followReferences = true;
    }
    if (options.followReferences && !options.inspectExternalReferences) {
        result.error =
            "reference traversal, entity export, and scene reconstruction require RERL decoding";
    }
    return result;
}

} // namespace vmsourceconv::cli
