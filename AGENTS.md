# AGENTS

See `docs/Coding.md` for technical coding guidance.

Keep comments to a reasonable level. Do not overexplain or add/remove comments from unrelated parts of the codebase.

## Game bug reports

Include only changes belonging to the requested task. Fetch `origin/main` before
pushing and integrate any newer changes as needed. Never force-push `main` or
overwrite unrelated user changes. If a push is blocked by branch protection or
permissions, report the blocker instead of bypassing it.

# Local development builds

For repeated local Linux emulator testing, use `./tools/build-local.sh` from this
checkout. Run the resulting GUI with `./build-local/bin/orchid`. Keep the build
directory between edits; do not clean or recreate it routinely.

The script uses Ninja, ccache, the available faster linker, `-O2 -g1`, and no LTO.
It builds the `yuzu` GUI target (output name `orchid`) and its dependencies, without
tests, CLI or the standalone room server. Normal emulation and graphics features
remain enabled. This is a development configuration: use a separate release/PGO
build for performance comparisons, or full debug information for variable inspection.

- Required tools: normal Linux build dependencies plus `ccache` and Ninja.
- Compiler cache: `${XDG_CACHE_HOME:-$HOME/.cache}/orchid/ccache`, shared across
  worktrees. Prefer the script so its ccache environment and debug path mapping
  remain consistent. Do not clear the cache routinely.
- Parallelism is capped by CPU count and available RAM. Override with
  `CMAKE_BUILD_PARALLEL_LEVEL=6 ./tools/build-local.sh` if necessary. Avoid running
  several full builds concurrently on the same host.
- Existing builds skip explicit reconfiguration to avoid timestamp-induced rebuilds.
  Ninja detects changed CMake inputs automatically. Configure explicitly with
  `./tools/build-local.sh --configure-only`, or pass extra CMake options.
- Extra arguments are CMake configure options. Use `ORCHID_BUILD_DIR` for a separate
  output directory. Do not reuse one build directory across different source worktrees.
- Dependency downloads can be shared via `-DCPM_SOURCE_CACHE=/absolute/cache/path`.
  CMake preserves this choice in that build directory. On Leon's PC an existing
  dependency cache is `/home/leon/src/Eglantine/.cache/cpm`; reuse it when available
  instead of downloading the same sources for every worktree.
- First builds populate the compiler cache and can still take time. Do not claim a
  measured speedup without measurement. Verify the build and report any limitations.

See [docs/Build.md](docs/Build.md#faster-local-linux-test-builds) for details. These
settings are for local iteration; preserve release/CI configurations.
Measured results and their limits are recorded in
[docs/LocalBuildBenchmark.md](docs/LocalBuildBenchmark.md). In particular, cached
rebuild timings must not be presented as timings for genuinely changed source.

For GitHub Actions, preserve Release/PGO flags and packaging targets. The release
workflow already uses compiler caching and Ninja; normal Linux builds use mold,
while PGO builds retain LLD. Consult per-job build durations and cache statistics
in Actions summaries before claiming a CI speedup. Local timings do not establish
hosted-runner performance. Do not cancel or restart an important running release
to apply build optimizations; prepare them for subsequent runs.
