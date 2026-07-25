# VMSourceCONV

VMSourceCONV is a native C++17 command-line project for inspecting and eventually converting compiled Source 2 maps into lossy Source 1 content.

## Current milestone: recursive inspect with native VPK reading

```text
VMSourceCONV inspect <resource.vmap_c> [options]
```

The inspector reads Source 2 `_c` resource containers, decodes `RERL`, follows structural dependencies, and loads referenced resources from either loose files or Valve VPK packages. It does not invoke Workshop Tools or a third-party executable.

A map-package folder containing a root `.vmap_c` and adjacent numbered `.vpk` files can be passed directly as a resource root. VMSourceCONV scans adjacent `.vpk` files, mounts files that contain a valid VPK directory tree, and ignores raw split data chunks until an index references them.

## Build

```text
cmake -S . -B build
cmake --build build --config Release
```

The Windows executable is normally located at:

```text
build/src/VMSourceCONV/Release/VMSourceCONV.exe
```

## Inspect a map package

```text
VMSourceCONV inspect de_cache.vmap_c --follow-references --resource-root D:\VMAT
```

Explicit VPK package paths or package directories can be mounted more than once:

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
