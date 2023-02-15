// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNodeDocs.h"
#include "VoxelNode.h"
#include "Serialization/Csv/CsvParser.h"

#if WITH_EDITOR
FVoxelNodeDocs& FVoxelNodeDocs::Get()
{
	static FVoxelNodeDocs Singleton;
	return Singleton;
}

FString FVoxelNodeDocs::GetPinTooltip(const UScriptStruct* Node, FName PinName)
{
	if (Map.Num() == 0)
	{
		// Not initialized yet
		return {};
	}

	if (Node->HasMetaData(STATIC_FNAME("Abstract")) ||
		Node->HasMetaData(STATIC_FNAME("Internal")))
	{
		return {};
	}

	const FString* Tooltip = Map.Find({ Node->GetFName(), PinName });
	if (!Tooltip)
	{
		ensure(!Node->GetPathName().StartsWith("/Script/Voxel"));
		return {};
	}

	return *Tooltip;
}

void FVoxelNodeDocs::Initialize()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Map.Num() == 0);

	const FString CsvPath = FVoxelSystemUtilities::GetPlugin().GetBaseDir() / "Documentation" / "VoxelNodes.csv";

	FString Csv;
	ensure(FFileHelper::LoadFileToString(Csv, *CsvPath));

	TMap<TPair<FName, FName>, FString> ExistingData;

	const FCsvParser Parser(Csv);
	for (const TArray<const TCHAR*>& Row : Parser.GetRows())
	{
		if (Row.Num() != 3)
		{
			continue;
		}

		ExistingData.Add({ Row[0], Row[1] }, Row[2]);
	}

	FString NewCsv;
	
	TArray<UScriptStruct*> Structs = GetDerivedStructs<FVoxelNode>();
	Structs.Sort([](const UScriptStruct& A, const UScriptStruct& B)
	{
		return A.GetName() < B.GetName();
	});

	TMap<TPair<FName, FName>, FString> NewMap;
	for (UScriptStruct* Struct : Structs)
	{
		if (Struct->HasMetaData("Abstract") ||
			Struct->HasMetaData("Internal") ||
			!Struct->GetPathName().StartsWith("/Script/Voxel"))
		{
			continue;
		}

		TVoxelInstancedStruct<FVoxelNode> Node(Struct);

		ensureAlways(!Node->GetStruct()->HasMetaData("Tooltip"));
		ensureAlways(!Node->GetStruct()->HasMetaData("ShortTooltip"));

		TArray<FName> PinNames;
		PinNames.Add({});

		TArray<FName> Pins;
		TArray<FName> CategoryNames;
		TSet<FName> AddedCategories;
		Node->GetExternalPinsData(Pins, CategoryNames);
		for (const FName CategoryName : CategoryNames)
		{
			if (AddedCategories.Contains(CategoryName))
			{
				continue;
			}

			TArray<FString> Path;
			CategoryName.ToString().ParseIntoArray(Path, TEXT("|"));
			if (Path.Num() > 1)
			{
				FString CurrentPath = Path[0];
				FName CurrentPathName = FName(CurrentPath);

				if (!AddedCategories.Contains(CurrentPathName))
				{
					PinNames.Add("CAT_" + CurrentPathName);
					AddedCategories.Add(CurrentPathName);
				}

				for (int32 Index = 1; Index < Path.Num(); Index++)
				{
					CurrentPath += "|" + Path[Index];
					CurrentPathName = FName(CurrentPath);

					if (!AddedCategories.Contains(CurrentPathName))
					{
						PinNames.Add("CAT_" + CurrentPathName);
						AddedCategories.Add(CurrentPathName);
					}
				}
			}
			else
			{
				AddedCategories.Add(CategoryName);
				PinNames.Add("CAT_" + CategoryName);
			}
		}
		for (const FName PinName : Pins)
		{
			PinNames.Add(PinName);
		}

		for (const FName& PinName : PinNames)
		{
			FString Description;
			const FString* ExistingDescription = ExistingData.Find({ Struct->GetFName(), PinName });
			if (ExistingDescription && !ExistingDescription->IsEmpty())
			{
				Description = *ExistingDescription;
			}
			else
			{
				Description = "TODO";
			}
			Description.TrimStartAndEndInline();

			FString CsvDescription = Description;
			CsvDescription.ReplaceInline(TEXT("\""), TEXT("\"\""));

			if (CsvDescription.Contains(",") ||
				CsvDescription.Contains("\""))
			{
				CsvDescription = "\"" + CsvDescription + "\"";
			}

			if (Description == "TODO")
			{
				Description = PinName.IsNone() ? Node->GetDisplayName() : PinName.ToString();
			}

			NewCsv += Struct->GetName() + "," + (PinName.IsNone() ? FString() : PinName.ToString()) + "," + CsvDescription + "\n";
			NewMap.Add({ Struct->GetFName(), PinName }, Description);
		}
	}

	Csv.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

	if (!NewCsv.Equals(Csv))
	{
		ensure(FFileHelper::SaveStringToFile(NewCsv, *CsvPath));
	}

	Map = MoveTemp(NewMap);
}
#endif