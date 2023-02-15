// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraph.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPHEDITOR_API, bool, GVoxelMetaGraphAllowSettingIsMacroGraph, false,
	"voxel.metagraph.AllowSettingIsMacroGraph",
	"");

VOXEL_CUSTOMIZE_CLASS(UVoxelMetaGraph)(IDetailLayoutBuilder& DetailLayout)
{
	const TSharedRef<IPropertyHandle> MacroGraphHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelMetaGraph, bIsMacroGraph));
	DetailLayout.EditDefaultProperty(MacroGraphHandle)->Visibility(MakeAttributeLambda([]
	{
		return GVoxelMetaGraphAllowSettingIsMacroGraph ? EVisibility::Visible : EVisibility::Collapsed; 
	}));

	const TSharedRef<IPropertyHandle> ParametersHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelMetaGraph, Parameters));
	ParametersHandle->MarkHiddenByCustomization();

	const TSharedRef<IPropertyHandle> OverrideNameHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelMetaGraph, bOverrideMacroName));
	const TSharedRef<IPropertyHandle> NameHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelMetaGraph, MacroName));

	bool bIsMacroGraph = true;
	ensure(MacroGraphHandle->GetValue(bIsMacroGraph) == FPropertyAccess::Success);

	if (!bIsMacroGraph)
	{
		OverrideNameHandle->MarkHiddenByCustomization();
		NameHandle->MarkHiddenByCustomization();
	}
}