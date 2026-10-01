/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Gecko WebExtensions Background Script Runtime.
 * Parses extension manifests, matches content scripts against a URL, and
 * routes IPC messages coming from injected content scripts.
 */

#pragma once

#include <AK/HashMap.h>
#include <AK/NonnullRefPtr.h>
#include <AK/Optional.h>
#include <AK/RefCounted.h>
#include <AK/String.h>
#include <AK/Types.h>
#include <AK/Vector.h>

namespace GeckoShim {

struct ExtensionManifest {
    String name;
    String version;
    String description;
    i32 manifest_version { 2 };
    // Firefox lets an extension declare its own stable ID; Chrome derives one
    // from the signing key. We keep whatever the manifest declared, if anything.
    String declared_extension_id;
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

    // Load an extension from a directory containing manifest.json.
    bool load_extension(StringView extension_path);

    // Unload an extension by its ID (a hex digest of the extension path).
    void unload_extension(StringView extension_id);

    Vector<ExtensionManifest> const& loaded_extensions() const { return m_extensions; }

    // Handle an IPC message from a content script. Returns a JSON response string.
    String handle_content_script_message(StringView extension_id, StringView api_name, StringView payload_json);

    // Content script sources that should be injected into the given URL.
    Vector<String> get_content_scripts_for_url(StringView url) const;

    bool extension_has_permission(StringView extension_id, StringView permission) const;

private:
    ExtensionHost();

    Optional<ExtensionManifest> parse_manifest(StringView extension_path);
    bool run_background_script(ExtensionManifest& manifest);
    Optional<size_t> index_for_extension_id(StringView extension_id) const;

    Vector<ExtensionManifest> m_extensions;
    HashMap<String, size_t> m_extension_index_by_id;
};

} // namespace GeckoShim
