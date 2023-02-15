// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNodeCodeGen.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelNodeCodeGenImpl.ispc.generated.h"

const TMap<FName, int32> GVoxelNodeCodeGenIds
{
#include "VoxelNodeCodeGen_Ids.h"	
};

#ifndef VOXEL_NUM_NODE_IDS
#define VOXEL_NUM_NODE_IDS 1
#endif

TMap<int32, FString> GVoxelNodeCodeGenNames;

VOXEL_RUN_ON_STARTUP_GAME(InitializeGVoxelNodeCodeGenNames)
{
	for (const auto& It : GVoxelNodeCodeGenIds)
	{
		GVoxelNodeCodeGenNames.Add(It.Value, It.Key.ToString());
	}
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_SHADER_PERMUTATION_DOMAIN(VoxelNodeCodeGen)
{
	class FNodeId : SHADER_PERMUTATION_RANGE_INT("NODE_ID", 0, VOXEL_NUM_NODE_IDS);
}
END_VOXEL_SHADER_PERMUTATION_DOMAIN(VoxelNodeCodeGen, FNodeId)

BEGIN_VOXEL_COMPUTE_SHADER(VoxelNodeCodeGen)
	VOXEL_SHADER_ALL_PARAMETERS_ARE_OPTIONAL()
	SHADER_PARAMETER(uint32, Num)

	SHADER_PARAMETER_SCALAR_ARRAY(uint32, ByteBuffersConstant, [8])
	SHADER_PARAMETER_SCALAR_ARRAY(uint32, FloatBuffersConstant, [8])
	SHADER_PARAMETER_SCALAR_ARRAY(uint32, Int32BuffersConstant, [8])

	SHADER_PARAMETER_RDG_BUFFER_SRV_ARRAY(Buffer<uint>, ByteBuffersIn, [8])
	SHADER_PARAMETER_RDG_BUFFER_SRV_ARRAY(Buffer<float>, FloatBuffersIn, [8])
	SHADER_PARAMETER_RDG_BUFFER_SRV_ARRAY(Buffer<int>, Int32BuffersIn, [8])

	SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY(RWBuffer<uint>, ByteBuffersOut, [8])
	SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY(RWBuffer<float>, FloatBuffersOut, [8])
	SHADER_PARAMETER_RDG_BUFFER_UAV_ARRAY(RWBuffer<int>, Int32BuffersOut, [8])
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(VoxelNodeCodeGen)
{
	TArray<UScriptStruct*> Structs = GetDerivedStructs<FVoxelNode>();
	Structs.Sort([](const UScriptStruct& A, const UScriptStruct& B)
	{
		return A.GetName() < B.GetName();
	});

	FString IdsFile;
	FString CasesFile;
	FString FunctionsFile;
	FString HLSLFile;

	TSet<UEnum*> UsedEnums;

	int32 NodeIndex = 0;
	for (UScriptStruct* Struct : Structs)
	{
		if (Struct->IsChildOf(FVoxelTemplateNode::StaticStruct()) ||
			Struct->HasMetaData(STATIC_FNAME("Abstract")))
		{
			continue;
		}

		TVoxelInstancedStruct<FVoxelNode> Node(Struct);
		if (!Node->IsCodeGen())
		{
			continue;
		}

		for (const FVoxelPin& Pin : Node->GetPins())
		{
			if (!Pin.GetType().IsEnum())
			{
				continue;
			}

			UsedEnums.Add(Pin.GetType().GetEnum());
		}

		IdsFile += "{ \"" + Struct->GetName() + "\", " + FString::FromInt(NodeIndex) + " },\n";
		CasesFile += "case " + FString::FromInt(NodeIndex) + ": { Execute_" + Struct->GetName() + "(Buffers, Num); return; }";
		FunctionsFile += FVoxelNodeCodeGen::GenerateFunction(*Node);

		HLSLFile += "#if NODE_ID == " + FString::FromInt(NodeIndex) + " // " + Struct->GetStructCPPName() + "\n";
		HLSLFile += FVoxelNodeCodeGen::GenerateHLSLFunction(*Node);
		HLSLFile += "#endif\n\n";

		NodeIndex++;
	}

	{
		FString ISPCEnumDeclarations;
		FString HLSLEnumDeclarations;

		TArray<UEnum*> OrderedEnums = UsedEnums.Array();
		OrderedEnums.Sort([](const UEnum& A, const UEnum& B)
		{
			return A.GetName() < B.GetName();
		});

		for (UEnum* EnumType : OrderedEnums)
		{
			{
				ISPCEnumDeclarations += "enum " + EnumType->GetName();
				ISPCEnumDeclarations += "{";
				for (int32 EnumIndex = 0; EnumIndex < EnumType->NumEnums(); EnumIndex++)
				{
					ISPCEnumDeclarations += "\t" + EnumType->GetNameByIndex(EnumIndex).ToString().Replace(TEXT("::"), TEXT("_")) + " = " + LexToString(EnumType->GetValueByIndex(EnumIndex)) + ",\n";
				}
				ISPCEnumDeclarations += "};\n";
			}

			{
				for (int32 EnumIndex = 0; EnumIndex < EnumType->NumEnums(); EnumIndex++)
				{
					HLSLEnumDeclarations += "#define " + EnumType->GetNameByIndex(EnumIndex).ToString().Replace(TEXT("::"), TEXT("_")) + " " + LexToString(EnumType->GetValueByIndex(EnumIndex)) + "\n";
				}
				HLSLEnumDeclarations += "\n";
			}
		}

		FunctionsFile = ISPCEnumDeclarations + FunctionsFile;
		HLSLFile = HLSLEnumDeclarations + HLSLFile;
	}

	FVoxelNodeCodeGen::FormatCode(CasesFile);
	FVoxelNodeCodeGen::FormatCode(FunctionsFile);
	FVoxelNodeCodeGen::FormatCode(HLSLFile);

	IdsFile += "\n#define VOXEL_NUM_NODE_IDS " + FString::FromInt(NodeIndex);

	const FString BasePath = FVoxelSystemUtilities::GetPlugin().GetBaseDir() / "Source" / "VoxelMetaGraph" / "Private";

	const FString IdsFilePath = BasePath / "VoxelNodeCodeGen_Ids.h";
	const FString CasesFilePath = BasePath / "VoxelNodeCodeGenImpl_Cases.isph";
	const FString FunctionsFilePath = BasePath / "VoxelNodeCodeGenImpl_Functions.isph";
	const FString HLSLFilePath = FVoxelSystemUtilities::GetPlugin().GetBaseDir() / "Shaders" / "Private" / "MetaGraph" / "VoxelNodeCodeGen.ush";

	FString ExistingIdsFile;
	FString ExistingCasesFile;
	FString ExistingFunctionsFile;
	FString ExistingHLSLFile;
	FFileHelper::LoadFileToString(ExistingIdsFile, *IdsFilePath);
	FFileHelper::LoadFileToString(ExistingCasesFile, *CasesFilePath);
	FFileHelper::LoadFileToString(ExistingFunctionsFile, *FunctionsFilePath);
	FFileHelper::LoadFileToString(ExistingHLSLFile, *HLSLFilePath);

	// Normalize line endings
	ExistingIdsFile.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	ExistingCasesFile.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	ExistingFunctionsFile.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	ExistingHLSLFile.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

	bool bModified = false;

	if (!ExistingIdsFile.Equals(IdsFile))
	{
		bModified = true;
		IFileManager::Get().Delete(*IdsFilePath);
		FFileHelper::SaveStringToFile(IdsFile, *IdsFilePath);
		LOG_VOXEL(Error, "%s written", *IdsFilePath);
	}
	if (!ExistingCasesFile.Equals(CasesFile))
	{
		bModified = true;
		IFileManager::Get().Delete(*CasesFilePath);
		FFileHelper::SaveStringToFile(CasesFile, *CasesFilePath);
		LOG_VOXEL(Error, "%s written", *CasesFilePath);
	}
	if (!ExistingFunctionsFile.Equals(FunctionsFile))
	{
		bModified = true;
		IFileManager::Get().Delete(*FunctionsFilePath);
		FFileHelper::SaveStringToFile(FunctionsFile, *FunctionsFilePath);
		LOG_VOXEL(Error, "%s written", *FunctionsFilePath);
	}
	if (!ExistingHLSLFile.Equals(HLSLFile))
	{
		bModified = true;
		IFileManager::Get().Delete(*HLSLFilePath);
		FFileHelper::SaveStringToFile(HLSLFile, *HLSLFilePath);
		LOG_VOXEL(Error, "%s written", *HLSLFilePath);
	}

	if (bModified)
	{
		ensure(false);
		FPlatformMisc::RequestExit(true);
	}
}
#endif

int32 FVoxelNodeCodeGen::GetNodeId(const UScriptStruct* Struct)
{
	return GVoxelNodeCodeGenIds.FindChecked(Struct->GetFName());
}

int32 FVoxelNodeCodeGen::GetRegisterWidth(const FVoxelPinType& Type)
{
	const FVoxelPinType InnerType = Type.GetInnerType();

	if (InnerType.Is<bool>() ||
		InnerType.Is<uint8>() ||
		InnerType.Is<float>() ||
		InnerType.Is<int32>())
	{
		return 1;
	}
	else if (
		InnerType.Is<FVector2D>() ||
		InnerType.Is<FIntPoint>())
	{
		return 2;
	}
	else if (
		InnerType.Is<FVector>() ||
		InnerType.Is<FIntVector>())
	{
		return 3;
	}
	else if (
		InnerType.Is<FQuat>() ||
		InnerType.Is<FLinearColor>())
	{
		return 4;
	}
	else
	{
		check(false);
		return 0;
	}
}

FVoxelPinType FVoxelNodeCodeGen::GetRegisterType(const FVoxelPinType& Type)
{
	const FVoxelPinType InnerType = Type.GetInnerType();

	if (InnerType.Is<bool>())
	{
		return FVoxelPinType::Make<bool>();
	}
	else if (InnerType.Is<uint8>())
	{
		return FVoxelPinType::Make<uint8>();
	}
	else if (
		InnerType.Is<float>() ||
		InnerType.Is<FVector2D>() ||
		InnerType.Is<FVector>() ||
		InnerType.Is<FQuat>() ||
		InnerType.Is<FLinearColor>())
	{
		return FVoxelPinType::Make<float>();
	}
	else if (
		InnerType.Is<int32>() ||
		InnerType.Is<FIntPoint>() ||
		InnerType.Is<FIntVector>())
	{
		return FVoxelPinType::Make<int32>();
	}
	else
	{
		check(false);
		return {};
	}
}

void FVoxelNodeCodeGen::FormatCode(FString& Code)
{
	VOXEL_FUNCTION_COUNTER();

	Code.ReplaceInline(TEXT(";"), TEXT(";\n"));
	Code.ReplaceInline(TEXT("{"), TEXT("\n{\n"));
	Code.ReplaceInline(TEXT("}"), TEXT("\n}\n"));

	{
		TArray<FString> Lines;
		Code.ParseIntoArray(Lines, TEXT("\n"), false);

		for (FString& Line : Lines)
		{
			Line.TrimStartAndEndInline();
		}
		
		Code = FString::Join(Lines, TEXT("\n"));
		Code.ReplaceInline(TEXT("\n\n}"), TEXT("\n}"));
		Code.ParseIntoArray(Lines, TEXT("\n"), false);

		int32 Padding = 0;
		for (FString& Line : Lines)
		{
			FString Prefix;
			for (int32 Index = 0; Index < Padding; Index++)
			{
				Prefix += "\t";
			}

			if (Line == "{")
			{
				Padding++;
			}
			else if (Line == "}")
			{
				Padding--;
				Prefix.RemoveFromEnd(TEXT("\t"));
			}

			Line = Prefix + Line;
		}

		Code = FString::Join(Lines, TEXT("\n"));
	}
}

FString FVoxelNodeCodeGen::GenerateFunction(const FVoxelNode& Node)
{
	VOXEL_FUNCTION_COUNTER();

	FString Result;
	Result += "FORCEINLINE void Execute_" + Node.GetStruct()->GetName() + "(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)";
	Result += "{";
	Result += "FOREACH(Index, 0, Num)";
	Result += "{";

	int32 BufferIndex = 0;
	for (const FVoxelPin& Pin : Node.GetPins())
	{
		if (!Pin.bIsInput)
		{
			continue;
		}

		const FVoxelPinType Type = Pin.GetType().GetInnerType();

		if (Type.Is<bool>())
		{
			const int32 Index = BufferIndex++;
			Result += "const bool " + Pin.Name.ToString() + " = LoadBool(Buffers[" + FString::FromInt(Index) + "], Index);";
		}
		else if (Type.Is<uint8>())
		{
			const int32 Index = BufferIndex++;
			Result += "const uint8 " + Pin.Name.ToString() + " = LoadByte(Buffers[" + FString::FromInt(Index) + "], Index);";
		}
		else if (Type.Is<float>())
		{
			const int32 Index = BufferIndex++;
			Result += "const float " + Pin.Name.ToString() + " = LoadFloat(Buffers[" + FString::FromInt(Index) + "], Index);";
		}
		else if (Type.Is<int32>())
		{
			const int32 Index = BufferIndex++;
			Result += "const int32 " + Pin.Name.ToString() + " = LoadInt32(Buffers[" + FString::FromInt(Index) + "], Index);";
		}
		else if (Type.Is<FVector2D>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			Result += "const float2 " + Pin.Name.ToString() + " = MakeFloat2(" +
				"LoadFloat(Buffers[" + FString::FromInt(IndexX) + "], Index), " +
				"LoadFloat(Buffers[" + FString::FromInt(IndexY) + "], Index));";
		}
		else if (Type.Is<FVector>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			const int32 IndexZ = BufferIndex++;
			Result += "const float3 " + Pin.Name.ToString() + " = MakeFloat3(" +
				"LoadFloat(Buffers[" + FString::FromInt(IndexX) + "], Index), " +
				"LoadFloat(Buffers[" + FString::FromInt(IndexY) + "], Index), " +
				"LoadFloat(Buffers[" + FString::FromInt(IndexZ) + "], Index));";
		}
		else if (Type.Is<FIntPoint>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			Result += "const int2 " + Pin.Name.ToString() + " = MakeInt2(" +
				"LoadInt32(Buffers[" + FString::FromInt(IndexX) + "], Index), " +
				"LoadInt32(Buffers[" + FString::FromInt(IndexY) + "], Index));";
		}
		else if (Type.Is<FIntVector>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			const int32 IndexZ = BufferIndex++;
			Result += "const int3 " + Pin.Name.ToString() + " = MakeInt3(" +
				"LoadInt32(Buffers[" + FString::FromInt(IndexX) + "], Index), " +
				"LoadInt32(Buffers[" + FString::FromInt(IndexY) + "], Index), " +
				"LoadInt32(Buffers[" + FString::FromInt(IndexZ) + "], Index));";
		}
		else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			const int32 IndexZ = BufferIndex++;
			const int32 IndexW = BufferIndex++;
			Result += "const float4 " + Pin.Name.ToString() + " = MakeFloat4(" +
				"LoadFloat(Buffers[" + FString::FromInt(IndexX) + "], Index), " +
				"LoadFloat(Buffers[" + FString::FromInt(IndexY) + "], Index), " +
				"LoadFloat(Buffers[" + FString::FromInt(IndexZ) + "], Index), " +
				"LoadFloat(Buffers[" + FString::FromInt(IndexW) + "], Index));";
		}
		else
		{
			check(false);
		}
	}

	for (const FVoxelPin& Pin : Node.GetPins())
	{
		if (Pin.bIsInput)
		{
			continue;
		}

		const FVoxelPinType Type = Pin.GetType().GetInnerType();

		if (Type.Is<bool>())
		{
			Result += "bool " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<float>())
		{
			Result += "float " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<int32>())
		{
			Result += "int32 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FVector2D>())
		{
			Result += "float2 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FVector>())
		{
			Result += "float3 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FIntPoint>())
		{
			Result += "int2 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FIntVector>())
		{
			Result += "int3 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
		{
			Result += "float4 " + Pin.Name.ToString() + ";";
		}
		else
		{
			check(false);
		}
	}

	Result += "\n";

	FString Body = Node.GenerateCode(false);

	for (const FVoxelPin& Pin : Node.GetPins())
	{
		FString Name = Pin.Name.ToString();
		if (!Pin.bIsInput)
		{
			Name.RemoveFromStart(TEXT("Out"));
		}

		Body.TrimStartAndEndInline();
		if (!Body.EndsWith(TEXT(";")))
		{
			Body += ";";
		}

		const FString Id = "{" + Name + "}";
		ensure(Body.Contains(Id));

		Body.ReplaceInline(*Id, *Pin.Name.ToString(), ESearchCase::CaseSensitive);
	}
	Result += Body;
	
	Result += "\n";

	for (const FVoxelPin& Pin : Node.GetPins())
	{
		if (Pin.bIsInput)
		{
			continue;
		}
		const FVoxelPinType Type = Pin.GetType().GetInnerType();

		if (Type.Is<bool>())
		{
			const int32 Index = BufferIndex++;
			Result += "StoreBool(Buffers[" + FString::FromInt(Index) + "], Index, " + Pin.Name.ToString() + ");";
		}
		else if (Type.Is<uint8>())
		{
			const int32 Index = BufferIndex++;
			Result += "StoreByte(Buffers[" + FString::FromInt(Index) + "], Index, " + Pin.Name.ToString() + ");";
		}
		else if (Type.Is<float>())
		{
			const int32 Index = BufferIndex++;
			Result += "StoreFloat(Buffers[" + FString::FromInt(Index) + "], Index, " + Pin.Name.ToString() + ");";
		}
		else if (Type.Is<int32>())
		{
			const int32 Index = BufferIndex++;
			Result += "StoreInt32(Buffers[" + FString::FromInt(Index) + "], Index, " + Pin.Name.ToString() + ");";
		}
		else if (Type.Is<FVector2D>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexX) + "], Index, " + Pin.Name.ToString() + ".x);";
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexY) + "], Index, " + Pin.Name.ToString() + ".y);";
		}
		else if (Type.Is<FVector>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			const int32 IndexZ = BufferIndex++;
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexX) + "], Index, " + Pin.Name.ToString() + ".x);";
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexY) + "], Index, " + Pin.Name.ToString() + ".y);";
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexZ) + "], Index, " + Pin.Name.ToString() + ".z);";
		}
		else if (Type.Is<FIntPoint>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			Result += "StoreInt32(Buffers[" + FString::FromInt(IndexX) + "], Index, " + Pin.Name.ToString() + ".x);";
			Result += "StoreInt32(Buffers[" + FString::FromInt(IndexY) + "], Index, " + Pin.Name.ToString() + ".y);";
		}
		else if (Type.Is<FIntVector>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			const int32 IndexZ = BufferIndex++;
			Result += "StoreInt32(Buffers[" + FString::FromInt(IndexX) + "], Index, " + Pin.Name.ToString() + ".x);";
			Result += "StoreInt32(Buffers[" + FString::FromInt(IndexY) + "], Index, " + Pin.Name.ToString() + ".y);";
			Result += "StoreInt32(Buffers[" + FString::FromInt(IndexZ) + "], Index, " + Pin.Name.ToString() + ".z);";
		}
		else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
		{
			const int32 IndexX = BufferIndex++;
			const int32 IndexY = BufferIndex++;
			const int32 IndexZ = BufferIndex++;
			const int32 IndexW = BufferIndex++;
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexX) + "], Index, " + Pin.Name.ToString() + ".x);";
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexY) + "], Index, " + Pin.Name.ToString() + ".y);";
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexZ) + "], Index, " + Pin.Name.ToString() + ".z);";
			Result += "StoreFloat(Buffers[" + FString::FromInt(IndexW) + "], Index, " + Pin.Name.ToString() + ".w);";
		}
		else
		{
			check(false);
		}
	}

	Result += "}";
	Result += "}";
	Result += "\n";

	return Result;
}

