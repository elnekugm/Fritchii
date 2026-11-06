<!--
# SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
-->

<p align="center">
  <img src="dist/branding/lichii_icon_with_logo.png" alt="Lichii" width="600">
</p>

# Lichii

Lichii is an experimental, open-source Nintendo Switch and Switch 2 emulator written in C++. It builds on Eglantine, Eden, yuzu and Sudachi, preserving their contributions while developing this fork's game compatibility and renderer fixes.

This is an independent fork, not an official release of those upstream projects. Lichii is not affiliated with Nintendo.

## Downloads and setup

Published builds belong on this fork's [releases page](https://github.com/zortos293/Eglantine/releases). The repository still uses its historical Eglantine name; the desktop application is now Lichii.

On Windows, extract the entire build archive and run **lichii.exe**. Only use games and system files you have legally obtained. Games, keys and firmware are not included.

### Existing saves and settings

Fritchii keeps its own profile: `%APPDATA%\fritchii` on Windows, and the `fritchii` directories under `$XDG_DATA_HOME`, `$XDG_CONFIG_HOME` and `$XDG_CACHE_HOME` on Linux and macOS. An existing portable `user/` directory still takes priority and is not imported or changed.

On the first launch Fritchii copies your newest existing profile (an earlier Fritchii or Eden in `%APPDATA%\eden`, Lichii 150+ in `%APPDATA%\lichii`, else yuzu, suyu, sudachi or citron) into its folder and shows what it copied. Settings, keys, mods and saves are copied; caches are not, so shaders are prepared again once. The old folder is never changed. If the emulator using it is still open, close it and start Fritchii again. An interrupted copy is put back and retried on the next launch.

## Compatibility and issue reports

Compatibility and performance vary by game, hardware and driver. Rendering errors, stutters and crashes remain possible. Passing automated tests does not establish that a game renders correctly, and this project makes no universal frame-rate guarantee.

