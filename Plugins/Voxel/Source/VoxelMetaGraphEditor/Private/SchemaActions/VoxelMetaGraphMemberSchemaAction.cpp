// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphMemberSchemaAction.h"
#include "VoxelMetaGraph.h"
#include "VoxelNodeLibrary.h"
#include "VoxelMetaGraphSeed.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphToolkit.h"
#include "Widgets/SVoxelMetaGraphMembers.h"

#include "EditorCategoryUtils.h"

FVoxelMetaGraphParameter* FVoxelMetaGraphMemberSchemaAction::GetParameter() const
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	ensure(
		SectionID == GetSectionId(EMembersNodeSection::Parameters) ||
		SectionID == GetSectionId(EMembersNodeSection::MacroInputs) ||
		SectionID == GetSectionId(EMembersNodeSection::MacroOutputs) ||
		SectionID == GetSectionId(EMembersNodeSection::LocalVariables));
	
	return MetaGraph->Parameters.FindByKey(ParameterGuid);
}

void FVoxelMetaGraphMemberSchemaAction::MovePersistentItemToCategory(const FText& NewCategoryName)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	FVoxelMetaGraphParameter* Parameter = GetParameter();
	if (!ensure(Parameter))
	{
		return;
	}

	FString TargetCategory;
	
	// Need to lookup for original category, since NewCategoryName is returned as display string
	const TArray<FString>& OriginalCategories = MetaGraph->GetCategories(Parameter->ParameterType);
	for (int32 Index = 0; Index < OriginalCategories.Num(); Index++)
	{
		FString ReformattedCategory = FEditorCategoryUtils::GetCategoryDisplayString(OriginalCategories[Index]);
		if (ReformattedCategory == NewCategoryName.ToString())
		{
			TargetCategory = OriginalCategories[Index];
			break;
		}
	}

	if (TargetCategory.IsEmpty())
	{
		TargetCategory = NewCategoryName.ToString();
	}

	const FVoxelTransaction Transaction(MetaGraph, "Move parameter to category");
	Parameter->Category = TargetCategory;

	if (const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin())
	{
		Toolkit->SelectParameter(ParameterGuid, true);
	}
}

int32 FVoxelMetaGraphMemberSchemaAction::GetReorderIndexInContainer() const
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	ensure(
		SectionID == GetSectionId(EMembersNodeSection::Parameters) ||
		SectionID == GetSectionId(EMembersNodeSection::MacroInputs) ||
		SectionID == GetSectionId(EMembersNodeSection::MacroOutputs) ||
		SectionID == GetSectionId(EMembersNodeSection::LocalVariables));
	
	return MetaGraph->Parameters.IndexOfByKey(ParameterGuid);
}

bool FVoxelMetaGraphMemberSchemaAction::ReorderToBeforeAction(TSharedRef<FEdGraphSchemaAction> OtherAction)
{
	const TSharedRef<FVoxelMetaGraphMemberSchemaAction> TargetAction = StaticCastSharedRef<FVoxelMetaGraphMemberSchemaAction>(OtherAction);
	
	if (ParameterGuid == TargetAction->ParameterGuid ||
		MetaGraph != TargetAction->MetaGraph ||
		SectionID != TargetAction->SectionID)
	{
		return false;
	}

	const FVoxelMetaGraphParameter* ParameterToMove = GetParameter();
	if (!ParameterToMove)
	{
		return false;
	}

	int32 TargetIndex = TargetAction->GetReorderIndexInContainer();
	if (TargetIndex == -1)
	{
		return false;
	}
	
	const int32 CurrentIndex = GetReorderIndexInContainer();
	if (TargetIndex > CurrentIndex)
	{
		TargetIndex--;
	}
	
	FVoxelMetaGraphParameter CopiedParameter = *ParameterToMove;

	const FVoxelTransaction Transaction(MetaGraph, "Reorder parameter");

	CopiedParameter.Category = TargetAction->GetParameter()->Category;

	MetaGraph->Parameters.RemoveAt(CurrentIndex);
	MetaGraph->Parameters.Insert(CopiedParameter, TargetIndex);
	
	return true;
}

FEdGraphSchemaActionDefiningObject FVoxelMetaGraphMemberSchemaAction::GetPersistentItemDefiningObject() const
{
	return FEdGraphSchemaActionDefiningObject(MetaGraph.Get());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType FVoxelMetaGraphMemberSchemaAction::GetPinType()
{
	if (const FVoxelMetaGraphParameter* Parameter = GetParameter())
	{
		return Parameter->Type;
	}

	return FVoxelPinType::Make<float>();
}

TArray<FVoxelPinType> FVoxelMetaGraphMemberSchemaAction::GetPinTypes()
{
	return FVoxelNodeLibrary::GetParameterTypes();
}

void FVoxelMetaGraphMemberSchemaAction::SetPinType(const FVoxelPinType& NewPinType)
{
	const FVoxelTransaction Transaction(MetaGraph, "Change parameter type");

	if (FVoxelMetaGraphParameter* Parameter = GetParameter())
	{
		Parameter->Type = NewPinType;

		const FVoxelPinType ExposedType = Parameter->Type.GetExposedType();
		Parameter->DefaultValue = FVoxelPinValue(ExposedType);

		if (ExposedType.Is<FVoxelMetaGraphSeed>())
		{
			Parameter->DefaultValue.Get<FVoxelMetaGraphSeed>().Randomize();
		}

		if (const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin())
		{
			Toolkit->SelectParameter(ParameterGuid, true);
		}
	}
}

FName FVoxelMetaGraphMemberSchemaAction::GetName()
{
	const FVoxelMetaGraphParameter* Parameter = GetParameter();
	if (!Parameter)
	{
		return {};
	}

	return Parameter->Name;
}

void FVoxelMetaGraphMemberSchemaAction::SetName(const FString& NewName, const TSharedPtr<SVoxelMetaGraphMembers>& MembersWidget)
{
	const FVoxelTransaction Transaction(MetaGraph, "Rename parameter");

	FVoxelMetaGraphParameter* Parameter = GetParameter();
	if (ensure(Parameter))
	{
		Parameter->Name = *NewName;
	}

	MembersWidget->SelectByName(*NewName, ESelectInfo::OnMouseClick, SectionID);
}
