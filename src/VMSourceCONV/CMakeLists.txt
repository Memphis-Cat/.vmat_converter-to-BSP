set(VMSOURCECONV_SOURCES
    main.cpp

    cli/CommandLine.cpp
    cli/CommandRouter.cpp
    cli/Usage.cpp

    core/Application.cpp
    core/Diagnostic.cpp
    core/LogSession.cpp
    core/ParseError.cpp
    core/TeeStreamBuf.cpp

    io/BinaryReader.cpp
    io/FileReader.cpp
    io/PathUtils.cpp

    discovery/CompiledResourcePath.cpp
    discovery/ResourceLocator.cpp
    discovery/VpkSearchPolicy.cpp

    vpk/VpkArchive.cpp
    vpk/VpkRepository.cpp

    resource/FourCC.cpp
    resource/RelativeOffset.cpp
    resource/ResourceBlock.cpp
    resource/ResourceDocument.cpp
    resource/ResourceHeader.cpp
    resource/ResourceParser.cpp
    resource/ResourceValidator.cpp

    graph/DependencyFilter.cpp
    graph/ResourceGraph.cpp
    graph/ResourceGraphBuilder.cpp

    inspect/BlockDumper.cpp
    inspect/InspectionReport.cpp
    inspect/InspectCommand.cpp
    inspect/Inspector.cpp
    inspect/JsonReportWriter.cpp
    inspect/RerlInspector.cpp
    inspect/TextReportWriter.cpp

    analyzing/AnalysisStage.cpp
    decision/DecisionStage.cpp
    convert/ConversionStage.cpp
    compiling/CompilationStage.cpp
    fix/FixStage.cpp
    repair/RepairStage.cpp
)

add_executable(VMSourceCONV ${VMSOURCECONV_SOURCES})
target_compile_features(VMSourceCONV PRIVATE cxx_std_17)
target_include_directories(VMSourceCONV PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(VMSourceCONV PRIVATE /W4 /permissive- /EHsc)
    if(VMSOURCECONV_WARNINGS_AS_ERRORS)
        target_compile_options(VMSourceCONV PRIVATE /WX)
    endif()
else()
    target_compile_options(VMSourceCONV PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(VMSOURCECONV_WARNINGS_AS_ERRORS)
        target_compile_options(VMSourceCONV PRIVATE -Werror)
    endif()
endif()
