// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"
#include "EngineModule.h"

class FVoxelViewExtensionHelper : public FSceneViewExtensionBase
{
public:
	using FSceneViewExtensionBase::FSceneViewExtensionBase;

	TArray<FSceneView*> QueuedViews;
	TWeakPtr<FSceneViewExtensionBase> WeakBase;

	//~ Begin FSceneViewExtensionBase Interface
	virtual void SetupViewFamily(FSceneViewFamily& ViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& ViewFamily, FSceneView& View) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& ViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& View) override
	{
		QueuedViews.Add(&View);
	}

	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
	{
		const TSharedPtr<FSceneViewExtensionBase> PinnedBase = WeakBase.Pin();
		return PinnedBase && PinnedBase->IsActiveThisFrame(Context);
	}
	// End FSceneViewExtensionBase Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelViewExtension::~FVoxelViewExtension()
{
	check(ViewExtensionHelper);
}

void FVoxelViewExtension::Initialize()
{
	check(IsInGameThread());

	check(!ViewExtensionHelper);
	ViewExtensionHelper = FSceneViewExtensions::NewExtension<FVoxelViewExtensionHelper>();
	ViewExtensionHelper->WeakBase = AsShared();

	ENQUEUE_RENDER_COMMAND(VoxelViewExtension_Init)(MakeWeakPtrLambda(this, [=](FRHICommandListImmediate& RHICmdList)
	{
		GetRendererModule().RegisterPostOpaqueRenderDelegate(MakeWeakPtrDelegate(this, [=](FPostOpaqueRenderParameters& Parameters)
		{
			PostRenderOpaque_RenderThread(Parameters);
		}));
		FVoxelRenderUtilities::OnPreRender().Add(MakeWeakPtrDelegate(this, [=](FRDGBuilder& GraphBuilder)
		{
			PreRender_RenderThread(GraphBuilder, ViewExtensionHelper->QueuedViews);

			ViewExtensionHelper->QueuedViews.Reset();
		}));
	}));
}