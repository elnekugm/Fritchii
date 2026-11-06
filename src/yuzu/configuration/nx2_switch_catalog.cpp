// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <QCoreApplication>
#include "yuzu/configuration/nx2_switch_catalog.h"

namespace Nx2SwitchCatalog {
namespace {

constexpr Choice ACQUIRE_QUEUED_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: off")},
    {"gequal", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "gequal: acquire-gequal")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: every acquire (Selevet)")},
};

constexpr Choice AMD_SHUFFLE_LOOP_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: keep the shuffles")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: loop on every driver")},
};

constexpr Choice ASYNC_SKIP_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: always wait")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: skip while building")},
};

constexpr Choice GPU_ACCURACY_CHOICES[] = {
    {"high", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "High")},
    {"low", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Low")},
};

constexpr Choice GPU_TIMESTAMP_CHOICES[] = {
    {"ns", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "ns: nanoseconds for every title")},
    {"ticks", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "ticks: GPU ticks for every title")},
};

constexpr Choice PB_RESNAPSHOT_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: read memory as before")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: copy the current list")},
    {"2", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2: copy it and the lists queued behind it")},
};

constexpr Choice SPLIT_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: one GPU thread")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: engine methods on a second thread")},
    {"2", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2: macros on the first thread too")},
};

constexpr Choice SPLIT_METHODS_CHOICES[] = {
    {"continue", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "continue: go on in that entry")},
    {"park", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "park: wait for the rest")},
    {"auto", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "auto: experimental heuristic")},
};

constexpr Choice HELD_WRITE_BATCH_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: off")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: batch and join writes")},
    {"2", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2: batch without joining")},
};

constexpr Choice NATIVE_SSBO_MAPPED_CHOICES[] = {
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: every binding")},
    {"w", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "w: written compute bindings")},
    {"r", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "r: read compute bindings")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: none")},
};

constexpr Choice ASTC_FLUSH_CHOICES[] = {
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: submit after each decode")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: wait for the GPU to go idle")},
};

constexpr Choice SPARSE_UNBACKED_CHOICES[] = {
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: on every driver")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: on none")},
};

constexpr Choice EXPERIMENTAL_GEOMETRY_CHOICES[] = {
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: every geometry program")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: none")},
};

constexpr Choice NATIVE_SYNC_PIPELINES_CHOICES[] = {
    {"budget", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "budget: wait briefly")},
    {"full", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "full: always wait")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: skip while building")},
};

constexpr Choice CPU_SETS_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: never place threads")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: also with few P-cores")},
};

constexpr Choice FRAMES_AHEAD_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: wait for this frame")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: wait for the previous frame")},
    {"auto", QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "auto: this frame, then the previous one if a channel stalls")},
};

constexpr Choice MKW_FPS_MODE_CHOICES[] = {
    {"off", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "off: as released")},
    {"dynamic", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "dynamic: interval from the frame time")},
    {"30", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "30: locked to 30 fps")},
};

constexpr Choice INLINE_DEFER_CHOICES[] = {
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: defer every cache")},
    {"2", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2: tell the texture cache at once")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: invalidate at each write")},
};

constexpr Choice PBFLUSH_CHOICES[] = {
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: off")},
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: blocks on GPU-written pages")},
    {"2", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2: every block")},
};

constexpr Choice RUN_CACHE_CHOICES[] = {
    {"1", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1: per page")},
    {"0", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: per address")},
    {"2", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2: per page, checked")},
};

constexpr Choice DLSS_PRESET_CHOICES[] = {
    {"default", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "default: NVIDIA's choice")},
    {"J", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "J")},
    {"K", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "K: the default")},
    {"L", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "L")},
    {"M", QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "M")},
};

