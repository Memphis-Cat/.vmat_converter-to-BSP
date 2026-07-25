# VMSourceCONV Foundation Contract

This document freezes the container, VPK, resource-provider, dependency-graph, and logging foundation at version 1.0. Semantic decoders may consume these APIs, but should not bypass or duplicate them.

## Completed foundation areas

### 1. Canonical resource identity

`discovery::ResourceIdentity` is the only canonicalization path for Source 2 logical resources.

It:

- converts backslashes to forward slashes;
- removes redundant `.` and duplicate separators;
- uses ASCII lowercase for Valve resource identity;
- strips exactly one compiled `_c` suffix;
- rejects absolute paths, drive prefixes, empty paths, and parent traversal;
- exposes a logical name and compiled `_c` lookup key.

Graph keys, VPK paths, loose-file paths, caches, collision reports, and output names must derive from this type.

### 2–3. Complete dependency graph

The root input is node zero. Every RERL reference is represented by a `ResourceGraphEdge`, even when its target was already loaded or cannot be followed.

Edge dispositions distinguish:

- resolved dependencies;
- missing and failed resources;
- repeated dependencies from another parent;
- ancestry cycles;
- direct self-references;
- filtered references;
- invalid identities;
- depth limits;
- resource-count limits.

A resource is loaded at most once, while all incoming edges remain visible.

RERL ID/name conflicts are diagnostics rather than silent aliasing.

### 4–7. VPK integrity and package policy

The native VPK reader supports versions 1 and 2, preload bytes, directory-embedded data, and numbered chunks.

Normal reads verify entry CRC32 by default. `--no-vpk-crc` exists only for controlled debugging.

VPK v2 section sizes are retained and range-validated. Archive-MD5 records and signature-section framing are structurally checked. Cryptographic signature trust is intentionally outside the converter's current security model.

`verify-vpk` performs a package-wide pass covering:

- directory-entry ranges;
- referenced and missing chunks;
- orphan chunks;
- duplicate internal paths;
- entry CRC32;
- total package size;
- VPK v2 metadata ranges.

Exact-path collisions are retained. First-match behavior is deterministic and can be reported or promoted to an error.

Permanent lookup order:

1. explicit loose `--override-root` directories;
2. explicit `--vpk` packages;
3. the package matching the input resource name;
4. normal loose `--resource-root` directories;
5. recursive shared `vpk` fallback packages, with `pak01_dir.vpk` first.

### 8. Compiled-resource container validation

The parser and validator check:

- header and directory bounds;
- unreasonable block counts and arithmetic overflow;
- blocks outside the file;
- blocks overlapping the header or directory;
- block-to-block overlap;
- empty blocks;
- 4-byte alignment diagnostics;
- duplicate singleton blocks;
- declared-size mismatch;
- trailing bytes after declared regions.

When a singleton block is duplicated, readers deterministically use the first block and emit a diagnostic.

### 9. Logging

Logging is owned by `core::Application`, not individual commands.

Supported behavior:

- overwrite and append modes;
- stdout and stderr duplication;
- `--quiet`, `--verbose`, and `--debug` verbosity states;
- timestamped INFO/ERROR copied lines;
- automatic parent-directory creation;
- UTF-8 Windows console setup;
- version, resolved log path, command line, elapsed time, and final exit code;
- severe-error line flushing;
- `--name.txt` shorthand only as the final argument when no prior log target exists.

### 10. Resource limits and range reads

`io::ResourceReadLimits` defines independent limits for:

- loose input allocation;
- a single VPK entry;
- cumulative dependency-graph bytes.

`IResourceProvider` exposes bounded full reads and range reads. Existing compiled-resource parsers still require full resource bytes, but future mesh and texture decoders must use the range interface when practical.

### 11–12. Tests and CI

`VMSourceCONVFoundationTests` generates synthetic resources and packages at runtime. It does not depend on installed games or copyrighted assets.

The suite covers resource identities, malformed containers, VPK v1/v2, embedded/split/preload data, CRC failures, missing/orphan chunks, duplicate paths, lookup tiers, collisions, graph repeats/cycles/self-references, limits, command parsing, Unicode logs, append mode, quiet mode, and stream restoration.

GitHub Actions builds and tests with warnings as errors under:

- MSVC on Windows;
- GCC on Linux;
- Clang on Linux.

### 13. Frozen provider API

Semantic code must depend on `discovery::IResourceProvider` or on objects already returned by it.

The frozen read-only contract is:

```cpp
ResourceLocation Locate(std::string_view logicalName) const;
io::FileData Read(const ResourceLocation& location) const;
io::FileData ReadRange(
    const ResourceLocation& location,
    std::uint64_t offset,
    std::uint64_t length) const;
```

It also exposes immutable search-root, override-root, mounted-package, and mount-warning reports.

Changing the provider contract requires a dedicated foundation PR, migration notes, and foundation-test updates. Semantic milestones must not modify foundation behavior opportunistically.

## Completion definition

The foundation is considered complete when:

1. all foundation tests pass locally;
2. MSVC, GCC, and Clang CI are green;
3. a real map dependency traversal succeeds with CRC enabled;
4. the selected package passes `verify-vpk`, or any package defects are explicitly documented.

After those gates, new work proceeds one semantic area at a time without reopening this subsystem except for confirmed defects.
