// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "VoxelTextureAtlas.generated.h"

class UTexture2D;
class FVoxelTextureAtlasEntry;

DECLARE_UNIQUE_VOXEL_ID(FVoxelTextureAtlasEntryUniqueId);

DECLARE_STATS_GROUP(TEXT("Voxel Texture Atlas"), STATGROUP_VoxelTextureAtlas, STATCAT_Advanced);

DECLARE_VOXEL_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_FreeSlots, "Free Slots");
DECLARE_VOXEL_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_UsedSlots, "Used Slots");
DECLARE_VOXEL_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_NumEntries, "Num Entries");

DECLARE_VOXEL_FRAME_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_NumTextureUpdates, "Num Texture Updates");
DECLARE_VOXEL_FRAME_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_NumRHIUpdateTexture2D, "Num RHIUpdateTexture2D");
DECLARE_VOXEL_FRAME_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_TextureUpdatesSize, "Texture Updates Size");

DECLARE_VOXEL_MEMORY_STAT_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_UsedData, "Used Data");
DECLARE_VOXEL_MEMORY_STAT_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_WastedData, "Wasted Data");
DECLARE_VOXEL_MEMORY_STAT_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_TextureData, "Texture Data");

DECLARE_VOXEL_COUNTER_WITH_CATEGORY(VOXELCORE_API, STATGROUP_VoxelTextureAtlas, STAT_VoxelTextureAtlas_NumTextures, "Num Textures");

class VOXELCORE_API FVoxelTextureAtlasTextureData
{
public:
	const uint32 Stride;
	const EPixelFormat PixelFormat;

	TVoxelArray<uint8> Data;

	explicit FVoxelTextureAtlasTextureData(EPixelFormat PixelFormat);

	int64 GetAllocatedSize() const
	{
		return Data.GetAllocatedSize();
	}
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelTextureAtlas_TextureData);
};

class VOXELCORE_API FVoxelTextureAtlas : public TSharedFromThis<FVoxelTextureAtlas>
{
public:
	DECLARE_TYPED_VOXEL_SPARSE_ARRAY_ID(FEntryId);
	using FEntryUniqueId = FVoxelTextureAtlasEntryUniqueId;
	
public:
	const uint32 Stride;
	const EPixelFormat PixelFormat;
	const int32 TextureSize;

	explicit FVoxelTextureAtlas(EPixelFormat PixelFormat);

	TSharedPtr<FVoxelTextureAtlasEntry> AddEntry(
		const TSharedRef<const FVoxelTextureAtlasTextureData>& TextureData,
		const TSharedRef<FVoxelMaterialRef>& MaterialInstance,
		FName ParameterName);
	
	void RemoveEntry(FEntryId EntryId, FEntryUniqueId UniqueId);

	// Reallocate all the entries, reducing fragmentation
	void Compact();
	void AddReferencedObjects(FReferenceCollector& Collector);
	
private:
	struct FTextureSlot
	{
		int32 StartIndex = 0;
		int32 Num = 0;

		int32 EndIndex() const { return StartIndex + Num; }
	};
	struct FUsedTextureSlot : FTextureSlot
	{
		FEntryUniqueId EntryId;
	};

	DECLARE_TYPED_VOXEL_SPARSE_ARRAY_ID(FTextureSlotId);
	
	class FTextureInfo
	{
	public:
		const uint32 Stride;
		UTexture2D* Texture = nullptr;

		TArray<FTextureSlot> FreeSlots;
		TVoxelTypedSparseArray<FTextureSlotId, FUsedTextureSlot> UsedSlots;

		explicit FTextureInfo(uint32 Stride);
		~FTextureInfo();
		UE_NONCOPYABLE(FTextureInfo);

		VOXEL_NUM_INSTANCES_TRACKER(STAT_VoxelTextureAtlas_NumTextures);
		
		void Check() const;
		void FreeSlot(FTextureSlotId SlotId, FEntryUniqueId EntryId);

	private:
		void CompactFreeSlots();
	};
	TArray<TSharedPtr<FTextureInfo>> TextureInfos;

private:
	struct FTextureSlotRef
	{
		TWeakPtr<FTextureInfo> TextureInfo;
		FTextureSlotId SlotId;

		bool IsValid() const
		{
			return SlotId.IsValid();
		}
		const FTextureInfo* GetSlot(FUsedTextureSlot& OutSlot) const;
	};
	
	FTextureSlotRef AllocateSlot(int32 Size, FEntryUniqueId EntryId);

	UTexture2D* CreateTexture() const;

private:
	struct FEntry
	{
		const FEntryUniqueId Id = FEntryUniqueId::New();

		const TSharedRef<const FVoxelTextureAtlasTextureData> ColorData;
		const TWeakPtr<FVoxelMaterialRef> MaterialInstance;
		const FName ParameterName;

		FTextureSlotRef SlotRef;

		FEntry(
			const TSharedRef<const FVoxelTextureAtlasTextureData>& ColorData,
			const TWeakPtr<FVoxelMaterialRef>& MaterialInstance,
			FName ParameterName);
		~FEntry();
		UE_NONCOPYABLE(FEntry);
		
		void CopyDataToTexture(bool bJustClearData = false) const;
		void SetupMaterialInstance() const;

		void AllocateSlot(FVoxelTextureAtlas& Atlas);
		void FreeSlot();
	};
	TVoxelTypedSparseArray<FEntryId, TSharedPtr<FEntry>> Entries;
};

class VOXELCORE_API FVoxelTextureAtlasEntry : public FVirtualDestructor
{
public:
	explicit FVoxelTextureAtlasEntry(
		FVoxelTextureAtlas::FEntryId EntryId,
		FVoxelTextureAtlas::FEntryUniqueId UniqueId,
		const TWeakPtr<FVoxelTextureAtlas>& Atlas)
		: EntryId(EntryId)
		, UniqueId(UniqueId)
		, WeakAtlas(Atlas)
	{
	}
	virtual ~FVoxelTextureAtlasEntry() override;

private:
	const FVoxelTextureAtlas::FEntryId EntryId;
	const FVoxelTextureAtlas::FEntryUniqueId UniqueId;
	const TWeakPtr<FVoxelTextureAtlas> WeakAtlas;

	friend class FVoxelTextureAtlas;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UCLASS()
class VOXELCORE_API UVoxelTextureAtlasSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	void Compact();

	FVoxelTextureAtlas& GetAtlas(EPixelFormat PixelFormat);

protected:
	//~ Begin UGameInstanceSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End UGameInstanceSubsystem Interface

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

private:
	TMap<EPixelFormat, TSharedPtr<FVoxelTextureAtlas>> Atlases;
};