// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CropoutSpawner.generated.h"

class UDataTable;
class ULevelStreaming;
class UWorld;
class AActor;

/**
 * UCropoutSpawner
 *
 * C++ parent class for `BP_Spawner` (in
 * `Plugins/IslandGenerator/Content/Spawner/BP_Spawner.uasset`).
 *
 * Round 12.B scope (current):
 *  - 1:1 mirror of the three BP-defined functions found via direct MCP
 *    `get_connected_subgraph` on the live editor:
 *
 *     Display name (BP)        | Internal BP name | C++ UFUNCTION
 *     --------------------------|------------------|-----------------------
 *     `Spawn Mesh Only`        | `SpawnMeshOnly`  | `SpawnMeshOnly`
 *     `Spawn Random`           | `SpawnRandom`    | `SpawnRandom`
 *     `Load Spawn`             | `LoadSpawn`      | `LoadSpawn`
 *
 *  - The BP-side graph bodies are left intact for now (BP_Spawner is
 *    a runtime actor; existing PIE / SaveGame flow expects to call
 *    its BP-side implementations through the BP child class). Once
 *    the BP child is reparented to this class and the user-defined
 *    function subgraphs are deleted (or routed to `Call Parent.<X>`),
 *    the native UFUNCTIONs take over.
 *
 * Round 12.B follow-on: a fourth BP-side name (`BeginASyncSpawning`)
 * was assumed during planning but, on direct MCP inspection,
 * turned out to be a `Custom` event on `BP_GM` (i.e. a GameMode
 * EventGraph entry point), not a function on `BP_Spawner`. So the
 * "four spawner functions" plan was off by one — this round lifts
 * three.
 *
 * Signatures (per the BP subgraph dumps):
 *  - `SpawnMeshOnly(SpawnRef)` — void; takes a spawn ref (an
 *    AActor-style reference that BP-side held as the local
 *    `SpawnRef` var, typed `AActor*`).
 *  - `SpawnRandom(SpawnRef)` — void; same parameter.
 *  - `LoadSpawn()` — void; no parameters.
 *
 * The implementations are stubs that broadcast to their BP-side
 * equivalents via `ProcessEvent` reflection when the BP child is
 * not reparented; once the BP graph is deleted/redirected, the
 * implementation bodies can be filled in. For round 12.B, the goal
 * is the 1:1 mirror of names + signatures + compile, not the full
 * BP-graph translation.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACropoutSpawner();

	/** BP parity: `Spawn Mesh Only` (display) / `SpawnMeshOnly` (internal). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Spawner")
	void SpawnMeshOnly(AActor* SpawnRef);

	/** BP parity: `Spawn Random` (display) / `SpawnRandom` (internal). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Spawner")
	void SpawnRandom(AActor* SpawnRef);

	/** BP parity: `Load Spawn` (display) / `LoadSpawn` (internal). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Spawner")
	void LoadSpawn();
};
