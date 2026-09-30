/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Gecko WebExtensions Background Script Runtime.
 * Runs extension background scripts in an isolated JS context
 * and bridges IPC messages between content scripts and native APIs.
 */

#pragma once

#include <AK/String.h>
#include <AK/HashMap.h>
#include <AK/Function.h>
#include <AK/RefCounted.h>
#include <AK/NonnullRefPtr.h>
#include <AK/OwnPtr.h>
#include <AK/Vector.h>

namespace GeckoShim {

struct ExtensionManifest {
    String name;
    String version;
    String description;
    Vector<String> permissions;
    Vector<String> content_scripts_matches;
    String content_scripts_js;
    String background_script;
    bool is_active { false };
};

class ExtensionHost final : public RefCounted<ExtensionHost> {
public:
    static NonnullRefPtr<ExtensionHost> create();
    ~ExtensionHost();

    bool load_extension(StringView extension_path);
    void unload_extension(StringView extension_id);
    Vector<ExtensionManifest> const& loaded_extensions() const { return m_extensions; }

    String handle_content_script_message(StringView extension_id, StringView api_name, StringView payload_json);
    Vector<String> get_content_scripts_for_url(StringView url) const;
    bool extension_has_permission(StringView extension_id, StringView permission) const;

private:
    ExtensionHost();
    Optional<ExtensionManifest> parse_manifest(StringView extension_path);
    bool run_background_script(ExtensionManifest& manifest);

    struct JSRuntimeState;
    OwnPtr<JSRuntimeState> m_js_runtime;
    Vector<ExtensionManifest> m_extensions;
    HashMap<String, size_t> m_extension_index_by_id;
};

} // namespace GeckoShim
