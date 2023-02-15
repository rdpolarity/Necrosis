// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelTextureAtlas.h"
#include "Rendering/Texture2DResource.h"

DEFINE_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumTextures);

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_FreeSlots);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_UsedSlots);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_NumEntries);

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_NumTextureUpdates);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_NumRHIUpdateTexture2D);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_TextureUpdatesSize);

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_UsedData);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_WastedData);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelTextureAtlas_TextureData);

DEFINE_UNIQUE_VOXEL_ID(FVoxelTextureAtlasEntryUniqueId);

static FAutoConsoleCommand CompactTextureAtlasCmd(
	TEXT("voxel.TextureAtlas.compact"),
	TEXT("Reallocate all the entries, reducing fragmentation & saving memory"),
	MakeLambdaDelegate([]()
	{
		GEngine->GetEngineSubsystem<UVoxelTextureAtlasSubsystem>()->Compact();
	}));

VOXEL_CONSOLE_VARIABLE(
	VOXELCORE_API, int32, GVoxelTextureAtlasTextureSize, 1024,
	"voxel.TextureAtlas.TextureSize",
	"");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTextureAtlasTextureData::FVoxelTextureAtlasTextureData(EPixelFormat PixelFormat)
	: Stride(GPixelFormats[PixelFormat].BlockBytes)
	, PixelFormat(PixelFormat)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTextureAtlas::FVoxelTextureAtlas(EPixelFormat PixelFormat)
	: Stride(GPixelFormats[PixelFormat].BlockBytes) 
	, PixelFormat(PixelFormat)
	, TextureSize(FMath::Clamp(GVoxelTextureAtlasTextureSize, 128, 16384))
{
}

TSharedPtr<FVoxelTextureAtlasEntry> FVoxelTextureAtlas::AddEntry(
	const TSharedRef<const FVoxelTextureAtlasTextureData>& TextureData,
	const TSharedRef<FVoxelMaterialRef>& MaterialInstance,
	FName ParameterName)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(TextureData->PixelFormat == PixelFormat) ||
		!ensure(TextureData->Data.Num() % GPixelFormats[PixelFormat].BlockBytes == 0))
	{
		return nullptr;
	}
	
	ensure(TextureData->Data.Num() > 0);
	ensure(MaterialInstance->IsInstance());

	const TSharedRef<FEntry> Entry = MakeShared<FEntry>(TextureData, MaterialInstance, ParameterName);
	Entry->AllocateSlot(*this);
	
	const FEntryId EntryId = Entries.Add(Entry);

	const TSharedRef<FVoxelTextureAtlasEntry> AtlasEntry = MakeShared_GameThread<FVoxelTextureAtlasEntry>(EntryId, Entry->Id, SharedThis(this));
	MaterialInstance->AddResource(AtlasEntry);
	return AtlasEntry;
}

void FVoxelTextureAtlas::RemoveEntry(FEntryId EntryId, FEntryUniqueId UniqueId)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	
	if (!ensure(Entries.IsValidIndex(EntryId)))
	{
		return;
	}

	FEntry& Entry = *Entries[EntryId];
	if (ensure(Entry.Id == UniqueId))
	{
		Entry.FreeSlot();
		Entries.RemoveAt(EntryId);
	}
}

