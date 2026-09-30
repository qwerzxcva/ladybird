/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/String.h>
#include <AK/HashMap.h>
#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>

namespace Privacy {

enum class ProtectionLevel {
    Off,
    Standard,
    Strict
};

class PrivacyConfig final : public RefCounted<PrivacyConfig> {
public:
    static NonnullRefPtr<PrivacyConfig> create();

    ProtectionLevel fingerprint_protection_level() const { return m_fingerprint_level; }
    void set_fingerprint_protection_level(ProtectionLevel level) { m_fingerprint_level = level; }

    bool should_spoof_user_agent() const { return m_spoof_ua; }
    void set_spoof_user_agent(bool value) { m_spoof_ua = value; }

    bool should_isolate_cookies() const { return m_isolate_cookies; }
    void set_isolate_cookies(bool value) { m_isolate_cookies = value; }

    bool should_block_webgl() const { return m_block_webgl; }
    void set_block_webgl(bool value) { m_block_webgl = value; }

    String get_isolated_user_agent(StringView origin) const;
    u32 get_noise_seed_for_origin(StringView origin) const;

private:
    PrivacyConfig() = default;

    ProtectionLevel m_fingerprint_level { ProtectionLevel::Strict };
    bool m_spoof_ua { true };
    bool m_isolate_cookies { true };
    bool m_block_webgl { false };

    mutable HashMap<String, String> m_ua_cache;
    mutable HashMap<String, u32> m_noise_seed_cache;
};

} // namespace Privacy
