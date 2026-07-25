# VMSourceCONV

VMSourceCONV is a native C++ command-line project for inspecting and eventually converting compiled Source 2 maps into lossy Source 1 content.

## Current milestone: inspect and discover

The inspector reads the generic Source 2 compiled-resource container used by files such as `.vmap_c`:

```text
VMSourceCONV inspect <resource.vmap_c>
```

It can also follow the map's loose compiled dependencies:

```text
VMSourceCONV inspect de_cache.vmap_c ^
  --follow-references ^
  --resource-root "C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo" ^
  --json de_cache-report.json
```

`--follow-references` follows structural map resources such as `.vrman`, `.vwrld`, `.vwnod`, `.vents`, `.vvis`, and world-physics resources. Add `--include-assets` to also inspect models, textures, materials, and other referenced assets.

The current inspector:

- reads the 16-byte Source 2 resource header;
- resolves the relative block-directory offset;
- enumerates and bounds-checks block entries;
- validates declared size, header version, block ranges, and overlaps;
- decodes `RERL` external resource references;
- converts logical resource names such as `maps/de_cache/world.vwrld` into loose compiled paths such as `maps/de_cache/world.vwrld_c`;
- searches explicit and inferred Source 2 content roots;
- recursively inspects dependencies with cycle, depth, and resource-count protection;
- reports loaded, missing, skipped, duplicate, and failed resources;
- writes text and JSON reports;
- optionally dumps each valid root-resource block.

It does not read dependencies from VPK archives or decode `DATA`, KV3, world-node geometry, meshes, or entities yet.

## Build

```text
cmake -S . -B build
cmake --build build --config Release
```

On Windows with a multi-configuration generator, the executable is usually under `build/src/VMSourceCONV/Release/`.

## Useful commands

```text
VMSourceCONV inspect de_cache.vmap_c
VMSourceCONV inspect de_cache.vmap_c --follow-references --resource-root D:\cs2\game\csgo
VMSourceCONV inspect de_cache.vmap_c --follow-references --max-depth 6 --max-resources 2000
VMSourceCONV inspect de_cache.vmap_c --include-assets --resource-root D:\extracted_cs2
```

## Source SDK 2013

The existing `external/source-sdk-2013` submodule remains separate. The inspect and discovery milestones have no SDK dependency because Source SDK 2013 describes the Source 1 target side, not the Source 2 compiled-resource input format.
