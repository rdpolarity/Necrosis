// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelAdvancedNoiseNode.h"
#include "VoxelAdvancedNoiseNodesImpl.ispc.generated.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_SHADER_PERMUTATION_DOMAIN(VoxelNode_AdvancedNoise2D)
{
	class FNumOctaves : SHADER_PERMUTATION_RANGE_INT("NUM_OCTAVES", 1, 31);
}
END_VOXEL_SHADER_PERMUTATION_DOMAIN(VoxelNode_AdvancedNoise2D, FNumOctaves)

BEGIN_VOXEL_COMPUTE_SHADER(VoxelNode_AdvancedNoise2D)
	VOXEL_SHADER_ALL_PARAMETERS_ARE_OPTIONAL()

	VOXEL_SHADER_PARAMETER_CST(uint32, Num)
	VOXEL_SHADER_PARAMETER_CST(int32, Seed)

	SHADER_PARAMETER_SCALAR_ARRAY(uint32, OctaveTypes, [32])
	SHADER_PARAMETER_SCALAR_ARRAY(float, OctaveStrengths, [32])

	VOXEL_SHADER_PARAMETER_CST(uint32, PositionsXConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, PositionsXBuffer)

	VOXEL_SHADER_PARAMETER_CST(uint32, PositionsYConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, PositionsYBuffer)

	VOXEL_SHADER_PARAMETER_CST(uint32, AmplitudeConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, AmplitudeBuffer)

	VOXEL_SHADER_PARAMETER_CST(uint32, FeatureScaleConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, FeatureScaleBuffer)

	VOXEL_SHADER_PARAMETER_CST(uint32, LacunarityConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, LacunarityBuffer)

	VOXEL_SHADER_PARAMETER_CST(uint32, GainConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, GainBuffer)

	VOXEL_SHADER_PARAMETER_CST(uint32, CellularJitterConstant)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, CellularJitterBuffer)

	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, Result)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

DEFINE_VOXEL_NODE_GPU(FVoxelNode_AdvancedNoise2D, Value)
{
	const TValue<FVoxelVector2DBuffer> Positions = Get(PositionPin, Query);
	const TValue<FVoxelFloatBuffer> Amplitudes = Get(AmplitudePin, Query);
	const TValue<FVoxelFloatBuffer> FeatureScales = Get(FeatureScalePin, Query);
	const TValue<FVoxelFloatBuffer> Lacunarities = Get(LacunarityPin, Query);
	const TValue<FVoxelFloatBuffer> Gains = Get(GainPin, Query);
	const TValue<FVoxelFloatBuffer> CellularJitters = Get(CellularJitterPin, Query);
	const TValue<int32> NumOctaves = Get(NumOctavesPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);
	const TValue<uint8> DefaultOctaveType = Get(DefaultOctaveTypePin, Query);
	const TArray<TValue<uint8>> OctaveTypes = Get(OctaveTypePins, Query);
	const TArray<TValue<TBufferView<float>>> OctaveStrengths = GetBufferView(OctaveStrengthPins, Query);

	return VOXEL_ON_COMPLETE(RenderThread, Positions, Amplitudes, FeatureScales, Lacunarities, Gains, CellularJitters, NumOctaves, Seed, DefaultOctaveType, OctaveTypes, OctaveStrengths)
	{
		VOXEL_USE_NAMESPACE(MetaGraph);

		const int32 Num = ComputeVoxelBuffersNum(Positions, Amplitudes, FeatureScales, Lacunarities, Gains, CellularJitters);

		for (const TBufferView<float>& OctaveStrength : OctaveStrengths)
		{
			if (!OctaveStrength.IsConstant())
			{
				VOXEL_MESSAGE(Error, "{0}: Strenghts need to be constant when running on GPU", this);
				return {};
			}
		}

		if (NumOctaves > 32)
		{
			VOXEL_MESSAGE(Error, "{0}: Num Octaves is limited to 32 when running on GPU", this);
			return {};
		}

		const FVoxelRDGBuffer Result = MakeVoxelRDGBuffer_float(Result, Num);

		BEGIN_VOXEL_SHADER_CALL(VoxelNode_AdvancedNoise2D)
		{
			PermutationDomain.Set<FNumOctaves>(FMath::Clamp(NumOctaves, 1, 32));

			Parameters.Num = Num;
			Parameters.Seed = Seed;

			for (int32 Index = 0; Index < 32; Index++)
			{
				GET_SCALAR_ARRAY_ELEMENT(Parameters.OctaveTypes, Index) = OctaveTypes.IsValidIndex(Index) ? int32(OctaveTypes[Index]) : int32(DefaultOctaveType);
				GET_SCALAR_ARRAY_ELEMENT(Parameters.OctaveStrengths, Index) = OctaveStrengths.IsValidIndex(Index) ? OctaveStrengths[Index].GetConstant() : 1.f;
			}

			Parameters.PositionsXConstant = Positions.X.IsConstant();
			Parameters.PositionsXBuffer = Positions.X.GetGpuBuffer();

			Parameters.PositionsYConstant = Positions.Y.IsConstant();
			Parameters.PositionsYBuffer = Positions.Y.GetGpuBuffer();

			Parameters.AmplitudeConstant = Amplitudes.IsConstant();
			Parameters.AmplitudeBuffer = Amplitudes.GetGpuBuffer();

			Parameters.FeatureScaleConstant = FeatureScales.IsConstant();
			Parameters.FeatureScaleBuffer = FeatureScales.GetGpuBuffer();

			Parameters.LacunarityConstant = Lacunarities.IsConstant();
			Parameters.LacunarityBuffer = Lacunarities.GetGpuBuffer();

			Parameters.GainConstant = Gains.IsConstant();
			Parameters.GainBuffer = Gains.GetGpuBuffer();

			Parameters.CellularJitterConstant = CellularJitters.IsConstant();
			Parameters.CellularJitterBuffer = CellularJitters.GetGpuBuffer();

			Parameters.Result = Result;

			Execute(FComputeShaderUtils::GetGroupCount(Num, 64));
		}
		END_VOXEL_SHADER_CALL()

		FVoxelFloatBuffer ResultBuffer;
		ResultBuffer = FVoxelBufferData::MakeGpu(FVoxelPinType::Make<float>(), Result);
		return ResultBuffer;
	};
}
	