void FVoxelTextureAtlas::Compact()
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<FEntry>& Entry : Entries)
	{
		Entry->FreeSlot();
	}

	TSet<TWeakPtr<FTextureInfo>> UsedTextures;
	for (const TSharedPtr<FEntry>& Entry : Entries)
	{
		Entry->AllocateSlot(*this);
		UsedTextures.Add(Entry->SlotRef.TextureInfo);
	}

	TextureInfos.RemoveAllSwap([&](const TSharedPtr<FTextureInfo>& TextureInfo)
	{
		return !UsedTextures.Contains(TextureInfo);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTextureAtlas::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();
	
	for (const TSharedPtr<FTextureInfo>& TextureInfo : TextureInfos)
	{
		TextureInfo->Check();
		ensure(TextureInfo->Texture);
		Collector.AddReferencedObject(TextureInfo->Texture);
		ensure(TextureInfo->Texture);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTextureAtlas::FTextureInfo::FTextureInfo(uint32 Stride)
	: Stride(Stride)
{
}

FVoxelTextureAtlas::FTextureInfo::~FTextureInfo()
{
	DEC_VOXEL_COUNTER_BY(STAT_VoxelTextureAtlas_FreeSlots, FreeSlots.Num());
	DEC_VOXEL_COUNTER_BY(STAT_VoxelTextureAtlas_UsedSlots, UsedSlots.Num());

	for (auto& Slot : FreeSlots)
	{
		DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_WastedData, Stride * Slot.Num);
	}
	for (auto& Slot : UsedSlots)
	{
		DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_UsedData, Stride * Slot.Num);
	}
}

void FVoxelTextureAtlas::FTextureInfo::Check() const
{
#if VOXEL_DEBUG
	for (int32 Index = 1; Index < FreeSlots.Num(); Index++)
	{
		auto& SlotA = FreeSlots[Index - 1];
		auto& SlotB = FreeSlots[Index];

		ensure(SlotA.Num > 0);
		ensure(SlotB.Num > 0);
		
		ensure(SlotA.StartIndex < SlotB.StartIndex);
		// If equal, should be merged
		ensure(SlotA.EndIndex() < SlotB.StartIndex);
	}
#endif
}

void FVoxelTextureAtlas::FTextureInfo::FreeSlot(FTextureSlotId SlotId, FEntryUniqueId EntryId)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());
	ensure(EntryId.IsValid());
	Check();
	
	if (!ensure(UsedSlots.IsValidIndex(SlotId)))
	{
		return;
	}

	const FUsedTextureSlot UsedSlot = UsedSlots[SlotId];
	UsedSlots.RemoveAt(SlotId);
	DEC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_UsedSlots);
	ensure(UsedSlot.EntryId == EntryId);
	
	INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_WastedData, Stride * UsedSlot.Num);
	DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_UsedData, Stride * UsedSlot.Num);

	int32 Index;
	for (Index = 0; Index < FreeSlots.Num(); Index++)
	{
		if (FreeSlots[Index].StartIndex > UsedSlot.StartIndex)
		{
			break;
		}
	}
	
	INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_FreeSlots);
	FreeSlots.Insert(FTextureSlot(UsedSlot), Index);

	CompactFreeSlots();
	Check();
}

void FVoxelTextureAtlas::FTextureInfo::CompactFreeSlots()
{
	VOXEL_FUNCTION_COUNTER();
	check(FreeSlots.Num() > 0);
	
	TArray<FTextureSlot> NewSlots;
	NewSlots.Add(FreeSlots[0]);
	
	for (int32 Index = 1; Index < FreeSlots.Num(); Index++)
	{
		const FTextureSlot& Slot = FreeSlots[Index];
		FTextureSlot& LastSlot = NewSlots.Last();

		if (LastSlot.EndIndex() == Slot.StartIndex)
		{
			LastSlot.Num += Slot.Num;
		}
		else
		{
			NewSlots.Add(Slot);
		}
	}

	DEC_VOXEL_COUNTER_BY(STAT_VoxelTextureAtlas_FreeSlots, FreeSlots.Num());
	FreeSlots = MoveTemp(NewSlots);
	INC_VOXEL_COUNTER_BY(STAT_VoxelTextureAtlas_FreeSlots, FreeSlots.Num());
}

