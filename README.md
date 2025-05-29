
# Godot Patch Loader

A GDExtension for Godot 4.4+ that enables loading patch PCK files early during game startup.

## Purpose

Godot 4.4 introduced creating patch PCK files, but doesn't provide an built-in way to load them. This extension solves that problem by loading patches at `MODULE_INITIALIZATION_LEVEL_CORE`, allowing patches to properly override GDScript files, .NET Assemblies*, Scenes, and Resources.

## Features

- Automatically creates a `patches` directory beside your game executable
- Loads PCK patch files in a specified order based on filename
- Handles errors with customizable alerts

## Installation

1. Download the latest release for your platform
2. Add the extension to your project
3. Export your game

## Usage

### Creating Patch Files

1. Export a [patch PCK file](https://docs.godotengine.org/en/stable/tutorials/export/exporting_pcks.html) from your Godot project containing the updated files
**Make sure you include your base PCK and all existing patches in the "Patches" tab to reduce patch file size and avoid unexpected behaviour!**
2. Name the PCK file following this pattern: `patch_<order>.pck` or `patch_<order>_additionalInfo.pck`
   - `<order>` is a number that determines loading priority (lower numbers load first)
   - Example: `patch_1.pck` loads before `patch_2.pck`
   - Example with description: `patch_1_fixSomeBug.pck`

### Adding Patches to your Game
- Place the PCK file in the `patches` directory beside your game executable **Note: Patches will only be applied once at game startup.** 

### Project Settings

The extension adds the following project settings:

- `patch_loader/settings/alert/show_on_error`: Show an alert dialog on error   
(default: true)  

- `patch_loader/settings/alert/title`: Title for the error alert  
(default: "Launch Error")  

- `patch_loader/settings/alert/message`: Message for the error alert  
(default: "An unexpected error during loading has occurred and the game cannot start.")  

- `patch_loader/settings/crash_on_error`: Whether to crash on error  
(default: true)

## Special Notes for C# Projects

If your project uses C#, you must enable `dotnet/embed_build_outputs` when exporting your project to ensure .NET assemblies can be overridden by patches.

## Known Issues

- GDExtension library files (.dll, .so, .dylib) cannot be patched with this method

## Error Codes

- `PL-1`: Lacking permissions to create patches directory
- `PL-2`: Patches directory not found
- `PL-3`: Invalid patch filename
- `PL-4`: Invalid patch order
- `PL-5`: Patch file not found
- `PL-6`: Patch load error

## License
MIT