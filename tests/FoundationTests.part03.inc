vmsourceconv::cli::ParsedCommandLine ParseArguments(
    std::vector<std::string> arguments) {
    std::vector<char*> pointers;
    pointers.reserve(arguments.size());
    for (auto& argument : arguments) {
        pointers.push_back(argument.data());
    }
    return vmsourceconv::cli::CommandLine::Parse(
        static_cast<int>(pointers.size()), pointers.data());
}

void TestCommandLine() {
    {
        const auto parsed = ParseArguments({
            "VMSourceCONV", "inspect", "map.vmap_c", "--logs.txt",
        });
        CHECK(parsed.error.empty());
        CHECK(parsed.logOptions.path == std::filesystem::path("logs.txt"));
    }
    {
        const auto parsed = ParseArguments({
            "VMSourceCONV", "inspect", "map.vmap_c",
            "--mistake.txt", "--strict",
        });
        CHECK(!parsed.error.empty());
    }
    {
        const auto parsed = ParseArguments({
            "VMSourceCONV", "inspect", "map.vmap_c",
            "--log", "run.txt", "--log-mode", "append",
            "--override-root", "override", "--report-vpk-collisions",
            "--max-input-bytes", "1024", "--max-vpk-entry-bytes", "2048",
            "--max-graph-bytes", "4096",
        });
        CHECK(parsed.error.empty());
        CHECK(parsed.logOptions.mode == vmsourceconv::core::LogMode::Append);
        CHECK(parsed.inspectOptions.overrideRoots.size() == 1U);
        CHECK(parsed.inspectOptions.reportVpkCollisions);
        CHECK(parsed.inspectOptions.readLimits.maximumInputBytes == 1024U);
        CHECK(parsed.inspectOptions.readLimits.maximumVpkEntryBytes == 2048U);
        CHECK(parsed.inspectOptions.readLimits.maximumGraphBytes == 4096U);
    }
    {
        const auto parsed = ParseArguments({
            "VMSourceCONV", "verify-vpk", "pak01_dir.vpk",
            "--no-vpk-crc", "--no-orphan-scan", "--verify.txt",
        });
        CHECK(parsed.error.empty());
        CHECK(parsed.command == vmsourceconv::cli::CommandKind::VerifyVpk);
        CHECK(!parsed.verifyVpkOptions.verification.verifyCrc);
        CHECK(!parsed.verifyVpkOptions.verification.scanOrphanChunks);
        CHECK(parsed.logOptions.path == std::filesystem::path("verify.txt"));
    }
}

void TestLogging() {
    TemporaryDirectory temporary;
    const auto logPath = temporary.Path()
        / std::filesystem::u8path(u8"logs/üñïçødé.txt");

    std::ostringstream consoleOutput;
    std::ostringstream consoleError;
    auto* savedOutput = std::cout.rdbuf(consoleOutput.rdbuf());
    auto* savedError = std::cerr.rdbuf(consoleError.rdbuf());
    {
        vmsourceconv::core::LogOptions options;
        options.path = logPath;
        options.timestamps = false;
        vmsourceconv::core::LogSession session(options);
        char program[] = "VMSourceCONV";
        char inspect[] = "inspect";
        char* arguments[]{program, inspect};
        session.WriteHeader("test", 2, arguments);
        std::cout << "alpha\n";
        std::cerr << "beta\n";
        session.Finish(4);
    }
    std::cout.rdbuf(savedOutput);
    std::cerr.rdbuf(savedError);

    const auto firstLog = ReadText(logPath);
    CHECK(firstLog.find("VMSourceCONV test") != std::string::npos);
    CHECK(firstLog.find("[INFO] alpha") != std::string::npos);
    CHECK(firstLog.find("[ERROR] beta") != std::string::npos);
    CHECK(firstLog.find("Exit code: 4") != std::string::npos);
    CHECK(consoleOutput.str().find("alpha") != std::string::npos);
    CHECK(consoleError.str().find("beta") != std::string::npos);

    {
        vmsourceconv::core::LogOptions options;
        options.path = logPath;
        options.mode = vmsourceconv::core::LogMode::Append;
        options.timestamps = false;
        vmsourceconv::core::LogSession session(options);
        std::cout << "second-session\n";
        session.Finish(0);
    }
    const auto appendedLog = ReadText(logPath);
    CHECK(appendedLog.size() > firstLog.size());
    CHECK(appendedLog.find("second-session") != std::string::npos);

    {
        std::ostringstream quietConsole;
        savedOutput = std::cout.rdbuf(quietConsole.rdbuf());
        vmsourceconv::core::LogOptions options;
        options.path = temporary.Path() / "quiet.log";
        options.verbosity = vmsourceconv::core::Verbosity::Quiet;
        options.timestamps = false;
        {
            vmsourceconv::core::LogSession session(options);
            std::cout << "hidden-from-console\n";
            session.Finish(0);
        }
        std::cout.rdbuf(savedOutput);
        CHECK(quietConsole.str().empty());
        CHECK(ReadText(options.path).find("hidden-from-console")
            != std::string::npos);
    }

    {
        const auto blockedParent = temporary.Path() / "blocked";
        WriteBytes(blockedParent, {1U});
        vmsourceconv::core::LogOptions options;
        options.path = blockedParent / "child.log";
        bool threw = false;
        try {
            vmsourceconv::core::LogSession session(options);
        } catch (const std::exception&) {
            threw = true;
        }
        CHECK(threw);
    }
}

void TestSource2VpkSignatureDescriptor() {
    TemporaryDirectory temporary;
    const auto path = temporary.Path() / "source2-signed_dir.vpk";
    const auto payload = std::vector<std::uint8_t>{
        's', 'i', 'g', 'n', 'e', 'd',
    };

    VpkFixtureOptions options;
    options.version = 2U;
    options.source2SignatureDescriptor = true;
    WriteSingleVpk(path, "maps/test/signed.bin", payload, options);

    const vmsourceconv::vpk::VpkArchive archive(path);
    CHECK(archive.Version() == 2U);
    CHECK(archive.Sections().signatureSize == 20U);
    const auto* entry = archive.Find("maps/test/signed.bin");
    CHECK(entry != nullptr);
    CHECK(archive.Read(*entry).bytes == payload);
    CHECK(archive.Verify().Success());
}

using TestFunction = void (*)();

struct NamedTest {
    const char* name;
    TestFunction function;
};

} // namespace

int main() {
    const std::vector<NamedTest> tests{
        {"resource identity", &TestResourceIdentity},
        {"resource container validation", &TestResourceContainerValidation},
        {"file reader limits", &TestFileReaderLimits},
        {"VPK read and verification", &TestVpkReadAndVerification},
        {"Source 2 VPK signature", &TestSource2VpkSignatureDescriptor},
        {"resource locator priority", &TestResourceLocatorPriorityAndCollisions},
        {"dependency graph", &TestDependencyGraph},
        {"command line", &TestCommandLine},
        {"logging", &TestLogging},
    };

    std::size_t failures = 0;
    for (const auto& test : tests) {
        try {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    std::cout << tests.size() - failures << '/' << tests.size()
              << " foundation test group(s) passed\n";
    return failures == 0U ? 0 : 1;
}
