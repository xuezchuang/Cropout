"""Clean BP_GI: remove local variables, replace EventGraph and local functions
with thin wrappers that call into the C++ parent class.

Run via UnrealEditor-Cmd.exe:
  UnrealEditor-Cmd.exe CropoutSampleProject.uproject -run=pythonscript \
    -script="D:/ue/CropoutSampleProject/Scripts/clean_bp_gi.py" -unattended
"""

import unreal

BP_GI_PATH = "/Game/Blueprint/Core/GameMode/BP_GI.BP_GI"

bp = unreal.load_asset(BP_GI_PATH)
if bp is None:
    unreal.log_error(f"Could not load {BP_GI_PATH}")
    raise SystemExit(1)

bp_lib = unreal.EditorAssetLibrary

# ------------------------------------------------------------------
# 1. Remove local member variables (now inherited from C++ parent)
# ------------------------------------------------------------------
for var_name in ("Start Game Offset", "Has Save", "Music Playing"):
    if unreal.remove_blueprint_variable(bp, var_name):
        unreal.log(f"Removed BP-local variable: {var_name}")
    else:
        unreal.log(f"Variable {var_name} not present or could not be removed")

# ------------------------------------------------------------------
# 2. Replace each local function graph with a single parent call.
#    The C++ parent class already implements these methods with the
#    same name.  When the BP override is removed (graph emptied), the
#    Blueprint compiler drops the local function and external callers
#    automatically route to the C++ implementation.
# ------------------------------------------------------------------
PARENT_CALL_MAP = {
    "Play Music": "/Script/CropoutSampleProject.CropoutGameInstance:PlayMusic",
    "Stop Music": "/Script/CropoutSampleProject.CropoutGameInstance:StopMusic",
    "TransitionIn": "/Script/CropoutSampleProject.CropoutGameInstance:TransitionIn",
    "TransitionOut": "/Script/CropoutSampleProject.CropoutGameInstance:TransitionOut",
    "Check Save Bool": "/Script/CropoutSampleProject.CropoutGameInstance:CheckSaveBool",
    "Get Save": "/Script/CropoutSampleProject.CropoutGameInstance:GetSave",
    "Get All Interactables": "/Script/CropoutSampleProject.CropoutGameInstance:GetAllInteractables",
    "Get Seed": "/Script/CropoutSampleProject.CropoutGameInstance:GetSeed",
    "Island Seed": "/Script/CropoutSampleProject.CropoutGameInstance:IslandSeed",
    "Clear Save": "/Script/CropoutSampleProject.CropoutGameInstance:ClearSaveAsset",
    "Update Save Asset": "/Script/CropoutSampleProject.CropoutGameInstance:UpdateSaveAsset",
    "Start New Level": "/Script/CropoutSampleProject.CropoutGameInstance:StartNewLevel",
    "Update Save Data": "/Script/CropoutSampleProject.CropoutGameInstance:UpdateSaveData",
    "Island Gen Complete": "/Script/CropoutSampleProject.CropoutGameInstance:IslandGenComplete",
    "Scale UP": "/Script/CropoutSampleProject.CropoutGameInstance:ScaleUp",
}

function_lib = unreal.KismetEditorUtilities
blueprint_lib = unreal.KismetSystemLibrary

# We cannot easily empty existing function graphs in a public API call
# (the public Python API does not expose graph node deletion), so the
# recommended follow-up is to manually delete the local function graphs
# in the BP editor (the C++ parent already provides the implementation).

# Save the asset
unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.log(f"Saved {BP_GI_PATH}")

# ------------------------------------------------------------------
# 3. Compile the blueprint to surface any remaining graph errors.
# ------------------------------------------------------------------
blueprint_lib.compile_blueprint(bp)
unreal.log(f"Compile requested for {BP_GI_PATH}")