const FVoxelTextureAtlas::FTextureInfo* FVoxelTextureAtlas::FTextureSlotRef::GetSlot(FUsedTextureSlot& OutSlot) const
{
	if (!ensure(IsValid()))
	{
		return nullptr;
	}

	const auto PinnedTextureInfo = TextureInfo.Pin();
	if (!ensure(PinnedTextureInfo))
	{
		return nullptr;
	}

	if (!ensure(PinnedTextureInfo->UsedSlots.IsValidIndex(SlotId)))
	{
		return nullptr;
	}

	OutSlot = PinnedTextureInfo->UsedSlots[SlotId];
	return PinnedTextureInfo.Get();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTextureAtlas::FTextureSlotRef FVoxelTextureAtlas::AllocateSlot(int32 Size, FEntryUniqueId EntryId)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());
	ensure(EntryId.IsValid());

	if (Size > FMath::Square(TextureSize))
	{
		VOXEL_MESSAGE(Error, 
			"The voxel texture atlas texture size is too small: tried to allocate {0} values, but textures are max {1}x{2}!\n"
			"You can change it in the project settings",
			Size,
			TextureSize,
			TextureSize);
		return {};
	}

	// Try to find a free slot
	for (auto& TextureInfo : TextureInfos)
	{
		auto& FreeSlots = TextureInfo->FreeSlots;
		for (int32 Index = 0; Index < FreeSlots.Num(); Index++)
		{
			auto& FreeSlot = FreeSlots[Index];
			if (FreeSlot.Num == Size)
			{
				FUsedTextureSlot UsedSlot;
				UsedSlot.StartIndex = FreeSlot.StartIndex;
				UsedSlot.Num = FreeSlot.Num;
				UsedSlot.EntryId = EntryId;

				DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_WastedData, Stride * FreeSlot.Num);
				DEC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_FreeSlots);
				FreeSlots.RemoveAt(Index);

				FTextureSlotRef Ref;
				Ref.TextureInfo = TextureInfo;
				Ref.SlotId = TextureInfo->UsedSlots.Add(UsedSlot);
				INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_UsedSlots);
				INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_UsedData, Stride * UsedSlot.Num);
				return Ref;
			}
			else if (FreeSlot.Num > Size)
			{
				FUsedTextureSlot UsedSlot;
				UsedSlot.StartIndex = FreeSlot.StartIndex;
				UsedSlot.Num = Size;
				UsedSlot.EntryId = EntryId;

				FreeSlot.StartIndex += Size;
				FreeSlot.Num -= Size;
				DEC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_WastedData, Stride * Size);

				FTextureSlotRef Ref;
				Ref.TextureInfo = TextureInfo;
				Ref.SlotId = TextureInfo->UsedSlots.Add(UsedSlot);
				INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_UsedSlots);
				INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_UsedData, Stride * UsedSlot.Num);
				return Ref;
			}
		}
	}
	
	// Allocate a new texture
	UTexture2D* NewTexture = CreateTexture();
	if (!ensure(NewTexture))
	{
		return {};
	}

	const auto NewTextureInfo = MakeShared<FTextureInfo>(Stride);
	NewTextureInfo->Texture = NewTexture;
	TextureInfos.Add(NewTextureInfo);

	if (Size < FMath::Square(TextureSize))
	{
		// Add free slot
		FTextureSlot FreeSlot;
		FreeSlot.StartIndex = Size;
		FreeSlot.Num = FMath::Square(TextureSize) - Size;
		NewTextureInfo->FreeSlots.Add(FreeSlot);
		INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_FreeSlots);
		INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_WastedData, Stride * FreeSlot.Num);
	}

	FUsedTextureSlot UsedSlot;
	UsedSlot.StartIndex = 0;
	UsedSlot.Num = Size;
	UsedSlot.EntryId = EntryId;

	FTextureSlotRef Ref;
	Ref.TextureInfo = NewTextureInfo;
	Ref.SlotId = NewTextureInfo->UsedSlots.Add(UsedSlot);
	INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_UsedSlots);
	INC_VOXEL_MEMORY_STAT_BY(STAT_VoxelTextureAtlas_UsedData, Stride * UsedSlot.Num);
	return Ref;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UTexture2D* FVoxelTextureAtlas::CreateTexture() const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PixelFormat);
	if (!ensure(Texture))
	{
		return nullptr;
	}

	Texture->SRGB = false;
	Texture->Filter = TF_Nearest;

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	{
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memzero(Data, Stride * FMath::Square(TextureSize));
		Mip.BulkData.Unlock();
	}
	Texture->UpdateResource();
	
	return Texture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTextureAtlas::FEntry::FEntry(
	const TSharedRef<const FVoxelTextureAtlasTextureData>& ColorData,
	const TWeakPtr<FVoxelMaterialRef>& MaterialInstance,
	FName ParameterName)
	: ColorData(ColorData)
	, MaterialInstance(MaterialInstance)
	, ParameterName(ParameterName)
{
	INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumEntries);
}

FVoxelTextureAtlas::FEntry::~FEntry()
{
	DEC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumEntries);
}

