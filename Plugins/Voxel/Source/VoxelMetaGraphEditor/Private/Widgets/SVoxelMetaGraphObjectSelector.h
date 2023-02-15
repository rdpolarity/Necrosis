// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class SVoxelMetaGraphObjectSelector : public SCompoundWidget
{
private:
	struct FObjectOrAssetData
	{
		UObject* Object;
		FSoftObjectPath ObjectPath;
		FAssetData AssetData;

		FObjectOrAssetData(UObject* InObject = nullptr)
			: Object(InObject)
			, ObjectPath(Object)
		{
			AssetData =
				InObject != nullptr &&
				!InObject->IsA<AActor>() ? FAssetData( InObject ) : FAssetData();
		}

		FObjectOrAssetData(const FSoftObjectPath& InObjectPath)
			: Object(nullptr)
			, ObjectPath(InObjectPath)
		{}

		FObjectOrAssetData(const FAssetData& InAssetData)
			: Object(nullptr)
			, ObjectPath(InAssetData.ToSoftObjectPath())
			, AssetData(InAssetData)
		{}

		bool IsValid() const
		{
			return
				Object != nullptr ||
				ObjectPath.IsValid() ||
				AssetData.IsValid();
		}
	};

public:
	VOXEL_SLATE_ARGS()
	{
		FArguments()
			: _AllowedClass(nullptr)
			, _ThumbnailSize(FIntPoint(48, 48))
		{
		}
		SLATE_ATTRIBUTE(FString, ObjectPath)
		SLATE_ARGUMENT(UClass*, AllowedClass)
		SLATE_ARGUMENT(TArray<UClass*>, AllowedClasses)
		SLATE_ARGUMENT(FIntPoint, ThumbnailSize)
		SLATE_ARGUMENT(TSharedPtr<FAssetThumbnailPool>, ThumbnailPool)
		SLATE_EVENT(FOnSetObject, OnObjectChanged)
	};

	void Construct(const FArguments& InArgs);

private:
	bool OnAssetDraggedOver(TArrayView<FAssetData> InAssets, FText& OutReason) const;
	void OnAssetDropped(const FDragDropEvent&, TArrayView<FAssetData> InAssets) const;

	TSharedRef<SWidget> OnGetMenuContent() const;
	void OnMenuOpenChanged(bool bOpen) const;

	bool IsFilteredActor(const AActor* Actor) const;
	void CloseComboButton() const;

	FText OnGetAssetName() const;

	FReply OnAssetThumbnailDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) const;

	void OnUseClicked() const;
	void OnBrowseClicked() const;

	void OnGetAllowedClasses(TArray<const UClass*>& OutAllowedClasses) const;
	void OnActorSelected(AActor* Actor) const;

private:
	FObjectOrAssetData GetValue() const;
	void SetValue(const FAssetData& AssetData) const;

	bool CanSetBasedOnCustomClasses(const FAssetData& AssetData) const;
	bool CanSetBasedOnAssetReferenceFilter( const FAssetData& InAssetData, FText& OutOptionalFailureReason) const;
	bool IsClassAllowed(const UClass* InClass) const;

private:
	TAttribute<FString> ObjectPath;

	TSharedPtr<SComboButton> AssetComboButton;
	TSharedPtr<SBorder> ThumbnailBorder;

	TArray<const UClass*> AllowedClasses;
	UClass* ObjectClass = nullptr;
	TSharedPtr<FAssetThumbnail> AssetThumbnail;
	mutable FAssetData CachedAssetData;
	TArray<FAssetData> OwnerAssetDataArray;

	bool bIsActor = false;

	FOnSetObject OnSetObject;
	FOnShouldFilterAsset OnShouldFilterAsset;
};