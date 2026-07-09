// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutGameInstance.h"

#include "CropoutGameMode.h"
#include "CropoutSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"
#include "Components/AudioComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

UCropoutGameInstance::UCropoutGameInstance()
{
	// Empty constructor: UGameInstance's defaults apply.
}

void UCropoutGameInstance::Init()
{
	Super::Init();
	HandleEventInit();
}

// ============================================================================
// Helpers
// ============================================================================

namespace
{
	/**
	 * Invoke a BP-side function on the BPSaveGM USaveGame object via
	 * UFunction reflection. Used by the event handlers below to mutate
	 * SaveRef until round 8+ lifts BPSaveGM to a native USaveGame
	 * subclass with C++ UPROPERTYs.
	 */
	bool CallBPSaveGameFunc(USaveGame* InSaveRef, const TCHAR* FuncName)
	{
		if (!InSaveRef) return false;
		UFunction* Func = InSaveRef->GetClass()->FindFunctionByName(FuncName);
		if (!Func) return false;
		InSaveRef->ProcessEvent(Func, nullptr);
		return true;
	}

	bool CallBPSaveGameFuncWithMap(USaveGame* InSaveRef, const TCHAR* FuncName, const TMap<ECropoutResourceType, int32>& InMap)
	{
		if (!InSaveRef) return false;
		UFunction* Func = InSaveRef->GetClass()->FindFunctionByName(FuncName);
		if (!Func) return false;
		struct FCallBPSaveGameFuncWithMap_Params { TMap<ECropoutResourceType, int32> Resources; };
		FCallBPSaveGameFuncWithMap_Params Params;
		Params.Resources = InMap;
		InSaveRef->ProcessEvent(Func, &Params);
		return true;
	}

	USaveGame* LoadBPSaveGameObject(UCropoutGameInstance* Self)
	{
		// BPSaveGM is BP-defined; round 8+ will lift to UCropoutSaveGame
		// and we can use a direct TObjectPtr. For round 7/8, locate the
		// BP-side SaveRef via the BP property name.
		USaveGame* Found = nullptr;
		if (FObjectProperty* ObjProp = FindFProperty<FObjectProperty>(Self->GetClass(), TEXT("SaveRef")))
		{
			void* PropPtr = ObjProp->ContainerPtrToValuePtr<void>(Self);
			Found = *static_cast<USaveGame**>(PropPtr);
		}
		return Found;
	}

	/**
	 * Round 8+ helper: return SaveRef as a UCropoutSaveGame* if the
	 * BP-side BP_SaveGM has been reparented to native UCropoutSaveGame
	 * (the post-reparent world). Returns nullptr otherwise so callers
	 * can fall back to UFunction reflection on the BP-side class.
	 */
	UCropoutSaveGame* GetNativeSaveGame(UCropoutGameInstance* Self)
	{
		return Cast<UCropoutSaveGame>(LoadBPSaveGameObject(Self));
	}

	UCropoutSaveGame* EnsureNativeSaveGame(UCropoutGameInstance* Self)
	{
		if (!Self)
		{
			return nullptr;
		}

		if (UCropoutSaveGame* Native = GetNativeSaveGame(Self))
		{
			return Native;
		}

		if (LoadBPSaveGameObject(Self))
		{
			return nullptr;
		}

		UCropoutSaveGame* Created = Cast<UCropoutSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UCropoutSaveGame::StaticClass()));
		if (Created)
		{
			Self->SaveRef = Created;
		}
		return Created;
	}

	/**
	 * Invoke a no-parameter function on an arbitrary UObject via
	 * UFunction reflection. Used for widget-bound events such as
	 * UI_Transition->TransIn / TransOut until those widgets are lifted.
	 */
	void CallNoParamFunction(UObject* Target, const TCHAR* FunctionName)
	{
		if (!Target)
		{
			return;
		}
		if (UFunction* Function = Target->GetClass()->FindFunctionByName(FunctionName))
		{
			Target->ProcessEvent(Function, nullptr);
		}
	}
}

// ============================================================================
// 9 EventGraph handler bodies
// ============================================================================

