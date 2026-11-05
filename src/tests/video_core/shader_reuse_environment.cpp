// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>

#include <catch2/catch_test_macros.hpp>
#include "video_core/renderer_vulkan/vk_shader_reuse.h"

namespace {
// An environment that answers only what the recorder copies or forwards without asking.
class MeshEnvironment final : public Shader::Environment {
public:
    MeshEnvironment() {
        stage = Shader::Stage::Mesh;
        mesh_registers = {0x00402404, 0x02000000, 0x00401001};
        std::memset(&mesh_primitive_header, 0xa5, sizeof(mesh_primitive_header));
    }
    u64 ReadInstruction(u32) override { return 0; }
    u32 ReadCbufValue(u32, u32) override { return 0; }
    Shader::TextureType ReadTextureType(u32) override { return {}; }
    Shader::TexturePixelFormat ReadTexturePixelFormat(u32) override { return {}; }
    bool IsTexturePixelFormatInteger(u32) override { return false; }
    u32 ReadViewportTransformState() override { return 0; }
    u32 TextureBoundBuffer() const override { return 2; }
    u32 SurfaceBoundBuffer() const override { return 3; }
    u32 LocalMemorySize() const override { return 0; }
    u32 SharedMemorySize() const override { return 0; }
    std::array<u32, 3> WorkgroupSize() const override { return {}; }
    bool HasHLEMacroState() const override { return false; }
    std::optional<Shader::ReplaceConstant> GetReplaceConstBuffer(u32, u32) override {
        return std::nullopt;
    }
    void Dump(u64, u64) override {}
};
} // Anonymous namespace

// Shader reuse translates through a RecordingEnvironment: mesh programs read their output
// topology from MeshRegisters(), which the recorder answered as zero (Ocarina's mesh draws were
// skipped with "Mesh output topology 0").
TEST_CASE("RecordingEnvironment keeps the mesh state and surface bank", "[video_core]") {
    MeshEnvironment inner;
    Vulkan::RecordingEnvironment recorder{inner};
    CHECK(recorder.ShaderStage() == Shader::Stage::Mesh);
    CHECK(recorder.MeshRegisters() == inner.MeshRegisters());
    CHECK(std::memcmp(&recorder.MeshPrimitiveHeader(), &inner.MeshPrimitiveHeader(),
                      sizeof(Shader::ProgramHeader)) == 0);
    CHECK(recorder.SurfaceBoundBuffer() == 3);
}
