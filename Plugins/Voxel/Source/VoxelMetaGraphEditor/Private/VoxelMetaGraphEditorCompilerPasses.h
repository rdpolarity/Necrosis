// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphEditorCompiler.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

struct FEditorCompilerPasses
{
	static void SetupPinPreview(FEditorCompiler& Compiler, const FEdGraphPinReference& PreviewedPinRef);
	static bool AddToArrayNodes(FEditorCompiler& Compiler);
	static bool RemoveSplitPins(FEditorCompiler& Compiler);
	static bool FixupLocalVariables(FEditorCompiler& Compiler);
};

END_VOXEL_NAMESPACE(MetaGraph)