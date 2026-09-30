/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Gecko WebExtensions API compatibility shim.
 * Translates browser.* API calls to Ladybird internal IPC messages.
 */

#pragma once

#include <AK/String.h>
#include <AK/HashMap.h>
#include <AK/Function.h>
#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>

namespace GeckoShim {

enum class ExtensionAPI : u32 {
    RuntimeSendMessage,
    RuntimeGetURL,
    StorageLocalGet,
    StorageLocalSet,
    TabsQuery,
    TabsCreate,
    WebRequestOnBeforeRequest,
    CookiesGetAll,
    CookiesSet
};

class ExtensionBridge final : public RefCounted<ExtensionBridge> {
public:
    static NonnullRefPtr<ExtensionBridge> create();

    using IPCResponseCallback = Function<void(String const& response_json)>;

    // Called from injected JS shim when extension makes an API call
    void handle_api_call(ExtensionAPI api, StringView payload_json, IPCResponseCallback callback);

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
