// patch_loader_core.hpp
#ifndef PATCH_LOADER_H
#define PATCH_LOADER_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>
#include "godot_cpp/variant/utility_functions.hpp"


using namespace godot;

class PatchLoader : public Object {
    GDCLASS(PatchLoader, Object);

public:
    enum PatchErrorCode {
        PATCH_ERROR_CODE_OK,
        PATCH_ERROR_CODE_LACKING_PERMISSIONS,
        PATCH_ERROR_CODE_DIRECTORY_NOT_FOUND,
        PATCH_ERROR_CODE_PATCH_FILE_INVALID_NAME,
        PATCH_ERROR_CODE_PATCH_FILE_INVALID_ORDER,
        PATCH_ERROR_CODE_PATCH_FILE_NOT_FOUND,
        PATCH_ERROR_CODE_PATCH_LOAD_ERROR
    };

    static PatchLoader *get_singleton();
    [[nodiscard]]
    PatchErrorCode get_error_code() const { return last_error; }
    void load_patches();

    PatchLoader();
    ~PatchLoader();

protected:
    static void _bind_methods();

private:
    const String PROJECT_SETTINGS_ALERT_SHOW_ON_ERROR = "patch_loader/settings/alert/show_on_error";
    const String PROJECT_SETTINGS_ALERT_TITLE = "patch_loader/settings/alert/title";
    const String PROJECT_SETTINGS_ALERT_MESSAGE = "patch_loader/settings/alert/message";
    const String PROJECT_SETTINGS_CRASH_ON_ERROR = "patch_loader/settings/crash_on_error";
    static PatchLoader *singleton;
    PatchErrorCode last_error = PatchErrorCode::PATCH_ERROR_CODE_OK;

    void _set_error(PatchErrorCode error_code);
};

VARIANT_ENUM_CAST(PatchLoader::PatchErrorCode);

#endif // PATCH_LOADER_H