void UCropoutGameInstance::HandleEventUpdateAllInteractables()
{
	// BP DSL:
	//   Clear(SaveRef.Interactables)
	//   OutActors = GetAllActorsOfClass(BP_Interactable)
	//   ForEachLoop OutActors:
	//     SetArrayElem(SaveRef.Interactables, Actor, GetActorTransform(Actor),
	//                  GetClass(Actor), BPInteractable.GetProgressionState(Actor),
	//                  Get(Actor.Tags, index 0), true)
	//   SaveGame()
	UCropoutSaveGame* Native = EnsureNativeSaveGame(this);
	USaveGame*       Legacy = Native ? nullptr : LoadBPSaveGameObject(this);

	// Round 8.5: prefer native UCropoutSaveGame dispatch; fall back to
	// BP-side UFunction reflection when the BP_SaveGM reparent has not
	// landed (pre-reparent world).
	if (Native)
	{
		Native->ClearInteractables();
	}
	else
	{
		CallBPSaveGameFunc(Legacy, TEXT("ClearInteractables"));
	}

	TArray<AActor*> Interactables;
	UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), Interactables);

	for (AActor* Actor : Interactables)
	{
		if (!Actor) continue;

		// `GetProgressionState` is a BP-side helper on BP_Interactable
		// (no native mirror yet, round 11+ when interactable is lifted).
		// We try the BP-side helper via reflection; if it returns a
		// default int32 the save still has correct Transform/Class/Tag.
		int32 ProgressionState = 0;
		if (UFunction* ProgFunc = Actor->GetClass()->FindFunctionByName(TEXT("GetProgressionState")))
		{
			struct FProg_Params {};
			struct FProg_Result { int32 ReturnValue; };
			FProg_Result Result{};
			Actor->ProcessEvent(ProgFunc, &Result);
			ProgressionState = Result.ReturnValue;
		}

		FName ActorTag = NAME_None;
		if (Actor->Tags.Num() > 0)
		{
			ActorTag = Actor->Tags[0];
		}

		if (Native)
		{
			Native->AddInteractable(Actor->GetActorTransform(), Actor->GetClass(), ProgressionState, ActorTag, Actor);
		}
		else if (Legacy)
		{
			if (UFunction* AddFunc = Legacy->GetClass()->FindFunctionByName(TEXT("AddInteractable")))
			{
				struct FAddInteractable_Params { FTransform Transform; UClass* Class; int32 ProgressionState; FName Tag; AActor* Source; };
				FAddInteractable_Params Params;
				Params.Transform = Actor->GetActorTransform();
				Params.Class = Actor->GetClass();
				Params.ProgressionState = ProgressionState;
				Params.Tag = ActorTag;
				Params.Source = Actor;
				Legacy->ProcessEvent(AddFunc, &Params);
			}
		}
	}

	SaveGame_Implementation();
}

void UCropoutGameInstance::HandleEventInit()
{
	// BP DSL:
	//   ReturnValue = CreateWidget(UI_Transition)
	//   SetUI_Transition(ReturnValue)
	//   LoadGame()
	static const TSoftClassPtr<UUserWidget> TransitionClass(
		FSoftObjectPath(TEXT("/Game/UI/UI_Transition.UI_Transition_C")));
	if (UClass* LoadedClass = TransitionClass.LoadSynchronous())
	{
		UI_Transition = CreateWidget<UUserWidget>(GetWorld(), LoadedClass);
	}

	// LoadGame() call is via reflection until BP-side SaveRef
	// invocation is native-ified. Round 9 prefers the native
	// HandleLoadGame(); falls back to reflection if the BP-side
	// has its own LoadGame event graph that we haven't ported yet.
	if (UFunction* NativeFunc = GetClass()->FindFunctionByName(TEXT("HandleLoadGame")))
	{
		ProcessEvent(NativeFunc, nullptr);
	}
	else if (UFunction* LegacyFunc = GetClass()->FindFunctionByName(TEXT("LoadGame")))
	{
		ProcessEvent(LegacyFunc, nullptr);
	}
}

