# VMSourceCONV source layout

The command-line application is split by responsibility:

- `cli/`: argument parsing, usage, and command routing.
- `core/`: application lifecycle, diagnostics, errors, and exit codes.
- `io/`: bounded binary reads, files, and paths.
- `resource/`: generic Source 2 `_c` resource container structures and validation.
- `inspect/`: the implemented `inspect` objective.
- `analyzing/`, `decision/`, `convert/`, `compiling/`, `fix/`, `repair/`: reserved objective modules. They compile as inert stage declarations but are not exposed by the CLI yet.

The Source SDK 2013 submodule is intentionally not linked into the inspector. The inspector reads Source 2 resource containers directly; SDK 2013 will become relevant when Source 1 target analysis and output generation are implemented.
