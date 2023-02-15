// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelSharedPinValue.h"

TSharedRef<FVoxelSharedPinValue::FOpaque> FVoxelSharedPinValue::MakeSharedStruct(UScriptStruct* Struct, const void* SourceStructMemory)
{
	check(Struct);
	
	void* StructMemory = FMemory::Malloc(FMath::Max(1, Struct->GetStructureSize()));;
	Struct->InitializeStruct(StructMemory);

	if (SourceStructMemory)
	{
		Struct->CopyScriptStruct(StructMemory, SourceStructMemory);
	}

	return TSharedPtr<FOpaque>(static_cast<FOpaque*>(StructMemory), [Struct, StructMemory](const void* InStructMemory)
	{
		if (GExitPurge)
		{
			return;
		}

		check(InStructMemory == StructMemory);

		Struct->DestroyStruct(StructMemory);
		FMemory::Free(StructMemory);
	}).ToSharedRef();
}

void FVoxelSharedPinValue::AllocateStruct(UScriptStruct* Struct, const void* SourceStructMemory)
{
	SharedStructType = Struct;
	SharedStruct = MakeSharedStruct(Struct, SourceStructMemory);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSharedPinValue::FVoxelSharedPinValue(const FVoxelPinType& Type)
	: Type(Type)
{
	ensure(!Type.IsWildcard());
	ensure(!Type.IsObject());

	if (Type.IsStruct())
	{
		AllocateStruct(Type.GetStruct(), nullptr);
	}
}

FVoxelSharedPinValue::FVoxelSharedPinValue(const FVoxelPinValue& Value)
	: Type(Value.GetType())
{
	ensure(!Type.IsObject());

	if (Type.Is<bool>())
	{
		bBool = Value.Get<bool>();
	}
	else if (Type.Is<uint8>())
	{
		Byte = Value.Get<uint8>();
	}
	else if (Type.Is<float>())
	{
		Float = Value.Get<float>();
	}
	else if (Type.Is<int32>())
	{
		Int32 = Value.Get<int32>();
	}
	else if (Type.Is<FName>())
	{
		Name = Value.Get<FName>();
	}
	else if (Type.IsEnum())
	{
		Enum = Value.GetEnum();
	}
	else if (Type.IsStruct())
	{
		AllocateStruct(Value.GetStruct().GetScriptStruct(), Value.GetStruct().GetStructMemory());
	}
	else
	{
		ensure(false);
	}
}

FVoxelSharedPinValue::FVoxelSharedPinValue(const FVoxelPinType& Type, TConstVoxelArrayView<uint8> View)
	: Type(Type)
{
	ensure(!Type.IsWildcard());
	ensure(!Type.IsObject());

	if (Type.Is<bool>())
	{
		if (!ensure(View.Num() == sizeof(bool)))
		{
			return;
		}
		bBool = ReinterpretCastVoxelArrayView<bool>(View)[0];
	}
	else if (Type.Is<uint8>())
	{
		if (!ensure(View.Num() == sizeof(uint8)))
		{
			return;
		}
		Byte = ReinterpretCastVoxelArrayView<uint8>(View)[0];
	}
	else if (Type.Is<float>())
	{
		if (!ensure(View.Num() == sizeof(float)))
		{
			return;
		}
		Float = ReinterpretCastVoxelArrayView<float>(View)[0];
	}
	else if (Type.Is<int32>())
	{
		if (!ensure(View.Num() == sizeof(int32)))
		{
			return;
		}
		Int32 = ReinterpretCastVoxelArrayView<int32>(View)[0];
	}
	else if (Type.Is<FName>())
	{
		if (!ensure(View.Num() == sizeof(FName)))
		{
			return;
		}
		Name = ReinterpretCastVoxelArrayView<FName>(View)[0];
	}
	else if (Type.IsEnum())
	{
		if (!ensure(View.Num() == sizeof(int64)))
		{
			return;
		}
		Enum = ReinterpretCastVoxelArrayView<int64>(View)[0];
	}
	else if (Type.IsStruct())
	{
		if (!ensure(View.Num() == Type.GetStruct()->GetStructureSize()))
		{
			AllocateStruct(Type.GetStruct(), nullptr);
			return;
		}

		AllocateStruct(Type.GetStruct(), View.GetData());
	}
	else
	{
		ensure(false);
	}
}

int64 FVoxelSharedPinValue::GetAllocatedSize() const
{
	int64 AllocatedSize = sizeof(FVoxelSharedPinValue);
	if (Type.IsStruct())
	{
		if (IsDerivedFrom<FVoxelVirtualStruct>())
		{
			AllocatedSize += Get<FVoxelVirtualStruct>().GetAllocatedSize();
		}
		else
		{
			AllocatedSize += Type.GetStruct()->GetStructureSize();
		}
	}
	return AllocatedSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TConstVoxelArrayView<uint8> FVoxelSharedPinValue::MakeByteView() const
{
	if (Type.Is<bool>())
	{
		return ReinterpretCastVoxelArrayView<uint8>(MakeVoxelArrayView(&bBool, 1));
	}
	else if (Type.Is<uint8>())
	{
		return ReinterpretCastVoxelArrayView<uint8>(MakeVoxelArrayView(&Byte, 1));
	}
	else if (Type.Is<float>())
	{
		return ReinterpretCastVoxelArrayView<uint8>(MakeVoxelArrayView(&Float, 1));
	}
	else if (Type.Is<int32>())
	{
		return ReinterpretCastVoxelArrayView<uint8>(MakeVoxelArrayView(&Int32, 1));
	}
	else if (Type.Is<FName>())
	{
		return ReinterpretCastVoxelArrayView<uint8>(MakeVoxelArrayView(&Name, 1));
	}
	else if (Type.IsEnum())
	{
		return ReinterpretCastVoxelArrayView<uint8>(MakeVoxelArrayView(&Enum, 1));
	}
	else if (Type.IsStruct())
	{
		if (!ensure(SharedStruct) ||
			!ensure(SharedStructType))
		{
			return {};
		}

		return MakeVoxelArrayView(reinterpret_cast<const uint8*>(SharedStruct.Get()), SharedStructType->GetStructureSize());
	}
	else
	{
		ensure(false);
		return {};
	}
}

FVoxelPinValue FVoxelSharedPinValue::MakeValue() const
{
	if (Type.Is<bool>())
	{
		return FVoxelPinValue::Make(Get<bool>());
	}
	else if (Type.Is<float>())
	{
		return FVoxelPinValue::Make(Get<float>());
	}
	else if (Type.Is<int32>())
	{
		return FVoxelPinValue::Make(Get<int32>());
	}
	else if (Type.Is<FName>())
	{
		return FVoxelPinValue::Make(Get<FName>());
	}
	else if (Type.IsEnum())
	{
		FVoxelPinValue Value(GetType());
		Value.GetEnum() = Enum;
		return Value;
	}
	else if (Type.IsStruct())
	{
		FVoxelPinValue Value(GetType());
		Value.GetStruct().InitializeAs(SharedStructType, SharedStruct.Get());
		return Value;
	}
	else
	{
		ensure(false);
		return {};
	}
}

uint64 FVoxelSharedPinValue::GetHash() const
{
	if (!IsValid())
	{
		return 0;
	}

	if (Type.Is<bool>())
	{
		return FVoxelUtilities::MurmurHash64(Get<bool>());
	}
	else if (Type.Is<float>())
	{
		return FVoxelUtilities::MurmurHash64(Get<float>());
	}
	else if (Type.Is<int32>())
	{
		return FVoxelUtilities::MurmurHash64(Get<int32>());
	}
	else if (Type.Is<FName>())
	{
		return FVoxelUtilities::MurmurHash64(GetTypeHash(Get<FName>().GetComparisonIndex()));
	}
	else if (Type.IsEnum())
	{
		return FVoxelUtilities::MurmurHash64(Enum);
	}
	else if (Type.IsStruct())
	{
		if (!ensure(SharedStruct) ||
			!ensure(SharedStructType))
		{
			return {};
		}

		if (SharedStructType->IsChildOf(FVoxelCustomHash::StaticStruct()))
		{
			return Get<FVoxelCustomHash>().GetHash();
		}

		ensure(!SharedStructType->GetCppStructOps()->HasDestructor());
		return FVoxelUtilities::MurmurHash(MakeVoxelArrayView(reinterpret_cast<const uint8*>(SharedStruct.Get()), SharedStructType->GetStructureSize()));
	}
	else
	{
		ensure(false);
		return {};
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelSharedPinValue::operator==(const FVoxelSharedPinValue& Other) const
{
	if (Type != Other.Type)
	{
		return false;
	}

	if (!Type.IsValid())
	{
		return true;
	}
	else if (Type.Is<bool>())
	{
		return Get<bool>() == Other.Get<bool>();
	}
	else if (Type.Is<float>())
	{
		return Get<float>() == Other.Get<float>();
	}
	else if (Type.Is<int32>())
	{
		return Get<int32>() == Other.Get<int32>();
	}
	else if (Type.Is<FName>())
	{
		return Get<FName>() == Other.Get<FName>();
	}
	else if (Type.IsEnum())
	{
		return Enum == Other.Enum;
	}
	else if (Type.IsStruct())
	{
		if (SharedStructType != Other.SharedStructType)
		{
			return false;
		}

		if (!SharedStructType ||
			SharedStruct == Other.SharedStruct)
		{
			return true;
		}

		if (!ensure(SharedStruct) ||
			!ensure(Other.SharedStruct))
		{
			return false;
		}

		VOXEL_SCOPE_COUNTER("CompareScriptStruct");
		return SharedStructType->CompareScriptStruct(SharedStruct.Get(), Other.SharedStruct.Get(), PPF_None);
	}
	else
	{
		ensure(false);
		return false;
	}
}