/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ExtensionHost.h"
#include <AK/Debug.h>
#include <AK/JsonArray.h>
#include <AK/JsonObject.h>
#include <AK/JsonParser.h>
#include <AK/StringBuilder.h>
#include <LibCore/File.h>

namespace GeckoShim {

// FNV-1a: used to derive a stable extension ID from its install path.
static u32 path_hash(StringView path)
{
    u32 hash = 2166136261u;
    for (auto byte : path.bytes()) {
        hash ^= byte;
        hash *= 16777619u;
    }
    return hash;
}

static Optional<String> read_text_file(StringView path)
{
    auto file = Core::File::open(path, Core::File::OpenMode::Read);
    if (file.is_error())
        return {};
    auto data = file.value()->read_until_eof();
    if (data.is_error())
        return {};
    return String::from_utf8_without_validation(data.value().bytes());
}

NonnullRefPtr<ExtensionHost> ExtensionHost::create()
{
    return adopt_ref(*new ExtensionHost());
}

ExtensionHost::ExtensionHost() = default;
ExtensionHost::~ExtensionHost() = default;

Optional<ExtensionManifest> ExtensionHost::parse_manifest(StringView extension_path)
{
    auto manifest_path = String::formatted("{}/manifest.json", extension_path);
    if (manifest_path.is_error())
        return {};

    auto content = read_text_file(manifest_path.value());
    if (!content.has_value())
        return {};

    auto parsed = AK::JsonParser::parse(*content);
    if (parsed.is_error())
        return {};

    auto& value = parsed.value();
    if (!value.is_object())
        return {};

    auto const& object = value.as_object();
    ExtensionManifest manifest;

    if (auto name = object.get_string("name"sv); name.has_value())
        manifest.name = name.value();
    if (auto version = object.get_string("version"sv); version.has_value())
        manifest.version = version.value();
    if (auto description = object.get_string("description"sv); description.has_value())
        manifest.description = description.value();
    if (auto manifest_version = object.get_i32("manifest_version"sv); manifest_version.has_value())
        manifest.manifest_version = manifest_version.value();

    // Firefox (browser_specific_settings) and older Firefox (applications) both
    // carry a gecko.id that extensions use as their stable identity.
    auto read_gecko_id = [&manifest](JsonObject const& container) {
        if (auto gecko = container.get_object("gecko"sv); gecko.has_value()) {
            if (auto id = gecko->get_string("id"sv); id.has_value())
                manifest.declared_extension_id = id.value();
        }
    };
    if (auto settings = object.get_object("browser_specific_settings"sv); settings.has_value())
        read_gecko_id(*settings);
    else if (auto applications = object.get_object("applications"sv); applications.has_value())
        read_gecko_id(*applications);

    if (auto permissions = object.get_array("permissions"sv); permissions.has_value()) {
        for (auto const& permission : permissions->values()) {
            if (permission.is_string())
                manifest.permissions.append(permission.as_string());
        }
    }

    if (auto content_scripts = object.get_array("content_scripts"sv); content_scripts.has_value()) {
        for (auto const& entry : content_scripts->values()) {
            if (!entry.is_object())
                continue;
            auto const& script = entry.as_object();

            if (auto matches = script.get_array("matches"sv); matches.has_value()) {
                for (auto const& match : matches->values()) {
                    if (match.is_string())
                        manifest.content_scripts_matches.append(match.as_string());
                }
            }

            if (auto js_files = script.get_array("js"sv); js_files.has_value()) {
                StringBuilder builder;
                for (auto const& js_file : js_files->values()) {
                    if (!js_file.is_string())
                        continue;
                    auto js_path = String::formatted("{}/{}", extension_path, js_file.as_string());
                    if (js_path.is_error())
                        continue;
                    if (auto source = read_text_file(js_path.value()); source.has_value()) {
                        builder.append(*source);
                        builder.append('\n');
                    }
                }
                auto joined = builder.to_string();
                if (!joined.is_error())
                    manifest.content_scripts_js = joined.release_value();
            }
        }
    }

    if (auto background = object.get_object("background"sv); background.has_value()) {
        // Manifest V3 (Chrome) declares a service worker; V2 declares scripts.
        if (auto service_worker = background->get_string("service_worker"sv); service_worker.has_value()) {
            auto service_worker_path = String::formatted("{}/{}", extension_path, service_worker.value());
            if (!service_worker_path.is_error()) {
                if (auto source = read_text_file(service_worker_path.value()); source.has_value())
                    manifest.background_script = source.release_value();
            }
        } else if (auto scripts = background->get_array("scripts"sv); scripts.has_value() && !scripts->values().is_empty()) {
            auto const& first_script = scripts->values().first();
            if (first_script.is_string()) {
                auto background_path = String::formatted("{}/{}", extension_path, first_script.as_string());
                if (!background_path.is_error()) {
                    if (auto source = read_text_file(background_path.value()); source.has_value())
                        manifest.background_script = source.release_value();
                }
            }
        }
    }

    manifest.is_active = true;
    return manifest;
}

bool ExtensionHost::load_extension(StringView extension_path)
{
    auto manifest = parse_manifest(extension_path);
    if (!manifest.has_value())
        return false;

    auto extension_id = String::formatted("{:08x}", path_hash(extension_path));
    if (extension_id.is_error())
        return false;

    size_t index = m_extensions.size();
    m_extensions.append(manifest.release_value());
    m_extension_index_by_id.set(extension_id.release_value(), index);

    if (!m_extensions[index].background_script.is_empty())
        run_background_script(m_extensions[index]);

    return true;
}

void ExtensionHost::unload_extension(StringView extension_id)
{
    auto index = index_for_extension_id(extension_id);
    if (!index.has_value())
        return;

    if (*index < m_extensions.size())
        m_extensions[*index].is_active = false;

    m_extension_index_by_id.remove(String::from_utf8_without_validation(extension_id.bytes()));
}

Optional<size_t> ExtensionHost::index_for_extension_id(StringView extension_id) const
{
    auto key = String::from_utf8_without_validation(extension_id.bytes());
    auto it = m_extension_index_by_id.find(key);
    if (it == m_extension_index_by_id.end())
        return {};
    return it->value;
}

String ExtensionHost::handle_content_script_message(StringView extension_id, StringView api_name, StringView)
{
    auto index = index_for_extension_id(extension_id);
    if (!index.has_value())
        return "{\"error\":\"unknown_extension\"}"_string;

    if (*index >= m_extensions.size() || !m_extensions[*index].is_active)
        return "{\"error\":\"extension_inactive\"}"_string;

    if (api_name == "storage.local.get"sv)
        return "{}"_string;
    if (api_name == "storage.local.set"sv)
        return "{\"success\":true}"_string;
    if (api_name == "runtime.sendMessage"sv)
        return "{\"success\":true}"_string;
    if (api_name == "tabs.query"sv)
        return "[{\"id\":1,\"url\":\"about:blank\",\"active\":true}]"_string;
    if (api_name == "cookies.getAll"sv)
        return "[]"_string;
    if (api_name == "cookies.set"sv)
        return "{\"success\":true}"_string;
    return "{\"error\":\"not_implemented\"}"_string;
}

Vector<String> ExtensionHost::get_content_scripts_for_url(StringView url) const
{
    Vector<String> scripts;
    for (auto const& extension : m_extensions) {
        if (!extension.is_active || extension.content_scripts_js.is_empty())
            continue;

        for (auto const& pattern : extension.content_scripts_matches) {
            auto pattern_view = pattern.bytes_as_string_view();

            bool matches = false;
            if (pattern_view == "<all_urls>"sv) {
                matches = true;
            } else if (pattern_view.ends_with("/*"sv)) {
                matches = url.starts_with(pattern_view.substring_view(0, pattern_view.length() - 2));
            } else {
                matches = (url == pattern_view);
            }

            if (matches) {
                scripts.append(extension.content_scripts_js);
                break;
            }
        }
    }
    return scripts;
}

bool ExtensionHost::extension_has_permission(StringView extension_id, StringView permission) const
{
    auto index = index_for_extension_id(extension_id);
    if (!index.has_value() || *index >= m_extensions.size())
        return false;

    for (auto const& entry : m_extensions[*index].permissions) {
        if (entry == permission)
            return true;
    }
    return false;
}

bool ExtensionHost::run_background_script(ExtensionManifest& manifest)
{
    if (manifest.background_script.is_empty())
        return false;

    dbgln("[GeckoShim] Loaded background script for '{}' ({} bytes)", manifest.name, manifest.background_script.bytes_as_string_view().length());
    return true;
}

} // namespace GeckoShim
