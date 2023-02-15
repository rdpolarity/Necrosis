// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphNodeCustomization.h"
#include "VoxelMetaGraph.h"
#include "Nodes/VoxelMetaGraphStructNode.h"
#include "VoxelMetaGraphNodePinArrayCustomization.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

inline const UScriptStruct* GetCommonScriptStruct(TSharedPtr<IPropertyHandle> StructHandle)
{
	const UScriptStruct* CommonStructType = nullptr;

	StructHandle->EnumerateConstRawData([&CommonStructType](const void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
	{
		if (RawData)
		{
			const FVoxelInstancedStruct* InstancedStruct = static_cast<const FVoxelInstancedStruct*>(RawData);

			const UScriptStruct* StructTypePtr = InstancedStruct->GetScriptStruct();
			if (CommonStructType && CommonStructType != StructTypePtr)
			{
				// Multiple struct types on the sources - show nothing set
				CommonStructType = nullptr;
				return false;
			}
			CommonStructType = StructTypePtr;
		}

		return true;
	});

	return CommonStructType;
}

void FVoxelNodeCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	RefreshDelegate = FVoxelEditorUtilities::MakeRefreshDelegate(DetailLayout);

	const TArray<TWeakObjectPtr<UObject>> SelectedObjects = DetailLayout.GetSelectedObjects();

	if (SelectedObjects.Num() != 1)
	{
		return;
	}

	UVoxelMetaGraphStructNode* Node = Cast<UVoxelMetaGraphStructNode>(SelectedObjects[0]);
	if (!ensure(Node) ||
		!ensure(Node->Struct))
	{
		return;
	}

	NodeDefinition = Node->GetNodeDefinition();

	Node->Struct->OnExposedPinsUpdated.Add(MakeWeakPtrDelegate(this, [this]()
	{
		RefreshDelegate.ExecuteIfBound();
	}));

	const TSharedRef<IPropertyHandle> StructHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelMetaGraphStructNode, Struct), Node->GetClass());

	const TMap<FName, TSharedPtr<IPropertyHandle>> ChildrenHandles = InitializeStructChildren(StructHandle);
	if (ChildrenHandles.Num() == 0)
	{
		return;
	}

	DetailLayout.HideProperty(StructHandle);

	const TSharedPtr<IPropertyHandle> ExposedPinsValuesHandle = ChildrenHandles.FindRef(GET_MEMBER_NAME_STATIC(FVoxelNode, ExposedPinsValues));
	if (!ensure(ExposedPinsValuesHandle))
	{
		return;
	}

	IDetailCategoryBuilder& DefaultCategoryBuilder = DetailLayout.EditCategory("Default", VOXEL_LOCTEXT("Default"));

	TMap<FName, TSharedPtr<IPropertyHandle>> MappedHandles;

	uint32 NumValues = 0;
	ExposedPinsValuesHandle->GetNumChildren(NumValues);

	for (uint32 Index = 0; Index < NumValues; Index++)
	{
		const TSharedPtr<IPropertyHandle> ValueHandle = ExposedPinsValuesHandle->GetChildHandle(Index);
		const TSharedPtr<IPropertyHandle> KeyHandle = ValueHandle->GetKeyHandle();

		if (!ensure(ValueHandle) ||
			!ensure(KeyHandle))
		{
			continue;
		}

		FName PinName;
		KeyHandle->GetValue(PinName);

		if (!ensure(!PinName.IsNone()))
		{
			continue;
		}

		MappedHandles.Add(PinName, ValueHandle);
	}

	for (const FName PinName : Node->Struct->InternalPinsOrder)
	{
		if (TSharedPtr<FVoxelNode::FPinArray> PinArray = Node->Struct->InternalPinArrays.FindRef(PinName))
		{
			if (!PinArray->PinTemplate.Metadata.bPropertyBind)
			{
				continue;
			}

			IDetailCategoryBuilder* TargetBuilder = &DefaultCategoryBuilder;
			if (!PinArray->PinTemplate.Metadata.Category.IsEmpty())
			{
				TargetBuilder = &DetailLayout.EditCategory(FName(PinArray->PinTemplate.Metadata.Category), FText::FromString(PinArray->PinTemplate.Metadata.Category));
			}

			TSharedRef<FVoxelNodePinArrayCustomization> PinArrayCustomization = MakeShared<FVoxelNodePinArrayCustomization>();
			PinArrayCustomization->PinName = PinName.ToString();
			PinArrayCustomization->Tooltip = PinArray->PinTemplate.Metadata.Tooltip; // TODO:
			PinArrayCustomization->WeakCustomization = SharedThis(this);
			PinArrayCustomization->WeakStructNode = Node;
			PinArrayCustomization->WeakNodeDefinition = NodeDefinition;

			for (FName ArrayElementPinName : PinArray->Pins)
			{
				if (TSharedPtr<IPropertyHandle> ValueHandle = MappedHandles.FindRef(ArrayElementPinName))
				{
					PinArrayCustomization->Properties.Add(ValueHandle);
				}
			}

			TargetBuilder->AddCustomBuilder(PinArrayCustomization);
			continue;
		}

		TSharedPtr<FVoxelPin> VoxelPin = Node->Struct->FindPin(PinName);
		if (!VoxelPin)
		{
			continue;
		}

		if (!VoxelPin->Metadata.bPropertyBind)
		{
			continue;
		}

		if (!VoxelPin->ArrayOwner.IsNone())
		{
			continue;
		}

		IDetailCategoryBuilder* TargetBuilder = &DefaultCategoryBuilder;
		if (!VoxelPin->Metadata.Category.IsEmpty())
		{
			TargetBuilder = &DetailLayout.EditCategory(FName(VoxelPin->Metadata.Category), FText::FromString(VoxelPin->Metadata.Category));
		}

		TSharedPtr<IPropertyHandle> ValueHandle = MappedHandles.FindRef(PinName);
		if (!ValueHandle)
		{
			continue;
		}

		const FVoxelPinValue& PinValue = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelPinValue>(ValueHandle);

		FString DisplayName = VoxelPin->Metadata.DisplayName;
		if (DisplayName.IsEmpty())
		{
			DisplayName = FName::NameToDisplayString(PinName.ToString(), false);
		}
		else
		{
			DisplayName = FName::NameToDisplayString(DisplayName, false);
		}

		AddPinValueCustomization(
			*TargetBuilder,
			ValueHandle.ToSharedRef(),
			PinValue,
			DisplayName,
			VoxelPin->Metadata.Tooltip, // TODO:
			[this, WeakNode = MakeWeakObjectPtr(Node), PinName](FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)
			{
				Row
				.NameContent()
				[
					NameWidget
				]
				.ValueContent()
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.Visibility_Lambda([WeakNode, PinName]
						{
							const UVoxelMetaGraphStructNode* TargetNode = WeakNode.Get();
							if (!ensure(TargetNode))
							{
								return EVisibility::Visible;
							}

							return TargetNode->Struct->ExposedPins.Contains(PinName) ? EVisibility::Collapsed : EVisibility::Visible;
						})
						.MinDesiredWidth(125.f)
						[
							ValueWidget
						]
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.FillWidth(1.f)
					[
						CreateExposePinButton(WeakNode, PinName)
					]
				];
			});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FPinValueCustomization> FVoxelNodeCustomization::GetSelfSharedPtr()
{
	return StaticCastSharedRef<FVoxelNodeCustomization>(AsShared());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> FVoxelNodeCustomization::CreateExposePinButton(const TWeakObjectPtr<UVoxelMetaGraphStructNode>& WeakNode, FName PinName) const
{
	return
		PropertyCustomizationHelpers::MakeVisibilityButton(
			FOnClicked::CreateLambda([WeakNode, PinName]() -> FReply
			{
				UVoxelMetaGraphStructNode* TargetNode = WeakNode.Get();
				if (!ensure(TargetNode))
				{
					return FReply::Handled();
				}

				const FVoxelTransaction Transaction(TargetNode, "Expose Pin");

				if (TargetNode->Struct->ExposedPins.Remove(PinName) == 0)
				{
					TargetNode->Struct->ExposedPins.Add(PinName);
				}

				TargetNode->ReconstructNode(false);
				return FReply::Handled();
			}),
			{},
			MakeAttributeLambda([WeakNode, PinName]
			{
				const UVoxelMetaGraphStructNode* TargetNode = WeakNode.Get();
				if (!ensure(TargetNode))
				{
					return false;
				}

				return TargetNode->Struct->ExposedPins.Contains(PinName) ? true : false;
			}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TMap<FName, TSharedPtr<IPropertyHandle>> FVoxelNodeCustomization::InitializeStructChildren(const TSharedRef<IPropertyHandle>& StructHandle)
{
	TSharedPtr<FStructOnScope> StructInstanceData;
	{
		StructInstanceData.Reset();
		if (const UScriptStruct* CommonStructType = GetCommonScriptStruct(StructHandle))
		{
			StructInstanceData = MakeShared<FStructOnScope>(CommonStructType);

			// Make sure the struct also has a valid package set, so that properties that rely on this (like FText) work correctly
			{
				TArray<UPackage*> OuterPackages;
				StructHandle->GetOuterPackages(OuterPackages);
				if (OuterPackages.Num() > 0)
				{
					StructInstanceData->SetPackage(OuterPackages[0]);
				}
			}
		}
	}

	if (!StructInstanceData)
	{
		return {};
	}

	AddStructOnScope(StructHandle, StructInstanceData.ToSharedRef());

	const TArray<TSharedPtr<IPropertyHandle>> ChildrenHandles = StructHandle->AddChildStructure(StructInstanceData.ToSharedRef());
	BindStructPropertyValueChanges(StructHandle, ChildrenHandles);

	TMap<FName, TSharedPtr<IPropertyHandle>> MappedChildrenHandles;
	for (TSharedPtr<IPropertyHandle> ChildHandle : ChildrenHandles)
	{
		MappedChildrenHandles.Add(ChildHandle->GetProperty()->GetFName(), ChildHandle);
	}

	return MappedChildrenHandles;
}

END_VOXEL_NAMESPACE(MetaGraph)