void UCropoutGameInstance::HandleOpenLevel(TSoftObjectPtr<UWorld> Level)
{
	// BP DSL:
	//   TransitionIn()
	//   Delay 1.1
	//   OpenLevel(byObjectReference, Level)
	// TransitionIn / OpenLevel are BP-side flow control + level open.
	// OpenLevel can be called natively via UGameplayStatics.
	//
	// Round 9 (renamed): the underlying function is now `DoTransitionIn`
	// so BP_GI's empty local `TransitionIn` graph does not shadow it.
	if (UFunction* TransInFunc = GetClass()->FindFunctionByName(TEXT("DoTransitionIn")))
	{
		ProcessEvent(TransInFunc, nullptr);
	}

	// Round 8+ can implement a proper async transition; for round 7
	// we use UGameplayStatics::OpenLevel by Level name (FName), falling
	// back to the BP-side OpenLevel if the soft path doesn't resolve.
	if (!Level.IsNull())
	{
		// UGameplayStatics::OpenLevel takes FName for level name; derive
		// from the soft path's asset name.
		const FString LevelName = Level.GetAssetName();
		UGameplayStatics::OpenLevel(this, FName(*LevelName));
	}
}

void UCropoutGameInstance::HandleEventUpdateAllResources(const TMap<ECropoutResourceType, int32>& Resources)
{
	// BP DSL:
	//   SaveRef.Resources = NewParam
	//   Delay 5.0
	//   SaveGame()
	if (UCropoutSaveGame* Native = EnsureNativeSaveGame(this))
	{
		Native->SetResources(Resources);
	}
	else
	{
		CallBPSaveGameFuncWithMap(LoadBPSaveGameObject(this), TEXT("SetResources"), Resources);
	}

	// Round 7 doesn't reproduce the 5s delay; round 8+ will replace
	// with FTimerHandle-based deferred save.
	SaveGame_Implementation();
}

void UCropoutGameInstance::HandleEventClearSave(bool ClearSeed)
{
	// BP DSL:
	//   ClearSave()
	//   if ClearSeed: PCGSettings.SetSeed(MakeRandomStream(RandomInteger(2147483647)), SaveRef)
	//   SetHasSave(false)
	if (UCropoutSaveGame* Native = EnsureNativeSaveGame(this))
	{
		Native->ClearSave();
		if (ClearSeed)
		{
			Native->SetSeed();
		}
	}
	else
	{
		CallBPSaveGameFunc(LoadBPSaveGameObject(this), TEXT("ClearSave"));
		if (ClearSeed)
		{
			// PCG-based random seed init is BP-side; we call the BP-side
			// helper "SetSeed" if it exists.
			CallBPSaveGameFunc(LoadBPSaveGameObject(this), TEXT("SetSeed"));
		}
	}
	HasSave = false;
}

void UCropoutGameInstance::HandleEventLoadLevel()
{
	// BP DSL: SetHasSave(true)
	HasSave = true;
}

void UCropoutGameInstance::HandleEventUpdateAllVillagers()
{
	// BP DSL:
	//   Clear(SaveRef.Villagers)
	//   OutActors = GetAllActorsOfClass(Pawn)
	//   ForEachLoop OutActors:
	//     if OutActor != GetPlayerPawn(0):
	//       SetArrayElem(SaveRef.Villagers, Actor, GetActorLocation(Actor), Get(Actor.Tags, 0), true)
	//   SaveGame()
	UCropoutSaveGame* Native = EnsureNativeSaveGame(this);
	USaveGame*       Legacy = Native ? nullptr : LoadBPSaveGameObject(this);

	if (Native)
	{
		Native->ClearVillagers();
	}
	else
	{
		CallBPSaveGameFunc(Legacy, TEXT("ClearVillagers"));
	}

	UWorld* World = GetWorld();
	if (!World) return;
	TArray<AActor*> AllPawns;
	UGameplayStatics::GetAllActorsOfClass(this, APawn::StaticClass(), AllPawns);
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	for (AActor* Actor : AllPawns)
	{
		if (!Actor || Actor == PlayerPawn) continue;

		FName ActorTag = NAME_None;
		if (Actor->Tags.Num() > 0)
		{
			ActorTag = Actor->Tags[0];
		}

		if (Native)
		{
			Native->AddVillager(Actor->GetActorLocation(), ActorTag, Actor);
		}
		else if (Legacy)
		{
			if (UFunction* AddVillagerFunc = Legacy->GetClass()->FindFunctionByName(TEXT("AddVillager")))
			{
				struct FAddVillager_Params { FVector Location; FName Tag; AActor* Source; };
				FAddVillager_Params Params;
				Params.Location = Actor->GetActorLocation();
				Params.Tag = ActorTag;
				Params.Source = Actor;
				Legacy->ProcessEvent(AddVillagerFunc, &Params);
			}
		}
	}

	SaveGame_Implementation();
}

