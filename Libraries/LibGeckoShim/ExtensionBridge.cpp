/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ExtensionBridge.h"

namespace GeckoShim {

NonnullRefPtr<ExtensionBridge> ExtensionBridge::create()
{
    return adopt_ref(*new ExtensionBridge());
}

void ExtensionBridge::handle_api_call(StringView api_name, StringView, IPCResponseCallback callback)
{
    auto callback_id = m_next_callback_id++;
    m_pending_callbacks.set(callback_id, move(callback));

    // A real implementation would forward this to the extension host process over
    // IPC and answer from there. Until that is wired up, answer with shapes that
    // let an extension finish loading instead of throwing.
    String response;
    if (api_name == "storage.local.get"sv || api_name == "storage.local.remove"sv || api_name == "storage.local.clear"sv)
        response = "{}"_string;
    else if (api_name == "tabs.query"sv)
        response = "[{\"id\":1,\"url\":\"about:blank\",\"active\":true}]"_string;
    else if (api_name == "cookies.getAll"sv)
        response = "[]"_string;
    else if (api_name == "permissions.contains"sv)
        response = "false"_string;
    else if (api_name == "runtime.sendMessage"sv || api_name == "storage.local.set"sv || api_name == "cookies.set"sv)
        response = "{\"success\":true}"_string;
    else
        response = "{}"_string;

    if (auto pending = m_pending_callbacks.take(callback_id); pending.has_value())
        pending.value()(response);
}

void ExtensionBridge::register_content_script(StringView origin, StringView script_source)
{
    m_content_scripts.set(String::from_utf8_without_validation(origin.bytes()), String::from_utf8_without_validation(script_source.bytes()));
}

void ExtensionBridge::unregister_content_script(StringView origin)
{
    m_content_scripts.remove(String::from_utf8_without_validation(origin.bytes()));
}

String ExtensionBridge::get_shim_javascript()
{
    return R"SHIM(
(function () {
    if (window.__ladybird_extension_shim_installed) return;
    window.__ladybird_extension_shim_installed = true;

    var pending = {};
    var nextId = 1;

    function callNative(apiName, payload) {
        return new Promise(function (resolve) {
            var id = nextId++;
            pending[id] = resolve;
            window.postMessage({
                type: 'LADYBIRD_EXTENSION_CALL',
                id: id,
                api: apiName,
                payload: payload
            }, '*');
        });
    }

    window.addEventListener('message', function (event) {
        if (!event.data || event.data.type !== 'LADYBIRD_EXTENSION_RESULT') return;
        var resolve = pending[event.data.id];
        if (!resolve) return;
        delete pending[event.data.id];
        var value = event.data.value;
        try { value = JSON.parse(event.data.value); } catch (e) { /* keep the raw string */ }
        resolve(value);
    });

    // Firefox's browser.* always returns a promise. Chrome's chrome.* takes a
    // callback, and only returns a promise in Manifest V3. `dual` supports both
    // calling conventions so the same extension code works on either API.
    function dual(apiName) {
        return function (payload, callback) {
            var promise = callNative(apiName, JSON.stringify(payload === undefined ? {} : payload));
            if (typeof callback === 'function') {
                promise.then(function (value) { callback(value); });
                return undefined;
            }
            return promise;
        };
    }

    function extensionURL(path) {
        return (window.__ladybird_extension_scheme || 'moz-extension') + '://ladybird/' + path;
    }

    function eventStub() {
        return { addListener: function () {}, removeListener: function () {}, hasListener: function () { return false; } };
    }

    var storageArea = {
        get: dual('storage.local.get'),
        set: dual('storage.local.set'),
        remove: dual('storage.local.remove'),
        clear: dual('storage.local.clear')
    };

    var api = {
        runtime: {
            sendMessage: dual('runtime.sendMessage'),
            getManifest: function () { return window.__ladybird_extension_manifest || {}; },
            getURL: extensionURL,
            connect: function () {
                return { postMessage: function () {}, disconnect: function () {}, onMessage: eventStub() };
            },
            onMessage: eventStub(),
            onInstalled: eventStub(),
            lastError: null
        },
        storage: { local: storageArea, sync: storageArea, managed: storageArea, session: storageArea },
        tabs: {
            query: dual('tabs.query'),
            create: dual('tabs.create'),
            update: dual('tabs.update'),
            sendMessage: dual('tabs.sendMessage')
        },
        cookies: {
            get: dual('cookies.get'),
            getAll: dual('cookies.getAll'),
            set: dual('cookies.set'),
            remove: dual('cookies.remove')
        },
        webRequest: {
            onBeforeRequest: eventStub(),
            onBeforeSendHeaders: eventStub(),
            onHeadersReceived: eventStub()
        },
        scripting: { executeScript: dual('scripting.executeScript') },
        permissions: { contains: dual('permissions.contains'), request: dual('permissions.request') }
    };

    window.browser = api;
    window.chrome = api;
})();
)SHIM"_string;
}

} // namespace GeckoShim