When [reporting an issue](https://github.com/zortos293/Eglantine/issues), include:

- The exact build, game version, CPU, GPU and driver version.
- Relevant settings and steps to reproduce the problem.
- A screenshot and the log from the affected session, after checking it for personal information.

Do not upload game files, keys, firmware or private account information.

## Building and contributing

See the [build guide](docs/Build.md), [user documentation](docs/user/README.md) and [coding guidelines](docs/Coding.md). Some inherited documentation and internal build target names still use upstream names. The GUI build target remains `yuzu`; its output is `lichii.exe` on Windows and `lichii` on other desktop platforms.

Keep changes focused and distinguish code-level checks from actual gameplay testing.

## Artwork
Art created by YOB (yobslab)
The supplied Lichii logo, icon and library artwork are in [dist/branding](dist/branding). Windows, Linux and macOS icon resources are generated from that artwork by [Generate-LichiiIcons.ps1](tools/Generate-LichiiIcons.ps1).

## Support Lichii

Lichii is free and stays free. If you want to help, you can donate in crypto. Donations help pay for the AI tools and build machines used to develop this open-source project faster.

**Solana (SOL)**
```text
JepEjfyWjGjhJvHSrqj52KtLjcbbbmUh1iGqerYBF4w
```

**Ethereum (ETH)**
```text
0x055ABa1b80Bc35C836Ce7E9F47c2b3d6C75b5534
```

**Bitcoin (BTC)**
```text
bc1qcvxdpf4qvdhqp8ee4s355cjmxrzsphsdwpl8w4
```

**BNB Smart Chain (BNB)**
```text
0x055ABa1b80Bc35C836Ce7E9F47c2b3d6C75b5534
```

**Polygon**
```text
0x055ABa1b80Bc35C836Ce7E9F47c2b3d6C75b5534
```

Send each coin on its matching network. Coins sent on the wrong network can't be recovered.

> [!NOTE]
> Donations are voluntary gifts and are non-refundable. A donation does not buy any product, service, perk, early access, exclusive build, priority support or say in the project. Every release stays free and public for everyone.

## Credits and license

Lichii inherits the work of the Eglantine, Eden, yuzu and Sudachi projects and their contributors, including Eglantine's original developer Camille LaVey. Original copyright notices and third-party licenses are retained.

Releases are on the [releases page](https://github.com/MythosOfs/Orchid/releases). Each one has:

| File | For |
|---|---|
| `orchid-<version>-linux-x86_64-pgo.AppImage` | Linux, profile-guided build (preferred) |
| `orchid-<version>-linux-x86_64.AppImage` | Linux |
| `orchid-<version>-windows-x86_64-pgo.zip` | Windows, profile-guided MinGW build (preferred) |
| `orchid-<version>-windows-x86_64.zip` | Windows |
| `orchid-<version>-macos-arm64.zip` | macOS 15+ on Apple Silicon, experimental |

Linux: `chmod +x` the AppImage and run it; it needs glibc 2.39 or newer. Windows: extract the zip
and run `orchid.exe`.

## Build the newest commit yourself

For repeated local Linux testing, use the [faster development build](docs/Build.md#faster-local-linux-test-builds)
with `./tools/build-local.sh` after installing the dependencies and `ccache`.

Linux. Install the dependencies once:

```sh
# Arch / CachyOS
sudo pacman -Syu --needed base-devel boost catch2 cmake enet ffmpeg fmt git glslang libzip lz4 \
  ninja nlohmann-json openssl opus qt6-base qt6-multimedia qt6-charts qt6-wayland sdl3 zlib zstd \
  zip unzip vulkan-headers vulkan-utility-libraries libusb spirv-tools spirv-headers
```

Ubuntu 26.04, Debian 13 and other distributions: see [docs/Deps.md](docs/Deps.md).

Clone `main`, configure and build:

```sh
git clone https://github.com/MythosOfs/Orchid.git
cd Orchid
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DYUZU_TESTS=OFF \
  -DVulkanHeaders_FORCE_BUNDLED=ON -DVulkanUtilityLibraries_FORCE_BUNDLED=ON
cmake --build build
./build/bin/orchid
```

Later, to move to the newest commit (only changed files get rebuilt):

```sh
git pull
cmake --build build
```

The first configure downloads further dependencies into `.cache/cpm`; the two Vulkan flags use the
bundled headers, as the release builds do, so a newer system Vulkan package cannot break the build. A self-built binary has no
update checker; `git pull` is the update. Windows (MSVC) and macOS builds are described in the
[build guide](docs/Build.md).

## Documentation

- [User handbook](docs/user/README.md)
- [Build guide](docs/Build.md) and [dependencies](docs/Deps.md)
- [Fire Emblem: Fortune's Weave bring-up notes](docs/FE_FORTUNES_WEAVE.md)
- [Switch 2 loading notes](NX2_NOTES.md)
- [Development tools](tools)
- [Fork scan: taking fixes from Eglantine forks](docs/ForkScan.md)

## Credits

Orchid takes fixes from Eglantine and its forks; after every release
[tools/fork-scan.sh](docs/ForkScan.md) lists what they fixed since. Each ported commit names its
source and author in its message.

- [zortos293/Eglantine](https://github.com/zortos293/Eglantine) (Zortos), the project Orchid
  started from: paged scalar guest memory access, SPSCQueue value lifetime, timed Event waits on
  Windows, GPU teardown order, buffer cache and query fixes, native Ampere nanosecond timestamps,
  the separate pipeline cache file names.
  Radu Ursache: no mesh pipelines without `VK_EXT_mesh_shader`.
  Kaj (co-authored with Claude Opus 5.5): skip guest uploads for new render targets fully
  overwritten by a clear, adapted with framebuffer render-area, transformed-scissor,
  linear-attachment and zero-layer guards from
  [27226d2568c99c70cf78aa37adaf98b60457b2e5](https://github.com/zortos293/Eglantine/commit/27226d2568c99c70cf78aa37adaf98b60457b2e5).
  Paulo Tumabing: 32-bit global `ATOM.CAS`, adapted to Orchid's storage lowering and atomic routing.
- [Djihads80/Eglantine](https://github.com/Djihads80/Eglantine) and its contributors: merged
  regularly. Idle staging buffers freed (Takoroka), the RangeSet empty-query guard (néo Luigi),
  test fixes (kraibse).
- [ZnOw01/Primrose](https://github.com/ZnOw01/Primrose) (ZnOw01): shader cache invalidation
  lifetime fix, adapted to renderer-thread retirement.
- [exploitizm/Eglantine](https://github.com/exploitizm/Eglantine) (exploitizm): signed IADD32
  negative-overflow boundary fix for GLSL and SPIR-V, adapted from
  [c2f9811073bfe617524d4413742a4d4406fd287a](https://github.com/exploitizm/Eglantine/commit/c2f9811073bfe617524d4413742a4d4406fd287a).
- [Selevet Eglantine](https://github.com/sylvieistaken/selevet-eglantine) (sylvieistaken): the
  windowed GPU mapping table diff, hot loop shortcuts, F2I, local spill and texture query lowering,
  device memory rebuild on layout changes, decoder end of stream, Windows commit retries, and the
  GPU wait knobs (`EDEN_NX2_GPU_WAIT_SPIN_US`, `EDEN_NX2_BACK_SPIN`, `EDEN_NX2_ACQUIRE_QUEUED`,
  `EDEN_NX2_SWAP_STABLE`, `EDEN_NX2_AVX512`).
- [PokeDinner](https://git.eden-emu.dev/Franseven/PokeDinner-a-Eglantine-Fork) (Franseven,
  [mirror](https://github.com/Apollo422/PokeDinner)), partly from Eden-NS2: any cache hint on
  native LDG, SUST with a partial channel mask, `EDEN_AUTO_BUTTONS`.
- Forks of those: Dee1, Greencus and exploitizm (audio manager lifetime, macro binds past the
  start table), LucyferDev (A64 saturating shift and multiply-accumulate instructions), FZout (page
  table block ids, game list titles), sagaopc (NUP directory cycles), Quixomatic (eden-cli, Switch 2
  module names), YLTai (SDL motion after reconnect), Aidan Kinzett and oledz-i (NUP detection,
  geometry stream 0), NicholasBly and ominixisboss (dynarmic), kipavy (storage CAS loop),
  adamhahnHU and arisqz (sparse layered images), Pavel Barabanov and andrewleeog (F2I bits, host1x
  syncpoint actions), MFS-code (Switch 2 GOB order), CarlosKeese and Dennys Kaluzny Pereira
  (lowering), failaip12 (memory manager bounds).
- After 0.6.3: godpow and its contributors nowhere02, xDarkWindx, Duck912, ZnOw01 and CloudP4bbs
  (buffer, texture and staging cache guards, descriptor growth, GPU split wake-up recovery, NPad
  update filter), failaip12 via cira0100 (data races, UBSan fixes, audio and SDL locking),
  monteger8 for adamd (Donkey Kong Bananza buffer cache skips, indirect draw reset by hash), Aidan
  Kinzett (NV12 video decoder), Jonas via jojo-chaechae (x64 call reach, Switch 2 channel setup),
  SavageCore (mod root guards), cblock85, christianball123, Khobzpro, larryliu985, Doughston
  (n-popescu) and Xnos666.
- After 0.7.0: Zortos (native BRA.U, register-handle image access, uniform shifts, global memory
  helpers, the H.264 decoder reset, Aftermath dumps, Switch 2 mixer stream audio), zirulo via Zortos
  (the Ocarina of Time boot fix), Paulo Rossatto (loose base dumps, GPU-only regions, 2D blit
  formats), Raikaru (ssl:s contexts and socket timeouts), Mcm190 (H.264 packet framing), Yuri Olive
  (decoder end of stream), rdenno (fence download ranges), kraibse (tessellation pipelines), a55uka
  (contentless NUP sections), Andreas Heider (ANV barriers), Aidan Kinzett (button glyphs), Jonas
  (H0_NH1 halves), exploitizm (IMAD, I2F and FSET lowering, memory manager reference counts),
  christianball123 (device index fallback), kdegonia-star (global atomic add) and Takoroka (ISETP.EX
  and UIMAD.WIDE lowering).
- rdenno: the Mario Tennis Fever fixes in 0.5.1.
- [Drizz1le/Lichii](https://github.com/Drizz1le/Lichii) (Inc0rporate, with Claude Opus 5.5):
  count nested shader returns before reordering demotes for AMD drivers.
- [DrSergioRFilho/Lichii](https://github.com/DrSergioRFilho/Lichii) (Dr. Sergio, with Claude
  Fable 5.1): apply LayeredFS mods to NUP titles without reapplying their merged update, from
  [fe29f5c45ef8f8ab826f148827a1b3962c32d7d7](https://github.com/DrSergioRFilho/Lichii/commit/fe29f5c45ef8f8ab826f148827a1b3962c32d7d7).
- [chuppatricky/Lichii](https://github.com/chuppatricky/Lichii) (chuppatricky, with Claude Opus 5.5):
  shadow Ampere RED and SURED data registers, adapted from
  [fc5d6dc31b49ae614d06c05302f4ee2abdf8ce4c](https://github.com/chuppatricky/Lichii/commit/fc5d6dc31b49ae614d06c05302f4ee2abdf8ce4c).
- [SoyMoikas/Eglantine](https://github.com/SoyMoikas/Eglantine) (SoyMoikas): uniform-base
  native Ampere local stores and loads, adapted to Orchid's existing vector lowering and register
  shadowing from
  [3c30f286d9631581ea3611c0f30eae46e7f60694](https://github.com/SoyMoikas/Eglantine/commit/3c30f286d9631581ea3611c0f30eae46e7f60694)
  and
  [8c06623a4651e5f31d7297932f6b9887dfea9006](https://github.com/SoyMoikas/Eglantine/commit/8c06623a4651e5f31d7297932f6b9887dfea9006).
- [Hydro74000/Edlws](https://github.com/Hydro74000/Edlws) (Hydro74000): make `UniqueFunction`
  own copies of lvalue callables while moving rvalues, from
  [dd8039f6afc92803938d641a484312931acae9ca](https://github.com/Hydro74000/Edlws/commit/dd8039f6afc92803938d641a484312931acae9ca).
- [DrSergioRFilho/Lichii](https://github.com/DrSergioRFilho/Lichii) (Dr. Sergio,
  with Claude Fable 5.1): check nested config array bounds against the innermost array, from
  [08d3388f0f5496be67dd5f83dfdcddd0fcd59358](https://github.com/DrSergioRFilho/Lichii/commit/08d3388f0f5496be67dd5f83dfdcddd0fcd59358).
- [Shenopi/Lichii](https://github.com/Shenopi/Lichii) (999sian): preserve active custom settings
  while reloading global configuration, adapted from the config-only changes in
  [88ffd1f312f9a38fd45575dc8e1c7d66ef9b4f6e](https://github.com/Shenopi/Lichii/commit/88ffd1f312f9a38fd45575dc8e1c7d66ef9b4f6e).
- [trianglebraindev/Lichii](https://github.com/trianglebraindev/Lichii) (trianglebraindev,
  with Claude Opus 5.5): two-source `F2IP.U8.F32` byte packing, adapted to Orchid's register
  shadowing from
  [ac82c78b5ca4cfbcb11e3043f05a3354f444f027](https://github.com/trianglebraindev/Lichii/commit/ac82c78b5ca4cfbcb11e3043f05a3354f444f027).

- [leadaenima/Lichii](https://github.com/leadaenima/Lichii) (Cursor Agent, co-authored with
  leadaenima): AMDVLK dynamic subgroup shuffle workaround, adapted from
  [957913cf14ca1e3776a4984151b63434af543217](https://github.com/leadaenima/Lichii/commit/957913cf14ca1e3776a4984151b63434af543217)
  and [201450e90a87b494a8d1dc34d32558033a715438](https://github.com/leadaenima/Lichii/commit/201450e90a87b494a8d1dc34d32558033a715438).
  Graphics use the 32-lane broadcast fallback only when subgroup size 32 can be enforced for
  that stage. Compute and unsupported graphics stages retain Shuffle; Qualcomm keeps its
  existing full-subgroup fallback. Pipeline hash diagnostics are omitted.

## License

GPLv3 or any later version, see [LICENSE.txt](LICENSE.txt). Started from
[Eglantine](https://github.com/zortos293/Eglantine), which builds on
[Eden](https://git.eden-emu.dev/eden-emu/eden), yuzu and Sudachi.
