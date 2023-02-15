// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

#define __ISPC_STRUCT_float2__
#define __ISPC_STRUCT_float3__
#define __ISPC_STRUCT_float4__

#define __ISPC_STRUCT_int2__
#define __ISPC_STRUCT_int3__
#define __ISPC_STRUCT_int4__

#define __ISPC_STRUCT_float4x4__

namespace ispc
{
	struct float2
	{
		float X;
		float Y;
	};
	struct float3
	{
		float X;
		float Y;
		float Z;
	};
	struct float4
	{
		float X;
		float Y;
		float Z;
		float W;
	};

	struct int2
	{
		int32 X;
		int32 Y;
	};
	struct int3
	{
		int32 X;
		int32 Y;
		int32 Z;
	};
	struct int4
	{
		int32 X;
		int32 Y;
		int32 Z;
		int32 W;
	};

	struct float4x4
	{
		float M[16];
	};
}

FORCEINLINE ispc::float2 GetISPCValue(const FVector2f& Vector)
{
	return ReinterpretCastRef<ispc::float2>(Vector);
}
FORCEINLINE ispc::float3 GetISPCValue(const FVector3f& Vector)
{
	return ReinterpretCastRef<ispc::float3>(Vector);
}
FORCEINLINE ispc::float4 GetISPCValue(const FVector4f& Vector)
{
	return ReinterpretCastRef<ispc::float4>(Vector);
}

FORCEINLINE ispc::int2 GetISPCValue(const FIntPoint& Vector)
{
	return ReinterpretCastRef<ispc::int2>(Vector);
}
FORCEINLINE ispc::int3 GetISPCValue(const FIntVector& Vector)
{
	return ReinterpretCastRef<ispc::int3>(Vector);
}
FORCEINLINE ispc::int4 GetISPCValue(const FIntVector4& Vector)
{
	return ReinterpretCastRef<ispc::int4>(Vector);
}

FORCEINLINE ispc::float4x4 GetISPCValue(const FMatrix44f& Vector)
{
	return ReinterpretCastRef<ispc::float4x4>(Vector);
}