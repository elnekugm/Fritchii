// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>
#include "common/fs/fs_util.h"
#include "core/core.h"
#include "core/file_sys/vfs/vfs_real.h"
#include "core/loader/loose_directory.h"
#include "qt_common/game_list/discovery.h"

namespace {
namespace fs = std::filesystem;
struct LooseTestDirectory {
    fs::path root = fs::temp_directory_path() /
                    ("loose-program-test-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~LooseTestDirectory() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
    static void Put(const fs::path& file, std::string_view content = "main") {
        fs::create_directories(file.parent_path());
        std::ofstream(file, std::ios::binary) << content;
    }
    static void ExeFS(const fs::path& dir, std::string_view content = "main") {
        Put(dir / "main", content);
        Put(dir / "main.npdm", "npdm");
    }
};
} // namespace

TEST_CASE("Loose edition folders prefer a complete native program and retain legacy fallback",
          "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / fs::path{u8"Kirby 日本語"};
    const auto programs = game / "upgrade/65536/program";
    LooseTestDirectory::ExeFS(game / "base/exefs", "base");
    LooseTestDirectory::ExeFS(game / "updates/65536/exefs", "update");
    LooseTestDirectory::ExeFS(game / "updates/999999_old/exefs", "invalid update directory");
    LooseTestDirectory::ExeFS(programs / "0/exefs", "program0");
    LooseTestDirectory::ExeFS(programs / "1/exefs", "program1");
    for (const auto& directory : {game, game / "upgrade", programs.parent_path(), programs,
                                  programs / "0", programs / "1"}) {
        REQUIRE(Loader::ResolveLooseProgramPath(directory) == programs / "1/exefs/main");
    }
    REQUIRE(Loader::LooseGameRootFromMain(programs / "1/exefs/main") == game);
    auto main = Core::GetGameFileFromPath(std::make_shared<FileSys::RealVfsFilesystem>(),
                                         Common::FS::PathToUTF8String(game));
    REQUIRE(main);
    CHECK(main->ReadAllBytes() == std::vector<u8>{'p', 'r', 'o', 'g', 'r', 'a', 'm', '1'});
    main.reset();
    fs::remove(programs / "1/exefs/main.npdm");
    CHECK(Loader::ResolveLooseProgramPath(game) == programs / "0/exefs/main");
    fs::remove(programs / "0/exefs/main");
    CHECK(Loader::ResolveLooseProgramPath(game) == game / "updates/65536/exefs/main");
    fs::remove_all(game / "updates");
    CHECK(Loader::ResolveLooseProgramPath(game) == game / "base/exefs/main");
    CHECK(Loader::ResolveLooseProgramPath(game / "base/exefs") == game / "base/exefs/main");
    LooseTestDirectory::Put(game / "main", "direct main");
    CHECK(Loader::ResolveLooseProgramPath(game) == game / "main");
}

TEST_CASE("Loose edition discovery chooses one program in shallow and deep scans",
          "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / "Game";
    const auto programs = game / "upgrade/327680/program";
    LooseTestDirectory::ExeFS(game / "base/exefs");
    LooseTestDirectory::ExeFS(game / "updates/65536/exefs");
    const auto scan = [](const fs::path& root, bool deep) {
        return GameListDiscovery::Discover(root, deep,
                                           [](const fs::path& file) { return file.filename() == "main"; },
                                           [] { return false; });
    };
    for (const bool deep : {false, true}) {
        REQUIRE(scan(t.root, deep).files.size() == 2);
    }
    LooseTestDirectory::ExeFS(programs / "0/exefs");
    LooseTestDirectory::ExeFS(programs / "1/exefs");
    for (const auto& root : {t.root, game, programs, programs / "0", programs / "0/exefs"}) {
        for (const bool deep : {false, true}) {
            const auto result = scan(root, deep);
            REQUIRE(result.files.size() == 1);
            CHECK(result.files.front() == programs / "1/exefs/main");
        }
    }
    fs::remove(programs / "1/exefs/main.npdm");
    for (const bool deep : {false, true}) {
        const auto fallback = scan(t.root, deep);
        REQUIRE(fallback.files.size() == 1);
        CHECK(fallback.files.front() == programs / "0/exefs/main");
    }
}
