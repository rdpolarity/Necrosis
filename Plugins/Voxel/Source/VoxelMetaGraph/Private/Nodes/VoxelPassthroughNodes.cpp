// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelPassthroughNodes.h"

FString FVoxelNode_Passthrough::GenerateCode(bool bIsGpu) const
{
	TArray<FString> InputPins;
	TArray<FString> OutputPins;
	for (const FVoxelPin& Pin : GetPins())
	{
		const FVoxelPinType Type = Pin.GetType().GetInnerType();

		FString Name = Pin.Name.ToString();
		if (!Pin.bIsInput)
		{
			Name.RemoveFromStart(TEXT("Out"));
		}
		Name = "{" + Name + "}";

		TArray<FString>& Pins = Pin.bIsInput ? InputPins : OutputPins;

		if (Type.Is<float>())
		{
			Pins.Add(Name);
		}
		else if (Type.Is<int32>())
		{
			Pins.Add(Name);
		}
		else if (Type.Is<FVector2D>() || Type.Is<FIntPoint>())
		{
			Pins.Add(Name + ".x");
			Pins.Add(Name + ".y");
		}
		else if (Type.Is<FVector>() || Type.Is<FIntVector>())
		{
			Pins.Add(Name + ".x");
			Pins.Add(Name + ".y");
			Pins.Add(Name + ".z");
		}
		else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
		{
			Pins.Add(Name + ".x");
			Pins.Add(Name + ".y");
			Pins.Add(Name + ".z");
			Pins.Add(Name + ".w");
		}
		else
		{
			check(false);
		}
	}

	if (InputPins.Num() == 1)
	{
		// Scalar to vector

		FString Result;
		for (int32 Index = 0; Index < OutputPins.Num(); Index++)
		{
			Result += OutputPins[Index] + " = " + InputPins[0] + ";";
		}
		return Result;
	}
	else
	{
		check(InputPins.Num() >= OutputPins.Num());

		FString Result;
		for (int32 Index = 0; Index < OutputPins.Num(); Index++)
		{
			Result += OutputPins[Index] + " = " + InputPins[Index] + ";";
		}
		return Result;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FloatToDensity, ReturnValue)
{
	return Get(ValuePin, Query);
}

DEFINE_VOXEL_NODE(FVoxelNode_DensityToFloat, ReturnValue)
{
	return Get(ValuePin, Query);
}

DEFINE_VOXEL_NODE(FVoxelNode_IntToSeed, ReturnValue)
{
	return Get(ValuePin, Query);
}

DEFINE_VOXEL_NODE(FVoxelNode_SeedToInt, ReturnValue)
{
	return Get(ValuePin, Query);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeVector2D, ReturnValue)
{
	const FVoxelFutureValue X = Get(XPin, Query);
	const FVoxelFutureValue Y = Get(YPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, X, Y)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVector2DBuffer ReturnValue;
			ReturnValue.X = X.Get<FVoxelFloatBuffer>();
			ReturnValue.Y = Y.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector2D ReturnValue;
			ReturnValue.X = X.Get<float>();
			ReturnValue.Y = Y.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeVector, ReturnValue)
{
	const FVoxelFutureValue X = Get(XPin, Query);
	const FVoxelFutureValue Y = Get(YPin, Query);
	const FVoxelFutureValue Z = Get(ZPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, X, Y, Z)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVectorBuffer ReturnValue;
			ReturnValue.X = X.Get<FVoxelFloatBuffer>();
			ReturnValue.Y = Y.Get<FVoxelFloatBuffer>();
			ReturnValue.Z = Z.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y, ReturnValue.Z);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector ReturnValue;
			ReturnValue.X = X.Get<float>();
			ReturnValue.Y = Y.Get<float>();
			ReturnValue.Z = Z.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeQuaternion, ReturnValue)
{
	const FVoxelFutureValue X = Get(XPin, Query);
	const FVoxelFutureValue Y = Get(YPin, Query);
	const FVoxelFutureValue Z = Get(ZPin, Query);
	const FVoxelFutureValue W = Get(WPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, X, Y, Z, W)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelQuaternionBuffer ReturnValue;
			ReturnValue.X = X.Get<FVoxelFloatBuffer>();
			ReturnValue.Y = Y.Get<FVoxelFloatBuffer>();
			ReturnValue.Z = Z.Get<FVoxelFloatBuffer>();
			ReturnValue.W = W.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y, ReturnValue.Z, ReturnValue.W);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FQuat ReturnValue;
			ReturnValue.X = X.Get<float>();
			ReturnValue.Y = Y.Get<float>();
			ReturnValue.Z = Z.Get<float>();
			ReturnValue.W = W.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeLinearColor, ReturnValue)
{
	const FVoxelFutureValue R = Get(RPin, Query);
	const FVoxelFutureValue G = Get(GPin, Query);
	const FVoxelFutureValue B = Get(BPin, Query);
	const FVoxelFutureValue A = Get(APin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, R, G, B, A)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelLinearColorBuffer ReturnValue;
			ReturnValue.R = R.Get<FVoxelFloatBuffer>();
			ReturnValue.G = G.Get<FVoxelFloatBuffer>();
			ReturnValue.B = B.Get<FVoxelFloatBuffer>();
			ReturnValue.A = A.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.R, ReturnValue.G, ReturnValue.B, ReturnValue.A);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FLinearColor ReturnValue;
			ReturnValue.R = R.Get<float>();
			ReturnValue.G = G.Get<float>();
			ReturnValue.B = B.Get<float>();
			ReturnValue.A = A.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeIntPoint, ReturnValue)
{
	const FVoxelFutureValue X = Get(XPin, Query);
	const FVoxelFutureValue Y = Get(YPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, X, Y)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelIntPointBuffer ReturnValue;
			ReturnValue.X = X.Get<FVoxelInt32Buffer>();
			ReturnValue.Y = Y.Get<FVoxelInt32Buffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FIntPoint ReturnValue;
			ReturnValue.X = X.Get<int32>();
			ReturnValue.Y = Y.Get<int32>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeIntVector, ReturnValue)
{
	const FVoxelFutureValue X = Get(XPin, Query);
	const FVoxelFutureValue Y = Get(YPin, Query);
	const FVoxelFutureValue Z = Get(ZPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, X, Y, Z)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelIntVectorBuffer ReturnValue;
			ReturnValue.X = X.Get<FVoxelInt32Buffer>();
			ReturnValue.Y = Y.Get<FVoxelInt32Buffer>();
			ReturnValue.Z = Z.Get<FVoxelInt32Buffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y, ReturnValue.Z);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FIntVector ReturnValue;
			ReturnValue.X = X.Get<int32>();
			ReturnValue.Y = Y.Get<int32>();
			ReturnValue.Z = Z.Get<int32>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BreakVector2D, X)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelVector2DBuffer>().X);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Vector.Get<FVector2D>().X));
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakVector2D, Y)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelVector2DBuffer>().Y);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Vector.Get<FVector2D>().Y));
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BreakVector, X)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelVectorBuffer>().X);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Vector.Get<FVector>().X));
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakVector, Y)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelVectorBuffer>().Y);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Vector.Get<FVector>().Y));
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakVector, Z)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelVectorBuffer>().Z);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Vector.Get<FVector>().Z));
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BreakQuaternion, X)
{
	const FVoxelFutureValue Quaternion = Get(QuaternionPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Quaternion)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Quaternion.Get<FVoxelQuaternionBuffer>().X);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Quaternion.Get<FQuat>().X));
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakQuaternion, Y)
{
	const FVoxelFutureValue Quaternion = Get(QuaternionPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Quaternion)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Quaternion.Get<FVoxelQuaternionBuffer>().Y);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Quaternion.Get<FQuat>().Y));
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakQuaternion, Z)
{
	const FVoxelFutureValue Quaternion = Get(QuaternionPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Quaternion)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Quaternion.Get<FVoxelQuaternionBuffer>().Z);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Quaternion.Get<FQuat>().Z));
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakQuaternion, W)
{
	const FVoxelFutureValue Quaternion = Get(QuaternionPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Quaternion)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Quaternion.Get<FVoxelQuaternionBuffer>().W);
		}
		else
		{
			return FVoxelSharedPinValue::Make(float(Quaternion.Get<FQuat>().W));
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BreakLinearColor, R)
{
	const FVoxelFutureValue Color = Get(ColorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Color)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Color.Get<FVoxelLinearColorBuffer>().R);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Color.Get<FLinearColor>().R);
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakLinearColor, G)
{
	const FVoxelFutureValue Color = Get(ColorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Color)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Color.Get<FVoxelLinearColorBuffer>().G);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Color.Get<FLinearColor>().G);
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakLinearColor, B)
{
	const FVoxelFutureValue Color = Get(ColorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Color)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Color.Get<FVoxelLinearColorBuffer>().B);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Color.Get<FLinearColor>().B);
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakLinearColor, A)
{
	const FVoxelFutureValue Color = Get(ColorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Color)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Color.Get<FVoxelLinearColorBuffer>().A);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Color.Get<FLinearColor>().A);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BreakIntPoint, X)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelIntPointBuffer>().X);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FIntPoint>().X);
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakIntPoint, Y)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelIntPointBuffer>().Y);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FIntPoint>().Y);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BreakIntVector, X)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelIntVectorBuffer>().X);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FIntVector>().X);
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakIntVector, Y)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelIntVectorBuffer>().Y);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FIntVector>().Y);
		}
	};
}
DEFINE_VOXEL_NODE(FVoxelNode_BreakIntVector, Z)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FVoxelIntVectorBuffer>().Z);
		}
		else
		{
			return FVoxelSharedPinValue::Make(Vector.Get<FIntVector>().Z);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_Vector2DToVector, ReturnValue)
{
	const FVoxelFutureValue Vector2D = Get(Vector2DPin, Query);
	const FVoxelFutureValue Z = Get(ZPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector2D, Z)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVectorBuffer ReturnValue;
			ReturnValue.X = Vector2D.Get<FVoxelVector2DBuffer>().X;
			ReturnValue.Y = Vector2D.Get<FVoxelVector2DBuffer>().Y;
			ReturnValue.Z = Z.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y, ReturnValue.Z);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector ReturnValue;
			ReturnValue.X = Vector2D.Get<FVector2D>().X;
			ReturnValue.Y = Vector2D.Get<FVector2D>().Y;
			ReturnValue.Z = Z.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_Vector2DToColor, ReturnValue)
{
	const FVoxelFutureValue Vector2D = Get(Vector2DPin, Query);
	const FVoxelFutureValue B = Get(BPin, Query);
	const FVoxelFutureValue A = Get(APin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector2D, B, A)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelLinearColorBuffer ReturnValue;
			ReturnValue.R = Vector2D.Get<FVoxelVector2DBuffer>().X;
			ReturnValue.G = Vector2D.Get<FVoxelVector2DBuffer>().Y;
			ReturnValue.B = B.Get<FVoxelFloatBuffer>();
			ReturnValue.A = A.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.R, ReturnValue.G, ReturnValue.B, ReturnValue.A);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FLinearColor ReturnValue;
			ReturnValue.R = Vector2D.Get<FVector2D>().X;
			ReturnValue.G = Vector2D.Get<FVector2D>().Y;
			ReturnValue.B = B.Get<float>();
			ReturnValue.A = A.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_VectorToVector2D, ReturnValue)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVector2DBuffer ReturnValue;
			ReturnValue.X = Vector.Get<FVoxelVectorBuffer>().X;
			ReturnValue.Y = Vector.Get<FVoxelVectorBuffer>().Y;
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector2D ReturnValue;
			ReturnValue.X = Vector.Get<FVector>().X;
			ReturnValue.Y = Vector.Get<FVector>().Y;
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_VectorToColor, ReturnValue)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);
	const FVoxelFutureValue A = Get(APin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector, A)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelLinearColorBuffer ReturnValue;
			ReturnValue.R = Vector.Get<FVoxelVectorBuffer>().X;
			ReturnValue.G = Vector.Get<FVoxelVectorBuffer>().Y;
			ReturnValue.B = Vector.Get<FVoxelVectorBuffer>().Z;
			ReturnValue.A = A.Get<FVoxelFloatBuffer>();
			CheckVoxelBuffersNum(ReturnValue.R, ReturnValue.G, ReturnValue.B, ReturnValue.A);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FLinearColor ReturnValue;
			ReturnValue.R = Vector.Get<FVector>().X;
			ReturnValue.G = Vector.Get<FVector>().Y;
			ReturnValue.B = Vector.Get<FVector>().Z;
			ReturnValue.A = A.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_ColorToVector2D, ReturnValue)
{
	const FVoxelFutureValue Color = Get(ColorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Color)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVector2DBuffer ReturnValue;
			ReturnValue.X = Color.Get<FVoxelLinearColorBuffer>().R;
			ReturnValue.Y = Color.Get<FVoxelLinearColorBuffer>().G;
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector2D ReturnValue;
			ReturnValue.X = Color.Get<FLinearColor>().R;
			ReturnValue.Y = Color.Get<FLinearColor>().G;
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_ColorToVector, ReturnValue)
{
	const FVoxelFutureValue Color = Get(ColorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Color)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVectorBuffer ReturnValue;
			ReturnValue.X = Color.Get<FVoxelLinearColorBuffer>().R;
			ReturnValue.Y = Color.Get<FVoxelLinearColorBuffer>().G;
			ReturnValue.Z = Color.Get<FVoxelLinearColorBuffer>().B;
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y, ReturnValue.Z);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector ReturnValue;
			ReturnValue.X = Color.Get<FLinearColor>().R;
			ReturnValue.Y = Color.Get<FLinearColor>().G;
			ReturnValue.Z = Color.Get<FLinearColor>().B;
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_IntPointToIntVector, ReturnValue)
{
	const FVoxelFutureValue Vector2D = Get(Vector2DPin, Query);
	const FVoxelFutureValue Z = Get(ZPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector2D, Z)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelIntVectorBuffer ReturnValue;
			ReturnValue.X = Vector2D.Get<FVoxelIntPointBuffer>().X;
			ReturnValue.Y = Vector2D.Get<FVoxelIntPointBuffer>().Y;
			ReturnValue.Z = Z.Get<FVoxelInt32Buffer>();
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y, ReturnValue.Z);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FIntVector ReturnValue;
			ReturnValue.X = Vector2D.Get<FIntPoint>().X;
			ReturnValue.Y = Vector2D.Get<FIntPoint>().Y;
			ReturnValue.Z = Z.Get<int32>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_IntVectorToIntPoint, ReturnValue)
{
	const FVoxelFutureValue Vector = Get(VectorPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Vector)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelIntPointBuffer ReturnValue;
			ReturnValue.X = Vector.Get<FVoxelIntVectorBuffer>().X;
			ReturnValue.Y = Vector.Get<FVoxelIntVectorBuffer>().Y;
			CheckVoxelBuffersNum(ReturnValue.X, ReturnValue.Y);

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FIntPoint ReturnValue;
			ReturnValue.X = Vector.Get<FIntVector>().X;
			ReturnValue.Y = Vector.Get<FIntVector>().Y;
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FloatToVector, ReturnValue)
{
	const FVoxelFutureValue Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Value)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVectorBuffer ReturnValue;
			ReturnValue.X = Value.Get<FVoxelFloatBuffer>();
			ReturnValue.Y = Value.Get<FVoxelFloatBuffer>();
			ReturnValue.Z = Value.Get<FVoxelFloatBuffer>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{

			FVector ReturnValue;
			ReturnValue.X = Value.Get<float>();
			ReturnValue.Y = Value.Get<float>();
			ReturnValue.Z = Value.Get<float>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FloatToVector2D, ReturnValue)
{
	const FVoxelFutureValue Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Value)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelVector2DBuffer ReturnValue;
			ReturnValue.X = Value.Get<FVoxelFloatBuffer>();
			ReturnValue.Y = Value.Get<FVoxelFloatBuffer>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FVector2D ReturnValue;
			ReturnValue.X = Value.Get<float>();
			ReturnValue.Y = Value.Get<float>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FloatToColor, ReturnValue)
{
	const FVoxelFutureValue Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Value)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelLinearColorBuffer ReturnValue;
			ReturnValue.R = Value.Get<FVoxelFloatBuffer>();
			ReturnValue.G = Value.Get<FVoxelFloatBuffer>();
			ReturnValue.B = Value.Get<FVoxelFloatBuffer>();
			ReturnValue.A = Value.Get<FVoxelFloatBuffer>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FLinearColor ReturnValue;
			ReturnValue.R = Value.Get<float>();
			ReturnValue.G = Value.Get<float>();
			ReturnValue.B = Value.Get<float>();
			ReturnValue.A = Value.Get<float>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_Int32ToIntPoint, ReturnValue)
{
	const FVoxelFutureValue Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Value)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelIntPointBuffer ReturnValue;
			ReturnValue.X = Value.Get<FVoxelInt32Buffer>();
			ReturnValue.Y = Value.Get<FVoxelInt32Buffer>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FIntPoint ReturnValue;
			ReturnValue.X = Value.Get<int32>();
			ReturnValue.Y = Value.Get<int32>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_Int32ToIntVector, ReturnValue)
{
	const FVoxelFutureValue Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Value)
	{
		if (AreMathPinsBuffers())
		{
			FVoxelIntVectorBuffer ReturnValue;
			ReturnValue.X = Value.Get<FVoxelInt32Buffer>();
			ReturnValue.Y = Value.Get<FVoxelInt32Buffer>();
			ReturnValue.Z = Value.Get<FVoxelInt32Buffer>();

			return FVoxelSharedPinValue::Make(ReturnValue);
		}
		else
		{
			FIntVector ReturnValue;
			ReturnValue.X = Value.Get<int32>();
			ReturnValue.Y = Value.Get<int32>();
			ReturnValue.Z = Value.Get<int32>();
			return FVoxelSharedPinValue::Make(ReturnValue);
		}
	};
}