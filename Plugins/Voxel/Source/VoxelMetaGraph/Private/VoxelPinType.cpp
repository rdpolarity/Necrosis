// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelPinType.h"
#include "VoxelExposedPinType.h"

struct FVoxelPinTypeStatics
{
	bool bInitialized = false;
	TMap<FVoxelPinType, EPixelFormat> PixelFormats;
	TMap<FVoxelPinType, FVoxelPinType> InnerToBufferTypes;
	TMap<FVoxelPinType, FVoxelPinType> BufferToInnerTypes;
	TMap<FVoxelPinType, FVoxelPinType> InnerAndBufferToViewTypes;

	void Initialize()
	{
		VOXEL_FUNCTION_COUNTER();
		check(IsInGameThread());

		if (bInitialized)
		{
			return;
		}
		bInitialized = true;

		for (UScriptStruct* Struct : GetDerivedStructs<FVoxelBuffer>())
		{
			if (Struct == FVoxelTerminalBuffer::StaticStruct() ||
				Struct == FVoxelContainerBuffer::StaticStruct())
			{
				continue;
			}

			TVoxelInstancedStruct<FVoxelBuffer> Buffer(Struct);

			const FVoxelPinType InnerType = Buffer->GetInnerType();
			const FVoxelPinType BufferType = FVoxelPinType::MakeStruct(Struct);
			const FVoxelPinType ViewType = Buffer->GetViewType();

			if (Buffer.IsA<FVoxelTerminalBuffer>())
			{
				PixelFormats.Add(InnerType, Buffer.Get<FVoxelTerminalBuffer>().GetFormat());
			}

			ensure(!InnerToBufferTypes.Contains(InnerType));
			InnerToBufferTypes.Add(InnerType, BufferType);

			ensure(!BufferToInnerTypes.Contains(BufferType));
			BufferToInnerTypes.Add(BufferType, InnerType);

			ensure(!InnerAndBufferToViewTypes.Contains(InnerType));
			ensure(!InnerAndBufferToViewTypes.Contains(BufferType));
			InnerAndBufferToViewTypes.Add(InnerType, ViewType);
			InnerAndBufferToViewTypes.Add(BufferType, ViewType);
		}
	}
};
FVoxelPinTypeStatics GVoxelPinTypeStatics;

