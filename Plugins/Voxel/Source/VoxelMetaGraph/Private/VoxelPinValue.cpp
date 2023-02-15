// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelPinValue.h"
#include "VoxelPinType.h"
#include "Misc/DefaultValueHelper.h"

FVoxelPinValue::FVoxelPinValue(const FVoxelPinType& Type)
	: Type(Type)
{
	ensure(!Type.IsWildcard());

	if (Type.IsStruct())
	{
		Struct = FVoxelInstancedStruct(Type.GetStruct());
	}
}

FVoxelPinValue FVoxelPinValue::MakeFromPinDefaultValue(const UEdGraphPin& Pin)
{
	const FVoxelPinType Type = FVoxelPinType(Pin.PinType).GetExposedType();

	FVoxelPinValue Result(Type);
	if (Pin.DefaultObject)
	{
		ensure(Pin.DefaultValue.IsEmpty());

		if (ensure(Result.Type.IsObject()))
		{
			Result.GetObject() = TSoftObjectPtr<UObject>(FSoftObjectPath(Pin.DefaultObject));
		}
	}
	else if (!Pin.DefaultValue.IsEmpty())
	{
		ensure(!Type.IsObject());
		ensure(Result.ImportFromString(Pin.DefaultValue));
	}
	return Result;
}