FString FVoxelNodeCodeGen::GenerateHLSLFunction(const FVoxelNode& Node)
{
	VOXEL_FUNCTION_COUNTER();

	FString Result;
	{
		int32 BufferIndexBool = 0;
		int32 BufferIndexFloat = 0;
		int32 BufferIndexInt32 = 0;
		for (const FVoxelPin& Pin : Node.GetPins())
		{
			if (!Pin.bIsInput)
			{
				continue;
			}

			const FVoxelPinType Type = Pin.GetType().GetInnerType();

			if (Type.Is<bool>())
			{
				const int32 Index = BufferIndexBool++;
				Result += "const bool " + Pin.Name.ToString() + " = LoadByte(" + FString::FromInt(Index) + ");";
			}
			else if (Type.Is<uint8>())
			{
				const int32 Index = BufferIndexInt32++;
				Result += "const uint " + Pin.Name.ToString() + " = LoadByte(" + FString::FromInt(Index) + ");";
			}
			else if (Type.Is<float>())
			{
				const int32 Index = BufferIndexFloat++;
				Result += "const float " + Pin.Name.ToString() + " = LoadFloat(" + FString::FromInt(Index) + ");";
			}
			else if (Type.Is<int32>())
			{
				const int32 Index = BufferIndexInt32++;
				Result += "const int " + Pin.Name.ToString() + " = LoadInt32(" + FString::FromInt(Index) + ");";
			}
			else if (Type.Is<FVector2D>())
			{
				const int32 IndexX = BufferIndexFloat++;
				const int32 IndexY = BufferIndexFloat++;
				Result += "const float2 " + Pin.Name.ToString() + " = float2(" +
					"LoadFloat(" + FString::FromInt(IndexX) + "), " +
					"LoadFloat(" + FString::FromInt(IndexY) + "));";
			}
			else if (Type.Is<FVector>())
			{
				const int32 IndexX = BufferIndexFloat++;
				const int32 IndexY = BufferIndexFloat++;
				const int32 IndexZ = BufferIndexFloat++;
				Result += "const float3 " + Pin.Name.ToString() + " = float3(" +
					"LoadFloat(" + FString::FromInt(IndexX) + "), " +
					"LoadFloat(" + FString::FromInt(IndexY) + "), " +
					"LoadFloat(" + FString::FromInt(IndexZ) + "));";
			}
			else if (Type.Is<FIntPoint>())
			{
				const int32 IndexX = BufferIndexInt32++;
				const int32 IndexY = BufferIndexInt32++;
				Result += "const int2 " + Pin.Name.ToString() + " = int2(" +
					"LoadInt32(" + FString::FromInt(IndexX) + "), " +
					"LoadInt32(" + FString::FromInt(IndexY) + "));";
			}
			else if (Type.Is<FIntVector>())
			{
				const int32 IndexX = BufferIndexInt32++;
				const int32 IndexY = BufferIndexInt32++;
				const int32 IndexZ = BufferIndexInt32++;
				Result += "const int3 " + Pin.Name.ToString() + " = int3(" +
					"LoadInt32(" + FString::FromInt(IndexX) + "), " +
					"LoadInt32(" + FString::FromInt(IndexY) + "), " +
					"LoadInt32(" + FString::FromInt(IndexZ) + "));";
			}
			else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
			{
				const int32 IndexX = BufferIndexFloat++;
				const int32 IndexY = BufferIndexFloat++;
				const int32 IndexZ = BufferIndexFloat++;
				const int32 IndexW = BufferIndexFloat++;
				Result += "const float4 " + Pin.Name.ToString() + " = float4(" +
					"LoadFloat(" + FString::FromInt(IndexX) + "), " +
					"LoadFloat(" + FString::FromInt(IndexY) + "), " +
					"LoadFloat(" + FString::FromInt(IndexZ) + "), " +
					"LoadFloat(" + FString::FromInt(IndexW) + "));";
			}
			else
			{
				check(false);
			}
		}
	}

	for (const FVoxelPin& Pin : Node.GetPins())
	{
		if (Pin.bIsInput)
		{
			continue;
		}

		const FVoxelPinType Type = Pin.GetType().GetInnerType();

		if (Type.Is<bool>())
		{
			Result += "bool " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<float>())
		{
			Result += "float " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<int32>())
		{
			Result += "int " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FVector2D>())
		{
			Result += "float2 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FVector>())
		{
			Result += "float3 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FIntPoint>())
		{
			Result += "int2 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FIntVector>())
		{
			Result += "int3 " + Pin.Name.ToString() + ";";
		}
		else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
		{
			Result += "float4 " + Pin.Name.ToString() + ";";
		}
		else
		{
			check(false);
		}
	}

	Result += "\n";

	FString Body = Node.GenerateCode(true);

	for (const FVoxelPin& Pin : Node.GetPins())
	{
		FString Name = Pin.Name.ToString();
		if (!Pin.bIsInput)
		{
			Name.RemoveFromStart(TEXT("Out"));
		}

		Body.TrimStartAndEndInline();
		if (!Body.EndsWith(TEXT(";")))
		{
			Body += ";";
		}

		const FString Id = "{" + Name + "}";
		ensure(Body.Contains(Id));

		Body.ReplaceInline(*Id, *Pin.Name.ToString(), ESearchCase::CaseSensitive);
	}
	Result += Body;
	
	Result += "\n";
	
	{
		int32 BufferIndexByte = 0;
		int32 BufferIndexFloat = 0;
		int32 BufferIndexInt32 = 0;
		for (const FVoxelPin& Pin : Node.GetPins())
		{
			if (Pin.bIsInput)
			{
				continue;
			}
			const FVoxelPinType Type = Pin.GetType().GetInnerType();

			if (Type.Is<bool>())
			{
				const int32 Index = BufferIndexByte++;
				Result += "StoreByte(" + FString::FromInt(Index) + ", " + Pin.Name.ToString() + ");";
			}
			else if (Type.Is<uint8>())
			{
				const int32 Index = BufferIndexByte++;
				Result += "StoreByte(" + FString::FromInt(Index) + ", " + Pin.Name.ToString() + ");";
			}
			else if (Type.Is<float>())
			{
				const int32 Index = BufferIndexFloat++;
				Result += "StoreFloat(" + FString::FromInt(Index) + ", " + Pin.Name.ToString() + ");";
			}
			else if (Type.Is<int32>())
			{
				const int32 Index = BufferIndexInt32++;
				Result += "StoreInt32(" + FString::FromInt(Index) + ", " + Pin.Name.ToString() + ");";
			}
			else if (Type.Is<FVector2D>())
			{
				const int32 IndexX = BufferIndexFloat++;
				const int32 IndexY = BufferIndexFloat++;
				Result += "StoreFloat(" + FString::FromInt(IndexX) + ", " + Pin.Name.ToString() + ".x);";
				Result += "StoreFloat(" + FString::FromInt(IndexY) + ", " + Pin.Name.ToString() + ".y);";
			}
			else if (Type.Is<FVector>())
			{
				const int32 IndexX = BufferIndexFloat++;
				const int32 IndexY = BufferIndexFloat++;
				const int32 IndexZ = BufferIndexFloat++;
				Result += "StoreFloat(" + FString::FromInt(IndexX) + ", " + Pin.Name.ToString() + ".x);";
				Result += "StoreFloat(" + FString::FromInt(IndexY) + ", " + Pin.Name.ToString() + ".y);";
				Result += "StoreFloat(" + FString::FromInt(IndexZ) + ", " + Pin.Name.ToString() + ".z);";
			}
			else if (Type.Is<FIntPoint>())
			{
				const int32 IndexX = BufferIndexInt32++;
				const int32 IndexY = BufferIndexInt32++;
				Result += "StoreInt32(" + FString::FromInt(IndexX) + ", " + Pin.Name.ToString() + ".x);";
				Result += "StoreInt32(" + FString::FromInt(IndexY) + ", " + Pin.Name.ToString() + ".y);";
			}
			else if (Type.Is<FIntVector>())
			{
				const int32 IndexX = BufferIndexInt32++;
				const int32 IndexY = BufferIndexInt32++;
				const int32 IndexZ = BufferIndexInt32++;
				Result += "StoreInt32(" + FString::FromInt(IndexX) + ", " + Pin.Name.ToString() + ".x);";
				Result += "StoreInt32(" + FString::FromInt(IndexY) + ", " + Pin.Name.ToString() + ".y);";
				Result += "StoreInt32(" + FString::FromInt(IndexZ) + ", " + Pin.Name.ToString() + ".z);";
			}
			else if (Type.Is<FQuat>() || Type.Is<FLinearColor>())
			{
				const int32 IndexX = BufferIndexFloat++;
				const int32 IndexY = BufferIndexFloat++;
				const int32 IndexZ = BufferIndexFloat++;
				const int32 IndexW = BufferIndexFloat++;
				Result += "StoreFloat(" + FString::FromInt(IndexX) + ", " + Pin.Name.ToString() + ".x);";
				Result += "StoreFloat(" + FString::FromInt(IndexY) + ", " + Pin.Name.ToString() + ".y);";
				Result += "StoreFloat(" + FString::FromInt(IndexZ) + ", " + Pin.Name.ToString() + ".z);";
				Result += "StoreFloat(" + FString::FromInt(IndexW) + ", " + Pin.Name.ToString() + ".w);";
			}
			else
			{
				check(false);
			}
		}
	}

	return Result;
}

void FVoxelNodeCodeGen::ExecuteCpu(
	const int32 Id,
	const TVoxelArray<FCpuBuffer>& Buffers,
	const int32 Num)
{
	VOXEL_SCOPE_COUNTER_FORMAT("%s Num=%d", *GVoxelNodeCodeGenNames[Id], Num);

	TVoxelArray<ispc::FVoxelBuffer> ISPCBuffers;
	for (const FCpuBuffer& Buffer : Buffers)
	{
		if (!ensure(Buffer.Num == Num || Buffer.Num == 1))
		{
			return;
		}

		ISPCBuffers.Add({ Buffer.Data, Buffer.Num == 1 });
	}

	ispc::VoxelCodeGen_Execute(Id, ISPCBuffers.GetData(), Num);
}

void FVoxelNodeCodeGen::ExecuteGpu(
	FRDGBuilder& GraphBuilder,
	const int32 Id,
	const TVoxelArray<FGpuBuffer>& BuffersIn,
	const TVoxelArray<FGpuBuffer>& BuffersOut,
	const int32 Num)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);

	for (const FGpuBuffer& Buffer : BuffersIn)
	{
		if (!ensure(Buffer.Num == Num || Buffer.Num == 1))
		{
			return;
		}
	}
	for (const FGpuBuffer& Buffer : BuffersOut)
	{
		if (!ensure(Buffer.Num == Num))
		{
			return;
		}
	}

	TVoxelArray<FGpuBuffer> ByteBuffersIn;
	TVoxelArray<FGpuBuffer> FloatBuffersIn;
	TVoxelArray<FGpuBuffer> Int32BuffersIn;

	for (const FGpuBuffer& Buffer : BuffersIn)
	{
		if (Buffer.Buffer.GetFormat() == PF_R8_UINT)
		{
			ByteBuffersIn.Add(Buffer);
		}
		else if (Buffer.Buffer.GetFormat() == PF_R32_FLOAT)
		{
			FloatBuffersIn.Add(Buffer);
		}
		else
		{
			check(Buffer.Buffer.GetFormat() == PF_R32_SINT);
			Int32BuffersIn.Add(Buffer);
		}
	}

	TVoxelArray<FGpuBuffer> ByteBuffersOut;
	TVoxelArray<FGpuBuffer> FloatBuffersOut;
	TVoxelArray<FGpuBuffer> Int32BuffersOut;

	for (const FGpuBuffer& Buffer : BuffersOut)
	{
		if (Buffer.Buffer.GetFormat() == PF_R8_UINT)
		{
			ByteBuffersOut.Add(Buffer);
		}
		else if (Buffer.Buffer.GetFormat() == PF_R32_FLOAT)
		{
			FloatBuffersOut.Add(Buffer);
		}
		else
		{
			check(Buffer.Buffer.GetFormat() == PF_R32_SINT);
			Int32BuffersOut.Add(Buffer);
		}
	}

	BEGIN_VOXEL_SHADER_CALL(VoxelNodeCodeGen)
	{
		PermutationDomain.Set<FNodeId>(Id);

		Parameters.Num = Num;

		for (int32 Index = 0; Index < 8; Index++)
		{
			GET_SCALAR_ARRAY_ELEMENT(Parameters.ByteBuffersConstant, Index) = ByteBuffersIn.IsValidIndex(Index) ? ByteBuffersIn[Index].Num == 1 : false;
			GET_SCALAR_ARRAY_ELEMENT(Parameters.FloatBuffersConstant, Index) = FloatBuffersIn.IsValidIndex(Index) ? FloatBuffersIn[Index].Num == 1 : false;
			GET_SCALAR_ARRAY_ELEMENT(Parameters.Int32BuffersConstant, Index) = Int32BuffersIn.IsValidIndex(Index) ? Int32BuffersIn[Index].Num == 1 : false;

			Parameters.ByteBuffersIn[Index] = ByteBuffersIn.IsValidIndex(Index) ? ByteBuffersIn[Index].Buffer : FVoxelRDGBuffer();
			Parameters.FloatBuffersIn[Index] = FloatBuffersIn.IsValidIndex(Index) ? FloatBuffersIn[Index].Buffer : FVoxelRDGBuffer();
			Parameters.Int32BuffersIn[Index] = Int32BuffersIn.IsValidIndex(Index) ? Int32BuffersIn[Index].Buffer : FVoxelRDGBuffer();

			Parameters.ByteBuffersOut[Index] = ByteBuffersOut.IsValidIndex(Index) ? ByteBuffersOut[Index].Buffer : FVoxelRDGBuffer();
			Parameters.FloatBuffersOut[Index] = FloatBuffersOut.IsValidIndex(Index) ? FloatBuffersOut[Index].Buffer : FVoxelRDGBuffer();
			Parameters.Int32BuffersOut[Index] = Int32BuffersOut.IsValidIndex(Index) ? Int32BuffersOut[Index].Buffer : FVoxelRDGBuffer();
		}

		Execute(FComputeShaderUtils::GetGroupCount(Num, 64));
	}
	END_VOXEL_SHADER_CALL()
}