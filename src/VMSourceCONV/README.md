# VMSourceCONV source layout

The command-line application is split by responsibility:

- `cli/`: argument parsing, usage, and command routing.
- `core/`: application lifecycle, diagnostics, errors, exit codes, and logging streams.
- `io/`: bounded binary reads, files, and paths.
- `resource/`: generic Source 2 `_c` resource containers and validation.
- `discovery/`: compiled-path normalization, selected-package detection, and tiered loose/VPK lookup.
- `vpk/`: native Valve VPK v1/v2 directory parsing, recursive package discovery, priority ordering, and entry reading.
- `graph/`: dependency filtering and cycle-safe traversal.
- `inspect/`: text/JSON reports, RERL inspection, and raw block dumping.
- `analyzing/`, `decision/`, `convert/`, `compiling/`, `fix/`, `repair/`: reserved future objectives.

VPK resolution is deliberately ordered: loose files, explicit or map-matching primary packages, then the recursive shared `vpk` fallback library. Exact internal resource paths are used and the first match wins.

The Source SDK 2013 submodule remains separate and will be used on the Source 1 target side. No Source 2 external application is invoked by the inspector.