void FVoxelPinValue::ApplyToPinDefaultValue(UEdGraphPin& Pin) const
{
	if (Type.IsObject())
	{
		Pin.DefaultValue.Reset();
		Pin.DefaultObject = GetObject().LoadSynchronous();
	}
	else
	{
		Pin.DefaultValue = ExportToString();
		Pin.DefaultObject = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelPinValue::ExportToString() const
{
	if (!ensure(IsValid()))
	{
		return {};
	}

	if (Type.Is<bool>())
	{
		return LexToString(Get<bool>());
	}
	else if (Type.Is<float>())
	{
		return LexToString(Get<float>());
	}
	else if (Type.Is<int32>())
	{
		return LexToString(Get<int32>());
	}
	else if (Type.Is<FName>())
	{
		return Get<FName>().ToString();
	}
	else if (Type.IsEnum())
	{
		return Type.GetEnum()->GetNameStringByValue(GetEnum());
	}
	else if (Type.IsStruct())
	{
		if (Type.Is<FVector>())
		{
			const FVector& Vector = Struct.Get<FVector>();
			return FString::Printf(TEXT("%f,%f,%f"), Vector.X, Vector.Y, Vector.Z);
		}
		else if (Type.Is<FRotator>())
		{
			const FRotator& Rotator = Struct.Get<FRotator>();
			return FString::Printf(TEXT("P=%f,Y=%f,R=%f"), Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
		}
		else if (Type.Is<FColor>())
		{
			const FColor& Color = Struct.Get<FColor>();
			return FString::Printf(TEXT("%d,%d,%d,%d"), Color.R, Color.G, Color.B, Color.A);
		}
		else
		{
			return FVoxelObjectUtilities::PropertyToText_Direct(
				*FVoxelObjectUtilities::MakeStructProperty(Type.GetStruct()),
				Struct.GetStructMemory(),
				nullptr);
		}
	}
	else if (Type.IsObject())
	{
		return GetObject().ToString();
	}
	else
	{
		ensure(false);
		return {};
	}
}

bool FVoxelPinValue::ImportFromString(const FString& Value)
{
	if (Type.Is<bool>())
	{
		bBool = FCString::ToBool(*Value);
		return true;
	}
	else if (Type.Is<uint8>())
	{
		const int32 ByteValue = FCString::Atoi(*Value);
		ensure(0 <= ByteValue && ByteValue < 256);

		Byte = ByteValue;
		return true;
	}
	else if (Type.Is<float>())
	{
		Float = FCString::Atof(*Value);
		return true;
	}
	else if (Type.Is<int32>())
	{
		Int32 = FCString::Atoi(*Value);
		return true;
	}
	else if (Type.Is<FName>())
	{
		Name = *Value;
		return true;
	}
	else if (Type.IsEnum())
	{
		Enum = Type.GetEnum()->GetValueByNameString(Value);
		return Enum != -1;
	}
	else if (Type.IsStruct())
	{
		Struct = FVoxelInstancedStruct(Type.GetStruct());

		if (Value.IsEmpty())
		{
			return true;
		}

		if (Type.Is<FVector>())
		{
			return FDefaultValueHelper::ParseVector(Value, Struct.Get<FVector>());
		}
		else if (Type.Is<FVector2D>())
		{
			return FDefaultValueHelper::ParseVector2D(Value, Struct.Get<FVector2D>());
		}
		else if (Type.Is<FRotator>())
		{
			return FDefaultValueHelper::ParseRotator(Value, Struct.Get<FRotator>());
		}
		else if (Type.Is<FColor>())
		{
			return FDefaultValueHelper::ParseColor(Value, Struct.Get<FColor>());
		}
		else
		{
			return FVoxelObjectUtilities::PropertyFromText_Direct(
				*FVoxelObjectUtilities::MakeStructProperty(Type.GetStruct()),
				Value,
				Struct.GetStructMemory(),
				nullptr);
		}
	}
	else if (Type.IsObject())
	{
		Object = TSoftObjectPtr<UObject>(Value);

		if (Object.LoadSynchronous() &&
			!Object.LoadSynchronous()->IsA(Type.GetClass()))
		{
			Object = nullptr;
			return false;
		}

		return true;
	}
	else
	{
		ensure(false);
		return false;
	}
}

bool FVoxelPinValue::ImportFromUnrelated(FVoxelPinValue Other)
{
	if (GetType() == Other.GetType())
	{
		*this = Other;
		return true;
	}

	if (Other.Is<FColor>())
	{
		Other = Make<FLinearColor>(FLinearColor(
			Other.Get<FColor>().R,
			Other.Get<FColor>().G,
			Other.Get<FColor>().B,
			Other.Get<FColor>().A));
	}
	if (Other.Is<FRotator>())
	{
		Other = Make<FVector>(FVector(
			Other.Get<FRotator>().Pitch,
			Other.Get<FRotator>().Yaw,
			Other.Get<FRotator>().Roll));
	}
	if (Other.Is<int32>())
	{
		Other = Make<float>(Other.Get<int32>());
	}
	if (Other.Is<FIntPoint>())
	{
		Other = Make<FVector2D>(FVector2D(
			Other.Get<FIntPoint>().X,
			Other.Get<FIntPoint>().Y));
	}
	if (Other.Is<FIntVector>())
	{
		Other = Make<FVector>(FVector(
			Other.Get<FIntVector>().X,
			Other.Get<FIntVector>().Y,
			Other.Get<FIntVector>().Z));
	}

#define CHECK(NewType, OldType, ...) \
	if (Is<NewType>() && Other.Is<OldType>()) \
	{ \
		const OldType Value = Other.Get<OldType>(); \
		Get<NewType>() = __VA_ARGS__; \
		return true; \
	}

	CHECK(float, FVector2D, Value.X);
	CHECK(float, FVector, Value.X);
	CHECK(float, FLinearColor, Value.R);

	CHECK(FVector2D, float, FVector2D(Value));
	CHECK(FVector2D, FVector, FVector2D(Value));
	CHECK(FVector2D, FLinearColor, FVector2D(Value));

	CHECK(FVector, float, FVector(Value));
	CHECK(FVector, FVector2D, FVector(Value, 0.f));
	CHECK(FVector, FLinearColor, FVector(Value));

	CHECK(FLinearColor, float, FLinearColor(Value, Value, Value, Value));
	CHECK(FLinearColor, FVector2D, FLinearColor(Value.X, Value.Y, 0.f));
	CHECK(FLinearColor, FVector, FLinearColor(Value));

	CHECK(int32, float, Value);
	CHECK(int32, FVector2D, Value.X);
	CHECK(int32, FVector, Value.X);
	CHECK(int32, FLinearColor, Value.R);

	CHECK(FIntPoint, float, FIntPoint(Value));
	CHECK(FIntPoint, FVector2D, FIntPoint(Value.X, Value.Y));
	CHECK(FIntPoint, FVector, FIntPoint(Value.X, Value.Y));
	CHECK(FIntPoint, FLinearColor, FIntPoint(Value.R, Value.G));

	CHECK(FIntVector, float, FIntVector(Value));
	CHECK(FIntVector, FVector2D, FIntVector(Value.X, Value.Y, 0));
	CHECK(FIntVector, FVector, FIntVector(Value.X, Value.Y, Value.Z));
	CHECK(FIntVector, FLinearColor, FIntVector(Value.R, Value.G, Value.B));

#undef CHECK

	return ImportFromString(Other.ExportToString());
}

FVoxelPinValue FVoxelPinValue::WithoutTag() const
{
	FVoxelPinValue Result = *this;
	Result.Type = Type.WithoutTag();
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelPinValue::operator==(const FVoxelPinValue& Other) const
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
		return GetEnum() == Other.GetEnum();
	}
	else if (Type.IsStruct())
	{
		return GetStruct() == Other.GetStruct();
	}
	else if (Type.IsObject())
	{
		return GetObject() == Other.GetObject();
	}
	else
	{
		ensure(false);
		return false;
	}
}