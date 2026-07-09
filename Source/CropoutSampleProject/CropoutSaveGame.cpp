// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutSaveGame.h"

#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

UCropoutSaveGame::UCropoutSaveGame()
{
	// Empty constructor: USaveGame's defaults apply. Interactables /
	// Villagers / Resources are default-constructed empty by their
	// own default-initialisers.
}

// ============================================================================
// BP-parity bodies
//
// Each body is a direct translation of the corresponding BP-side
// function on BP_SaveGM (captured via CropoutGameInstance.cpp's
// BP DSL comments for round 7's reflection calls). Once
// BP_SaveGM is reparented to UCropoutSaveGame and the reflection
// calls in CropoutGameInstance.cpp are rewritten, these bodies
// take over the runtime path.
// ============================================================================

void UCropoutSaveGame::ClearInteractables()
{
	// BP DSL: Clear(SaveRef.Interactables)
	Interactables.Reset();
}

void UCropoutSaveGame::AddInteractable(const FTransform& Transform, UClass* Class, int32 ProgressionState, FName Tag, AActor* Source)
{
	// BP DSL: SetArrayElem(SaveRef.Interactables, Actor, GetActorTransform(Actor),
	//                       GetClass(Actor), BPInteractable.GetProgressionState(Actor),
	//                       Get(Actor.Tags, 0), true)
	//
	// `Source` is the source BP_GI was iterating. We do not keep
	// a hard reference to the source actor in the snapshot
	// (linearly serialised saves must not pin live actors); the
	// class+transform+state is enough to rehydrate.
	FCropoutSaveInteract Entry;
	Entry.Transform = Transform;
	Entry.Class = Class;
	Entry.ProgressionState = ProgressionState;
	Entry.Tag = Tag;
	Entry.bIsValid = true;
	Interactables.Add(MoveTemp(Entry));
}

void UCropoutSaveGame::ClearVillagers()
{
	// BP DSL: Clear(SaveRef.Villagers)
	Villagers.Reset();
}

void UCropoutSaveGame::AddVillager(const FVector& Location, FName Tag, AActor* Source)
{
	// BP DSL: SetArrayElem(SaveRef.Villagers, Actor, GetActorLocation(Actor),
	//                       Get(Actor.Tags, 0), true)
	FCropoutSaveVillager Entry;
	Entry.Location = Location;
	Entry.Tag = Tag;
	Entry.bIsValid = true;
	Villagers.Add(MoveTemp(Entry));
}

void UCropoutSaveGame::SetResources(const TMap<ECropoutResourceType, int32>& InResources)
{
	// BP DSL: SaveRef.Resources = NewParam
	// Round 12.C: native key type is ECropoutResourceType (BP-internal
	// values Wood=0, None=1, Stone=5, Food=6 are preserved 1:1).
	Resources = InResources;
}

TMap<ECropoutResourceType, int32> UCropoutSaveGame::GetResources() const
{
	// BP DSL: return SaveRef.Resources
	return Resources;
}

TArray<FCropoutSaveInteract> UCropoutSaveGame::GetInteractables() const
{
	// BP DSL: return SaveRef.Interactables
	return Interactables;
}

TArray<FCropoutSaveVillager> UCropoutSaveGame::GetVillagers() const
{
	// BP DSL: return SaveRef.Villagers
	return Villagers;
}

void UCropoutSaveGame::ClearSave()
{
	// BP DSL: ClearSave() — full wipe of all snapshot state.
	Interactables.Reset();
	Villagers.Reset();
	Resources.Reset();
	Seed = 0;
	PlayTime = 0.0f;
}

void UCropoutSaveGame::SetSeed()
{
	// BP DSL (from HandleEventClearSave):
	//   PCGSettings.SetSeed(MakeRandomStream(RandomInteger(2147483647)), SaveRef)
	// The literal `2147483647` is `MAX_int32`. We roll a fresh seed
	// from a non-deterministic entropy source; round 8+ may swap
	// to a PCG-side RNG once the PCG plugin is wired natively.
	Seed = FMath::Rand() ^ static_cast<int32>(FPlatformTime::Cycles());
}

int32 UCropoutSaveGame::GetSeed() const
{
	// BP DSL: return SaveRef.Seed
	return Seed;
}

void UCropoutSaveGame::SetPlayTime(float InPlayTime)
{
	// BP DSL: SetPlayTime(0.0, SaveRef)
	PlayTime = InPlayTime;
}
