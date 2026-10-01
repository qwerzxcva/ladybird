/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Gecko WebExtensions API compatibility shim.
 * Translates browser.* API calls to Ladybird internal IPC messages.
 */

#pragma once

#include <AK/Function.h>
#include <AK/HashMap.h>
#include <AK/NonnullRefPtr.h>
#include <AK/RefCounted.h>
#include <AK/String.h>
#include <AK/Types.h>

namespace GeckoShim {

class ExtensionBridge final : public RefCounted<ExtensionBridge> {
public:
    static NonnullRefPtr<ExtensionBridge> create();

    using IPCResponseCallback = Function<void(String const& response_json)>;

    // Called with the string API name that the injected JS shim sent (for
    // example "storage.local.get"), and answers with a JSON string.
    void handle_api_call(StringView api_name, StringView payload_json, IPCResponseCallback callback);

    // Register/unregister content script for a given origin
    void register_content_script(StringView origin, StringView script_source);
    void unregister_content_script(StringView origin);

    // Get the shim JS code to inject into pages
    static String get_shim_javascript();

private:
    ExtensionBridge() = default;

    HashMap<String, String> m_content_scripts;
    HashMap<u64, IPCResponseCallback> m_pending_callbacks;
    u64 m_next_callback_id { 1 };
};

} // namespace GeckoShim
