/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PrivacyConfig.h"
#include <AK/Random.h>
#include <AK/StringBuilder.h>
#include <LibCrypto/Hash/SHA2.h>

namespace Privacy {

NonnullRefPtr<PrivacyConfig> PrivacyConfig::create()
{
    return adopt_ref(*new PrivacyConfig());
}

String PrivacyConfig::get_isolated_user_agent(StringView origin) const
{
    if (!m_spoof_ua || m_fingerprint_level == ProtectionLevel::Off)
        return "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"_string;

    auto it = m_ua_cache.find(origin);
    if (it != m_ua_cache.end())
        return it->value;

    // Generate a stable but unique UA per origin using SHA-256 hash
    Crypto::Hash::SHA256 hasher;
    hasher.update(origin);
    hasher.update("ladybird-privacy-salt-v1"sv);
    auto digest = hasher.digest();

    // Use first 4 bytes to pick from a pool of realistic UAs
    u32 selector = (digest.data[0] << 24) | (digest.data[1] << 16) | (digest.data[2] << 8) | digest.data[3];

    static constexpr Array<StringView, 4> ua_pool = {
        "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"sv,
        "Mozilla/5.0 (Linux; Android 13; SM-S908B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Mobile Safari/537.36"sv,
        "Mozilla/5.0 (Linux; Android 14; ASUS_AI2401) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Mobile Safari/537.36"sv,
        "Mozilla/5.0 (Linux; Android 13; V2254A) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118.0.0.0 Mobile Safari/537.36"sv
    };

    auto selected_ua = String::from_utf8_without_validation(ua_pool[selector % ua_pool.size()].bytes());
    m_ua_cache.set(origin.to_string(), selected_ua);
    return selected_ua;
}

u32 PrivacyConfig::get_noise_seed_for_origin(StringView origin) const
{
    if (m_fingerprint_level == ProtectionLevel::Off)
        return 0;

    auto it = m_noise_seed_cache.find(origin);
    if (it != m_noise_seed_cache.end())
        return it->value;

    Crypto::Hash::SHA256 hasher;
    hasher.update(origin);
    hasher.update("noise-seed-salt-v1"sv);
    auto digest = hasher.digest();

    u32 seed = (digest.data[0] << 24) | (digest.data[1] << 16) | (digest.data[2] << 8) | digest.data[3];
    m_noise_seed_cache.set(origin.to_string(), seed);
    return seed;
}

} // namespace Privacy
