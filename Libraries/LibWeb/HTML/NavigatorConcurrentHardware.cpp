/*
 * Copyright (c) 2022, Andrew Kaster <akaster@serenityos.org>
 * Copyright (c) 2024, Shannon Booth <shannon@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/System.h>
#include <LibPrivacy/PrivacyConfig.h>
#include <LibWeb/HTML/NavigatorConcurrentHardware.h>

namespace Web::HTML {

// https://html.spec.whatwg.org/multipage/workers.html#dom-navigator-hardwareconcurrency
WebIDL::UnsignedLongLong NavigatorConcurrentHardwareMixin::hardware_concurrency()
{
    // === Privacy: Hardware Concurrency Spoofing ===
    // The real core count is stable and highly identifying, so report a common
    // value for a modern Android device instead of exposing the host's.
    static auto privacy_config = Privacy::PrivacyConfig::create();
    if (privacy_config->fingerprint_protection_level() != Privacy::ProtectionLevel::Off)
        return 8;
    // === End Privacy Hook ===

    return Core::System::hardware_concurrency();
}

}
