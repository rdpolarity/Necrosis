// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "Launch/Resources/Version.h"

#define VOXEL_ENGINE_VERSION (ENGINE_MAJOR_VERSION * 100 + ENGINE_MINOR_VERSION)

#if VOXEL_ENGINE_VERSION >= 501
#define UE_501_SWITCH(Before, AfterOrEqual) AfterOrEqual
#define UE_501_ONLY(...) __VA_ARGS__
#else
#define UE_501_SWITCH(Before, AfterOrEqual) Before
#define UE_501_ONLY(...)
#endif

#if VOXEL_ENGINE_VERSION < 501
struct CStaticStructProvider
{
	template <typename T>
	auto Requires(UScriptStruct*& StructRef) -> decltype(
		StructRef = T::StaticStruct()
	);
};
#endif

#ifndef UE_ASSUME
#define UE_ASSUME(X) ASSUME(X)
#endif

using FEditorAppStyle = class UE_501_SWITCH(FEditorStyle, FAppStyle);