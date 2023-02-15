// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraph.h"
#include "VoxelActor.h"
#include "VoxelRuntime/VoxelRuntimeUtilities.h"

DEFINE_VOXEL_FACTORY(UVoxelMetaGraph);

UVoxelMetaGraph::UVoxelMetaGraph()
{
	for (const EVoxelMetaGraphParameterType Type : TEnumRange<EVoxelMetaGraphParameterType>())
	{
		ParametersCategories.Add(Type, {});
	}
}

void UVoxelMetaGraph::PostLoad()
{
	Super::PostLoad();

	FixupParameters();
}

#if WITH_EDITOR
void UVoxelMetaGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FixupParameters();

	OnParametersChanged.Broadcast();
}
#endif

FString UVoxelMetaGraph::GetMacroName() const
{
	ensure(bIsMacroGraph);

	if (bOverrideMacroName &&
		!MacroName.IsEmpty())
	{
		return MacroName;
	}

	FString Name = GetName();
	Name.RemoveFromStart("VMGM_");
	Name.RemoveFromStart("VMG_");

	Name = FName::NameToDisplayString(Name, false);

	return Name;
}

void UVoxelMetaGraph::UpdateVoxelActors() const
{
	VOXEL_FUNCTION_COUNTER();

	for (const AVoxelActor* VoxelActor : TObjectRange<AVoxelActor>())
	{
		if (VoxelActor->GetRuntime() &&
			VoxelActor->MetaGraph == this)
		{
			FVoxelRuntimeUtilities::RecreateRuntime(*VoxelActor->GetRuntime());
		}
	}
}

void UVoxelMetaGraph::FixupParameters()
{
	VOXEL_FUNCTION_COUNTER();

	// Fixup types
	for (FVoxelMetaGraphParameter& Parameter : Parameters)
	{
		if (!Parameter.Type.IsValid())
		{
			Parameter.Type = FVoxelPinType::Make<float>();
		}
		const FVoxelPinType ExposedType = Parameter.Type.GetExposedType();

		if (Parameter.DefaultValue.GetType() != ExposedType)
		{
			Parameter.DefaultValue = FVoxelPinValue(ExposedType);
		}

		if (Parameter.Type.IsEnum())
		{
			const UEnum* Enum = Parameter.Type.GetEnum();
			if (!Enum->IsValidEnumValue(Parameter.DefaultValue.GetEnum()))
			{
				Parameter.DefaultValue = FVoxelPinValue(ExposedType);
			}
			else if (
				Enum->NumEnums() > 1 &&
				Parameter.DefaultValue.GetEnum() == Enum->GetMaxEnumValue())
			{
				Parameter.DefaultValue = FVoxelPinValue(ExposedType);
			}
		}
	}

	// Fixup names & GUIDs
	{
		TSet<FName> Names;
		TSet<FGuid> Guids;

		for (FVoxelMetaGraphParameter& Parameter : Parameters)
		{
			while (Names.Contains(Parameter.Name) || Parameter.Name.IsNone())
			{
				Parameter.Name.SetNumber(Parameter.Name.GetNumber() + 1);
			}
			while (Guids.Contains(Parameter.Guid) || !Parameter.Guid.IsValid())
			{
				Parameter.Guid = FGuid::NewGuid();
			}

			Names.Add(Parameter.Name);
			Guids.Add(Parameter.Guid);
		}
	}

	// Fixup categories
	FixupCategories();
}

void UVoxelMetaGraph::FixupCategories()
{
	VOXEL_FUNCTION_COUNTER();

	const TCHAR* CategoryDelim = TEXT("|");
	struct FChainedCategory
	{
		FString Name;
		TArray<FChainedCategory> InnerCategories;

		FChainedCategory(FString Name)
		: Name(Name)
		{
			
		}

		bool operator==(const FString& RHS) const
		{
			return Name == RHS;
		}

		bool operator!=(const FString& RHS) const
		{
			return !(*this == RHS);
		}

		TArray<FString> GetCategories() const
		{
			TArray<FString> ResultCategories;
			for (const FChainedCategory& InnerCategory : InnerCategories)
			{
				TArray<FString> PreparedCategories = InnerCategory.GetCategories();
				for (const FString& PreparedCategory : PreparedCategories)
				{
					ResultCategories.Add(Name + "|" + PreparedCategory);
				}
			}
			ResultCategories.Add(Name);
			return ResultCategories;
		}
	};

	const auto AddChainedCategory = [](const FString& CategoryName, TArray<FChainedCategory>& TargetCategories)
	{
		FChainedCategory* TargetCategory = TargetCategories.FindByKey(CategoryName);
		if (!TargetCategory)
		{
			TargetCategory = &TargetCategories[TargetCategories.Add(CategoryName)];
		}
		return TargetCategory;
	};

	TMap<EVoxelMetaGraphParameterType, TSet<FString>> TypeToUsedCategories;

	// Add missing categories and fill used categories
	for (const FVoxelMetaGraphParameter& Parameter : Parameters)
	{
		TSet<FString>& UsedCategories = TypeToUsedCategories.FindOrAdd(Parameter.ParameterType);

		UsedCategories.Add(Parameter.Category);
		GetCategories(Parameter.ParameterType).AddUnique(Parameter.Category);
	}

	// Remove unused categories
	for (const EVoxelMetaGraphParameterType Type : TEnumRange<EVoxelMetaGraphParameterType>())
	{
		TSet<FString>& UsedCategories = TypeToUsedCategories.FindOrAdd(Type);

		GetCategories(Type).RemoveAll([&](const FString& InCategory)
		{
			return !UsedCategories.Contains(InCategory);
		});
	}

	// Move subcategories before higher level categories
	for (const EVoxelMetaGraphParameterType Type : TEnumRange<EVoxelMetaGraphParameterType>())
	{
		TArray<FChainedCategory> TopLevelCategories;

		TArray<FString>& TypeCategories = GetCategories(Type);
		TArray<FString> CopiedCategories = TypeCategories;
		TypeCategories = {};

		// Buildup chained categories for proper ordering
		for (const FString& InCategory : CopiedCategories)
		{
			TArray<FString> CategoryChain;
			InCategory.ParseIntoArray(CategoryChain, CategoryDelim, true);

			FChainedCategory* TargetCategory = nullptr;
			for (int32 Index = 0; Index < CategoryChain.Num(); Index++)
			{
				if (Index == 0)
				{
					TargetCategory = AddChainedCategory(CategoryChain[Index], TopLevelCategories);
				}
				else
				{
					TargetCategory = AddChainedCategory(CategoryChain[Index], TargetCategory->InnerCategories);
				}
			}
		}

		// Re add all existing chained categories
		for (FChainedCategory& InCategory : TopLevelCategories)
		{
			for (const FString& GeneratedCategory : InCategory.GetCategories())
			{
				if (TypeToUsedCategories[Type].Contains(GeneratedCategory))
				{
					TypeCategories.Add(GeneratedCategory);
				}
			}
		}
	}
}