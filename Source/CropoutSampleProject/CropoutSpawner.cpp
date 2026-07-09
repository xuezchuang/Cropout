// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutSpawner.h"

#include "GameFramework/Actor.h"

ACropoutSpawner::ACropoutSpawner()
{
	// Empty constructor: AActor's defaults apply.
}

void ACropoutSpawner::SpawnMeshOnly(AActor* SpawnRef)
{
	// BP DSL (BP_Spawner.Spawn Mesh Only): spawns mesh-only actors at
	// the SpawnRef's transform. Full implementation is BP-side; once
	// the BP child is reparented and the BP function body deleted,
	// this becomes the authoritative implementation.
	//
	// Round 12.B stub: try ProcessEvent on the BP-side function name
	// first (works before the BP graph is redirected to Call Parent);
	// this keeps runtime behaviour identical until R13+ redirects.
	if (UFunction* Fn = GetClass()->FindFunctionByName(TEXT("SpawnMeshOnly")))
	{
		struct FParams { AActor* SpawnRef = nullptr; };
		FParams P; P.SpawnRef = SpawnRef;
		ProcessEvent(Fn, &P);
	}
}

void ACropoutSpawner::SpawnRandom(AActor* SpawnRef)
{
	// BP DSL (BP_Spawner.Spawn Random): randomised spawning around the
	// SpawnRef. Same forward-to-BP pattern as SpawnMeshOnly above.
	if (UFunction* Fn = GetClass()->FindFunctionByName(TEXT("SpawnRandom")))
	{
		struct FParams { AActor* SpawnRef = nullptr; };
		FParams P; P.SpawnRef = SpawnRef;
		ProcessEvent(Fn, &P);
	}
}

void ACropoutSpawner::LoadSpawn()
{
	// BP DSL (BP_Spawner.Load Spawn): rehydrates saved spawn instances
	// from the save game. Forward-to-BP for round 12.B.
	if (UFunction* Fn = GetClass()->FindFunctionByName(TEXT("LoadSpawn")))
	{
		ProcessEvent(Fn, nullptr);
	}
}