VOXEL_RUN_ON_STARTUP(FVoxelPinTypeStatics_Initialize, Game, 999)
{
	GVoxelPinTypeStatics.Initialize();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType::FVoxelPinType(const FEdGraphPinType& PinType)
	: PropertyClass(PinType.PinCategory)
	, PropertyObject(PinType.PinSubCategoryObject.Get())
	, Tag(PinType.PinSubCategory)
{
	ensure(IsValid());
}

FVoxelPinType::FVoxelPinType(const FProperty& Property)
{
	PropertyClass = Property.GetClass()->GetFName();
	
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		PropertyObject = EnumProperty->GetEnum();
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		PropertyObject = StructProperty->Struct;
	}
	else if (const FSoftObjectProperty* ObjectProperty = CastField<FSoftObjectProperty>(Property))
	{
		PropertyObject = ObjectProperty->PropertyClass;
	}

	ensure(IsValid());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelPinType::IsValid() const
{
	if (IsWildcard())
	{
		return ensure(!PropertyObject);
	}
	else if (
		Is<bool>() ||
		Is<uint8>() ||
		Is<float>() ||
		Is<int32>() ||
		Is<FName>())
	{
		return ensure(!PropertyObject);
	}
	else if (IsEnum())
	{
		return ensure(Cast<UEnum>(PropertyObject));
	}
	else if (IsStruct())
	{
		return ensure(Cast<UScriptStruct>(PropertyObject));
	}
	else if (IsObject())
	{
		return ensure(Cast<UClass>(PropertyObject));
	}
	else
	{
		return false;
	}
}

FString FVoxelPinType::ToString() const
{
#if WITH_EDITOR
	if (!ensure(IsValid()))
	{
		return "INVALID";
	}

	FString Result = INLINE_LAMBDA -> FString
	{
		if (IsWildcard())
		{
			return "Wildcard";
		}
		else if (Is<bool>())
		{
			return "Boolean";
		}
		else if (Is<uint8>())
		{
			return "Byte";
		}
		else if (Is<float>())
		{
			return "Float";
		}
		else if (Is<int32>())
		{
			return "Integer";
		}
		else if (Is<FName>())
		{
			return "Name";
		}
		else if (IsEnum())
		{
			return GetEnum()->GetDisplayNameText().ToString();
		}
		else if (IsStruct())
		{
			return GetStruct()->GetDisplayNameText().ToString();
		}
		else if (IsObject())
		{
			return GetClass()->GetDisplayNameText().ToString();
		}
		else
		{
			ensure(false);
			return {};
		}
	};

	if (HasTag())
	{
		Result += " (" + Tag.ToString() + ")";
	}

	return Result;
#else
	return ToCppName();
#endif
}

FString FVoxelPinType::ToCppName() const
{
	if (!ensure(IsValid()))
	{
		return "INVALID";
	}

	if (IsWildcard())
	{
		return "wildcard";
	}
	else if (Is<bool>())
	{
		return "bool";
	}
	else if (Is<float>())
	{
		return "float";
	}
	else if (Is<int32>())
	{
		return "int32";
	}
	else if (Is<FName>())
	{
		return "FName";
	}
	else if (IsEnum())
	{
		return GetEnum()->GetName();
	}
	else if (IsStruct())
	{
		return GetStruct()->GetName();
	}
	else if (IsObject())
	{
		return GetClass()->GetName();
	}
	else
	{
		ensure(false);
		return {};
	}
}

int32 FVoxelPinType::GetTypeSize() const
{
	if (IsWildcard())
	{
		ensure(false);
		return 0;
	}
	else if (Is<bool>())
	{
		return sizeof(bool);
	}
	else if (Is<float>())
	{
		return sizeof(float);
	}
	else if (Is<int32>())
	{
		return sizeof(int32);
	}
	else if (Is<FName>())
	{
		return sizeof(FName);
	}
	else if (IsEnum())
	{
		return sizeof(int64);
	}
	else if (IsStruct())
	{
		return GetStruct()->GetStructureSize();
	}
	else if (IsObject())
	{
		ensure(false);
		return 0;
	}
	else
	{
		ensure(false);
		return 0;
	}
}

FEdGraphPinType FVoxelPinType::GetEdGraphPinType() const
{
	FEdGraphPinType PinType;
	PinType.PinCategory = PropertyClass;
	PinType.PinSubCategoryObject = PropertyObject;
	PinType.PinSubCategory = Tag;
	return PinType;
}

bool FVoxelPinType::IsDerivedFrom(const FVoxelPinType& Other) const
{
	if (*this == Other)
	{
		return true;
	}

	if (!ensure(IsValid()))
	{
		return false;
	}

	if (Other.Tag != Tag)
	{
		return false;
	}

	if (IsStruct())
	{
		if (!Other.IsStruct())
		{
			return false;
		}

		return GetStruct()->IsChildOf(Other.GetStruct());
	}
	else if (IsObject())
	{
		if (!Other.IsObject())
		{
			return false;
		}

		return GetClass()->IsChildOf(Other.GetClass());
	}
	else
	{
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelPinType::IsBuffer() const
{
	return WithoutTag().IsDerivedFrom<FVoxelBuffer>();
}

EPixelFormat FVoxelPinType::GetPixelFormat() const
{
	check(GVoxelPinTypeStatics.bInitialized);

	if (const EPixelFormat* Format = GVoxelPinTypeStatics.PixelFormats.Find(WithoutTag()))
	{
		return *Format;
	}

	return PF_Unknown;
}

FVoxelPinType FVoxelPinType::GetBufferType() const
{
	check(GVoxelPinTypeStatics.bInitialized);

	if (const FVoxelPinType* BufferType = GVoxelPinTypeStatics.InnerToBufferTypes.Find(WithoutTag()))
	{
		return BufferType->WithTag(GetTag());
	}

	return *this;
}

FVoxelPinType FVoxelPinType::GetInnerType() const
{
	check(GVoxelPinTypeStatics.bInitialized);

	if (const FVoxelPinType* InnerType = GVoxelPinTypeStatics.BufferToInnerTypes.Find(WithoutTag()))
	{
		return InnerType->WithTag(GetTag());
	}

	return *this;
}

FVoxelPinType FVoxelPinType::GetViewType() const
{
	check(GVoxelPinTypeStatics.bInitialized);
	return GVoxelPinTypeStatics.InnerAndBufferToViewTypes.FindChecked(WithoutTag()).WithTag(GetTag());
}

FVoxelPinType FVoxelPinType::GetExposedType() const
{
	check(GVoxelPinTypeStatics.bInitialized);

	const FVoxelPinType InnerType = GetInnerType();

	if (InnerType.Is<uint8>())
	{
		static TMap<FName, UEnum*> Enums;
		if (Enums.Num() == 0)
		{
			VOXEL_FUNCTION_COUNTER();

			for (UEnum* Enum : TObjectRange<UEnum>())
			{
				Enums.Add(Enum->GetFName(), Enum);
			}
		}

		if (UEnum* Enum = Enums.FindRef(InnerType.GetTag()))
		{
			return MakeEnum(Enum);
		}
	}

	if (const TVoxelInstancedStruct<FVoxelExposedPinType>& ExposedTypeInfo = InnerType.GetExposedTypeInfo())
	{
		return ExposedTypeInfo->GetExposedType();
	}
	return InnerType;
}

const TVoxelInstancedStruct<FVoxelExposedPinType>& FVoxelPinType::GetExposedTypeInfo() const
{
	check(IsInGameThread());

	static TMap<FVoxelPinType, const TVoxelInstancedStruct<FVoxelExposedPinType>> Map;

	if (const TVoxelInstancedStruct<FVoxelExposedPinType>* Struct = Map.Find(*this))
	{
		return *Struct;
	}

	UScriptStruct* ObjectStruct = nullptr;
	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelExposedPinType>())
	{
		TVoxelInstancedStruct<FVoxelExposedPinType> Instance(Struct);
		if (Instance->GetComputedType() == *this)
		{
			check(!ObjectStruct);
			ObjectStruct = Struct;
		}
	}

	if (ObjectStruct)
	{
		Map.Add(*this, ObjectStruct);
	}
	else
	{
		Map.Add(*this, nullptr);
	}

	return Map[*this];
}

FVoxelPinValue FVoxelPinType::MakeSafeDefault() const
{
	FVoxelPinValue Value(*this);
	if (IsBuffer())
	{
		Value.Get<FVoxelBuffer>().InitializeFromConstant(FVoxelSharedPinValue(GetInnerType()));
	}
	return Value;
}

const TArray<FVoxelPinType>& FVoxelPinType::GetAllBufferTypes()
{
	check(GVoxelPinTypeStatics.bInitialized);

	static TArray<FVoxelPinType> Types;
	if (Types.Num() == 0)
	{
		for (const auto& It : GVoxelPinTypeStatics.BufferToInnerTypes)
		{
			Types.Add(It.Key);
		}
	}
	return Types;
}