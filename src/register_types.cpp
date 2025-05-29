#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>

#include "patch_loader.hpp"

using namespace godot;

static PatchLoader *patch_loader;

void gdextension_initialize(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
	{
        GDREGISTER_CLASS(PatchLoader);

        patch_loader = memnew(PatchLoader);
        Engine::get_singleton()->register_singleton("PatchLoader", PatchLoader::get_singleton());

        // Must run early to ensure patches are loaded before scripts.
        patch_loader->load_patches();
	}
}

void gdextension_terminate(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
        Engine::get_singleton()->unregister_singleton("PatchLoader");
        memdelete(patch_loader );
    }
}

extern "C"
{
	GDExtensionBool GDE_EXPORT gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(gdextension_initialize);
		init_obj.register_terminator(gdextension_terminate);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
