#include <algorithm>
#include <string_view>

#include "video_core/host1x/nx2_video_decoder.h"

namespace Tegra::Host1x {

std::optional<Nx2VideoConfiguration> ParseNx2VideoConfiguration(std::span<const u8> configuration) {
    if (configuration.empty()) return std::nullopt;
    // Check for MIME string (starting with 'v')
    if (configuration[0] == 'v') {
        // Find null terminator if present
        auto end = std::find(configuration.begin(), configuration.end(), '\0');
        std::string_view mime{reinterpret_cast<const char*>(configuration.data()), end - configuration.begin()};
        std::optional<VideoCodec> codec;
        if (mime == "video/vp9") codec = VideoCodec::VP9;
        else if (mime == "video/vp8") codec = VideoCodec::VP8;
        else if (mime == "video/avc" || mime == "video/h264") codec = VideoCodec::H264;
        else if (mime == "video/hevc" || mime == "video/h265") codec = VideoCodec::H265;
        else if (mime == "video/av01" || mime == "video/av1") codec = VideoCodec::AV1;
        if (!codec) return std::nullopt;
        return Nx2VideoConfiguration{*codec, {}};
    }
    // Binary configuration: expect H264 with start code 0x67 or 0x68 after optional zero padding
    // Find first non-zero byte
    auto it = std::find_if(configuration.begin(), configuration.end(), [](u8 b){ return b != 0; });
    if (it == configuration.end()) return std::nullopt; // all zeros
    u8 first = *it;
    if (first != 0x67 && first != 0x68) return std::nullopt; // not a valid H264 start code
    return Nx2VideoConfiguration{VideoCodec::H264, std::vector<u8>{configuration.begin(), configuration.end()}};
}

} // namespace Tegra::Host1x
