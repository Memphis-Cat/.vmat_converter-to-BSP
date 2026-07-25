# VMSourceCONV

VMSourceCONV is a native C++ command-line project for inspecting and eventually converting compiled Source 2 maps into lossy Source 1 content.

## Current milestone: inspect

The first command reads the generic Source 2 compiled-resource container used by files such as `.vmap_c`:

```text
VMSourceCONV inspect <resource.vmap_c>
```

Optional outputs:

```text
VMSourceCONV inspect map.vmap_c --json report.json --dump-blocks blocks
```

The inspector currently:

- reads the 16-byte Source 2 resource header;
- resolves the relative block-directory offset;
- enumerates and bounds-checks block entries;
- validates declared size, header version, block ranges, and overlaps;
- decodes `RERL` external resource references;
- writes text and JSON reports;
- optionally dumps each valid raw block.

It does not decode `DATA`, KV3, world nodes, meshes, or entities yet.

## Build

```text
cmake -S . -B build
cmake --build build --config Release
```

On Windows with a multi-configuration generator, the executable is usually under `build/src/VMSourceCONV/Release/`.

## Source SDK 2013

The existing `external/source-sdk-2013` submodule remains separate. The inspect milestone has no SDK dependency because Source SDK 2013 describes the Source 1 target side, not the Source 2 compiled-resource input format.
