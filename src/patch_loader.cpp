// patch_loader_core.cpp
#include "patch_loader.hpp"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <algorithm>

#define LOG_ERROR(...) \
    UtilityFunctions::push_error("PatchLoader:", __func__, ":", __LINE__, ": ", __VA_ARGS__);

#define LOG_VERBOSE(...) \
    do { if (OS::get_singleton()->is_stdout_verbose()) UtilityFunctions::print("[PatchLoader] ", __VA_ARGS__); } while (false)

#define DEFINE_SETTING(path, def, type)                                 \
    {                                                                   \
        if (!ProjectSettings::get_singleton()->has_setting(path)) {     \
            ProjectSettings::get_singleton()->set_setting(path, def);   \
        }                                                               \
        Dictionary info;                                                \
        info["name"] = path;                                            \
        info["type"] = type;                                            \
        ProjectSettings::get_singleton()->add_property_info(info);      \
        ProjectSettings::get_singleton()->set_initial_value(path, def); \
    }

PatchLoader *PatchLoader::singleton = nullptr;

PatchLoader::PatchLoader() {
    ERR_FAIL_COND(singleton != nullptr);
    singleton = this;
}

PatchLoader::~PatchLoader() {
    ERR_FAIL_COND(singleton != this);
    singleton = nullptr;
}

PatchLoader *PatchLoader::get_singleton() {
    return singleton;
}

void PatchLoader::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_error_code"), &PatchLoader::get_error_code);

    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_OK)
    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_LACKING_PERMISSIONS)
    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_DIRECTORY_NOT_FOUND)
    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_PATCH_FILE_INVALID_NAME)
    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_PATCH_FILE_INVALID_ORDER)
    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_PATCH_FILE_NOT_FOUND)
    BIND_ENUM_CONSTANT(PATCH_ERROR_CODE_PATCH_LOAD_ERROR)
}

void PatchLoader::load_patches() {
    LOG_VERBOSE("Initializing");

    // Add to project settings.
    DEFINE_SETTING(PROJECT_SETTINGS_ALERT_SHOW_ON_ERROR, true, Variant::BOOL);
    DEFINE_SETTING(PROJECT_SETTINGS_ALERT_TITLE, "Launch Error", Variant::STRING);
    DEFINE_SETTING(PROJECT_SETTINGS_ALERT_MESSAGE,"An unexpected error during loading has occurred and the game cannot start.", Variant::STRING);
    DEFINE_SETTING(PROJECT_SETTINGS_CRASH_ON_ERROR, true, Variant::BOOL);

    // Skip in Editor
    if (OS::get_singleton()->has_feature("editor") || Engine::get_singleton()->is_editor_hint()) {
        LOG_VERBOSE("Editor build detected, skipping patch loading.");
        return;
    }

    // Patch dir is beside binary
    String patch_directory = OS::get_singleton()->get_executable_path().get_base_dir().path_join("patches/");
    LOG_VERBOSE("Patch directory is: ", patch_directory);


    // Create the dir when needed.
    if (!DirAccess::dir_exists_absolute(patch_directory)) {
        LOG_VERBOSE("Patch directory does not exist, creating: " + patch_directory);
        Error error = DirAccess::make_dir_absolute(patch_directory);
        if (error != OK) {
            LOG_ERROR("Could not create patch directory, either create it manually or update your permissions: " +
                      patch_directory);
            _set_error(PATCH_ERROR_CODE_LACKING_PERMISSIONS);
            return;
        } else {
            LOG_VERBOSE("Patch directory created successfully.");
        }
    }

    // open folder
    Ref<DirAccess> dir = DirAccess::open(patch_directory);
    if (dir.is_null()) {
        LOG_ERROR("Could not open patch directory.");
        _set_error(PATCH_ERROR_CODE_DIRECTORY_NOT_FOUND);
        return;
    }

    // collect (order, filename) pairs
    std::vector<std::pair<int, String>> entries;
    dir->list_dir_begin();

    for (const String &file_name: dir->get_files()) {
        if (!file_name.get_extension().ends_with("pck"))
            continue;

        PackedStringArray split = file_name.split("_");
        if (split.size() < 2 || split[0] != String("patch") || !split[1].is_valid_int()) {
            LOG_ERROR("Invalid patch filename: " + file_name + ". Expected name to begin with: patch_<order>.pck");
            _set_error(PATCH_ERROR_CODE_PATCH_FILE_INVALID_NAME);
            return;
        }

        int64_t order = split[1].to_int();
        if (order < 0) {
            LOG_ERROR("Invalid patch order in filename: " + file_name);
            _set_error(PATCH_ERROR_CODE_PATCH_FILE_INVALID_ORDER);
            return;
        }

        String filePath = patch_directory.path_join(file_name);

        entries.emplace_back(order, filePath);
    }

    LOG_VERBOSE("Found " + String::num_int64(entries.size()) + " patches in directory: " + patch_directory);
    for (const auto &entry: entries) {
        LOG_VERBOSE("Found patch: " + entry.second + " with order: " + String::num_int64(entry.first));
    }

    // Sort by order
    std::sort(entries.begin(), entries.end(), [](const std::pair<int, String> &a, const std::pair<int, String> &b) {
        return a.first < b.first;
    });
    LOG_VERBOSE("Sorted patches by order.");

    // Load patches in order
    for (const auto &entry: entries) {
        String patch_file = entry.second;
        LOG_VERBOSE("Loading patch: " + patch_file);

        // Literally cannot happen unless the file somehow got deleted right after we checked it.
        if (!FileAccess::file_exists(patch_file)) {
            LOG_ERROR("Patch file not found: " + patch_file);
            _set_error(PATCH_ERROR_CODE_PATCH_FILE_NOT_FOUND);
            return;
        }

        if (!ProjectSettings::get_singleton()->load_resource_pack(patch_file)) {
            LOG_ERROR("Failed to load patch: " + patch_file);
            _set_error(PATCH_ERROR_CODE_PATCH_LOAD_ERROR);
            return;
        }
    }
}

bool is_headless_mode() {
    // we have to do this because DisplayServer::get_singleton() is not available at this point
    PackedStringArray args = OS::get_singleton()->get_cmdline_args();

    // PackedStringArray doesn't seem to support std::any_of, so we have to use a loop
    for (const String &arg: args) {
        if (arg == "--headless" || arg.begins_with("--headless")) {
            return true;
        }
    }
    return false;
}

void PatchLoader::_set_error(PatchLoader::PatchErrorCode error_code) {
    last_error = error_code;
    String error_code_str = "PL-" + String::num_int64(error_code);
    LOG_ERROR("" + error_code_str);

    auto ps = ProjectSettings::get_singleton();

    if (!is_headless_mode() && ps->get_setting_with_override(PROJECT_SETTINGS_ALERT_SHOW_ON_ERROR)) {
        String alert_title = ps->get_setting_with_override(PROJECT_SETTINGS_ALERT_TITLE);
        String alert_message = ps->get_setting_with_override(PROJECT_SETTINGS_ALERT_MESSAGE);
        OS::get_singleton()->alert(alert_message, alert_title + ": " + error_code_str);
    }

    if (ps->get_setting_with_override(PROJECT_SETTINGS_CRASH_ON_ERROR)) {
        CRASH_NOW_MSG("PatchLoader Error: " + error_code_str +
                      ". An unexpected error during loading has occurred and the game cannot start.");
    }
}

