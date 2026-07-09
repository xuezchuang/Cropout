// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CropoutResourceType.generated.h"

/**
 * ECropoutResourceType
 *
 * Native mirror of `Content/Blueprint/Interactable/Extras/E_ResourceType.uasset`
 * (UserDefinedEnum). Aligned to BP-side internal values in
 * round 12.C so a `static_cast<ECropoutResourceType>(BPValue)`
 * translation is a no-op for direct TMap preservation.
 *
 * BP-side internal numbering (verified via binary + MCP-readable
 * DisplayNameMap of E_ResourceType.uasset at round 12.C):
 *   - NewEnumerator0 (internal value 0)  -> "Wood"
 *   - NewEnumerator1 (internal value 1)  -> "None"
 *   - NewEnumerator5 (internal value 5)  -> "Stone"
 *   - NewEnumerator6 (internal value 6)  -> "Food"
 *
 * The 2/3/4 gap is from a deleted-then-readded-enum history in the
 * BP-side Editor; preserving the internal values exactly avoids a
 * BP-side re-edit that would invalidate every TMap<E_ResourceType,...>
 * serialised against the old values.
 *
 * Round 12.C scope:
 *   - Provide a strongly-typed enum so the C++ UFUNCTION signatures
 *     that operate on `Resources` (SaveGame) can drop the int32 cast
 *     bridge used in rounds 8/8.5.
 *   - Update UCropoutSaveGame::Resources to
 *     `TMap<ECropoutResourceType, int32>`.
 *   - The BP-side `BP_SaveGM` still holds its own BP-local
 *     `TMap<E_ResourceType, int32> Resources` BP-variable; the
 *     native Resources (now inherited from UCropoutSaveGame) is a
 *     SEPARATE UPROPERTY until the BP-side variable is deleted in a
 *     follow-up round (a function-graph-level edit; see round
 *     12.C follow-on items).
 */
UENUM(BlueprintType)
enum class ECropoutResourceType : uint8
{
	Wood    = 0  UMETA(DisplayName = "Wood"),   // BP NewEnumerator0
	None    = 1  UMETA(DisplayName = "None"),   // BP NewEnumerator1
	Stone   = 5  UMETA(DisplayName = "Stone"),  // BP NewEnumerator5
	Food    = 6  UMETA(DisplayName = "Food"),   // BP NewEnumerator6
};
