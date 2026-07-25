set(VMSOURCECONV_CORE_SOURCES
    cli/CommandLine.cpp
    cli/CommandRouter.cpp
    cli/Usage.cpp

    core/Application.cpp
    core/Diagnostic.cpp
    core/LogOptions.cpp
    core/LogSession.cpp
    core/ParseError.cpp
    core/TeeStreamBuf.cpp

    io/BinaryReader.cpp
    io/FileReader.cpp
    io/PathUtils.cpp

    compression/CompressionError.cpp
    compression/Lz4Decoder.cpp
    compression/ZstdDecoder.cpp

    discovery/CompiledResourcePath.cpp
    discovery/ResourceIdentity.cpp
    discovery/ResourceLocator.cpp
    discovery/VpkSearchPolicy.cpp

    vpk/Crc32.cpp
    vpk/VpkArchive.cpp
    vpk/VpkRepository.cpp
    vpk/VpkVerification.cpp

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
    inspect/data/DataBlockInspector.cpp
    inspect/data/DataFormatDetector.cpp
    inspect/data/DataInspectionResult.cpp

    serialization/kv3/BinaryKv3Decoder.cpp
    serialization/kv3/BinaryKv3Header.cpp
    serialization/kv3/BinaryKv3HeaderReader.cpp
    serialization/kv3/BinaryKv3Magic.cpp
    serialization/kv3/Kv3Buffer.cpp
    serialization/kv3/Kv3Compression.cpp
    serialization/kv3/Kv3DocumentExporter.cpp
    serialization/kv3/Kv3JsonWriter.cpp
    serialization/kv3/Kv3NodeType.cpp
    serialization/kv3/Kv3PayloadDecoder.cpp
    serialization/kv3/Kv3Value.cpp

    entities/EntityLumpExport.cpp
    entities/EntityLumpJsonWriter.cpp

    scene/WorldScene.cpp
    scene/WorldSceneExtractor.cpp
    scene/WorldSceneJsonWriter.cpp

    verify/VerifyVpkCommand.cpp
    verify/VerifyVpkReportWriter.cpp

    analyzing/AnalysisStage.cpp
    decision/DecisionStage.cpp
    convert/ConversionStage.cpp
    compiling/CompilationStage.cpp
    fix/FixStage.cpp
    repair/RepairStage.cpp
)

function(vmsourceconv_apply_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /EHsc)
        if(VMSOURCECONV_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Clang folds signed-char range conversion into -Wconversion,
            # unlike GCC. These loops immediately cast to unsigned char before
            # ctype/byte processing, so align the two compiler warning sets.
            target_compile_options(${target_name} PRIVATE -Wno-sign-conversion)
        endif()
        if(VMSOURCECONV_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

add_library(vmsourceconv_core STATIC ${VMSOURCECONV_CORE_SOURCES})
target_compile_features(vmsourceconv_core PUBLIC cxx_std_17)
target_include_directories(vmsourceconv_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vmsourceconv_core PUBLIC vmsourceconv_compression)
vmsourceconv_apply_warnings(vmsourceconv_core)

add_executable(VMSourceCONV main.cpp)
target_compile_features(VMSourceCONV PRIVATE cxx_std_17)
target_link_libraries(VMSourceCONV PRIVATE vmsourceconv_core)
vmsourceconv_apply_warnings(VMSourceCONV)
