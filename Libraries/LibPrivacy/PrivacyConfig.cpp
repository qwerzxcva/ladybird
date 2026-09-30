/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PrivacyConfig.h"
#include <AK/Array.h>

namespace Privacy {

// FNV-1a: small, dependency-free, and deterministic across runs — which is what
// we need so that a given origin keeps the same spoofed identity and noise.
static u32 origin_hash(StringView origin)
{
    u32 hash = 2166136261u;
    for (auto byte : origin.bytes()) {
        hash ^= byte;
        hash *= 16777619u;
    }
    return hash;
}

NonnullRefPtr<PrivacyConfig> PrivacyConfig::create()
{
    return adopt_ref(*new PrivacyConfig());
}

String PrivacyConfig::get_isolated_user_agent(StringView origin) const
{
    static constexpr Array<StringView, 4> ua_pool = {
        "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"sv,
        "Mozilla/5.0 (Linux; Android 13; SM-S908B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Mobile Safari/537.36"sv,
        "Mozilla/5.0 (Linux; Android 14; ASUS_AI2401) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Mobile Safari/537.36"sv,
        "Mozilla/5.0 (Linux; Android 13; V2254A) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118.0.0.0 Mobile Safari/537.36"sv
    };

    if (!m_spoof_ua || m_fingerprint_level == ProtectionLevel::Off) {
        auto fallback = String::from_utf8(ua_pool[0]);
        if (fallback.is_error())
            return String {};
        return fallback.release_value();
    }

    auto key = origin_hash(origin);
    if (auto it = m_ua_cache.find(key); it != m_ua_cache.end())
        return it->value;

    auto selected_index = (key ^ 0x9E3779B9u) % ua_pool.size();
    auto selected = String::from_utf8(ua_pool[selected_index]);
    if (selected.is_error())
        return String {};
    auto selected_ua = selected.release_value();
    m_ua_cache.set(key, selected_ua);
    return selected_ua;
}

u32 PrivacyConfig::get_noise_seed_for_origin(StringView origin) const
{
    if (m_fingerprint_level == ProtectionLevel::Off)
        return 0;

    auto key = origin_hash(origin);
    if (auto it = m_noise_seed_cache.find(key); it != m_noise_seed_cache.end())
        return it->value;

    // Mix in a constant so the noise seed is not simply the UA selector.
    u32 seed = key ^ 0x85EBCA6Bu;
    m_noise_seed_cache.set(key, seed);
    return seed;
}

} // namespace Privacy
