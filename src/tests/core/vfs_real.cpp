// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/vfs/vfs.h"
#include "core/file_sys/vfs/vfs_real.h"
#include "core/file_sys/vfs/vfs_vector.h"

namespace {

namespace fs = std::filesystem;

struct TempDir {
    fs::path path = fs::temp_directory_path() /
                    ("vfs-real-test-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    TempDir() {
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

void Put(const fs::path& file, std::string_view content) {
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << content;
}

} // namespace

// `t.path / "out"` is the directory under test, everything else below `t.path` is outside.
TEST_CASE("RealVfsDirectory refuses child names that leave the directory", "[core][vfs_real]") {
    TempDir t;
    FileSys::RealVfsFilesystem vfs;
    fs::create_directories(t.path / "out");
    const auto dir = vfs.OpenDirectory((t.path / "out").string(), FileSys::OpenMode::ReadWrite);
    REQUIRE(dir != nullptr);

    REQUIRE(dir->CreateFile("../escape.txt") == nullptr);
    REQUIRE_FALSE(fs::exists(t.path / "escape.txt"));
    REQUIRE(dir->CreateSubdirectory("../evil") == nullptr);
    REQUIRE_FALSE(fs::exists(t.path / "evil"));
    REQUIRE(dir->CreateSubdirectory("..") == nullptr);
    REQUIRE(dir->CreateFileRelative("../made/escape.txt") == nullptr);
    REQUIRE_FALSE(fs::exists(t.path / "made"));
    REQUIRE(dir->CreateDirectoryRelative("../evil2") == nullptr);
    REQUIRE_FALSE(fs::exists(t.path / "evil2"));

    Put(t.path / "victim.txt", "x");
    REQUIRE_FALSE(dir->DeleteFile("../victim.txt"));
    REQUIRE(fs::exists(t.path / "victim.txt"));

    Put(t.path / "victimdir" / "keep.txt", "x");
    REQUIRE_FALSE(dir->DeleteSubdirectoryRecursive("../victimdir"));
    REQUIRE(fs::exists(t.path / "victimdir" / "keep.txt"));

    fs::create_directories(t.path / "emptydir");
    REQUIRE_FALSE(dir->DeleteSubdirectory("../emptydir"));
    REQUIRE(fs::exists(t.path / "emptydir"));
}

TEST_CASE("RealVfsDirectory creates and deletes plain children", "[core][vfs_real]") {
    TempDir t;
    FileSys::RealVfsFilesystem vfs;
    fs::create_directories(t.path / "out");
    const auto dir = vfs.OpenDirectory((t.path / "out").string(), FileSys::OpenMode::ReadWrite);
    REQUIRE(dir != nullptr);

    REQUIRE(dir->CreateFile("ok.txt") != nullptr);
    REQUIRE(fs::exists(t.path / "out" / "ok.txt"));
    REQUIRE(dir->CreateSubdirectory("sub") != nullptr);
    REQUIRE(fs::is_directory(t.path / "out" / "sub"));
    REQUIRE(dir->CreateFileRelative("a/b/c.txt") != nullptr);
    REQUIRE(fs::exists(t.path / "out" / "a" / "b" / "c.txt"));
    REQUIRE(dir->CreateDirectoryRelative("d/e") != nullptr);
    REQUIRE(fs::is_directory(t.path / "out" / "d" / "e"));

    REQUIRE(dir->DeleteFile("ok.txt"));
    REQUIRE(dir->DeleteSubdirectory("sub"));
    REQUIRE(dir->DeleteSubdirectoryRecursive("a"));
    REQUIRE_FALSE(fs::exists(t.path / "out" / "ok.txt"));
    REQUIRE_FALSE(fs::exists(t.path / "out" / "sub"));
    REQUIRE_FALSE(fs::exists(t.path / "out" / "a"));
}

TEST_CASE("A raw directory copy cannot write outside its target", "[core][vfs_real]") {
    TempDir t;
    FileSys::RealVfsFilesystem vfs;
    fs::create_directories(t.path / "out");
    const auto dir = vfs.OpenDirectory((t.path / "out").string(), FileSys::OpenMode::ReadWrite);
    REQUIRE(dir != nullptr);

    const FileSys::VirtualFile hostile =
        std::make_shared<FileSys::VectorVfsFile>(std::vector<u8>{1, 2, 3}, "../escape.bin");
    const FileSys::VirtualDir src = std::make_shared<FileSys::VectorVfsDirectory>(
        std::vector<FileSys::VirtualFile>{hostile}, std::vector<FileSys::VirtualDir>{}, "src");

    REQUIRE_FALSE(FileSys::VfsRawCopyD(src, dir));
    REQUIRE_FALSE(fs::exists(t.path / "escape.bin"));
}
