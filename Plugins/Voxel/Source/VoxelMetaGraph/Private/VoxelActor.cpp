// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelActor.h"
#include "VoxelMetaGraphRuntime.h"
#include "Components/BillboardComponent.h"

AVoxelActor::AVoxelActor()
{
	RootComponent = CreateDefaultSubobject<UVoxelActorRootComponent>("Root");

#if WITH_EDITOR
	if (UBillboardComponent* SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite")))
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> SpriteFinder(TEXT("/VoxelPlugin/EditorAssets/T_VoxelIcon"));
		SpriteComponent->Sprite = SpriteFinder.Object;
		SpriteComponent->SetRelativeScale3D(FVector(0.5f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Voxel Actor");
		SpriteComponent->SpriteInfo.DisplayName = FText::FromString("Voxel Actor");
		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->bReceivesDecals = false;
	}
#endif

	PrimaryActorTick.bCanEverTick = true;
}

void AVoxelActor::Fixup()
{
	VOXEL_FUNCTION_COUNTER();

	if (!MetaGraph.LoadSynchronous())
	{
		ensure(MetaGraph.IsNull());

		VariableCollection.Categories = {};
		VariableCollection.Fixup({});
		return;
	}

	VariableCollection.Categories = MetaGraph->GetCategories(EVoxelMetaGraphParameterType::Parameter);
	VariableCollection.Fixup(MetaGraph->Parameters);

	if (!MetaGraph->OnParametersChanged.IsBoundToObject(this))
	{
		MetaGraph->OnParametersChanged.AddWeakLambda(this, [=, BoundMetaGraph = MetaGraph]
		{
			VOXEL_SCOPE_COUNTER("OnParameterChanged");

			if (MetaGraph != BoundMetaGraph)
			{
				return;
			}

			VariableCollection.Categories = MetaGraph->GetCategories(EVoxelMetaGraphParameterType::Parameter);
			VariableCollection.Fixup(MetaGraph->Parameters);
			VariableCollection.RefreshDetails.Broadcast();
		});
	}

#if WITH_EDITOR
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// This is a blueprint CDO
		return;
	}

	UsedObjects.Reset();

	for (const FVoxelMetaGraphCompiledNode& Node : MetaGraph->CompiledGraph.Nodes)
	{
		UsedObjects.Add(Node.MetaGraph);

		for (const FVoxelMetaGraphCompiledPin& Pin : Node.InputPins)
		{
			if (Pin.DefaultValue.GetType().IsObject())
			{
				UsedObjects.Add(Pin.DefaultValue.GetObject().LoadSynchronous());
			}
		}
	}
	for (const auto& It : VariableCollection.Variables)
	{
		if (It.Value.GetType().IsObject())
		{
			UsedObjects.Add(It.Value.Value.GetObject().LoadSynchronous());
		}
	}

	if (!FCoreUObjectDelegates::OnObjectPropertyChanged.IsBoundToObject(this))
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddWeakLambda(this, [=](UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
		{
			if (Object->IsA<UEdGraphNode>())
			{
				// We don't want graph refresh to be triggered by nodes here
				// Otherwise they also get triggered by nodes used during compilation
				return;
			}

			if (!GetRuntime())
			{
				// Not created, nothing to refresh
				return;
			}

			for (UObject* Outer = Object; Outer; Outer = Outer->GetOuter())
			{
				if (UsedObjects.Contains(Outer))
				{
					QueueRefresh();
					break;
				}
			}
		});
	}
#endif
}

TSharedRef<IVoxelMetaGraphRuntime> AVoxelActor::MakeMetaGraphRuntime(FVoxelRuntime& Runtime) const
{
	return MakeShared<FVoxelMetaGraphRuntime>(Runtime, MetaGraph.LoadSynchronous(), VariableCollection);
}

void AVoxelActor::Tick(float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();

	// We don't want to tick the BP in preview
	if (GetWorld()->WorldType != EWorldType::Editor &&
		GetWorld()->WorldType != EWorldType::EditorPreview)
	{
		Super::Tick(DeltaTime);
	}

	if (bRefreshQueued)
	{
		bRefreshQueued = false;

		DestroyRuntime();
		CreateRuntime();
	}
}

void AVoxelActor::PostLoad()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostLoad();

	Fixup();
}

void AVoxelActor::PostEditImport()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostEditImport();

	Fixup();
}

#if WITH_EDITOR
void AVoxelActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	Fixup();

	if (PropertyChangedEvent.MemberProperty &&
		PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_STATIC(AVoxelActor, MetaGraph))
	{
		VariableCollection.RefreshDetails.Broadcast();
	}

	// TODO more granular updates
	QueueRefresh();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelActor::BindGraphEvent(const FName Name, const FVoxelDynamicGraphEvent Delegate)
{
	Events.FindOrAdd(Name).Add(Delegate);
}

void AVoxelActor::QueueRefresh()
{
	bRefreshQueued = true;
}