void FVoxelTextureAtlas::FEntry::CopyDataToTexture(bool bJustClearData) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	if (GExitPurge)
	{
		// Else the Texture->Resource ensure below is raised
		return;
	}

	FUsedTextureSlot Slot;
	const FTextureInfo* TextureInfo = SlotRef.GetSlot(Slot);
	if (!ensure(TextureInfo))
	{
		return;
	}

	if (!ensure(Slot.Num * ColorData->Stride == ColorData->Data.Num()))
	{
		return;
	}

	auto* Texture = TextureInfo->Texture;
	if (!ensure(Texture))
	{
		return;
	}

	INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumTextureUpdates);
	INC_VOXEL_COUNTER_BY(STAT_VoxelTextureAtlas_TextureUpdatesSize, ColorData->Data.Num());

	if (ensure(Texture->GetResource()))
	{
		ENQUEUE_RENDER_COMMAND(UpdateVoxelTextureAtlasRegionsData)(
        [
		  Resource = Texture->GetResource()->GetTexture2DResource(),
          ColorData = ColorData, 
		  Slot, 
		  bJustClearData](FRHICommandListImmediate& RHICmdList)
        {
			VOXEL_SCOPE_COUNTER("Update Region");

        	if (bJustClearData)
        	{
        		LOG_VOXEL(Verbose, "Clearing texture %p at %d:%d for entry %llu", Resource, Slot.StartIndex, Slot.Num, Slot.EntryId.GetId());
        	}
			else
			{
        		LOG_VOXEL(Verbose, "Updating texture %p at %d:%d for entry %llu", Resource, Slot.StartIndex, Slot.Num, Slot.EntryId.GetId());
			}
			
			const uint8* Data = ColorData->Data.GetData();
			TArray<uint8> EmptyData;
			if (bJustClearData)
			{
				EmptyData.SetNumZeroed(Slot.Num * ColorData->Stride);
				Data = EmptyData.GetData();
			}
			
			const int32 RowSize = Resource->GetSizeX();
			const FTexture2DRHIRef TextureRHI = Resource->GetTexture2DRHI();
			const uint8* const EndData = Data + Slot.Num * ColorData->Stride;

			if (!TextureRHI)
			{
				ensure(IsEngineExitRequested());
				return;
			}

			// SourcePitch - size in bytes of each row of the source image

			int32 FirstIndex = Slot.StartIndex;
			if (FirstIndex % RowSize != 0)
			{
				// Fixup any data at the start not aligned to the row
				
				const int32 StartX = FirstIndex % RowSize;
				const int32 Num = FMath::Min(RowSize - StartX, Slot.Num);
				
				FUpdateTextureRegion2D Region;
				Region.DestX = StartX;
				Region.DestY = FirstIndex / RowSize;
				Region.SrcX = 0;
				Region.SrcY = 0;
				Region.Width = Num;
				Region.Height = 1;
				
				VOXEL_SCOPE_COUNTER("RHIUpdateTexture2D");
				INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumRHIUpdateTexture2D);
				RHIUpdateTexture2D(TextureRHI, 0, Region, Num * ColorData->Stride, Data);

				FirstIndex += Num;
				Data += Region.Width * Region.Height * ColorData->Stride;
				check(Data <= EndData);
			}
			
			if (Data == EndData)
			{
				// Only one line
				return;
			}
			check(FirstIndex % RowSize == 0);
		
			const int32 LastIndex = Slot.EndIndex();
			const int32 FirstIndexRow = FirstIndex / RowSize;
			const int32 LastIndexRow = LastIndex / RowSize;
			if (FirstIndexRow != LastIndexRow)
			{
				FUpdateTextureRegion2D Region;
				Region.DestX = 0;
				Region.DestY = FirstIndexRow;
				Region.SrcX = 0;
				Region.SrcY = 0;
				Region.Width = RowSize;
				Region.Height = LastIndexRow - FirstIndexRow;
				
				VOXEL_SCOPE_COUNTER("RHIUpdateTexture2D");
				INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumRHIUpdateTexture2D);
				RHIUpdateTexture2D(TextureRHI, 0, Region, RowSize * ColorData->Stride, Data);

				Data += Region.Width * Region.Height * ColorData->Stride;
				check(Data <= EndData);
			}

			if (LastIndex % RowSize != 0)
			{
				// Fixup any data at the end not aligned to the row
				
				const int32 Num = LastIndex % RowSize;
				
				FUpdateTextureRegion2D Region;
				Region.DestX = 0;
				Region.DestY = LastIndex / RowSize;
				Region.SrcX = 0;
				Region.SrcY = 0;
				Region.Width = Num;
				Region.Height = 1;
				
				VOXEL_SCOPE_COUNTER("RHIUpdateTexture2D");
				INC_VOXEL_COUNTER(STAT_VoxelTextureAtlas_NumRHIUpdateTexture2D);
				RHIUpdateTexture2D(TextureRHI, 0, Region, Num * ColorData->Stride, Data);
				
				Data += Region.Width * Region.Height * ColorData->Stride;
			}
			check(Data == EndData);
		});
	}
	else if (ensure(Texture->GetPlatformData()))
	{
		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		{
			void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
			if (ensure(Data))
			{
				void* Dst = static_cast<FColor*>(Data) + Slot.StartIndex;
				const void* Src = ColorData->Data.GetData();
				const int32 Count = ColorData->Data.Num();

				if (bJustClearData)
				{
					FMemory::Memzero(Dst, Count);
				}
				else
				{
					FMemory::Memcpy(Dst, Src, Count);
				}
			}
			Mip.BulkData.Unlock();
		}
		Texture->UpdateResource();
	}
}