void UCropoutGameInstance::HandleEventSaveGame()
{
	// BP DSL: AsyncSaveGameToSlot(SaveRef, "SAVE", OnCompleted: SetHasSave(true))
	SaveGame_Implementation();
}

void UCropoutGameInstance::HandleAddLoadingUI()
{
	// BP DSL: AddToViewport(UI_Transition)
	if (UI_Transition)
	{
		UI_Transition->AddToViewport();
	}
}

void UCropoutGameInstance::HandleLoadGame()
{
	// BP DSL (BP_GI's LoadGame event graph):
	//   if DoesSaveGameExist("SAVE"):
	//     SaveRef = LoadGameFromSlot("SAVE")
	//     SetHasSave(true)
	//   else:
	//     SaveRef = CreateSaveGameObject(USaveGame)
	//     SetHasSave(false)
	//
	// Native implementation: synchronous load, or create an empty
	// native SaveRef when no slot exists.
	if (UGameplayStatics::DoesSaveGameExist(TEXT("SAVE"), 0))
	{
		if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TEXT("SAVE"), 0))
		{
			SaveRef = Loaded;
			HasSave = true;
			return;
		}
	}
	EnsureNativeSaveGame(this);
	HasSave = false;
}

// ============================================================================
// ICropoutGameInstanceInterface implementations
// ============================================================================

void UCropoutGameInstance::UpdateAllResources_Implementation(const TMap<ECropoutResourceType, int32>& Resources)
{
	HandleEventUpdateAllResources(Resources);
}

void UCropoutGameInstance::UpdateAllVillagers_Implementation()
{
	HandleEventUpdateAllVillagers();
}

void UCropoutGameInstance::UpdateAllInteractables_Implementation()
{
	HandleEventUpdateAllInteractables();
}

void UCropoutGameInstance::SaveGame_Implementation()
{
	// BP DSL: AsyncSaveGameToSlot(SaveRef, "SAVE", OnCompleted: SetHasSave(true))
	USaveGame* Save = LoadBPSaveGameObject(this);
	if (!Save)
	{
		Save = EnsureNativeSaveGame(this);
	}
	if (Save)
	{
		FAsyncSaveGameToSlotDelegate SavedDelegate;
		SavedDelegate.BindLambda([this](const FString&, const int32, bool bSuccess)
		{
			HasSave = bSuccess;
		});
		UGameplayStatics::AsyncSaveGameToSlot(Save, TEXT("SAVE"), 0, SavedDelegate);
	}
}

void UCropoutGameInstance::ClearSave_Implementation(bool ClearSeed)
{
	HandleEventClearSave(ClearSeed);
}

// ============================================================================
// BP_GI local logic lifted to C++ (Round 9)
// ============================================================================

namespace
{
	/**
	 * Helper to invoke the Audio Modulation node
	 * `SetGlobalControlBusMixValue` via reflection. This avoids taking a
	 * direct dependency on the AudioModulation module in Build.cs while
	 * still reproducing the BP graph behaviour.
	 */
	void SetGlobalControlBusMixValue(UObject* WorldContext, const FString& BusPath, float Value, float FadeTime)
	{
		UClass* BusClass = FindObject<UClass>(nullptr, TEXT("/Script/AudioModulation.SoundControlBus"));
		if (!BusClass)
		{
			BusClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/AudioModulation.SoundControlBus"));
		}
		if (!BusClass || !WorldContext)
		{
			return;
		}

		UObject* Bus = StaticLoadObject(BusClass, nullptr, *BusPath);
		if (!Bus)
		{
			return;
		}

		UFunction* Func = WorldContext->GetClass()->FindFunctionByName(TEXT("SetGlobalControlBusMixValue"));
		if (!Func)
		{
			// Try the gameplay statics class as a fallback.
			Func = UGameplayStatics::StaticClass()->FindFunctionByName(TEXT("SetGlobalControlBusMixValue"));
		}
		if (!Func)
		{
			return;
		}

		struct FSetGlobalControlBusMixValueParams
		{
			UObject* Bus = nullptr;
			float Value = 0.0f;
			float FadeTime = 0.0f;
		};
		FSetGlobalControlBusMixValueParams Params;
		Params.Bus = Bus;
		Params.Value = Value;
		Params.FadeTime = FadeTime;
		WorldContext->ProcessEvent(Func, &Params);
	}
}