DEFINE_VOXEL_NODE_CPU(FVoxelNode_AdvancedNoise2D, Value)
{
	const auto GetISPCNoise = [](EVoxelAdvancedNoiseOctaveType Noise)
	{
		switch (Noise)
		{
		default: ensure(false);

#define CASE(Name) case EVoxelAdvancedNoiseOctaveType::Name: return ispc::OctaveType_ ## Name;

		CASE(SmoothPerlin);
		CASE(BillowyPerlin);
		CASE(RidgedPerlin);

		CASE(SmoothCellular);
		CASE(BillowyCellular);
		CASE(RidgedCellular);

#undef CASE
		}
	};

	const TValue<TBufferView<FVector2D>> Positions = GetBufferView(PositionPin, Query);
	const TValue<TBufferView<float>> Amplitudes = GetBufferView(AmplitudePin, Query);
	const TValue<TBufferView<float>> FeatureScales = GetBufferView(FeatureScalePin, Query);
	const TValue<TBufferView<float>> Lacunarities = GetBufferView(LacunarityPin, Query);
	const TValue<TBufferView<float>> Gains = GetBufferView(GainPin, Query);
	const TValue<TBufferView<float>> CellularJitters = GetBufferView(CellularJitterPin, Query);
	const TValue<int32> NumOctaves = Get(NumOctavesPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);
	const TValue<uint8> DefaultOctaveType = Get(DefaultOctaveTypePin, Query);
	const TArray<TValue<uint8>> OctaveTypes = Get(OctaveTypePins, Query);
	const TArray<TValue<TBufferView<float>>> OctaveStrengths = GetBufferView(OctaveStrengthPins, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, Positions, Amplitudes, FeatureScales, Lacunarities, Gains, CellularJitters, NumOctaves, Seed, DefaultOctaveType, OctaveTypes, OctaveStrengths, GetISPCNoise)
	{
		const int32 Num = ComputeVoxelBuffersNum(Positions, Amplitudes, FeatureScales, Lacunarities, Gains, CellularJitters);
		const int32 SafeNumOctaves = FMath::Clamp(NumOctaves, 1, 255);

		TArray<ispc::FOctave> Octaves;
		Octaves.Reserve(SafeNumOctaves);

		for (int32 Index = 0; Index < SafeNumOctaves; Index++)
		{
			ispc::FOctave& Octave = Octaves.Emplace_GetRef(ispc::FOctave{});

			if (OctaveTypes.IsValidIndex(Index))
			{
				Octave.Type = GetISPCNoise(EVoxelAdvancedNoiseOctaveType(OctaveTypes[Index]));
			}
			else
			{
				Octave.Type = GetISPCNoise(EVoxelAdvancedNoiseOctaveType(DefaultOctaveType));
			}

			if (OctaveStrengths.IsValidIndex(Index))
			{
				const TBufferView<float>& Strength = OctaveStrengths[Index];

				if (Strength.IsConstant())
				{
					Octave.bStrengthIsConstant = true;
					Octave.StrengthConstant = Strength.GetConstant();
				}
				else
				{
					if (Strength.Num() != Num)
					{
						RaiseBufferError();
						return {};
					}

					Octave.bStrengthIsConstant = false;
					Octave.StrengthArray = Strength.GetData();
				}
			}
			else
			{
				Octave.bStrengthIsConstant = true;
				Octave.StrengthConstant = 1.f;
				Octave.StrengthArray = nullptr;
			}
		}

		TVoxelArray<float> ReturnValue = FVoxelFloatBuffer::Allocate(Num);

		ispc::VoxelNode_AdvancedNoise2D(
			Positions.X.GetData(),
			Positions.X.IsConstant(),
			Positions.Y.GetData(),
			Positions.Y.IsConstant(),
			Amplitudes.GetData(),
			Amplitudes.IsConstant(),
			FeatureScales.GetData(),
			FeatureScales.IsConstant(),
			Lacunarities.GetData(),
			Lacunarities.IsConstant(),
			Gains.GetData(),
			Gains.IsConstant(),
			CellularJitters.GetData(),
			CellularJitters.IsConstant(),
			Octaves.GetData(),
			Octaves.Num(),
			Seed,
			ReturnValue.GetData(),
			Num);

		return FVoxelFloatBuffer::MakeCpu(ReturnValue);
	};
}