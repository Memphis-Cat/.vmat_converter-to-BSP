# VMSourceCONV

VMSourceCONV is a native C++17 command-line project for inspecting and eventually converting compiled Source 2 maps into lossy Source 1 content.

## Current milestone: recursive inspect with native VPK reading

```text
VMSourceCONV inspect <resource.vmap_c> [options]
```

The inspector reads Source 2 `_c` resource containers, decodes `RERL`, follows structural dependencies, and loads referenced resources from loose files or Valve VPK packages. It does not invoke Workshop Tools or a third-party executable.

## Build

```text
cmake -S . -B build
cmake --build build --config Release
```

The Windows executable is normally located at:

```text
build/src/VMSourceCONV/Release/VMSourceCONV.exe
```

## VPK search order

VMSourceCONV uses deterministic first-match lookup:

1. Loose compiled files beside the selected resource and under `--resource-root` directories.
2. VPK files explicitly selected with `--vpk`.
3. A map-matching package such as `de_cache.vpk` or `de_cache_dir.vpk` for `de_cache.vmap_c`.
4. Shared fallback packages inside a folder named `vpk`.

The fallback folder is discovered beside the input map, beneath resource roots, and beneath the current working directory. It is scanned recursively. `pak01_dir.vpk` is indexed first, then other `pak*_dir.vpk` files, other `_dir.vpk` files, and finally remaining valid VPK directory files.

Raw numbered data chunks are ignored as indexes, but are opened automatically when their directory VPK references them. VPK entries are matched by their exact normalized internal resource path, and the first matching archive wins.

Recommended layout:

```text
VMAT/
├── de_cache.vmap_c
├── de_cache.vpk
└── vpk/
    ├── pak01_dir.vpk
    ├── pak01_000.vpk
    ├── pak01_001.vpk
    └── additional_packages/
        └── another_dir.vpk
```

Inspect the selected map package and use the shared library only for missing files:

```powershell
$exe = ".\build\src\VMSourceCONV\Release\VMSourceCONV.exe"

& $exe inspect ".\de_cache.vmap_c" `
    --follow-references `
    --resource-root "D:\VMAT" `
    --logs.txt
```

Explicit VPK package paths or directories form the highest-priority VPK tier:

```text
VMSourceCONV inspect de_cache.vmap_c --follow-references --vpk D:\cache-package
VMSourceCONV inspect de_cache.vmap_c --follow-references --vpk D:\cs2\game\csgo\pak01_dir.vpk
```

## Logging

All terminal output and errors can be copied to a text file while remaining visible:

```text
VMSourceCONV inspect de_cache.vmap_c --follow-references --log logs.txt
VMSourceCONV inspect de_cache.vmap_c --follow-references --log=logs.txt
VMSourceCONV inspect de_cache.vmap_c --follow-references --logs.txt
```

Normal shell redirection also works. In PowerShell, include the error stream when desired:

```powershell
& $exe inspect ".\de_cache.vmap_c" --follow-references > logs.txt 2>&1
```

## Current limits

The VPK reader supports Valve VPK versions 1 and 2, preload bytes, embedded data, and numbered split archives. CRC verification, `DATA`/KV3 decoding, meshes, entities, materials, physics conversion, and Source 1 output remain later milestones.