void UCropoutGameInstance::PlayMusic(USoundBase* Sound, float Volume, bool bPersist)
{
	// BP DSL (active branch): SetGlobalControlBusMixValue(Cropout_Music_Stop, 0.0, 0.0)
	// then CreateSound2D(Audio, Volume), Play.
	SetGlobalControlBusMixValue(this, TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_Stop.Cropout_Music_Stop"), 0.0f, 0.0f);

	if (Sound)
	{
		if (UAudioComponent* AudioComp = UGameplayStatics::CreateSound2D(this, Sound, Volume, 1.0f, 0.0f, nullptr, bPersist, true))
		{
			AudioComp->Play(0.0f);
		}
	}
}

void UCropoutGameInstance::StopMusic()
{
	// BP DSL (intended): SetGlobalControlBusMixValue(Cropout_Music_Stop, 1.0, 0.0)
	SetGlobalControlBusMixValue(this, TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_Stop.Cropout_Music_Stop"), 1.0f, 0.0f);
}

void UCropoutGameInstance::DoTransitionIn()
{
	if (UI_Transition)
	{
		if (!UI_Transition->IsInViewport())
		{
			UI_Transition->AddToViewport();
		}
		CallNoParamFunction(UI_Transition, TEXT("TransIn"));
	}
}

void UCropoutGameInstance::TransitionIn()
{
	DoTransitionIn();
}

void UCropoutGameInstance::DoTransitionOut()
{
	if (UI_Transition)
	{
		if (!UI_Transition->IsInViewport())
		{
			UI_Transition->AddToViewport();
		}
		CallNoParamFunction(UI_Transition, TEXT("TransOut"));
	}
}

void UCropoutGameInstance::TransitionOut()
{
	DoTransitionOut();
}

bool UCropoutGameInstance::CheckSaveBool() const
{
	return HasSave;
}

USaveGame* UCropoutGameInstance::GetSave() const
{
	return SaveRef;
}

TArray<FCropoutSaveInteract> UCropoutGameInstance::GetAllInteractables() const
{
	if (const UCropoutSaveGame* NativeSave = Cast<UCropoutSaveGame>(SaveRef))
	{
		return NativeSave->GetInteractables();
	}
	return TArray<FCropoutSaveInteract>();
}

int32 UCropoutGameInstance::GetSeed() const
{
	if (const UCropoutSaveGame* NativeSave = Cast<UCropoutSaveGame>(SaveRef))
	{
		return NativeSave->GetSeed();
	}
	return 0;
}

int32 UCropoutGameInstance::IslandSeed() const
{
	return GetSeed();
}

void UCropoutGameInstance::ClearSaveAsset()
{
	// BP DSL: SetPlayTime(0.0); Clear(Villagers); Clear(Interactables); Clear(Resources); SetResources({Wood:100}).
	if (UCropoutSaveGame* NativeSave = EnsureNativeSaveGame(this))
	{
		NativeSave->ClearSave();
		TMap<ECropoutResourceType, int32> DefaultResources;
		DefaultResources.Add(ECropoutResourceType::Wood, 100);
		NativeSave->SetResources(DefaultResources);
	}
}

void UCropoutGameInstance::UpdateSaveAsset()
{
	// BP DSL:
	//   Clear(SaveRef.Villagers); foreach Pawn (exclude player): add to Villagers
	//   Clear(SaveRef.Interactables); foreach BP_Interactable: add to Interactables
	//   SaveRef.Resources = GameMode.Resources
	UCropoutSaveGame* NativeSave = EnsureNativeSaveGame(this);
	USaveGame* LegacySave = NativeSave ? nullptr : LoadBPSaveGameObject(this);

	auto ClearVillagersLambda = [&]()
	{
		if (NativeSave)
		{
			NativeSave->ClearVillagers();
		}
		else
		{
			CallBPSaveGameFunc(LegacySave, TEXT("ClearVillagers"));
		}
	};

	auto ClearInteractablesLambda = [&]()
	{
		if (NativeSave)
		{
			NativeSave->ClearInteractables();
		}
		else
		{
			CallBPSaveGameFunc(LegacySave, TEXT("ClearInteractables"));
		}
	};

	ClearVillagersLambda();
	ClearInteractablesLambda();

	UWorld* World = GetWorld();
	if (World)
	{
		TArray<AActor*> AllPawns;
		UGameplayStatics::GetAllActorsOfClass(this, APawn::StaticClass(), AllPawns);
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

		for (AActor* Actor : AllPawns)
		{
			if (!Actor || Actor == PlayerPawn)
			{
				continue;
			}
			FName ActorTag = Actor->Tags.Num() > 0 ? Actor->Tags[0] : NAME_None;
			if (NativeSave)
			{
				NativeSave->AddVillager(Actor->GetActorLocation(), ActorTag, Actor);
			}
			else if (LegacySave)
			{
				if (UFunction* AddVillagerFunc = LegacySave->GetClass()->FindFunctionByName(TEXT("AddVillager")))
				{
					struct FAddVillager_Params { FVector Location; FName Tag; AActor* Source; };
					FAddVillager_Params Params;
					Params.Location = Actor->GetActorLocation();
					Params.Tag = ActorTag;
					Params.Source = Actor;
					LegacySave->ProcessEvent(AddVillagerFunc, &Params);
				}
			}
		}

		static const TSoftClassPtr<AActor> InteractableClass(
			FSoftObjectPath(TEXT("/Game/Blueprint/Interactable/BP_Interactable.BP_Interactable_C")));
		UClass* LoadedInteractableClass = InteractableClass.LoadSynchronous();
		if (LoadedInteractableClass)
		{
			TArray<AActor*> Interactables;
			UGameplayStatics::GetAllActorsOfClass(this, LoadedInteractableClass, Interactables);
			for (AActor* Actor : Interactables)
			{
				if (!Actor)
				{
					continue;
				}
				int32 ProgressionState = 0;
				if (UFunction* ProgFunc = Actor->GetClass()->FindFunctionByName(TEXT("GetProgressionState")))
				{
					struct FProg_Result { int32 ReturnValue; };
					FProg_Result Result{};
					Actor->ProcessEvent(ProgFunc, &Result);
					ProgressionState = Result.ReturnValue;
				}
				FName ActorTag = Actor->Tags.Num() > 0 ? Actor->Tags[0] : NAME_None;
				if (NativeSave)
				{
					NativeSave->AddInteractable(Actor->GetActorTransform(), Actor->GetClass(), ProgressionState, ActorTag, Actor);
				}
				else if (LegacySave)
				{
					if (UFunction* AddFunc = LegacySave->GetClass()->FindFunctionByName(TEXT("AddInteractable")))
					{
						struct FAddInteractable_Params { FTransform Transform; UClass* Class; int32 ProgressionState; FName Tag; AActor* Source; };
						FAddInteractable_Params Params;
						Params.Transform = Actor->GetActorTransform();
						Params.Class = Actor->GetClass();
						Params.ProgressionState = ProgressionState;
						Params.Tag = ActorTag;
						Params.Source = Actor;
						LegacySave->ProcessEvent(AddFunc, &Params);
					}
				}
			}
		}
	}

	if (AGameModeBase* GM = UGameplayStatics::GetGameMode(this))
	{
		if (ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(GM))
		{
			TMap<ECropoutResourceType, int32> TypedResources;
			for (const TPair<int32, int32>& Entry : CropoutGM->Resources)
			{
				TypedResources.Add(static_cast<ECropoutResourceType>(Entry.Key), Entry.Value);
			}
			if (NativeSave)
			{
				NativeSave->SetResources(TypedResources);
			}
			else
			{
				CallBPSaveGameFuncWithMap(LegacySave, TEXT("SetResources"), TypedResources);
			}
		}
	}
}

void UCropoutGameInstance::StartNewLevel()
{
	// BP_GI previously routed this through ExecuteUbergraphBPGI; the
	// real work is done by the GameMode's begin-play / island-gen flow.
	// No per-frame GameInstance work is required in C++.
}

void UCropoutGameInstance::UpdateSaveData()
{
	// BP_GI snapshot + persist. Re-use the existing C++ save path.
	HandleEventSaveGame();
}

void UCropoutGameInstance::IslandGenComplete()
{
	// BP_GI previously routed this through ExecuteUbergraphBPGI; the
	// GameMode handles island generation completion via
	// HandleEventIslandGenComplete().
}

void UCropoutGameInstance::ScaleUp(float Delay)
{
	// BP_GI stored Delay on the persistent frame and routed through
	// ExecuteUbergraphBPGI. No GameInstance-side behaviour remains.
}
