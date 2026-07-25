# VMSourceCONV source layout

The command-line application is split by responsibility:

- `cli/`: argument parsing, usage, and command routing.
- `core/`: application lifecycle, diagnostics, errors, and exit codes.
- `io/`: bounded binary reads, files, and ordinary paths.
- `discovery/`: logical Source 2 resource-path normalization and loose-file location.
- `resource/`: generic Source 2 `_c` resource container structures and validation.
- `graph/`: dependency filtering, cycle-safe traversal, limits, and resource summaries.
- `inspect/`: resource inspection, `RERL` decoding, reporting, and block dumping.
- `analyzing/`, `decision/`, `convert/`, `compiling/`, `fix/`, `repair/`: reserved objective modules.

The Source SDK 2013 submodule is intentionally not linked into the inspector. It becomes relevant when Source 1 target analysis and output generation are implemented.

The current graph traversal supports loose `_c` resources. VPK mounting is a separate future discovery/filesystem objective.
