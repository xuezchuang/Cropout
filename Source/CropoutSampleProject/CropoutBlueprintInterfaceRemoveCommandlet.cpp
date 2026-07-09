// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutBlueprintInterfaceRemoveCommandlet.h"

#if WITH_EDITOR

#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/EditorAssetSubsystem.h"

namespace
{
	TArray<FString> ParseTargetBlueprintPaths(const FString& Params)
	{
		FString TargetsParam;
		TArray<FString> Targets;
		if (FParse::Value(*Params, TEXT("Targets="), TargetsParam) ||
			FParse::Value(*Params, TEXT("Target="), TargetsParam))
		{
			TargetsParam.ParseIntoArray(Targets, TEXT(";"), true);
			if (Targets.Num() == 1)
			{
				TArray<FString> CommaTargets;
				TargetsParam.ParseIntoArray(CommaTargets, TEXT(","), true);
				if (CommaTargets.Num() > 1)
				{
					Targets = MoveTemp(CommaTargets);
				}
			}
		}
		return Targets;
	}

	void AppendGraphNames(const UBlueprint* Blueprint, TArray<TSharedPtr<FJsonValue>>& Out)
	{
		if (!Blueprint)
		{
			return;
		}

		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (const UEdGraph* Graph : Graphs)
		{
			if (Graph)
			{
				Out.Add(MakeShared<FJsonValueString>(Graph->GetName()));
			}
		}
	}

	void AppendInterfaceNames(const UBlueprint* Blueprint, TArray<TSharedPtr<FJsonValue>>& Out)
	{
		if (!Blueprint)
		{
			return;
		}

		for (const FBPInterfaceDescription& InterfaceDescription : Blueprint->ImplementedInterfaces)
		{
			Out.Add(MakeShared<FJsonValueString>(
				InterfaceDescription.Interface ? InterfaceDescription.Interface->GetPathName() : FString()));
		}
	}

	bool SaveBlueprint(UBlueprint* Blueprint)
	{
		if (!Blueprint)
		{
			return false;
		}

		if (GEditor)
		{
			if (UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>())
			{
				return AssetSubsystem->SaveLoadedAsset(Blueprint, false);
			}
		}

		return false;
	}

	void WriteResult(const TSharedRef<FJsonObject>& Result, const FString& ResultPath)
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Result, Writer);
		FFileHelper::SaveStringToFile(Output, *ResultPath);
	}
}

int32 UCropoutBlueprintInterfaceRemoveCommandlet::Main(const FString& Params)
{
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	FString InterfaceBlueprintPath;
	FString ResultPath = FPaths::ProjectSavedDir() / TEXT("codex_remove_blueprint_interface.json");
	FParse::Value(*Params, TEXT("Interface="), InterfaceBlueprintPath);
	FParse::Value(*Params, TEXT("Out="), ResultPath);
	const TArray<FString> TargetBlueprintPaths = ParseTargetBlueprintPaths(Params);

	Result->SetStringField(TEXT("interface_blueprint"), InterfaceBlueprintPath);
	Result->SetStringField(TEXT("result_path"), ResultPath);

	if (InterfaceBlueprintPath.IsEmpty() || TargetBlueprintPaths.IsEmpty())
	{
		Result->SetStringField(TEXT("error"), TEXT("missing Interface= or Target(s)="));
		WriteResult(Result, ResultPath);
		return 1;
	}

	UBlueprint* InterfaceBlueprint = LoadObject<UBlueprint>(nullptr, InterfaceBlueprintPath);
	UClass* InterfaceClass = InterfaceBlueprint ? InterfaceBlueprint->GeneratedClass : nullptr;
	if (!InterfaceClass)
	{
		Result->SetStringField(TEXT("error"), TEXT("interface_class_not_found"));
		WriteResult(Result, ResultPath);
		return 1;
	}

	Result->SetStringField(TEXT("interface_class"), InterfaceClass->GetPathName());

	TArray<TSharedPtr<FJsonValue>> BlueprintsJson;
	int32 RemovedCount = 0;
	bool bAllSaved = true;
	bool bAnyCompileError = false;

	for (const FString& TargetBlueprintPath : TargetBlueprintPaths)
	{
		TSharedRef<FJsonObject> BlueprintJson = MakeShared<FJsonObject>();
		BlueprintJson->SetStringField(TEXT("blueprint"), TargetBlueprintPath);

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *TargetBlueprintPath);
		if (!Blueprint)
		{
			BlueprintJson->SetStringField(TEXT("error"), TEXT("target_blueprint_not_found"));
			bAllSaved = false;
			BlueprintsJson.Add(MakeShared<FJsonValueObject>(BlueprintJson));
			continue;
		}

		TArray<TSharedPtr<FJsonValue>> BeforeGraphs;
		TArray<TSharedPtr<FJsonValue>> BeforeInterfaces;
		AppendGraphNames(Blueprint, BeforeGraphs);
		AppendInterfaceNames(Blueprint, BeforeInterfaces);
		BlueprintJson->SetArrayField(TEXT("before_graphs"), BeforeGraphs);
		BlueprintJson->SetArrayField(TEXT("before_interfaces"), BeforeInterfaces);

		const bool bImplemented = FBlueprintEditorUtils::ImplementsInterface(Blueprint, false, InterfaceClass);
		BlueprintJson->SetBoolField(TEXT("implemented_before"), bImplemented);
		if (bImplemented)
		{
			Blueprint->Modify();
			FBlueprintEditorUtils::RemoveInterface(Blueprint, InterfaceClass->GetClassPathName(), false);
			++RemovedCount;
		}

		FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const bool bSaved = SaveBlueprint(Blueprint);
		bAllSaved &= bSaved;
		bAnyCompileError |= Blueprint->Status == BS_Error;

		TArray<TSharedPtr<FJsonValue>> AfterGraphs;
		TArray<TSharedPtr<FJsonValue>> AfterInterfaces;
		AppendGraphNames(Blueprint, AfterGraphs);
		AppendInterfaceNames(Blueprint, AfterInterfaces);

		BlueprintJson->SetBoolField(TEXT("saved"), bSaved);
		BlueprintJson->SetStringField(TEXT("status"), StaticEnum<EBlueprintStatus>()->GetNameStringByValue(Blueprint->Status));
		BlueprintJson->SetBoolField(TEXT("implemented_after"), FBlueprintEditorUtils::ImplementsInterface(Blueprint, false, InterfaceClass));
		BlueprintJson->SetArrayField(TEXT("after_graphs"), AfterGraphs);
		BlueprintJson->SetArrayField(TEXT("after_interfaces"), AfterInterfaces);
		BlueprintsJson.Add(MakeShared<FJsonValueObject>(BlueprintJson));
	}

	Result->SetNumberField(TEXT("removed_count"), RemovedCount);
	Result->SetBoolField(TEXT("all_saved"), bAllSaved);
	Result->SetBoolField(TEXT("any_compile_error"), bAnyCompileError);
	Result->SetArrayField(TEXT("blueprints"), BlueprintsJson);
	WriteResult(Result, ResultPath);

	return (bAllSaved && !bAnyCompileError) ? 0 : 1;
}

#else

int32 UCropoutBlueprintInterfaceRemoveCommandlet::Main(const FString& Params)
{
	return 1;
}

#endif
