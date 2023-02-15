// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "SceneViewExtension.h"

class FVoxelViewExtensionHelper;

class VOXELCORE_API FVoxelViewExtension : public FSceneViewExtensionBase
{
public:
	using FSceneViewExtensionBase::FSceneViewExtensionBase;

	template<typename ExtensionType, typename... TArgs>
	static TSharedRef<ExtensionType> New(TArgs&&... Args)
	{
		const TSharedRef<ExtensionType> Extension = FSceneViewExtensions::NewExtension<ExtensionType>(Forward<TArgs>(Args)...);
		Extension->Initialize();
		return Extension;
	}

	virtual ~FVoxelViewExtension() override;

	virtual void PreRender_RenderThread(FRDGBuilder& GraphBuilder, const TArray<FSceneView*>& Views) {}
	virtual void PostRenderOpaque_RenderThread(const FPostOpaqueRenderParameters& Parameters) {}

	//~ Begin FSceneViewExtensionBase Interface
	virtual void SetupViewFamily(FSceneViewFamily& ViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& ViewFamily, FSceneView& View) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& ViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& View) override {}
	// End FSceneViewExtensionBase Interface

private:
	void Initialize();

	TSharedPtr<FVoxelViewExtensionHelper> ViewExtensionHelper;
};