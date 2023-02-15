// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"
#include "Widgets/Views/STreeView.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class SPinTypeSelector;

enum class EVoxelPinContainerType : uint8
{
	None,
	Buffer,
};

class SPinTypeComboBox : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnTypeChanged, FVoxelPinType)

public:
	VOXEL_SLATE_ARGS()
	{
		FArguments()
			: _DetailsWindow(true)
			, _WithContainerType(true)
		{
		}

		SLATE_ATTRIBUTE(TArray<FVoxelPinType>, PinTypes)
		SLATE_ARGUMENT(FVoxelPinType, CurrentType)
		SLATE_ARGUMENT(bool, DetailsWindow)
		SLATE_ATTRIBUTE(bool, ReadOnly)
		SLATE_ARGUMENT(bool, WithContainerType)
		SLATE_EVENT(FOnTypeChanged, OnTypeChanged)
	};

	void Construct(const FArguments& InArgs);

public:
	void SetType(const FVoxelPinType& NewPinType);

private:
	TSharedRef<SWidget> GetMenuContent();

	TSharedRef<SWidget> GetPinContainerTypeMenuContent() const;
	bool IsValidContainerType(EVoxelPinContainerType ContainerType) const;
	void OnContainerTypeSelectionChanged(EVoxelPinContainerType ContainerType) const;

private:
	const FSlateBrush* GetIcon(const FVoxelPinType& PinType) const;
	FLinearColor GetColor(const FVoxelPinType& PinType) const;

private:
	TSharedPtr<SComboButton> TypeComboButton;
	TSharedPtr<SImage> MainIcon;
	TSharedPtr<STextBlock> MainTextBlock;

	TSharedPtr<SMenuOwner> MenuContent;
	TSharedPtr<SPinTypeSelector> PinTypeSelector;

private:
	TAttribute<TArray<FVoxelPinType>> PinTypesAttribute;
	FVoxelPinType CurrentType;
	FVoxelPinType CurrentInnerType;
	TAttribute<bool> ReadOnly;
	FOnTypeChanged OnTypeChanged;
};

END_VOXEL_NAMESPACE(MetaGraph)