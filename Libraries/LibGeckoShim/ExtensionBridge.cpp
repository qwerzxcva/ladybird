/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ExtensionBridge.h"
#include <AK/StringBuilder.h>

namespace GeckoShim {

NonnullRefPtr<ExtensionBridge> ExtensionBridge::create()
{
    return adopt_ref(*new ExtensionBridge());
}

void ExtensionBridge::handle_api_call(ExtensionAPI api, StringView payload_json, IPCResponseCallback callback)
{
    auto callback_id = m_next_callback_id++;
    m_pending_callbacks.set(callback_id, move(callback));

    // In a full implementation, this would serialize the call and send it
    // via IPC to the native extension host process. For now, we provide
    // stub responses that allow extensions to load without crashing.
    StringBuilder response_builder;
    switch (api) {
    case ExtensionAPI::RuntimeSendMessage:
        response_builder.append("{\"success\":true}"sv);
        break;
    case ExtensionAPI::StorageLocalGet:
        response_builder.append("{}"sv);
        break;
    case ExtensionAPI::TabsQuery:
        response_builder.append("[{\"id\":1,\"url\":\"about:blank\",\"active\":true}]"sv);
        break;
    default:
        response_builder.append("{\"error\":\"not_implemented\"}"sv);
        break;
    }

    if (auto cb = m_pending_callbacks.take(callback_id); cb.has_value())
        cb.value()(response_builder.to_string_without_validation());
}

void ExtensionBridge::register_content_script(StringView origin, StringView script_source)
{
    m_content_scripts.set(origin.to_string(), script_source.to_string());
}

void ExtensionBridge::unregister_content_script(StringView origin)
{
    m_content_scripts.remove(origin.to_string());
}

String ExtensionBridge::get_shim_javascript()
{
    return R"SHIM(
(function() {
    if (window.__ladybird_ext_shim_loaded) return;
    window.__ladybird_ext_shim_loaded = true;

    var callbacks = {};
    var nextId = 1;

    function sendMessage(apiName, payload) {
        return new Promise(function(resolve, reject) {
            var id = nextId++;
            callbacks[id] = { resolve: resolve, reject: reject };
            window.postMessage({
                type: 'LADYBIRD_EXT_IPC',
                id: id,
                api: apiName,
                payload: payload
            }, '*');
        });
    }

    window.addEventListener('message', function(event) {
        if (event.data && event.data.type === 'LADYBIRD_EXT_RESPONSE') {
            var cb = callbacks[event.data.id];
            if (cb) {
                delete callbacks[event.data.id];
                try {
                    cb.resolve(JSON.parse(event.data.response));
                } catch(e) {
                    cb.resolve(event.data.response);
                }
            }
        }
    });

    window.browser = {
        runtime: {
            sendMessage: function(msg) { return sendMessage('runtime.sendMessage', JSON.stringify(msg)); },
            getURL: function(path) { return 'moz-extension://shim/' + path; },
            onMessage: { addListener: function(){}, removeListener: function(){} }
        },
        storage: {
            local: {
                get: function(keys) { return sendMessage('storage.local.get', JSON.stringify(keys)); },
                set: function(items) { return sendMessage('storage.local.set', JSON.stringify(items)); }
            }
        },
        tabs: {
            query: function(queryInfo) { return sendMessage('tabs.query', JSON.stringify(queryInfo)); },
            create: function(props) { return sendMessage('tabs.create', JSON.stringify(props)); }
        },
        webRequest: {
            onBeforeRequest: { addListener: function(){}, removeListener: function(){} }
        },
        cookies: {
            getAll: function(details) { return sendMessage('cookies.getAll', JSON.stringify(details)); },
            set: function(details) { return sendMessage('cookies.set', JSON.stringify(details)); }
        }
    };
})();
)SHIM"_string;
}

} // namespace GeckoShim
