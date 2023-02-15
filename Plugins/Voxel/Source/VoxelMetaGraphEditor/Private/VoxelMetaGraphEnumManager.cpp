#include "VoxelGraphNode.h"
#include "VoxelMetaGraph.h"
#include "Kismet2/EnumEditorUtils.h"
#include "NodeDependingOnEnumInterface.h"

class FVoxelMetaGraphEnumManager : public FEnumEditorUtils::INotifyOnEnumChanged
{
	virtual void PreChange(const UUserDefinedEnum* Changed, FEnumEditorUtils::EEnumEditorChangeInfo ChangedType) override
	{
	}

	virtual void PostChange(const UUserDefinedEnum* Enum, FEnumEditorUtils::EEnumEditorChangeInfo ChangedType) override
	{
		for (TObjectIterator<UVoxelMetaGraph> It(RF_Transient); It; ++It)
		{
			UVoxelMetaGraph* MetaGraph = *It;
			bool bFixup = false;
			for (const FVoxelMetaGraphParameter& Parameter : MetaGraph->Parameters)
			{
				if (!Parameter.Type.IsEnum() ||
					Parameter.Type.GetEnum() != Enum)
				{
					continue;
				}

				bFixup = true;
				break;
			}

			if (bFixup)
			{
				MetaGraph->FixupParameters();
			}
		}

		for (TObjectIterator<UVoxelGraphNode> It(RF_Transient); It; ++It)
		{
			UVoxelGraphNode* Node = *It;
			const INodeDependingOnEnumInterface* NodeDependingOnEnum = Cast<INodeDependingOnEnumInterface>(Node);

			if (!Node->HasAnyFlags(RF_ClassDefaultObject | RF_Transient) &&
				NodeDependingOnEnum &&
				Enum == NodeDependingOnEnum->GetEnum() &&
				NodeDependingOnEnum->ShouldBeReconstructedAfterEnumChanged())
			{
				Node->ReconstructNode();
			}
		}
	}
};

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterEnumManager)
{
	new FVoxelMetaGraphEnumManager();
}