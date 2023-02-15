// Copyright Voxel Plugin, Inc. All Rights Reserved.

#ifdef CANNOT_INCLUDE_VOXEL_MINIMAL
#error "VoxelMinimal.h recursively included"
#endif

#ifndef VOXEL_MINIMAL_INCLUDED
#define VOXEL_MINIMAL_INCLUDED
#define CANNOT_INCLUDE_VOXEL_MINIMAL

// Useful when compiling with clang
// Add this to "%appdata%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml"
/*
<?xml version="1.0" encoding="utf-8" ?>
<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">
    <WindowsPlatform>
         <Compiler>Clang</Compiler> 
    </WindowsPlatform>
	<ParallelExecutor>
		<ProcessorCountMultiplier>2</ProcessorCountMultiplier>
		<MemoryPerActionBytes>0</MemoryPerActionBytes>
	</ParallelExecutor>
</Configuration>
*/
#if 0
#undef DLLEXPORT
#undef DLLIMPORT
#define DLLEXPORT
#define DLLIMPORT
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "EngineUtils.h"
#include "Engine/EngineTypes.h"
#include "Engine/CollisionProfile.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "UObject/ObjectKey.h"
#include "UObject/ObjectSaveContext.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Modules/ModuleManager.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2DArray.h"
#include "CommonRenderResources.h"
#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "HAL/PlatformFileManager.h"
#else
#include "HAL/PlatformFilemanager.h"
#endif
#include "Serialization/BufferArchive.h"
#include "Serialization/LargeMemoryWriter.h"
#include "Serialization/LargeMemoryReader.h"
#include "Async/MappedFileHandle.h"
#include "Internationalization/Regex.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Containers/Queue.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "VoxelMinimal/VoxelCriticalSection.h"
#include "VoxelMinimal/VoxelDelegateHelpers.h"
#include "VoxelMinimal/VoxelEngineHelpers.h"
#include "VoxelMinimal/VoxelEngineVersionHelpers.h"
#include "VoxelMinimal/VoxelFunction.h"
#include "VoxelMinimal/VoxelBox.h"
#include "VoxelMinimal/VoxelBox2D.h"
#include "VoxelMinimal/VoxelIntBox.h"
#include "VoxelMinimal/VoxelGuid.h"
#include "VoxelMinimal/VoxelLog.h"
#include "VoxelMinimal/VoxelHash.h"
#include "VoxelMinimal/VoxelAxis.h"
#include "VoxelMinimal/VoxelMacros.h"
#include "VoxelMinimal/VoxelMessages.h"
#include "VoxelMinimal/VoxelTextureRef.h"
#include "VoxelMinimal/VoxelMaterialRef.h"
#include "VoxelMinimal/VoxelRTTI.h"
#include "VoxelMinimal/VoxelSharedPtr.h"
#include "VoxelMinimal/VoxelStats.h"
#include "VoxelMinimal/VoxelAutoFactoryInterface.h"
#include "VoxelMinimal/VoxelInstancedStruct.h"
#include "VoxelMinimal/VoxelInterval.h"
#include "VoxelMinimal/VoxelISPC.h"
#include "VoxelMinimal/VoxelKeyedLock.h"
#include "VoxelMinimal/VoxelVirtualStruct.h"
#include "VoxelMinimal/VoxelViewExtension.h"
#include "VoxelMinimal/VoxelRelativePointerArray.h"
#include "VoxelMinimal/VoxelOverridableSettings.h"

#include "VoxelMinimal/Containers/VoxelBVH.h"
#include "VoxelMinimal/Containers/VoxelArray.h"
#include "VoxelMinimal/Containers/VoxelArrayView.h"
#include "VoxelMinimal/Containers/VoxelBitArray.h"
#include "VoxelMinimal/Containers/VoxelFlatOctree.h"
#include "VoxelMinimal/Containers/VoxelSparseArray.h"
#include "VoxelMinimal/Containers/VoxelStaticBitArray.h"
#include "VoxelMinimal/Containers/VoxelPackedData.h"
#include "VoxelMinimal/Containers/VoxelPackedArray.h"
#include "VoxelMinimal/Containers/VoxelPackedArrayView.h"
#include "VoxelMinimal/Containers/VoxelPaletteArray.h"

#include "VoxelMinimal/Utilities/VoxelGameUtilities.h"
#include "VoxelMinimal/Utilities/VoxelMathUtilities.h"
#include "VoxelMinimal/Utilities/VoxelSystemUtilities.h"
#include "VoxelMinimal/Utilities/VoxelRenderUtilities.h"
#include "VoxelMinimal/Utilities/VoxelObjectUtilities.h"
#include "VoxelMinimal/Utilities/VoxelGreedyUtilities.h"
#include "VoxelMinimal/Utilities/VoxelTextureUtilities.h"
#include "VoxelMinimal/Utilities/VoxelMaterialUtilities.h"
#include "VoxelMinimal/Utilities/VoxelThreadingUtilities.h"
#include "VoxelMinimal/Utilities/VoxelDistanceFieldUtilities.h"

#undef CANNOT_INCLUDE_VOXEL_MINIMAL
#endif