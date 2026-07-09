// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutBlueprintInterfaceGraphCleanupCommandlet.h"

#if WITH_EDITOR

#include "Blueprint/UserWidget.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* TargetBlueprintPath = TEXT("/Game/Blueprint/Core/GameMode/BP_GM.BP_GM");
	const TCHAR* ResultPath = TEXT("D:/ue/CropoutSampleProject/Saved/codex_clean_bp_gm_interface_graphs_cpp.json");

	const TSet<FName>& TargetGraphNames()
	{
		static const TSet<FName> Names = {
			TEXT("Island Seed"),
			TEXT("Remove Resource"),
			TEXT("Get Current Resources"),
			TEXT("Remove Target Resource"),
			TEXT("Check Resource"),
		};
		return Names;
	}

	void AppendGraphNames(const UBlueprint* Blueprint, TArray<FString>& OutGraphNames)
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
				OutGraphNames.Add(Graph->GetName());
			}
		}
	}

	void WriteResult(const TSharedRef<FJsonObject>& Result)
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Result, Writer);
		FFileHelper::SaveStringToFile(Output, ResultPath);
	}
}

int32 UCropoutBlueprintInterfaceGraphCleanupCommandlet::Main(const FString& Params)
{
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("blueprint"), TargetBlueprintPath);

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, TargetBlueprintPath);
	if (!Blueprint)
	{
		Result->SetStringField(TEXT("error"), TEXT("target_blueprint_not_found"));
		WriteResult(Result);
		return 1;
	}

	TArray<FString> BeforeGraphs;
	AppendGraphNames(Blueprint, BeforeGraphs);
	Result->SetArrayField(TEXT("before_graphs"), [&BeforeGraphs]()
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		for (const FString& Name : BeforeGraphs)
		{
			Values.Add(MakeShared<FJsonValueString>(Name));
		}
		return Values;
	}());

	TArray<TSharedPtr<FJsonValue>> RemovedGraphsJson;
	TArray<TSharedPtr<FJsonValue>> InterfacesJson;
	int32 RemovedCount = 0;

	Blueprint->Modify();

	for (FBPInterfaceDescription& InterfaceDescription : Blueprint->ImplementedInterfaces)
	{
		TSharedRef<FJsonObject> InterfaceJson = MakeShared<FJsonObject>();
		InterfaceJson->SetStringField(
			TEXT("interface"),
			InterfaceDescription.Interface ? InterfaceDescription.Interface->GetPathName() : TEXT(""));

		TArray<TSharedPtr<FJsonValue>> RemovedForInterfaceJson;

		for (int32 GraphIndex = InterfaceDescription.Graphs.Num() - 1; GraphIndex >= 0; --GraphIndex)
		{
			UEdGraph* Graph = InterfaceDescription.Graphs[GraphIndex];
			if (!Graph || !TargetGraphNames().Contains(Graph->GetFName()))
			{
				continue;
			}

			Graph->Modify();
			const FString RemovedName = Graph->GetName();
			InterfaceDescription.Graphs.RemoveAt(GraphIndex);
			FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph, EGraphRemoveFlags::MarkTransient);

			RemovedGraphsJson.Add(MakeShared<FJsonValueString>(RemovedName));
			RemovedForInterfaceJson.Add(MakeShared<FJsonValueString>(RemovedName));
			++RemovedCount;
		}

		InterfaceJson->SetArrayField(TEXT("removed_graphs"), RemovedForInterfaceJson);
		InterfacesJson.Add(MakeShared<FJsonValueObject>(InterfaceJson));
	}

	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	TArray<FString> AfterGraphs;
	AppendGraphNames(Blueprint, AfterGraphs);

	Result->SetNumberField(TEXT("removed_count"), RemovedCount);
	Result->SetArrayField(TEXT("removed_graphs"), RemovedGraphsJson);
	Result->SetArrayField(TEXT("interfaces"), InterfacesJson);
	Result->SetStringField(TEXT("status"), StaticEnum<EBlueprintStatus>()->GetNameStringByValue(Blueprint->Status));
	Result->SetArrayField(TEXT("after_graphs"), [&AfterGraphs]()
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		for (const FString& Name : AfterGraphs)
		{
			Values.Add(MakeShared<FJsonValueString>(Name));
		}
		return Values;
	}());

	const FString PackageFilename = FPackageName::LongPackageNameToFilename(
		Blueprint->GetOutermost()->GetName(),
		FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Blueprint->GetOutermost(), nullptr, *PackageFilename, SaveArgs);
	Result->SetBoolField(TEXT("saved"), bSaved);
	WriteResult(Result);

	return (RemovedCount > 0 && bSaved && Blueprint->Status != BS_Error) ? 0 : 1;
}

#else

int32 UCropoutBlueprintInterfaceGraphCleanupCommandlet::Main(const FString& Params)
{
	return 1;
}

#endif