void FVoxelTextureAtlas::FEntry::SetupMaterialInstance() const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	const TSharedPtr<FVoxelMaterialRef> PinnedMaterialInstance = MaterialInstance.Pin();
	if (!ensure(PinnedMaterialInstance) ||
		!ensure(PinnedMaterialInstance->IsInstance()))
	{
		return;
	}

	UMaterialInstanceDynamic* MaterialInstanceObject = PinnedMaterialInstance->GetMaterialInstance();
	if (!ensure(MaterialInstanceObject))
	{
		return;
	}

	FUsedTextureSlot Slot;
	const FTextureInfo* TextureInfo = SlotRef.GetSlot(Slot);
	if (!ensure(TextureInfo) ||
		!ensure(TextureInfo->Texture))
	{
		return;
	}

	MaterialInstanceObject->SetTextureParameterValue("VoxelTextureAtlas_" + ParameterName + "Texture", TextureInfo->Texture);
	MaterialInstanceObject->SetScalarParameterValue(STATIC_FNAME("VoxelTextureAtlas_TextureSizeX"), TextureInfo->Texture->GetSizeX());
	MaterialInstanceObject->SetScalarParameterValue("VoxelTextureAtlas_" + ParameterName + "Index", Slot.StartIndex);
}

void FVoxelTextureAtlas::FEntry::AllocateSlot(FVoxelTextureAtlas& Atlas)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!SlotRef.IsValid());
	
	checkVoxelSlow(ColorData->Data.Num() % ColorData->Stride == 0);
	SlotRef = Atlas.AllocateSlot(ColorData->Data.Num() / ColorData->Stride, Id);

	if (SlotRef.IsValid())
	{
		// If we didn't fail to allocate
		CopyDataToTexture();
		SetupMaterialInstance();
	}
}

void FVoxelTextureAtlas::FEntry::FreeSlot()
{
	VOXEL_FUNCTION_COUNTER();
	
	if (!SlotRef.IsValid())
	{
		// Failed to allocate - nothing to do
		return;
	}

#if VOXEL_DEBUG
	// Clear texture to make spotting errors easier
	CopyDataToTexture(true);
#endif

	const auto TextureInfo = SlotRef.TextureInfo.Pin();
	if (!ensure(TextureInfo))
	{
		return;
	}

	TextureInfo->FreeSlot(SlotRef.SlotId, Id);

	SlotRef = {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTextureAtlasEntry::~FVoxelTextureAtlasEntry()
{
	check(IsInGameThread());

	if (const TSharedPtr<FVoxelTextureAtlas> Atlas = WeakAtlas.Pin())
	{
		Atlas->RemoveEntry(EntryId, UniqueId);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelTextureAtlasSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UVoxelTextureAtlasSubsystem::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER_LLM();
	
	for (auto& It : CastChecked<UVoxelTextureAtlasSubsystem>(InThis)->Atlases)
	{
		It.Value->AddReferencedObjects(Collector);
	}
}

void UVoxelTextureAtlasSubsystem::Compact()
{
	VOXEL_FUNCTION_COUNTER();
	
	for (auto& It : Atlases)
	{
		It.Value->Compact();
	}
}

FVoxelTextureAtlas& UVoxelTextureAtlasSubsystem::GetAtlas(EPixelFormat PixelFormat)
{
	TSharedPtr<FVoxelTextureAtlas>& Atlas = Atlases.FindOrAdd(PixelFormat);
	if (!Atlas)
	{
		Atlas = MakeShared<FVoxelTextureAtlas>(PixelFormat);
	}
	check(Atlas->PixelFormat == PixelFormat);
	return *Atlas;
}