// Sorted by category, then name. Texts say what changing the switch from its default
// brings: gains is what it can fix or speed up, downsides what it can cost or break.
constexpr Entry ENTRIES[] = {
    {
        .name = "EDEN_NX2_ACQUIRE_PEEK",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the split, an acquire-gequal checks the releases still queued for the second "
            "thread instead of draining it first."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fewer drains of the second thread."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World ran no faster with it on a GPU-bound machine."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_ACQUIRE_QUEUED",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = ACQUIRE_QUEUED_CHOICES,
        .suggested = "gequal",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the split, a semaphore acquire that a release still queued for the second "
            "thread will meet goes on without waiting for that thread. Without macros on the "
            "first thread it stops at gequal. 1 is what Selevet's 1 does; 2 is the same as 1."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer waits for the second thread (about 2,400 a second in Mario Kart World)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Experimental. At 1, an equal or mask acquire can go on too early."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_BACK_SPIN",
        .category = Category::GpuCommands,
        .type = ValueType::Number,
        .minimum = 0,
        .maximum = 100000000,
        .suggested = "2048",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "8192 pauses"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How many CPU pause instructions the second GPU thread spins for while it waits for "
            "work, before it sleeps."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lower values save power on power-limited CPUs (laptops); higher ones wake it "
            "sooner."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 adds wake-up latency to every batch; large values keep a core busy."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_CMD_BATCH",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "When a command list the GPU wrote has to be copied back, the other GPU-written "
            "command lists that needed it before and are out of date again are copied in the same "
            "wait for the GPU (Selevet's switch, on there)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer GPU waits in titles whose compute or draws write their own command lists "
            "(Donkey Kong Bananza, Mario Kart World)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Each wait copies more; lists copied ahead may be copied again if the GPU rewrites "
            "them first."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_DMA_RANGES",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "When a GPU macro on the split's first thread reads memory no GPU work or query has "
            "written, it waits only for the queued work that can write it (uploads to the same "
            "range, copies, compute, puller work) instead of everything queued (Selevet's name "
            "for EDEN_NX2_PERF_ON=split_overlap; EDEN_NX2_PERF_OFF=split_overlap or "
            "EDEN_NX2_PERF_OPT=0 still turn it off, and EDEN_NX2_PERF_VERIFY=1 checks each read "
            "against a full drain)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Less time with the first GPU thread waiting for the second: Lichii measured 100-245 "
            "ms of such waits per second in Donkey Kong Bananza."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Relies on draws making their writes visible only after a GPU wait, as on hardware; "
            "a game that doesn't may read old values."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_DMA_READ_QUERY_FLUSH",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "When a GPU macro reads occlusion query results, writes the finished ones to memory "
            "first instead of at the next fence."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brought back hidden characters in Mario Tennis Fever."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Tennis Fever"),
    },
    {
        .name = "EDEN_NX2_DRAW_COUNT_GUARD",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips draws whose first vertex or count is past 16 million: garbage left by a "
            "rejected compute program."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brings back the crash on entering new areas in Hyrule Warriors: Age of "
            "Imprisonment, and out-of-bounds reads."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Hyrule Warriors: Age of Imprisonment"),
    },
    {
        .name = "EDEN_NX2_FENCE_BATCH",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Releases together every fence the host GPU has already passed, taking each cache "
            "lock once per burst."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 handles one fence at a time: more lock spinning (7% instead of 5.5% of the GPU "
            "thread)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_FLUSH_DRAWS",
        .category = Category::GpuCommands,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 4096,
        .suggested = "128",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Not set: every 4096 draws"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How many draws a Switch 2 title records before they are submitted to the host GPU "
            "(Selevet's switch, 128 there)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The host GPU starts on a frame while it is still being recorded: Selevet measured "
            "26.1 ms frames at 128 against 29.7 ms at 4096 in Mario Kart World (RTX 3050)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "More submissions cost driver time; very small values (32) were slower again."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_FLUSH_HELD_WRITES",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On for the Yoshi demo only"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Applies the held guest CPU writes before any flush of GPU data (CPU reads, native "
            "shader reads, compute uploads), so the download can't put the GPU's older copy over "
            "them. 1 for every title, 0 for none."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps programs the CPU wrote over GPU-written memory from reading as zeros."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Applies the writes earlier than the next command list; not compared on other titles."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Yoshi and the Mysterious Book demo"),
    },
    {
        .name = "EDEN_NX2_GPU_ACCURACY",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = GPU_ACCURACY_CHOICES,
        .suggested = "high",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Low on Switch 2 titles while the GPU accuracy setting is at its default"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Forces the GPU accuracy of Switch 2 titles, over the setting."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "High may fix a rendering bug that Low causes."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "High cost Mario Kart World ~17 ms a frame (41.7 against 24.5 ms) with no visible "
            "difference."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_GPU_TIMESTAMP",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = GPU_TIMESTAMP_CHOICES,
        .suggested = "ns",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Nanoseconds for Ocarina of Time, GPU ticks for other titles"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The unit of the timestamps in Switch 2 GPU reports."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Nanoseconds raise Ocarina of Time's dynamic resolution."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World paces by them: 52.6 ms a frame and 60% speed with nanoseconds, "
            "against 27.1 ms and 99% with ticks."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time, Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_GPU_WRITTEN_CMDLISTS",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reads the command lists that lie on GPU-written pages through the caches, so their "
            "draws use this frame's commands."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 is faster (16.7 against 24.5 ms a frame in one Mario Kart World test)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 makes trees vanish, and races rendered black."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_INDIRECT_DRAWS",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Draws the indirect draws of Switch 2 draw macros straight from GPU memory, instead "
            "of reading their arguments on the CPU and running the macro."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 waits for all queued GPU work about 8 times a frame, so CPU and GPU stop "
            "overlapping (Mario Kart World took ~28-30 ms a frame)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_INLINE_DEFER",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = INLINE_DEFER_CHOICES,
        .suggested = "2",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Inline GPU memory writes are applied at once, but invalidate the other caches merged "
            "at the next cache flush instead of one write at a time. At 2 the texture cache is "
            "still told at once, so a CPU read or a present before that flush can't undo them or "
            "show the old image."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "2 can fix images that briefly show their old contents after inline writes."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 invalidates every cache for each of thousands of small writes between draws; 2 "
            "takes a texture cache lock per write."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_INLINE_INVALIDATE",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The texture, query and shader caches only hear about inline writes on pages they "
            "held something on when last asked; the buffer cache hears about all of them."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Less GPU thread time per draw (up to about 2% in Donkey Kong Bananza, per Lichii)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Experimental. A miss would show as a stale texture, shader or occlusion result; "
            "Selevet lists it among the switches to remove if objects flicker."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_INVALIDATE_BLOCKS",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Invalidation ranges cover whole 32-byte blocks; the old rounding could end a range "
            "short of the write."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 can leave stale data after a write."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_INVALIDATE_DEDUP",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips an invalidation range that lies inside one of the last few already waiting."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 keeps the duplicates: more invalidation work."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_MACRO_COPY",
        .category = Category::GpuCommands,
        .type = ValueType::Presence,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Always passes macro parameters through a growing list instead of a stack copy."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None; for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "About 2% of the GPU thread."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_MME_JIT",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Runs Switch 2 GPU macros compiled to x86-64 code instead of through the macro memo "
            "and the interpreter (x86-64 CPUs only)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 rules the compiler out when a macro bug is suspected."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 makes each call ~18% slower (174 against 148 ns) and also takes the macros off "
            "the split's first thread."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_MME_MEMO",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Replays the writes of repeated macro calls instead of interpreting them again (when "
            "the compiler is off, and for compute macros)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 rules the memo out when a macro bug is suspected."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 interprets every call: a slower GPU thread."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PARK_SPLIT_METHODS",
        .category = Category::GpuCommands,
        .type = ValueType::Presence,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Same as EDEN_NX2_SPLIT_METHODS=park, and wins over it."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "See EDEN_NX2_SPLIT_METHODS."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "See EDEN_NX2_SPLIT_METHODS."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PBFLUSH",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = PBFLUSH_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Flushes small GPU-written parameter blocks (64 words or fewer) of macro and compute "
            "calls before reading them from command lists. With the split, 1 waits for the second "
            "thread only for blocks on pages GPU work has written; 2 waits for every block."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Parameters the GPU has just written are read current."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Extra flushes on the GPU thread; 2 also waits for the second thread at every block."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PB_RESNAPSHOT",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = PB_RESNAPSHOT_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "After a channel waited on a semaphore, copies its command lists from memory again, "
            "since the game may have rewritten them in the meantime."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None expected; lower values are for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lower values brought back freezes from stale commands (Donkey Kong Bananza's first "
            "cave, Hyrule Warriors fights)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PB_SNAPSHOT",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Copies Switch 2 command lists when the game submits them, instead of reading them "
            "from memory when they run (unless a GPU map is still queued for them)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With 0, the end of a frame can run with the next frame's commands and show an image "
            "three frames old."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PENDING_WAKE",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "While GPU channels wait on semaphores, the GPU thread waits on its command queue "
            "instead of sleeping a fixed millisecond between checks."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 adds up to a millisecond to each such wait."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PERF_OFF",
        .category = Category::GpuCommands,
        .type = ValueType::Text,
        .suggested = "method,cb_batch",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Comma-separated list of the GPU thread optimizations from Lichii to turn off: method "
            "(faster 3D method dispatch on the second thread), cb_batch (uniform words uploaded "
            "in one write), index_rebind (repeated index buffer binds skipped), usage_tracker "
            "(buffer usage pages not filled again), sorted_page_batches (sorted page runs not "
            "sorted again), unswizzle_workers (large textures with an odd row size unswizzled half "
            "on a texture thread), ssbo_bindings (storage buffer bindings kept until the memory "
            "map changes), native_bank_upload (native 64 KiB uniform banks uploaded again without "
            "unprotecting their pages). Lichii's names for its other optimizations are accepted "
            "and ignored."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None expected: they give the same results. For finding one at fault."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Each one off costs some GPU thread time."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PERF_ON",
        .category = Category::GpuCommands,
        .type = ValueType::Text,
        .suggested = "fragment_barrier",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Comma-separated list of the GPU thread optimizations from Lichii that are off by "
            "default to turn on. fragment_barrier: a fragment barrier ends the render pass only "
            "at the next draw that reads one of the render targets, or at any draw once the pass "
            "stored to memory. async_pipelines: a new pipeline of a Switch 2 title has its shader "
            "code generated by the pipeline build (a builder thread, or the thread that records "
            "the draw) instead of the GPU thread. split_overlap: as EDEN_NX2_DMA_RANGES=1."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer render passes and less GPU work (Ocarina of Time ended about 5,900 a second at "
            "its fragment barriers, per Lichii). Shorter GPU thread stalls when new pipelines "
            "are built."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Experimental. fragment_barrier: a draw that reads earlier output some other way "
            "would see it old (flicker or stale effects). async_pipelines: with asynchronous "
            "shaders on, more draws may be skipped while their pipelines build."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "The Legend of Zelda: Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_PERF_OPT",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 turns off every GPU thread optimization from Lichii (see EDEN_NX2_PERF_OFF and "
            "EDEN_NX2_PERF_ON), including any EDEN_NX2_PERF_ON names."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 costs GPU thread time."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_RESETFIX",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Does the stores of Donkey Kong Bananza's terrain reset program on the CPU: the "
            "recompiler loses them, so every rock and dirt piece kept zero copies and wasn't "
            "drawn."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 brings back the missing terrain."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Donkey Kong Bananza"),
    },
    {
        .name = "EDEN_NX2_RESETFIX_EARLY",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Does the terrain reset of EDEN_NX2_RESETFIX before the pipeline lookup instead of "
            "inside draw setup."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 uses the older, slower checks."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Donkey Kong Bananza"),
    },
    {
        .name = "EDEN_NX2_SKIP_SAME",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops 3D state register writes that keep the value the register already holds "
            "(about 60% of them in Mario Kart World)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 makes macro calls and draw setup a little slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SPLIT",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = SPLIT_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2 on Switch 2 titles"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Splits the GPU thread of Switch 2 titles in two: the first parses command lists "
            "(and, at 2, runs the 3D and compute macros), the second runs the engine methods and "
            "records draws."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 or 1 rule the split out when a GPU bug might come from it."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "At 2, Mario Kart World drew ~1.46x more per second with ~17% shorter frames; 0 "
            "gives that up and 1 part of it."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SPLIT_METHODS",
        .category = Category::GpuCommands,
        .type = ValueType::Choice,
        .choices = SPLIT_METHODS_CHOICES,
        .suggested = "park",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "continue (auto for the Yoshi demo)"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "What to do when a GPU method's data goes on in a command list entry that starts "
            "elsewhere: continue there, park it until the rest arrives, or let a heuristic "
            "decide. Setting it applies to every title, the Yoshi demo included."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "park or auto may fix misread commands in a title that splits methods that way."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Both are experimental and can stall the command stream."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SPLIT_SKIP",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the GPU thread split, state writes that keep a register's value are not sent "
            "to the second thread."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 sends ~55% more 3D state writes through the ring."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_TITLE_DEFAULTS",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Gives switches the defaults some titles have instead of the global ones (Ocarina of "
            "Time: GPU_TIMESTAMP=ns, NATIVE_SSBO_MAPPED=1, NATIVE_UBO_REUPLOAD=1; Mario Kart "
            "World: NATIVE_SSBO_MAPPED=0). A switch you set still wins."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; 0 runs a title with the global defaults, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brings back what those defaults avoid: in Ocarina of Time a lower dynamic "
            "resolution, draws reading past their bindings and flashes; Mario Kart World took "
            "28-31 ms a frame instead of 21-22."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time, Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_ZPASS_SUBMIT_RESET",
        .category = Category::GpuCommands,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Resets the occlusion sample counter at each GPU submission instead of pausing it."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops the samples counted between a game's counter reset and its report when a "
            "submission lands in between."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_ASID_RESTORE_LEGACY",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps comparing against the first page's address space after a cross-process run, "
            "as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Hands the pages after such a run to the wrong process."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_BUFFER_DOWNLOADS",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Off on Switch 2 titles, on for others"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Copies every GPU-written buffer range back to guest memory at the next fence."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 on a Switch 2 title may fix data the CPU reads back stale, if the read traps miss "
            "it."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "In Mario Kart World it copied 250-330 MB/s back: fences took ~5 µs instead of ~1, "
            "and draw setup ~7% longer."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_BUFFER_SKIP",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps each vertex stream's buffer lookup while its registers, the page table and the "
            "buffers stay the same, skips the upload check of vertex buffers no CPU write reached, "
            "and touches each buffer's LRU entry once a frame."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Less GPU thread time per draw (vertex binding took 12% of it in Donkey Kong "
            "Bananza, per Lichii)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Experimental. A missed CPU write would leave stale vertices (stretched or flickering "
            "geometry); EDEN_NX2_FAST_DRAW_VERIFY=1 counts them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_COMMIT_ALL",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Commits all emulated memory at start instead of as the game touches it (Windows)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "No commit failure later in a session."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Needs up to 20 GiB of free commit (RAM plus page file) at boot."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_DEVICE_RANGE_CHECK",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Destroyed Switch 2 GPU memory releases its device ranges, and a CPU range mapped "
            "again reuses its device range only while it still reaches the same pages."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Stops a stale device range from being reused for memory mapped again."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Not compared yet on the titles it touches, so it stays off by default."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_EXACT_PREFLUSH",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Of a GPU write that covers pages the CPU has read, only those pages are queued for "
            "download at fences."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 copies whole buffers back at every fence (gigabytes a second with Mario Kart "
            "World's compute bindings)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_FENCE_RANGE_MERGE",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Merges the GPU-modified range sets of committed fences instead of subtracting each "
            "from the earlier ones."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 does more range work at every fence."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_GPU_ONLY_LAZY",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Backs Switch 2 GPU-only memory regions with host memory as they are used, instead "
            "of whole."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 commits the whole regions: much more RAM."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_GPU_ONLY_ZERO_SKIP",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Leaves zero writes to GPU-only regions without host memory."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Less memory committed for regions that are only cleared."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Opt-in and little measured."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_HELD_WRITES",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Guest CPU writes to GPU-written memory are applied before every submitted command "
            "list, copying only the written bytes."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 applies them only at fences, marking whole pages CPU-modified and uploading them "
            "over GPU writes that were never copied back."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_HELD_WRITE_BATCH",
        .category = Category::BufferCache,
        .type = ValueType::Choice,
        .choices = HELD_WRITE_BATCH_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Applies the held writes of one cache invalidation as one batch, each cache locked "
            "once for all of them; 1 also joins touching writes within a 4 KiB page."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer lock handoffs that wake guest CPU threads."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Not measured yet."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_LAYOUT_CACHE",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps the GPU memory kind run that the last storage buffer layout query fell in, "
            "instead of locking and walking the map for every buffer of every draw."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 is slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_LOW_COMMIT_WARNING_18G",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .flags = GuiOnly,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Warns about low virtual memory when less than 18 GiB can be committed instead of 12 "
            "GiB (Switch 2 titles)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "An earlier warning on machines that run out."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The old threshold warned on normal boots."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_NATIVE_SSBO_MAPPED",
        .category = Category::BufferCache,
        .type = ValueType::Choice,
        .choices = NATIVE_SSBO_MAPPED_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 for Ocarina of Time, 0 for Mario Kart World, w for other titles"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Which native storage buffer bindings are sized from their memory mapping (up to 8 "
            "MiB) instead of the size word after their pointer."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mapped bindings keep writes and reads past the stated size (Donkey Kong Bananza's "
            "command lists, Ocarina of Time's draws)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mapping them all costs Mario Kart World and Donkey Kong Bananza about a third of "
            "their frame rate (Mario Kart World: 28-31 against 21-22 ms a frame)."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Ocarina of Time, Mario Kart World, Donkey Kong Bananza"),
    },
    {
        .name = "EDEN_NX2_NATIVE_SSBO_WRITTEN_SIZE",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .flags = NotInThisBuild,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "When the word after a native binding's pointer reads as a smaller size, only that "
            "much is marked GPU-written."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 marks all of it, which also covered CPU-written data past the buffer (Pokopia "
            "lost the GPU device within a minute)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Pokopia"),
    },
    {
        .name = "EDEN_NX2_NATIVE_UBO_REUPLOAD",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "On for Ocarina of Time, off for other titles"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Uploads native 64 KiB uniform banks again once per frame, in case writes to them "
            "went unnoticed. 1 turns it on for every title, any other value off, Ocarina of Time "
            "included."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "May fix stale data and flicker (the corners of Splatoon Raiders' dialogue box)."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "An extra upload per bank and frame."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time, Splatoon Raiders"),
    },
    {
        .name = "EDEN_NX2_PRIVATE_DEVICE_RANGES",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Each GPU address space of a Switch 2 game maps memory outside the heap at a device "
            "range of its own, as before, instead of one range shared by the process. On with "
            "EDEN_NX2_DEVICE_RANGE_CHECK=1."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog", "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A game with two address spaces can draw into one copy of the memory while the screen "
            "shows another (Fortune's Weave stayed black in Eglantine)."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Switch 2 titles with two GPU address spaces (Welcome Tour, Yoshi demo, Fortune's "
            "Weave)"),
    },
    {
        .name = "EDEN_NX2_REACTIVE_FLUSHING",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Off on Switch 2 titles unless set per game"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Forces reactive flushing for Switch 2 titles, over the per-game setting."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 keeps every GPU-cached page CPU-visible: a fallback when the read traps miss "
            "something."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 read-protects every cached page (~12% of Mario Kart World's main core went to "
            "slow reads) and turns the read traps off."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_READ_AHEAD",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "When a guest thread scans GPU-written buffers page by page, each trap also flushes "
            "a growing window ahead of it (up to 1 MiB)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer waits for the GPU thread in such scans."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Flushes data that may never be read."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_READ_FLUSH_HELD_WRITES",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Applies the held writes before a guest CPU read flushes a range, so the download "
            "can't put the GPU's older copy over them."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Can fix stale data the CPU reads back."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Not compared yet on the titles it can change, so it stays off by default."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World, Donkey Kong Bananza, Pokopia"),
    },
    {
        .name = "EDEN_NX2_READ_TRAPS",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Takes read permission off pages the GPU wrote and doesn't copy back, so the guest's "
            "first read flushes them (fastmem only; not under NCE or with reactive flushing)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 sent 8x fewer GPU submissions in one test."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 leaves animated colliders at their default pose in Mario Kart World (Dino Dino "
            "Jungle's dino, DK Spaceport, Bowser's Castle)."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "All Switch 2 titles, Mario Kart World above all"),
    },
    {
        .name = "EDEN_NX2_RELEASE_GUEST_RAM",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Frees the guest RAM (every page the game touched, up to 12 GiB) when a Switch 2 game "
            "stops, instead of keeping it for the next game."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 keeps that memory until the next game reuses it or Nemyu closes."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_RUN_CACHE",
        .category = Category::BufferCache,
        .type = ValueType::Choice,
        .choices = RUN_CACHE_CHOICES,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How the GPU memory manager remembers how far the mapping runs on from an address, "
            "asked at every native storage buffer bind (about 90,000 times a second in Ocarina of "
            "Time's Kokiri village)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None expected; 0 and 2 are for comparisons and checks."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 misses at every new address; 2 walks the page tables for every answer."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SELECTED_WRITE_BOUND",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A storage buffer written only through addresses picked at run time is marked "
            "GPU-written up to the next buffer of the same program instead of over its whole span "
            "(up to 8 MiB)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer copies back to guest memory at fences (Ocarina of Time: 289 MB/s, per "
            "Lichii)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Experimental. A write past the bound would not reach the CPU: missing vegetation or "
            "objects that read GPU results."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_SPARSE_LAZY",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Backs Switch 2 sparse memory regions with host memory as they are used, instead of "
            "whole."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 commits the whole regions: much more RAM."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SPARSE_REMAP",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reports remap support to the Switch 2 GPU driver, so titles can map and unmap "
            "sparse texture tiles."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 makes Mario Kart World stream its terrain into unmapped sparse memory."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_STAGING_LEGACY",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Staging buffers with power-of-two sizes, no reuse of larger ones, and one size "
            "class checked per frame, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "More staging memory and allocations."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_STREAM_BUFFER_256",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps the 256 MiB stream buffer on GPUs with 7 GiB or more instead of 128 MiB."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Room for titles that stream more data per frame."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "128 MiB more video memory; 256 MiB sat mostly unused on Switch 2 titles."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_UBOREUSE",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reuses an uploaded small uniform buffer when the same data comes again, instead of "
            "copying it every time."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 copies every small uniform buffer."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_UPLOAD_BARRIER",
        .category = Category::BufferCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Records a barrier after mapped buffer uploads anyway; they carry their own, and "
            "this one only ended the render pass."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "More render pass splits."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_ASTC_ARRAY_MIB",
        .category = Category::TextureCache,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 65536,
        .suggested = "512",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "512 MiB, 256 MiB on GPUs with less than 7 GiB"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The most a trimmed ASTC texture array keeps on the host GPU, in MiB."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Higher keeps more terrain detail resident on large GPUs."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World's two terrain arrays took 724 MiB of a 6 GB card."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_ASTC_BC1",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "When ASTC textures are recompressed to BC3, those without alpha (about 70% of them) "
            "go to BC1 instead."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 keeps a little more quality in opaque textures."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 doubles their memory: more evictions and decodes (Mario Kart World's attract "
            "loop: 17 against 22-23 fps)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_ASTC_FLUSH",
        .category = Category::TextureCache,
        .type = ValueType::Choice,
        .choices = ASTC_FLUSH_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "No wait"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "What a GPU ASTC decode does once recorded: nothing (a barrier orders later uses), "
            "submit, or wait for the host GPU to go idle."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 or 0 rule out an ordering problem in texture decodes."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 stalls the GPU thread at every decode; 1 adds submissions."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_ASTC_VIEW_LEGACY",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lists the A8B8G8R8 view for recompressed ASTC images and decodes ASTC for the BC "
            "encoder as A8B8G8R8, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Brings back the old format handling."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_CLEAR_SKIP_UPLOAD",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips uploading a new render target's guest data when a clear overwrites the whole "
            "image."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 uploads data the clear throws away."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_FB_LAYERS_MAX",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Gives framebuffers the most layers of their attachments and clears the requested "
            "layers unclipped, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Clears can write past the framebuffer's layers."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_FORCE_LOD0",
        .category = Category::TextureCache,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Samples only the top mip level of every texture."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shows whether a glitch comes from the mip layout."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Shimmering, aliased textures."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_IGPU_HEAP_BUDGET",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "On integrated GPUs, follows the memory heap's budget instead of capping it at the "
            "share of RAM the integrated GPU gets (4 GiB, 6 GiB with aggressive VRAM use)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "More texture memory on APUs with plenty of RAM."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The texture cache can fill that much RAM before it collects, and ASTC stays "
            "uncompressed."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles on integrated GPUs"),
    },
    {
        .name = "EDEN_NX2_KEEP_ASTC",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps ASTC textures uncompressed on Switch 2 titles when ASTC recompression is set "
            "to Uncompressed and the GPU has less than 12 GiB, instead of recompressing them to "
            "BC3."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Full texture quality."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Uncompressed ASTC is 4x larger: running out of video memory evicts and decodes "
            "textures again (stutter, pop-in)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_NULL_VIEW_RESCALE_LEGACY",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Asks buffer and null image views whether their image is rescaled, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Those views have no image to ask."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_SPARSE_UNBACKED",
        .category = Category::TextureCache,
        .type = ValueType::Choice,
        .choices = SPARSE_UNBACKED_CHOICES,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "On, except on AMD drivers and RADV"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Whether images with no memory bound may be sampled and used as attachments."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 backs large images normally, a fix to try for GPU hangs on other drivers too."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 on AMD hangs the GPU shortly after the first such draw; 0 uses much more video "
            "memory."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_STORAGE_VIEW_BY_SIGN",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keys storage image views by signedness only, as before, instead of one view per "
            "format."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "An R32_UINT view was returned for a later R16_UINT or R32G32_UINT store."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TC_CRITICAL_MIB",
        .category = Category::TextureCache,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 1048576,
        .suggested = "6144",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Computed from the GPU's memory"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The texture cache's critical usage in MiB: above it, eviction turns aggressive."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Tunes eviction for a card the defaults misjudge."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Too high runs out of video memory; too low decodes textures again and again."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TC_EXPECTED_MIB",
        .category = Category::TextureCache,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 1048576,
        .suggested = "4096",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Computed from the GPU's memory"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The texture cache's expected usage in MiB: above it, unused images are evicted "
            "sooner."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Tunes eviction for a card the defaults misjudge."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Too high runs out of video memory; too low decodes textures again and again."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TC_MIN_AGE",
        .category = Category::TextureCache,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 100000,
        .suggested = "120",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 frames"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The texture cache never evicts images used within this many frames, while video "
            "memory allows: near its limit the age drops to 120 frames, and at the limit the "
            "cache evicts as usual."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Can stop eviction churn on a card near its limit."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps more images in video memory; 120 made no difference in Mario Kart World tests."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TC_SLACK",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lowers the texture cache thresholds by the video memory the allocator holds beyond "
            "the driver's figure."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Stays within budget on cards where allocations leave holes."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "On a 6 GB card it evicted 2-4x more costly images and frames were slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TRANSCODE_POOL",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reuses scratch images for texture transcodes instead of creating one each time."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 creates an image for every transcode."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TRANSCODE_THREADS",
        .category = Category::TextureCache,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 256,
        .suggested = "4",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A quarter of the CPU threads, at least 2"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How many threads decode and transcode textures on the CPU."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "More threads bring streamed textures in sooner."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Half the CPU threads took ~3.5 cores from the emulated CPU and the GPU thread on an "
            "8-core laptop; 2 left textures seconds late."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_UPLOAD_READ_BOUNDS",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Accelerated texture uploads read only the image's size from a reused staging "
            "buffer, not its whole capacity."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 reads past the texture data."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_VALID_ENTRY_LEGACY",
        .category = Category::TextureCache,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Sizes the memory check of native texture descriptors from the raw descriptor, as "
            "before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Legacy layer bits oversize the range, and buffers and 3D images assert."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_VRAM_LIMIT_MIB",
        .category = Category::TextureCache,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 1,
        .maximum = 1048576,
        .suggested = "6144",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Not set: the real size"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Treats every device-local memory heap as this many MiB and fails allocations past "
            "it, like a smaller card."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Tests how a title behaves on smaller GPUs."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Less video memory for everything."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_AMD_PARALLEL_PIPELINES",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps parallel pipeline compilation on AMD's Windows driver for Switch 2 titles."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Faster pipeline builds."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The driver can crash compiling pipelines at the same time (Donkey Kong Bananza on "
            "RDNA1/2)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Switch 2 titles on AMD GPUs under Windows"),
    },
    {
        .name = "EDEN_NX2_AMD_SHUFFLE_LOOP",
        .category = Category::Shaders,
        .type = ValueType::Choice,
        .choices = AMD_SHUFFLE_LOOP_CHOICES,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog", "Unset: on AMD's Windows driver and AMDVLK only"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Replaces warp shuffles from a lane computed at run time in vertex, geometry and "
            "fragment shaders with a loop of broadcasts."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Avoids the AMD driver crash while building such pipelines (Ocarina of Time goes "
            "black, then closes, after the intro)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Slower shaders where they shuffle; 0 brings the crash back on AMD."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Switch 2 titles on AMD GPUs"),
    },
    {
        .name = "EDEN_NX2_ASYNC_SKIP",
        .category = Category::Shaders,
        .type = ValueType::Choice,
        .choices = ASYNC_SKIP_CHOICES,
        .suggested = "0",
        .default_value =
            QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Unset: as EDEN_NX2_NATIVE_SYNC_PIPELINES"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Selevet's switch, read when EDEN_NX2_NATIVE_SYNC_PIPELINES is unset: 0 always waits "
            "for a native pipeline (full); 1 skips depth-tested draws and draws of more than six "
            "vertices or indices while their pipeline builds, and waits for the rest."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lets a Selevet nx2_switches.txt work unchanged."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 stutters on new pipelines; 1 leaves objects missing for a few frames."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_BUILD_WAITED_PIPELINES",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A draw or dispatch that waits for a queued pipeline builds it on the spot instead "
            "of waiting for its turn on the builder threads."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 can wait behind hundreds of queued builds."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_DLSS_AUTO_EXPOSURE",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "NVIDIA DLSS works out the exposure of the game's colour itself. Only builds with "
            "NVIDIA DLSS, with Use NVIDIA DLSS in place of a game's DLSS on."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 gives DLSS no exposure for colour that goes above 1."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fire Emblem: Fortune's Weave"),
    },
    {
        .name = "EDEN_NX2_DLSS_HDR",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "NVIDIA DLSS takes the game's colour as HDR (linear, above 1). Only builds with "
            "NVIDIA DLSS, with Use NVIDIA DLSS in place of a game's DLSS on."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 treats Fire Emblem's linear colour as display colour."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fire Emblem: Fortune's Weave"),
    },
    {
        .name = "EDEN_NX2_DLSS_PRESET",
        .category = Category::Shaders,
        .type = ValueType::Choice,
        .choices = DLSS_PRESET_CHOICES,
        .suggested = "default",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "K"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The model NVIDIA DLSS uses in every mode. Only builds with NVIDIA DLSS, with Use "
            "NVIDIA DLSS in place of a game's DLSS on."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Another model may look better; default leaves the choice to NVIDIA."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "NVIDIA's defaults for Performance and Ultra Performance (M, L) drew thin horizontal "
            "lines in Fire Emblem at 2x resolution scale."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fire Emblem: Fortune's Weave"),
    },
    {
        .name = "EDEN_NX2_DYNAMIC_STATE_LOOKUP",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Looks a draw's pipeline up again after setting its dynamic states, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Rebuilds the whole pipeline key on every draw with dynamic vertex input."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_DYNAMIC_SKIP",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A draw with none of the dirty flags that gate the dynamic state updates skips them. "
            "Never on AMD drivers, whose logic-op workaround changes a register every draw."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A little less GPU thread time per draw (dynamic states took 3% of it in Donkey Kong "
            "Bananza, per Lichii)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Experimental. A missed state would show as wrong culling, depth or blending; "
            "EDEN_NX2_FAST_DRAW_VERIFY=1 counts them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_EVICT_PIPELINES",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Always evicts unused Switch 2 pipelines under memory pressure, instead of keeping "
            "them while Windows still has 8 GiB or more to commit."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Less memory."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Coming back to a place builds its pipelines and translates their shaders again: "
            "hitches."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_EXPERIMENTAL_GEOMETRY",
        .category = Category::Shaders,
        .type = ValueType::Choice,
        .choices = EXPERIMENTAL_GEOMETRY_CHOICES,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Pass-through programs only"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Which native Switch 2 geometry programs are translated. Pass-through ones only "
            "forward vertices and set per-primitive state."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1 may draw geometry that is missing."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 is experimental and can break draws; 0 drops the pass-through ones."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_FORCE_SUBGROUP_ALL",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Requires the guest subgroup size for every compute pipeline, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Constrains pipelines that don't need it."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_GAME_DLSS",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .flags = Diagnostic,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Off: Fire Emblem: Fortune's Weave only"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Follows a game's DLSS evaluation in every title, not only in Fire Emblem: Fortune's "
            "Weave, the only one whose layout is known."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Looks for the same DLSS layout in other titles."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "One comparison more per compute dispatch."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_GENERIC_OUTPUT_GUARD",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops shader stores to output components that have no output variable, instead of "
            "emitting an invalid id."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brings back invalid shaders, such as Donkey Kong Bananza's geometry program on "
            "AMD."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_GS_PASSTHROUGH_KEEP_FLAGS",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps the NV pass-through flags on geometry programs lowered to emit their own "
            "vertices, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The pipeline then sees a geometry stage and pass-through decorations at once."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_GS_PRUNE_OUTPUTS",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops the generic outputs the fragment shader never reads from a geometry shader "
            "that outputs more components than the device allows."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brings back pipelines over the limit, which AMD's Windows driver closes the "
            "emulator on (Mario Tennis Fever)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_INTEL_CONDITIONAL_BARRIERS",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps shader barriers inside branches and loops on Intel's Windows driver, which "
            "otherwise drops them as a driver workaround."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Compute passes that share data between warps stop reading stale shared memory "
            "(Mario Kart World's lighting on Mesa had this)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The workaround exists because the driver mishandled such barriers; it may hang or "
            "crash on some driver versions. Intel on Windows only."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_LOWER_EXTENDED",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lowers Ocarina of Time's indexed byte constant loads, ULEA carries past UP1, "
            "two-source F2IP.U8 and I2F.U8 of a uniform byte instead of rejecting the program."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0: Eglantine measured ~8 ms less work on the GPU thread per 50 ms frame without "
            "these forms."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 skips the compute programs that use them every frame (LOD vegetation)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "The Legend of Zelda: Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_MESH_PUSH_LEGACY",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Leaves the task and mesh stages out of graphics push constants, and scales the null "
            "image for buffer views, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mesh shaders lose their rescaling and render area data."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_NATIVE_SYNC_BUDGET_US",
        .category = Category::Shaders,
        .type = ValueType::Number,
        .minimum = 0,
        .maximum = 1000000,
        .suggested = "3000",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "3000 µs"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "In budget mode, how long a draw waits for its pipeline, in microseconds; a frame "
            "waits 8 ms at most in total."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Higher draws more on the first frame."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Higher stutters longer on new pipelines."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_NATIVE_SYNC_PIPELINES",
        .category = Category::Shaders,
        .type = ValueType::Choice,
        .choices = NATIVE_SYNC_PIPELINES_CHOICES,
        .suggested = "full",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "budget"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Whether a draw waits for its native pipeline to finish building: briefly (budget), "
            "always (full), or never (the draw is skipped until it is ready)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "full draws everything on its first frame; 0 never stalls."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "full stutters on new pipelines; 0 leaves objects missing for a few frames."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_NO_MESH",
        .category = Category::Shaders,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips task and mesh shader draws, as before they were supported (over the mesh "
            "shader setting)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A/B comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World's demo stalls on the world map without them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_NO_MESH_FILE",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips task and mesh draws while this file exists (checked every 1024 draws), to "
            "compare one scene with and without them."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A/B comparisons in one run."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World's demo stalls on the world map without them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_NO_TENSOR",
        .category = Category::Shaders,
        .type = ValueType::Presence,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Leaves tensor-core matrix multiplies (HMMA) unimplemented: programs that use them "
            "are rejected, and upscalers fall back to a bilinear blit."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Avoids long shader compiles and slow frames from emulated neural upscalers."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lower-quality upscaling where a title uses them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_OOT_BLOOM_EXP_SCALE",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .suggested = "1.6",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1 (unchanged)"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the Ocarina of Time visual workaround, scales the bloom constant of the final "
            "pass; higher is weaker bloom. Values outside 0 to 16 keep the default."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Less glare (Selevet ships 1.6)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A tuning of the workaround, not a fix; too high removes bloom the game has."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_OOT_EXPOSURE_SCALE",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .suggested = "0.55",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0.65"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the Ocarina of Time visual workaround, scales the exposure before the filmic "
            "curve; lower is darker. Values outside 0 to 16 keep the default."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Less glare (Selevet ships 0.55)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A tuning of the workaround, not a fix; too low darkens the whole image."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_OOT_FOG_SCALE",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .suggested = "0.12",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0.25"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the Ocarina of Time visual workaround, scales the fog slice thickness; lower is "
            "less haze. Values outside 0 to 16 keep the default."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Less haze (Selevet ships 0.12)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A tuning of the workaround, not a fix; too low removes fog the game has."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_OOT_NETWORK",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off: a plain upscale stands in"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Emulates Ocarina of Time's gameplay upscaler, a tensor-core network of 768 matrix "
            "multiplies, instead of drawing a plain upscale of its input."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The game's own reconstruction of detail and anti-aliasing."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "GPU time every frame: minutes to compile and about 8 FPS on Mesa's Intel driver."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_OOT_VISUAL_WORKAROUND",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Temporary workaround, not a fix: thins Ocarina of Time's outdoor fog and lowers its "
            "bloom exposure, tuned against the console, while the cause of the haze is open."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 shows the game as emulated, to compare."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brings back outdoor areas far hazier than on the console."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Ocarina of Time"),
    },
    {
        .name = "EDEN_NX2_PRELOAD_ALL_PIPELINES",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Builds every cached pipeline at boot instead of only the ones used in recent "
            "sessions."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fewer build hitches in places not visited lately."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Slower boot and more memory; a boot that ran out of memory never saved its list."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_PRMT",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Translates the PRMT byte permute instruction in its default mode."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 leaves every PRMT unimplemented, so those shaders fail."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PTP_RUNTIME_OFFSETS",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Native shaders whose per-tap texture gather offsets can't be proven constant read "
            "them at run time instead of being rejected."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 rejects those programs, so their draws are skipped."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_REJECT",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Treats the native programs at these hex addresses (comma separated) as rejected."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Finds which newly translated program changes a game, without rebuilding."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Those draws and dispatches are skipped."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SKIP_VS",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops draws that use the vertex shaders at these hex addresses (comma separated)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Isolates which draw causes a glitch."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Those objects disappear."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SPIRV_CAPS_LEGACY",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Declares PrimitiveId and draw parameters without their SPIR-V capabilities, and "
            "enables quad control without maximal reconvergence, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shaders that spirv-val rejects, which drivers may miscompile."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_UNDEF_ZERO",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shader registers read before any write are zero, as on NVIDIA hardware, instead of "
            "undefined."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 brings back green patches on Mario Kart World's roads on AMD."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_USED_LIST_MERGE",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Merges short sessions into the used-pipeline list instead of starting a new "
            "two-session window at every save."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 can forget pipelines after a few short sessions."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_VIDS",
        .category = Category::Shaders,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps VK_EXT_vertex_input_dynamic_state on NVIDIA and Intel Windows drivers for "
            "Switch 2 titles."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fewer pipelines."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Per-draw setup cost more with it in Mario Kart World on an RTX 3050."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Switch 2 titles on NVIDIA and Intel GPUs under Windows"),
    },
    {
        .name = "EDEN_STREAMLINE_DIR",
        .category = Category::Shaders,
        .type = ValueType::Text,
        .flags = EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Not set: the streamline folders"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Folder searched first for NVIDIA Streamline's files (sl.interposer.dll and its "
            "plugins), before the streamline folder of the data folder and the one next to the "
            "executable. Only builds with NVIDIA DLSS read it."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps NVIDIA's files in one place for several installs."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fire Emblem: Fortune's Weave"),
    },
    {
        .name = "EDEN_CPU_ACCURATE_FLOAT",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With CPU accuracy on Unsafe or Auto, drops the reduced-error, NaN and FPCR "
            "floating point shortcuts, as Accurate does (Selevet's switch)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Accurate floating point results while the rest of the CPU stays fast."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Slower floating point code in the guest."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_CPU_FUSED_FMA",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With CPU accuracy on Unsafe or Auto, keeps fused multiply-adds fused instead of "
            "splitting them (Selevet's switch)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Bit-exact multiply-add results, which physics and animation code can depend on."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Slower on CPUs without FMA."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_CPU_GLOBAL_MONITOR",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With CPU accuracy on Unsafe or Auto, keeps exclusive memory accesses checked across "
            "cores instead of ignoring the global monitor (Selevet's switch)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lock-free hand-offs between guest threads fail when they should, which can fix rare "
            "hangs and corruption."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Slower exclusive loads and stores."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_CPU_FASTMEM_CHECK",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With CPU accuracy on Unsafe or Auto, keeps the bounds check on fast memory accesses "
            "(Selevet's switch)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Out-of-range guest accesses are caught instead of reaching host memory."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A little slower memory access in the guest."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_CPU_SETS",
        .category = Category::CpuTiming,
        .type = ValueType::Choice,
        .flags = EnvironmentOnly,
        .choices = CPU_SETS_CHOICES,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Hybrid CPUs (P- and E-cores) only"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Places the emulator's threads on the performance or efficiency cores of hybrid CPUs "
            "(Windows)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 lets Windows schedule freely; 1 places threads even with few P-cores."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Threads that land on E-cores run slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_AVX512",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lets the CPU JIT use AVX-512 code paths."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Possibly faster guest code on CPUs with AVX-512."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Zen 4 players crashed in Mario Kart World shortly after boot, and one path gave "
            "wrong results."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles on CPUs with AVX-512"),
    },
    {
        .name = "EDEN_NX2_CNTFRQ",
        .category = Category::CpuTiming,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 2000000000,
        .suggested = "19200000",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "31,250,000 Hz on Switch 2 titles, 19,200,000 Hz otherwise"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The guest system counter's frequency, in Hz."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A/B tests of timing code."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Switch 2 titles convert ticks at 31.25 MHz: any other value makes their times wrong "
            "(1.63x off at 19.2 MHz)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_CNTPCT_PAUSE",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Pauses the host core for a moment when guest code reads the system counter in a "
            "tight loop (two reads within ~1.7 µs)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 may help a title that polls the counter in timing-critical work."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 lets the polling thread slow down its SMT sibling, often the GPU thread."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_CONTEXT_GUARD_PAUSE",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Pauses the CPU between tries for a thread's context guard in the scheduler."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 retries back to back, starving the SMT sibling."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_DUMMY_WAKE_UNLOCKED",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Wakes host service threads after the global scheduler lock is released, instead of "
            "under it."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 makes every core wait for each wake-up system call."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_EVENT_WAIT_LEGACY",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Brings back the timed event wait from before the MWAITX/UMWAIT fix."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A wake the signal didn't cause returned false early, and the fallback returned true "
            "at the deadline."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_GPU_WAIT_SPIN_US",
        .category = Category::CpuTiming,
        .type = ValueType::Number,
        .minimum = 0,
        .maximum = 100000,
        .suggested = "50",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: no polling"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How long waits for the host GPU, and the Vulkan worker waiting for work, poll "
            "before they sleep, in microseconds."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower wake-up latency."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A busy core while waiting."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_HIGH_PRIORITY",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Runs the emulator at the high process priority class on Windows (Selevet's switch, on "
            "by default in its performance builds 1-6). Read at startup."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Background programs yield the CPU to the emulator, which can help on CPUs with few "
            "cores."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Other programs, the desktop included, get less CPU time while a game runs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_IDLE_SPIN_US",
        .category = Category::CpuTiming,
        .type = ValueType::Number,
        .minimum = 0,
        .maximum = 100000,
        .suggested = "50",
        .default_value = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "100 µs on Switch 2 titles with 8 or more CPU threads, else 0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How long an idle emulated core polls for new work before it blocks, in "
            "microseconds."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lower wake-up latency for job systems (Ocarina of Time: 42 ms frames without, 34 ms "
            "with 100 µs)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Spinning cores take CPU time from the GPU and shader threads, and heat and battery "
            "on laptops; 300 µs was slower again."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_TIMING_DROP_EXPIRED",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops a timing event that expired without being unscheduled."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 can spin forever with both timing locks held: a freeze."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TIMING_WAKE_EARLIEST",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Wakes the timing thread for a new event only when it became the earliest one."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 wakes it for every event."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_TIMING_COARSE",
        .category = Category::CpuTiming,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The timing thread sleeps on the OS through a wait longer than 2 ms (or four timer "
            "ticks) and only waits precisely for the last 1 ms (Selevet's switch)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Frees the core the timing thread kept busy while waiting; can help on CPUs with few "
            "cores or laptops."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "An OS wait that ends late delays timed events: frame pacing or audio could suffer."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_FRAME_GEN_NO_PACING",
        .category = Category::VideoAudio,
        .type = ValueType::Presence,
        .flags = EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Presents generated frames back to back instead of spacing them over the frame "
            "interval."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Uneven frame pacing with frame generation."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles with frame generation"),
    },
    {
        .name = "EDEN_NX2_APPLET_CAPTURE_EVERY_FRAME",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Draws the applet capture of Switch 2 frames at every composite, as before, instead "
            "of only when it is read."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Extra GPU work every frame."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_CAPTURE_SETS",
        .category = Category::VideoAudio,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 8,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "3"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How many sets of layer resources applet captures rotate through (1 to 8)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1 uses a little less memory."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With 1, each capture waits for the host GPU to finish the previous one."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_CROP_HEIGHT",
        .category = Category::VideoAudio,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 16384,
        .suggested = "1080",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Automatic"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Height of the shown region of Switch 2 present images; needs EDEN_NX2_CROP_WIDTH "
            "too."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fixes a title drawn into a corner of its images with black margins."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A wrong size cuts off or pads the picture."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_CROP_WIDTH",
        .category = Category::VideoAudio,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 16384,
        .suggested = "1920",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Automatic"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Width of the shown region of Switch 2 present images; needs EDEN_NX2_CROP_HEIGHT "
            "too."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fixes a title drawn into a corner of its images with black margins."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A wrong size cuts off or pads the picture."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_DECODER_IMAGE_MAP",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A video decoder output image the game maps for the GPU (gsv 426) is mapped at its "
            "own address when no GPU mapping covers its memory yet, and unmapped again with 427, "
            "430, 912 or 112."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The GPU reads the decoded frames instead of unmapped memory (black movies in "
            "Eglantine's Fortune's Weave)."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog", "0 is for comparisons; it brings back the unmapped images."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog", "Switch 2 titles that sample their movies on the GPU"),
    },
    {
        .name = "EDEN_NX2_FG_KEEP_FIFO",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With frame generation, keeps a FIFO VSync mode instead of switching to Mailbox, as "
            "before."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Keeps the VSync mode you picked."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Every generated frame waits for a vblank of its own, and the real frame waits "
            "behind them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles with frame generation"),
    },
    {
        .name = "EDEN_NX2_FRAMES_AHEAD",
        .category = Category::VideoAudio,
        .type = ValueType::Choice,
        .choices = FRAMES_AHEAD_CHOICES,
        .suggested = "auto",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1, but 0 for Donkey Kong Bananza"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Which frame's GPU work a Switch 2 present waits for. Waiting for the previous one "
            "lets the game prepare the next frame while the GPU thread runs this one."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 overlaps CPU and GPU work (8-56 ms of every frame were blocked in Mario Kart "
            "World); 0 or auto fix games that rewrite command memory too early."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 on Donkey Kong Bananza misreads commands: freezes, crashes and rubberbanding. 0 "
            "is slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World, Donkey Kong Bananza"),
    },
    {
        .name = "EDEN_NX2_MKW_FPS_MODE",
        .category = Category::VideoAudio,
        .type = ValueType::Choice,
        .choices = MKW_FPS_MODE_CHOICES,
        .suggested = "dynamic",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "The per-game setting"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Overrides Mario Kart World's frame-rate mode setting, which patches how the game "
            "picks its present interval."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "dynamic keeps the game speed right at uneven frame rates; 30 steadies it on slow "
            "PCs."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "dynamic forced with a CPU overclock or off-speed timing runs the game at the wrong "
            "speed; off is for performance comparisons."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_NV12_LEGACY",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Drops the check that an NV12 row fits the output pitch when decoded frames are "
            "copied."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The scaler can write past rows and past the buffer."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_OPUS_GUEST_STATE",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Brings back the old Opus decoder layout: the libopus state in the guest's work "
            "buffer, sized by the host libopus."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Work buffer sizes then depend on how the host libopus was built."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_PACE_HOST_SLEEP",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Present pacing sleeps the service thread on the host instead of in the guest "
            "kernel."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 frees guest core 3 during the wait, so game threads there keep running (Fortune's "
            "Weave's loader read one RomFS block per frame otherwise)."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 is opt-in and little tested."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Fire Emblem: Fortune's Weave; all Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_PRESENT_LEGACY_ACQUIRE",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Acquires swapchain images as before: with the swapchain's semaphore for the frame "
            "index, no timeout, a scheduler wait on the present thread, and the whole frame "
            "tried again when presenting fails."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Brings back hangs and stalls on the present thread."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_PRESENT_SHARED_QUEUE",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Presents on the graphics queue under the submit lock, as before, instead of on a "
            "second queue."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A fallback for drivers that misbehave with a separate present queue."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A FIFO present waiting for a vblank holds the lock that submissions need."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_PRESENT_WAIT_MS",
        .category = Category::VideoAudio,
        .type = ValueType::Number,
        .minimum = 250,
        .maximum = 600000,
        .suggested = "16000",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "8000 ms"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How long a Switch 2 present may wait while the GPU syncpoint stands still before it "
            "gives up (60 s at most in total)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Higher rides out very long shader compile stalls."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lower lets a game get ahead of the GPU thread: desynced commands and crashes."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_REPORT_VRR",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Makes the Switch 2 SDK report a display with variable refresh rate (VRR) to the game, "
            "by patching its QueryVrrSupported at load (Selevet's switch, on in its performance "
            "builds 4-6)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Games that support VRR time their frames themselves instead of assuming 60 Hz, so "
            "their speed stays steady while the frame rate moves."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A game may take paths it never takes on a TV without VRR."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SWAP_INTERVAL",
        .category = Category::VideoAudio,
        .type = ValueType::Number,
        .minimum = 0,
        .maximum = 4,
        .suggested = "2",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0: the game's own interval"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Holds every Switch 2 present for this many 60 Hz vblanks (but not Mario Kart World "
            "with a frame-rate mode)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "2 caps a title at 30 fps; 1 brings back one vblank per present."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A wrong value runs a game too fast or too slow: 1 on a 30 fps game doubles its "
            "speed."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_VDEC_EOS_RELEASE_NOW",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "After a movie's end of stream, a frame release completes at once instead of staying "
            "pending until Stop or Finalize."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World waits for those fences after its title movie."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Donkey Kong Bananza acquires a frame for every completed fence and aborts on an "
            "empty ring."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World, Donkey Kong Bananza"),
    },
    {
        .name = "EDEN_NX2_VDEC_FRAME_THREADS",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Decodes Switch 2 movies with frame and slice threads (4) instead of one thread."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 may help on CPUs with few cores."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 decoded 2560x1440 VP9 movies below 60 fps, and players abort when no frame is "
            "ready."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_VIDEO_READY_GATE",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Stops signalling the video decoder's ready event after Stop and Finalize, and "
            "answers Start and Finalize with their real results."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 brings back the old replies."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_VOICE_DROP",
        .category = Category::VideoAudio,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Switch 2 titles that ask for it drop audio voices when the audio renderer goes over "
            "its time budget, as Switch 1 titles do."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Bounds the audio renderer's time when it runs late."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Sounds can cut out while the renderer is behind."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_VSYNC_HZ",
        .category = Category::VideoAudio,
        .type = ValueType::Number,
        .minimum = 1,
        .maximum = 1000,
        .suggested = "120",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "60 Hz"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The rate of the vsync event Switch 2 titles start their frames on (an experiment)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A frame that misses a tick idles less at higher rates."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Games may run at the wrong speed."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_CLI_RESTART_HANDOFF",
        .category = Category::Services,
        .type = ValueType::Text,
        .flags = EnvironmentOnly | CliOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Not set"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Set by the launcher when nemyu-cli restarts for a game's ExecuteProgram: the new "
            "process reads the handoff file (program, indexes and user channel) and boots that "
            "program."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: the launcher sets it."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Set by hand, a damaged or missing handoff file stops the boot."),
        .games = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Switch 2 titles that launch another program"),
    },
    {
        .name = "EDEN_NX2_AM320_ZERO",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reports screenshots and recording as off in "
            "GetScreenShotAndContinuousRecordingInitiallyEnabled, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The old reply left callers reading stale message words."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_AUTO_STUB",
        .category = Category::Services,
        .type = ValueType::Presence,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Answers every unknown service command with an empty success, like the Auto Stub "
            "setting. Switch 2 titles are always auto-stubbed."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Switch 1 titles get past unimplemented commands."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A stubbed command can make a game misbehave later instead of stopping now."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Switch 1 titles"),
    },
    {
        .name = "EDEN_NX2_AUTO_STUB_ZERO",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Auto-stubbed commands reply with 32 zeroed words instead of the result alone."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 hands callers the request's leftover words as data (a slot mask, a device id)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_EMPTY_SYSTEM_DATA",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Serves an empty archive for Switch 2 system data that Switch 1 firmware doesn't "
            "have."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 fails the request, and titles abort (Nintendo Switch Sports Resort)."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_ERROR_NOTIFIER_CLEAR",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Clears a GPU channel's error notifier buffer when it is installed, like the host "
            "driver."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 leaves old data that NVN reads as a channel error."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_FONT_DATA_LEGACY",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Brings back the old guess at the Switch 2 GetBuiltInFontData layout."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Clamped font types and a wrong reply layout."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_GSV_HOST_THREAD",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Runs the Switch 2 graphics system service on its own host thread instead of on AM's "
            "server."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Presents stop occupying guest core 3."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Opt-in; not used with single-core CPU emulation."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_KEEP_SESSIONS",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps the server's reference to client-owned sessions (video decoders) after the "
            "client closes them, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Each kept a decoder thread, an FFmpeg context and a syncpoint, per movie."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_NFP_INIT_EX",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Implements the Switch 2 amiibo (NFP) commands, InitializeEx and the others from 300 "
            "up."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 leaves it unimplemented."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Switch 2 titles that use amiibo"),
    },
    {
        .name = "EDEN_NX2_SIXAXIS_LEGACY",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reports zeros for the motion sensor's ranges and calibration, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Zero ranges break the guest's sensor fusion: broken motion controls."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_STREAMPLAY_300",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Answers GameShare (stream play) user service command 300 with guest HID shared "
            "memory."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Experimental GameShare support."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Donkey Kong Bananza runs with the auto stub's reply instead."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "GameShare titles, Donkey Kong Bananza"),
    },
    {
        .name = "EDEN_NX2_SWKBD_NATIVE",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Reads the Switch 2 software keyboard config revisions with their own layout."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 reads them with the Switch 1 layout."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SYNCPT_POLL_DELAY_US",
        .category = Category::Services,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 400,
        .suggested = "200",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Test only: holds each syncpoint poll this long between its two reads, up to 400 µs "
            "(multicore only)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With EDEN_NX2_SYNCPT_VALUES_FIRST=1, reproduces the movie player abort."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Delays every wait it answers, so frame times mean nothing."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SYNCPT_PUBLISH_IN_ACTION",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Publishes syncpoint values as they change, instead of leaving them to the 500 µs "
            "poll."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 makes movie players find their fences unreached and wait more often."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SYNCPT_SIGNAL_NEW_SLOTS",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Creates the events of new syncpoint slots signalled."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 can leave a movie that got a fresh slot waiting forever."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_SYNCPT_VALUES_FIRST",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The syncpoint poll reads the values before the armed waits, as before."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Mario Kart World's movie player aborted (2162-0001) about once per two minutes of "
            "movie."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Mario Kart World"),
    },
    {
        .name = "EDEN_NX2_SYNC_SUBMIT",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Diagnostic: each GPU submission waits, up to 2 s, until the GPU has run it."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shows whether a title rewriting command memory it already submitted (the Yoshi "
            "demo, per rdenno) still misreads commands."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The game's CPU waits for the GPU every submission: much slower, and a submission "
            "waiting on a later CPU release stalls for the full 2 s."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Yoshi and the Mysterious Book demo"),
    },
    {
        .name = "EDEN_NX2_TSG_STUB_ALL",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lets every GPU TSG ioctl succeed, as before, instead of only the implemented ones."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "None; it brings back the old behaviour, for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The SDK probes the others and must see them unsupported."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_VI2_CLIENT_STATE",
        .category = Category::Services,
        .type = ValueType::Toggle,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Implements the client state commands of the Switch 2 display service (vi2)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None expected; 0 is for comparisons."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "0 leaves them to the auto stub, and command 11 to an event that never fires."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_LOG_DEBUG_OUTPUT",
        .category = Category::Diagnostics,
        .type = ValueType::Toggle,
        .flags = Diagnostic | EnvironmentOnly,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Sends log lines to the Windows debugger output even with no debugger attached (for "
            "DebugView)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Read the log live with DebugView."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Slower logging."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NATIVE_DRAW_CAPTURE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Writes the state of the first 16 native draws to native-draw-* files in the working "
            "folder."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Disk writes."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NATIVE_DRAW_DIAGNOSTICS",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Writes native macro, QMD and constant buffer dumps and a query trace to the working "
            "folder."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Slower, with many disk writes."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NATIVE_FRAME_CAPTURE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Writes each context's native graphics macro code to a file and logs the native "
            "display source."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NATIVE_SHADER_OVERRIDE_DIR",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "For a native shader that fails to translate, writes its code to this folder and "
            "pauses up to 30 minutes until a <key>.override.bin appears there."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Test hand-written shader replacements."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "The game stops while it waits."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_3D_HISTORY",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Keeps the last 48 3D register writes for error logs (always on in debug builds)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A small cost on every register write."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_3D_METHODS",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs the first write to each 3D register (finds methods the emulator doesn't "
            "model)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A little log noise."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_AUDIO_LEVEL",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs the audio output peak about every two seconds."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Shows whether a game is audible."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A little log noise."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_CB24_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "For draws whose vertex shader reads constant bank 24, logs the inline constant "
            "buffer upload target and its first words."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_CRASH_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic | CliOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "On a crash, prints the faulting stack as module+offset and writes a minidump to "
            "this folder (Windows)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Crash reports with a stack."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Disk space for the dump."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_DLSS_LOG",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 2,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs NVIDIA Streamline's messages: 1 its information lines, 2 verbose. Only builds "
            "with NVIDIA DLSS."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds why NVIDIA DLSS doesn't run."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "<present>:<hex address>:<hex size>[,<address>:<size>...]: at that present, flushes "
            "those GPU ranges and saves them as nx2-dump-<address>.bin in the "
            "EDEN_NX2_FRAME_CAPTURE folder."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Inspect what a pass rendered, offline."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A stall at that present."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FAST_DRAW_VERIFY",
        .category = Category::Diagnostics,
        .type = ValueType::Toggle,
        .flags = Diagnostic,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With EDEN_NX2_BUFFER_SKIP, EDEN_NX2_INLINE_INVALIDATE or EDEN_NX2_DYNAMIC_SKIP, does "
            "the skipped work anyway and logs every 10 s how often it would have changed "
            "something."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Shows whether those skips are safe."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Slower than without the skips."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Looks for flashes too short to capture by hand and writes reports to this folder: "
            "every frame, the outputs of every pass and the presented image are reduced to tiles, "
            "and a presented tile brighter than the frames around it is reported with the first "
            "output that was abnormal."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the pass behind a flash."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A compute pass over every output each frame: much slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH_DELTA",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .suggested = "0.15",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0.15"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the flash watch, how much brighter (mean, absolute) a presented tile must also "
            "be than the frames around it to count as a flash."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower values catch fainter flashes."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower values report more noise."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH_HOT",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 1,
        .maximum = 1000000,
        .suggested = "1024",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1024"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the flash watch, the channel magnitude from which a texel counts as hot in the "
            "tiles."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Matches the game's colour range."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH_MAX",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 10000,
        .suggested = "10",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "10"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the flash watch, the most reports written in a session; later flashes are only "
            "logged."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "More reports for a long session."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Each report takes disk space."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH_PASSES",
        .category = Category::Diagnostics,
        .type = ValueType::Toggle,
        .flags = Diagnostic,
        .suggested = "0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "On"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the flash watch, watches the outputs of every pass; 0 watches only the "
            "presented image."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 costs much less."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0 can't say which pass flashed."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH_PASS_RATIO",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .suggested = "2.0",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "2.0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the flash watch, how many times its mean in the frames around it an output must "
            "reach to count as abnormal in a report."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower values flag fainter outputs."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower values flag more outputs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FLASH_WATCH_RATIO",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .suggested = "1.5",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "1.5"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With the flash watch, how many times brighter than the frames around it a presented "
            "tile must be to count as a flash."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower values catch fainter flashes."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lower values report more noise."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_FRAME_CAPTURE",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Every 120th present, saves the shown frame as nx2-frame-<n>.tga in this folder."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Lets hidden test runs check the picture."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A readback and a file write every 120 presents."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All Switch 2 titles"),
    },
    {
        .name = "EDEN_NX2_GPU_PROFILE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Times where the GPU thread spends its time and logs the zones every 10 s."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Shows what limits the GPU thread."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A little overhead in every zone."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_HIDE_WINDOW",
        .category = Category::Diagnostics,
        .type = ValueType::Toggle,
        .flags = Diagnostic | CliOnly,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Creates the render window hidden and unfocusable."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Unattended test runs don't cover or interrupt the desktop."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "You can't see or use the game."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_IPC_SUMMARY",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Every 5 s, logs how often each service command ran and with which result."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Finds a game polling something that never becomes ready."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Counts every request: slower, and fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_IPC_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs every dispatched service command and the result it returned."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Very slow, and huge logs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_KEY_RECORD",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic | CliOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Appends each key press and release (present number and SDL scancode) to this file."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Records input for EDEN_NX2_KEY_SCRIPT."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A file write per key."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_KEY_SCRIPT",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic | CliOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Presses and releases keys by SDL scancode at set times: "
            "\"seconds:code,seconds:-code,...\" or \"@<file>\"; code 0 ends the run."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Drives menus in unattended test runs."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Keys pressed on their own."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_MEM_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs a title's memory layout queries and its heap and physical memory requests."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A little log noise."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_METHOD_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Logs every GPU engine method."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Very slow, and huge logs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_MME_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Writes the compute macro code to a file once per engine, and logs each compute "
            "macro call and macro data RAM changes."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_MME_LOG",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs every Switch 2 macro call at debug level (about 750,000 a second)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Very slow, and huge logs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_MME_MEMO_STATS",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs the macro memo's counters every 16.7 million calls."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A little log noise."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_MME_RECORD",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Writes the first 200,000 macro calls, and then the macro code, to this file, to "
            "check interpreter changes offline."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Disk writes while it records."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_MME_RECORD_SKIP",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 2147483647,
        .suggested = "1000000",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips this many macro calls before EDEN_NX2_MME_RECORD starts."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_PB_CHECK",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic | EnvironmentOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Hashes command lists when they are submitted and logs entries that changed before "
            "they ran."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Reads every list twice: slower."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_PB_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "At the first command list dump (from EDEN_NX2_PB_CHECK), also writes the whole "
            "command list to this file, for offline replay."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "One file write."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_PENDING_WRITE_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs bound buffer ranges that still hold guest CPU writes not yet applied to the "
            "host buffers, which a draw would read old (up to 100,000 lines)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds stale buffer reads."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Slower draws; fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_PERF_VERIFY",
        .category = Category::Diagnostics,
        .type = ValueType::Toggle,
        .flags = Diagnostic,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The GPU thread optimizations from Lichii also run the old code where they can "
            "(3D method dispatch, uniform uploads, texture unswizzles, storage buffer bindings and "
            "pipeline shader code), log any difference (\"Method fast:\", \"Nx2Perf\") and keep "
            "the old result."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Shows whether those optimizations are exact."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Slower than without them."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_PIPELINE_DIFF",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "For a new graphics pipeline whose shaders an earlier one also uses, logs which "
            "fixed state differs."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Finds state that splits the same draws into many pipelines."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_PULLER_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Logs every GPU puller method."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Very slow, and huge logs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_QMD_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs the compute QMD scheduler's loads, dependency counts and launches."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_QUERY_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs every GPU report (query) the game asks for."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_RT_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs the first draw into every distinct colour target, and the first 2D blits and "
            "DMA copies."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SAMPLE",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic | CliOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Sampling profiler for the host threads named here (comma separated; a trailing * "
            "matches a prefix): records their call stacks (Windows)."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shows where named threads spend their time."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Pauses those threads about 1000 times a second while it records."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SAMPLE_AT",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 2147483647,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "At the n-th upscale fallback, downloads its input, output and targets and counts "
            "their non-zero words."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The downloads froze a title for ~1.3 s."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SAMPLE_OUT",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic | CliOnly,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "nx2_samples.txt"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The file EDEN_NX2_SAMPLE writes its counts to."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shows where named threads spend their time."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Pauses those threads about 1000 times a second while it records."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SAMPLE_SECONDS",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic | CliOnly,
        .minimum = 1,
        .maximum = 86400,
        .suggested = "30",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "30 s"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "How many seconds EDEN_NX2_SAMPLE records for."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shows where named threads spend their time."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Pauses those threads about 1000 times a second while it records."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SAMPLE_START",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic | CliOnly,
        .minimum = 0,
        .maximum = 86400,
        .suggested = "60",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "60 s"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Seconds after start when EDEN_NX2_SAMPLE begins recording."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Shows where named threads spend their time."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Pauses those threads about 1000 times a second while it records."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SEM_WATCH",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs every release, acquire, report, inline write and macro parameter that touches "
            "the 16 bytes at this GPU address (hex). The command stream part only works from the "
            "real environment."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SEM_WATCH_EMITS",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With EDEN_NX2_SEM_WATCH, also logs every write of a watched compute macro."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SHADER_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Saves every captured native shader program, and the rejected ones, to this folder."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Lets the shader translation be tested offline."),
        .downsides = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "A disk write the first time each program is seen."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SPLIT_CHECK",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "While the split runs, reports with a stack trace any thread other than the second "
            "GPU thread that records into the Vulkan scheduler."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Finds the cause of crashes in command recording."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A small cost at every scheduler use."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SPLIT_VERIFY",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Every 4096 macro calls on the split's first thread, waits for the second and "
            "compares their register copies, logging and fixing differences."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Finds split bugs that leave wrong GPU state."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Stalls the GPU thread regularly."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_SVC_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "1 logs every SVC with its arguments and results; <first>:<last> logs only during "
            "presents first to last."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Very slow, and huge logs."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "All titles"),
    },
    {
        .name = "EDEN_NX2_TABLE_POINTER_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs each change of a native pointer table slot's entry or size, and whether the "
            "guest and host copies of the entry can differ."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Finds wrong buffers bound through pointer tables."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Slower binds; fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_TEXTURE_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "With EDEN_NX2_WINDOW_TRACE, saves the start of the last traced texture's memory to "
            "this file."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A file write per traced draw."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_TRACE_AFTER_UPSCALE",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 1,
        .maximum = 2147483647,
        .suggested = "1",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Traces the 40 draws that follow the n-th upscale fallback."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_TRACE_DST",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs DMA copies and 2D blits that write within 32 MiB above this GPU address (hex)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_TRACE_PS",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Limits EDEN_NX2_WINDOW_TRACE to the pixel shader at this address (hex)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_TRACE_VS",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Describes the next draws that use the vertex shader at this address (hex): render "
            "target, blending and textures."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_UPSCALE_SOURCE",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The upscale fallback blits from the same-sized image at this GPU address (hex) "
            "instead."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Wrong picture while set."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_UPSCALE_SOURCE_FORMAT",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "The render target format (hex) of EDEN_NX2_UPSCALE_SOURCE's image."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Wrong picture while set."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_VDEC_DUMP",
        .category = Category::Diagnostics,
        .type = ValueType::Text,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Saves the luma of every 120th decoded movie frame, at a quarter of its size, as "
            "nx2-vdec-<n>.pgm in this folder."),
        .gains = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Checks what the decoder wrote, apart from how the game shows it."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "A file write every 120 frames."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_VDEC_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Logs every video decoder call at Info level."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_WINDOW_TRACE",
        .category = Category::Diagnostics,
        .type = ValueType::Presence,
        .flags = Diagnostic,
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Off"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Describes clears and draws into 3840-pixel-wide colour targets (up to 400)."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
    {
        .name = "EDEN_NX2_WINDOW_TRACE_SKIP",
        .category = Category::Diagnostics,
        .type = ValueType::Number,
        .flags = Diagnostic,
        .minimum = 0,
        .maximum = 2147483647,
        .suggested = "1000",
        .default_value = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "0"),
        .description = QT_TRANSLATE_NOOP(
            "Nx2SwitchCatalog",
            "Skips this many events before EDEN_NX2_WINDOW_TRACE starts describing them."),
        .gains = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Finds the cause of a bug."),
        .downsides = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Fills the log."),
        .games = QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "None: a developer tool"),
    },
};

constexpr std::array<const char*, NUM_CATEGORIES> CATEGORY_NAMES{
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "GPU thread & command stream"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Buffer cache & memory"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Texture cache"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Shaders & pipelines"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "CPU, kernel & timing"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Video, presentation & audio"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "System services"),
    QT_TRANSLATE_NOOP("Nx2SwitchCatalog", "Diagnostics & logging"),
};

} // Anonymous namespace

std::span<const Entry> Entries() {
    return ENTRIES;
}

const Entry* Find(std::string_view name) {
    const auto it =
        std::ranges::find_if(ENTRIES, [name](const Entry& entry) { return name == entry.name; });
    return it != std::ranges::end(ENTRIES) ? &*it : nullptr;
}

QString Translate(const char* text) {
    if (text == nullptr || *text == '\0') {
        return {};
    }
    return QCoreApplication::translate("Nx2SwitchCatalog", text);
}

QString CategoryName(Category category) {
    return Translate(CATEGORY_NAMES[static_cast<size_t>(category)]);
}

} // namespace Nx2SwitchCatalog
