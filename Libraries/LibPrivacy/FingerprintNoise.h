/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

namespace Privacy {

// Applies deterministic ±1 LSB noise to a raw pixel buffer.
//
// Deterministic on purpose: a given origin must always produce the same output
// so that repeated reads look stable, while still differing from the real
// hardware so the value is useless as a fingerprint. ±1 LSB is below the
// perceptible threshold for 8-bit-per-channel imagery.
inline void apply_pixel_noise(u8* pixels, size_t byte_count, u32 seed)
{
    if (!pixels)
        return;

    u32 state = seed;
    for (size_t i = 0; i < byte_count; ++i) {
        state = state * 1664525u + 1013904223u;
        // Only touch ~25% of the bytes to keep the image visually identical.
        if ((state & 0x3u) != 0)
            continue;

        int delta = (state & 0x2u) ? 1 : -1;
        int value = static_cast<int>(pixels[i]) + delta;
        if (value < 0)
            value = 0;
        else if (value > 255)
            value = 255;
        pixels[i] = static_cast<u8>(value);
    }
}

} // namespace Privacy
