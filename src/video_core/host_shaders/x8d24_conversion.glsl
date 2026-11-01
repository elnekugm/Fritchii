// SPDX-License-Identifier: GPL-3.0-or-later

float DecodeX8D24(float red, float green, float blue) {
    uint depth = uint(round(red * 255.0f)) | (uint(round(green * 255.0f)) << 8) |
                 (uint(round(blue * 255.0f)) << 16);
    return float(depth) / 16777215.0f;
}

uint EncodeX8D24(float depth) {
    return uint(round(depth * 16777215.0f));
}
