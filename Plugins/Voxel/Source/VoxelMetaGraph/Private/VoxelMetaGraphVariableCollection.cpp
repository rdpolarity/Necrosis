// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphVariableCollection.h"

void FVoxelMetaGraphVariableCollection::Fixup(const TArray<FVoxelMetaGraphParameter>& Parameters)
{
	VOXEL_FUNCTION_COUNTER();

	for (auto& It : Variables)
	{
		if (!It.Value.GetType().IsValid())
		{
			continue;
		}

		if (It.Value.GetType().Is<FBodyInstance>())
		{
			It.Value.Value.Get<FBodyInstance>().LoadProfileData(false);
		}
	}

	for (auto& It : Variables)
	{
		It.Value.bIsOrphan = true;
	}

	int32 Index = 0;
	for (const FVoxelMetaGraphParameter& Parameter : Parameters)
	{
		if (Parameter.ParameterType != EVoxelMetaGraphParameterType::Parameter)
		{
			continue;
		}
		const FVoxelPinType ExposedType = Parameter.Type.GetExposedType();
		ensure(ExposedType == Parameter.DefaultValue.GetType());

		if (FVoxelMetaGraphVariable* Variable = Variables.Find(Parameter.Guid))
		{
			if (Variable->GetType() != ExposedType)
			{
				// Mark the variable as orphan and assign it a new GUID
				Variable->bIsOrphan = true;

				FVoxelMetaGraphVariable VariableCopy = *Variable;
				Variables.Add(FGuid::NewGuid(), VariableCopy);
				Variables.Remove(Parameter.Guid);
			}
			else if (Variable->DefaultValue != Parameter.DefaultValue)
			{
				if (Variable->IsDefault())
				{
					// Update to new default
					Variable->Value = Parameter.DefaultValue;
				}
			}
		}

		const bool bNewVariable = !Variables.Contains(Parameter.Guid);

		FVoxelMetaGraphVariable& Variable = Variables.FindOrAdd(Parameter.Guid);
		Variable.Name = Parameter.Name;
		Variable.bIsOrphan = false;
		Variable.DefaultValue = Parameter.DefaultValue;
		Variable.Category = Parameter.Category;
		Variable.Description = Parameter.Description;
		Variable.SortIndex = Index;
		
		if (bNewVariable)
		{
			Variable.Value = Variable.DefaultValue;
		}

		Index++;
	}

	// Remove orphans that aren't edited
	CheckOrphans();
}

void FVoxelMetaGraphVariableCollection::CheckOrphans()
{
	bool bTriggerRefresh = false;
	for (auto It = Variables.CreateIterator(); It; ++It)
	{
		FVoxelMetaGraphVariable& Variable = It.Value();
		if (!Variable.bIsOrphan ||
			!Variable.IsDefault())
		{
			continue;
		}

		It.RemoveCurrent();
		bTriggerRefresh = true;
	}

	if (bTriggerRefresh)
	{
		RefreshDetails.Broadcast();
	}
}