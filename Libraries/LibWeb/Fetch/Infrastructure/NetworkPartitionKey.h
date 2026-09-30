/*
 * Copyright (c) 2024, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <LibURL/Origin.h>
#include <LibWeb/Export.h>
#include <LibWeb/Forward.h>

namespace Web::Fetch::Infrastructure {

// https://fetch.spec.whatwg.org/#network-partition-key
struct NetworkPartitionKey {
    URL::Origin top_level_origin;
    // === Privacy: Per-Origin Cookie/Storage Isolation (WebLibre container model) ===
    // Replaced raw pointer with deterministic origin hash to enforce strict
    // per-origin partitioning. Each origin gets its own isolated cookie/storage/cache
    // bucket, preventing cross-site tracking via shared state.
    u64 second_key { 0 };
    // === End Privacy Hook ===

    bool operator==(NetworkPartitionKey const&) const = default;
};

NetworkPartitionKey determine_the_network_partition_key(HTML::Environment const& environment);

WEB_API Optional<NetworkPartitionKey> determine_the_network_partition_key(Infrastructure::Request const& request);

}

template<>
class AK::Traits<Web::Fetch::Infrastructure::NetworkPartitionKey> : public DefaultTraits<Web::Fetch::Infrastructure::NetworkPartitionKey> {
public:
    static unsigned hash(Web::Fetch::Infrastructure::NetworkPartitionKey const& partition_key)
    {
        // Combine top_level_origin hash with second_key for full isolation
        auto origin_hash = ::AK::Traits<URL::Origin>::hash(partition_key.top_level_origin);
        return origin_hash ^ static_cast<unsigned>(partition_key.second_key);
    }
};
