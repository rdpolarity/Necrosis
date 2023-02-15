// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelPointCanvasActor.h"
#include "VoxelPointCanvasData.h"
#include "VoxelPointCanvasAsset.h"
#include "DynamicMeshBuilder.h"

class FVoxelPointCanvasSceneProxy : public FPrimitiveSceneProxy
{
public:
	VOXEL_USE_NAMESPACE_TYPES(PointCanvas, FLODData);

	const float Scale;
	const TSharedRef<const FLODData> Data;

	mutable int32 RenderCounter = -1;
	mutable TVoxelArray<FVector4f> Points;
	mutable TVoxelArray<FVoxelIntBox> AllChunkBounds;
	mutable TVoxelArray<FVector3f> LastVertices;

	FVoxelPointCanvasSceneProxy(const UVoxelPointCanvasComponent& Component, const float Scale, const TSharedRef<const FLODData>& Data)
		: FPrimitiveSceneProxy(&Component)
		, Scale(Scale)
		, Data(Data)
	{
		bVerifyUsedMaterials = false;
	}

	//~ Begin FPrimitiveSceneProxy Interface
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector) const override
	{
		VOXEL_FUNCTION_COUNTER_LLM();

		if (RenderCounter != Data->RenderCounter.GetValue())
		{
			RenderCounter = Data->RenderCounter.GetValue();
			Points = Data->GetAllPoints();
			AllChunkBounds = Data->GetChunkBounds();

			FVoxelScopeLock_Read Lock(Data->CriticalSection);
			LastVertices = Data->LastVertices;
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (!(VisibilityMap & (1 << ViewIndex)))
			{
				continue;
			}

			FPrimitiveDrawInterface& PDI = *Collector.GetPDI(ViewIndex);
			const FMatrix Matrix = FScaleMatrix(Scale) * GetLocalToWorld();

			for (const FVector4f& Point : Points)
			{
				const int32 Direction = FMath::RoundToInt(FMath::Abs(Point.W));
				ensure(0 <= Direction && Direction < 3);

				const FColor Color =
					Direction == 0 ? FColorList::Red :
					Direction == 1 ? FColorList::Green :
					FColorList::Blue;

				PDI.DrawPoint(
					Matrix.TransformPosition(FVector(Point)),
					Color,
					10.f,
					SDPG_World);
			}

			for (const FVoxelIntBox& ChunkBounds : AllChunkBounds)
			{
				const FVector Corner000 = Matrix.TransformPosition(FVector(ChunkBounds.Min.X, ChunkBounds.Min.Y, ChunkBounds.Min.Z));
				const FVector Corner001 = Matrix.TransformPosition(FVector(ChunkBounds.Max.X, ChunkBounds.Min.Y, ChunkBounds.Min.Z));
				const FVector Corner010 = Matrix.TransformPosition(FVector(ChunkBounds.Min.X, ChunkBounds.Max.Y, ChunkBounds.Min.Z));
				const FVector Corner011 = Matrix.TransformPosition(FVector(ChunkBounds.Max.X, ChunkBounds.Max.Y, ChunkBounds.Min.Z));
				const FVector Corner100 = Matrix.TransformPosition(FVector(ChunkBounds.Min.X, ChunkBounds.Min.Y, ChunkBounds.Max.Z));
				const FVector Corner101 = Matrix.TransformPosition(FVector(ChunkBounds.Max.X, ChunkBounds.Min.Y, ChunkBounds.Max.Z));
				const FVector Corner110 = Matrix.TransformPosition(FVector(ChunkBounds.Min.X, ChunkBounds.Max.Y, ChunkBounds.Max.Z));
				const FVector Corner111 = Matrix.TransformPosition(FVector(ChunkBounds.Max.X, ChunkBounds.Max.Y, ChunkBounds.Max.Z));

				PDI.DrawLine(Corner000, Corner001, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner010, Corner011, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner100, Corner101, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner110, Corner111, FLinearColor::Green, SDPG_World);

				PDI.DrawLine(Corner000, Corner100, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner000, Corner010, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner100, Corner110, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner010, Corner110, FLinearColor::Green, SDPG_World);

				PDI.DrawLine(Corner001, Corner101, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner001, Corner011, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner101, Corner111, FLinearColor::Green, SDPG_World);
				PDI.DrawLine(Corner011, Corner111, FLinearColor::Green, SDPG_World);
			}
			
			FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());

			for (const FVector3f& Vertex : LastVertices)
			{
				MeshBuilder.AddVertex(
					Vertex,
					FVector2f::ZeroVector,
					FVector3f::ZeroVector,
					FVector3f::ZeroVector,
					FVector3f::ZeroVector,
					FColor::Red);
			}

			for (int32 Index = 0; Index < LastVertices.Num() / 3; Index++)
			{
				MeshBuilder.AddTriangle(
					3 * Index + 0,
					3 * Index + 1,
					3 * Index + 2);
			}

			class FDummyPDI : public FPrimitiveDrawInterface
			{
			public:
				const int32 ViewIndex;
				FPrimitiveDrawInterface& PDI;
				FMeshElementCollector& Collector;

				FDummyPDI(const FSceneView* View, int32 ViewIndex, FPrimitiveDrawInterface& PDI, FMeshElementCollector& Collector)
					: FPrimitiveDrawInterface(View)
					, ViewIndex(ViewIndex)
					, PDI(PDI)
					, Collector(Collector)
				{
				}

				virtual bool IsHitTesting() override
				{
					return false;
				}
				virtual void SetHitProxy(HHitProxy* HitProxy) override
				{
					ensure(false);
				}

				virtual void RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource) override
				{
					PDI.RegisterDynamicResource(DynamicResource);
				}
				virtual void AddReserveLines(uint8 DepthPriorityGroup, int32 NumLines, bool bDepthBiased, bool bThickLines) override
				{
					PDI.AddReserveLines(DepthPriorityGroup, NumLines, bDepthBiased, bThickLines);
				}
				virtual void DrawSprite(const FVector& Position, float SizeX, float SizeY, const FTexture* Sprite, const FLinearColor& Color, uint8 DepthPriorityGroup, float U, float UL, float V, float VL, uint8 BlendMode, float OpacityMaskRefVal) override
				{
					PDI.DrawSprite(Position, SizeX, SizeY, Sprite, Color, DepthPriorityGroup, U, UL, V, VL, BlendMode, OpacityMaskRefVal);
				}
				virtual void DrawLine(const FVector& Start, const FVector& End, const FLinearColor& Color, uint8 DepthPriorityGroup, float Thickness, float DepthBias, bool bScreenSpace) override
				{
					PDI.DrawLine(Start, End, Color, DepthPriorityGroup, Thickness, DepthBias, bScreenSpace);
				}
				virtual void DrawTranslucentLine(const FVector& Start, const FVector& End, const FLinearColor& Color, uint8 DepthPriorityGroup, float Thickness, float DepthBias, bool bScreenSpace) override
				{
					PDI.DrawTranslucentLine(Start, End, Color, DepthPriorityGroup, Thickness, DepthBias, bScreenSpace);
				}
				virtual void DrawPoint(const FVector& Position, const FLinearColor& Color, float PointSize, uint8 DepthPriorityGroup) override
				{
					PDI.DrawPoint(Position, Color, PointSize, DepthPriorityGroup);
				}
				virtual int32 DrawMesh(const FMeshBatch& Mesh) override
				{
					FMeshBatch& CollectorMesh = Collector.AllocateMesh();
					CollectorMesh = Mesh;
					Collector.AddMesh(ViewIndex, CollectorMesh);
					return 0;
				}
			};
			FDummyPDI DummyPDI(Views[ViewIndex], ViewIndex, PDI, Collector);

			FColoredMaterialRenderProxy* MaterialProxy = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial->GetRenderProxy(),
				FLinearColor::Gray);

			Collector.RegisterOneFrameMaterialProxy(MaterialProxy);

			MeshBuilder.Draw(
				&DummyPDI,
				Matrix,
				MaterialProxy,
				SDPG_World);
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = true;
		Result.bRenderInMainPass = true;
		Result.bDynamicRelevance = true;
		return Result;
	}
	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}
	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
	//~ End FPrimitiveSceneProxy Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UVoxelPointCanvasComponent::CreateSceneProxy()
{
	const AVoxelPointCanvasActor* Owner = Cast<AVoxelPointCanvasActor>(GetOwner());
	if (!ensure(Owner) ||
		!Owner->bDisplayPoints)
	{
		return nullptr;
	}

	const UVoxelPointCanvasAsset* Canvas = Owner->Brush.Canvas;
	if (!Canvas)
	{
		return nullptr;
	}

	const TSharedRef<FVoxelPointCanvasData> Data = Canvas->GetData();
	if (Data->LODs.Num() == 0)
	{
		return nullptr;
	}

	return new FVoxelPointCanvasSceneProxy(*this, Owner->Brush.Scale, Data->LODs[0].ToSharedRef());
}

FBoxSphereBounds UVoxelPointCanvasComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBox(-FVector(1e10), FVector(1e10));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AVoxelPointCanvasActor::AVoxelPointCanvasActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
	CanvasComponent = CreateDefaultSubobject<UVoxelPointCanvasComponent>("Canvas");
	CanvasComponent->SetupAttachment(RootComponent);
}