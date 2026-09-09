# TestProject — UE 5.7 + VibeUE MCP Template

Empty Unreal Engine 5.7.4 project with the **VibeUE** MCP plugin preinstalled
and preconfigured. Duplicate this folder to start a new project without
repeating the MCP setup.

## What's baked in

| Piece | Location |
|---|---|
| VibeUE plugin (with Linux binaries + proxy script) | `Plugins/VibeUE/` |
| VibeUE API key + MCP ports | `Config/DefaultEditorPerProjectUserSettings.ini` |
| opencode MCP client config | `opencode.json` |
| Editor + proxy launcher | `start-vibeue.sh` (self-locating) |
| AI assistant instructions | `AGENTS.md` |

## Using the template

1. Duplicate this folder and rename it (and the `.uproject`) to your project name.
2. Rename the C++ module:
   - `Source/TestProject/` → `Source/<NewName>/` (rename `TestProject.h/.cpp/.Build.cs` and the class names inside)
   - `Source/TestProject.Target.cs` → `Source/<NewName>.Target.cs` (class `<NewName>Target`, `ExtraModuleNames.Add("<NewName>")`)
   - `Source/TestProjectEditor.Target.cs` → `Source/<NewName>Editor.Target.cs` (class `<NewName>EditorTarget`)
   - In the `.uproject`: module `Name` → `<NewName>`
3. Regenerate project files, then build:
   ```sh
   "/mnt/data/UE 5.7.4/Engine/Build/BatchFiles/Linux/Build.sh" \
       <NewName>Editor Linux Development "$(pwd)/<NewName>.uproject" -waitmutex
   ```
4. Start the MCP stack: `./start-vibeue.sh` (editor + proxy on `http://127.0.0.1:8089/mcp`).

## Files safe to delete in a copy

`Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`, `Content/*` —
all regenerated. Keep `Plugins/VibeUE/Binaries/` so the plugin never needs
recompiling.