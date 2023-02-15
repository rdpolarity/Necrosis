// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTask.h"
#include "VoxelBuffer.generated.h"

struct FVoxelTerminalBuffer;
struct FVoxelTerminalBufferView;

USTRUCT()
struct FVoxelBufferView : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelBufferView() = default;

	FORCEINLINE const FVoxelPinType& GetInnerType() const
	{
		return PrivateInnerType;
	}

	FORCEINLINE int32 Num() const
	{
		return PrivateNum;
	}
	FORCEINLINE bool IsConstant() const
	{
		return Num() == 1;
	}
	FORCEINLINE bool IsValidIndex(int32 Index) const
	{
		return (0 <= Index && Index < Num()) || IsConstant();
	}

	virtual FVoxelSharedPinValue GetGenericConstant() const VOXEL_PURE_VIRTUAL({});
	virtual void ForeachBufferView(TVoxelFunctionRef<void(const FVoxelTerminalBufferView&)> Lambda) const VOXEL_PURE_VIRTUAL();

protected:
	FVoxelPinType PrivateInnerType;
	int32 PrivateNum = 0;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelBuffer : public FVoxelCustomHash
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual FVoxelPinType GetInnerType() const VOXEL_PURE_VIRTUAL({});
	virtual FVoxelPinType GetViewType() const VOXEL_PURE_VIRTUAL({});
	virtual TVoxelFutureValue<FVoxelBufferView> MakeGenericView() const VOXEL_PURE_VIRTUAL({});
	virtual int32 Num() const VOXEL_PURE_VIRTUAL({});
	virtual bool IsValid() const VOXEL_PURE_VIRTUAL({});
	virtual bool Identical(const FVoxelBuffer& Other) const VOXEL_PURE_VIRTUAL({});
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) VOXEL_PURE_VIRTUAL();
	
	virtual void ForeachBuffer(TVoxelFunctionRef<void(FVoxelTerminalBuffer&)> Lambda) VOXEL_PURE_VIRTUAL();
	virtual void ForeachBufferPair(const FVoxelBuffer& Other, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, const FVoxelTerminalBuffer&)> Lambda) VOXEL_PURE_VIRTUAL();
	virtual void ForeachBufferArray(TConstVoxelArrayView<const FVoxelBuffer*> Others, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, TConstVoxelArrayView<const FVoxelTerminalBuffer*>)> Lambda) VOXEL_PURE_VIRTUAL();

	void ForeachBuffer(TVoxelFunctionRef<void(const FVoxelTerminalBuffer&)> Lambda) const
	{
		VOXEL_CONST_CAST(this)->ForeachBuffer([&](FVoxelTerminalBuffer& Buffer)
		{
			Lambda(Buffer);
		});
	}

public:
	FORCEINLINE bool IsConstant() const
	{
		return Num() == 1;
	}

public:
	static int32 AlignNum(const int32 Num)
	{
		return FVoxelUtilities::DivideCeil(Num, 8) * 8;
	}
	static TVoxelArray<uint8> AllocateRaw(const int32 Num, const int32 TypeSize)
	{
		TVoxelArray<uint8> Result;
		Result.Reserve(AlignNum(Num) * TypeSize);
		Result.SetNumUninitialized(Num * TypeSize);
		return Result;
	}

	template<typename T>
	static TVoxelArray<T> Allocate(int32 Num)
	{
		TVoxelArray<T> Result;
		Result.Reserve(AlignNum(Num));
		Result.SetNumUninitialized(Num);
		return Result;
	}

	template<typename T>
	static void Reserve(TVoxelArray<T>& Array, int32 Num)
	{
		Array.Reserve(AlignNum(Num));
	}
	template<typename T>
	static void Shrink(TVoxelArray<T>& Array)
	{
		const int32 Num = Array.Num();
		const int32 AlignedNum = AlignNum(Num);
		check(AlignedNum >= Num);

		Array.SetNum(AlignedNum, false);
		Array.Shrink();
		Array.SetNum(Num, false);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXELMETAGRAPH_API FVoxelBufferAccessor
{
	FVoxelBufferAccessor() = default;

	template<typename... TArgs>
	explicit FVoxelBufferAccessor(const TArgs&... Args)
	{
		checkStatic(sizeof...(Args) > 1);

		TArray<int32, TInlineAllocator<16>> Nums;
		VOXEL_FOLD_EXPRESSION(Nums.Add(Args.Num()));

		PrivateNum = 1;
		for (const int32 Num : Nums)
		{
			if (PrivateNum == 1)
			{
				PrivateNum = Num;
			}
			else if (PrivateNum != Num && Num != 1)
			{
				PrivateNum = -1;
			}
		}
	}

	FORCEINLINE bool IsValid() const
	{
		return PrivateNum != -1;
	}

	FORCEINLINE int32 Num() const
	{
		return PrivateNum;
	}

private:
	int32 PrivateNum = -1;
};

template<typename T>
struct TVoxelBufferType;

template<typename T>
struct TVoxelBufferViewType;

template<typename T>
struct TVoxelBufferPixelFormat;

template<typename T>
struct TIsVoxelBuffer
{
	static constexpr bool Value = false;
};

template<typename T>
using TVoxelBuffer = typename TVoxelBufferType<T>::Type;

template<typename T>
using TVoxelBufferView = typename TVoxelBufferViewType<T>::Type;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELMETAGRAPH_API FVoxelBufferData : public TSharedFromThis<FVoxelBufferData>
{
public:
	static TSharedRef<FVoxelBufferData> MakeConstant(const FVoxelSharedPinValue& Constant);

	template<typename T>
	static TSharedRef<FVoxelBufferData> MakeConstant(T Constant)
	{
		return FVoxelBufferData::MakeConstant(FVoxelSharedPinValue::Make(Constant));
	}

public:
	static TSharedRef<FVoxelBufferData> MakeCpu(const FVoxelPinType& InnerType, const TSharedRef<const TVoxelArray<uint8>>& Data);

	template<typename T>
	static TSharedRef<FVoxelBufferData> MakeCpu(TVoxelArray<T>& Array)
	{
		TVoxelArray<uint8> ByteArray;

		checkStatic(sizeof(ByteArray) == sizeof(Array));
		FMemory::Memcpy(&ByteArray, &Array, sizeof(Array));
		FMemory::Memzero(&Array, sizeof(Array));

		struct FDummyArray
		{
			void* Data;
			int32 Num;
			int32 Max;
		};
		ReinterpretCastRef<FDummyArray>(ByteArray).Num *= sizeof(T);
		ReinterpretCastRef<FDummyArray>(ByteArray).Max *= sizeof(T);

		return MakeCpu(FVoxelPinType::Make<T>(), MakeSharedCopy(MoveTemp(ByteArray)));
	}

public:
	static TSharedRef<FVoxelBufferData> MakeGpu(const FVoxelPinType& InnerType, const FVoxelRDGBuffer& Buffer);

	template<typename T>
	static TSharedRef<FVoxelBufferData> MakeGpu(int32 InNum, EPixelFormat PixelFormat)
	{
		return FVoxelBufferData::MakeGpu(
			FVoxelPinType::Make<T>(),
			FVoxelRDGBuffer(
				PixelFormat,
				FRDGBufferDesc::CreateBufferDesc(sizeof(T), InNum),
				TEXT("FVoxelBufferData_MakeGpu")));
	}
	template<typename T>
	static TSharedRef<FVoxelBufferData> MakeGpu(int32 InNum)
	{
		checkStatic(TVoxelBufferPixelFormat<T>::Value != PF_Unknown);
		return FVoxelBufferData::MakeGpu<T>(InNum, TVoxelBufferPixelFormat<T>::Value);
	}

	FORCEINLINE const FVoxelPinType& GetInnerType() const
	{
		checkVoxelSlow(bHasCpuView || bHasGpuBuffer);
		checkVoxelSlow(PrivateInnerType.IsValid());
		return PrivateInnerType;
	}
	FORCEINLINE int32 Num() const
	{
		checkVoxelSlow(bHasCpuView || bHasGpuBuffer);
		checkVoxelSlow(PrivateInnerType.IsValid());
		return PrivateNum;
	}

	TVoxelFutureValue<FVoxelTerminalBufferView> MakeView() const;
	FVoxelRDGBuffer GetGpuBuffer() const;
	int64 GetAllocatedSize() const;
	
	TOptional<FFloatInterval> GetInterval() const
	{
		return Interval;
	}
	void SetInterval(const FFloatInterval& NewInterval) const
	{
		ensure(!Interval);
		Interval = NewInterval;
	}

private:
	class FGpuBuffer
	{
	public:
		FGpuBuffer() = default;
		
		void SetExternalBuffer(const TSharedRef<FVoxelRDGExternalBuffer>& InExternalBuffer);
		void SetExtractedBuffer(const TSharedPtr<TRefCountPtr<FRDGPooledBuffer>>& InExtractedBuffer, EPixelFormat InExtractedFormat);
		TSharedPtr<FVoxelRDGExternalBuffer> GetExternalBuffer() const;

		void SetRDGBuffer(const FVoxelRDGBuffer& InRDGBuffer);
		FVoxelRDGBuffer GetRDGBuffer() const;

	private:
		mutable TSharedPtr<FVoxelRDGExternalBuffer> ExternalBuffer;
		EPixelFormat ExtractedFormat = PF_Unknown;
		TSharedPtr<TRefCountPtr<FRDGPooledBuffer>> ExtractedBuffer;

		FVoxelRDGBuffer RDGBuffer;
		int32 RDGBuilderId = -1;
	};

	FVoxelPinType PrivateInnerType;
	int32 PrivateNum = 0;

	mutable TOptional<FFloatInterval> Interval;

	mutable FThreadSafeBool bHasCpuView = false;
	mutable TSharedPtr<TVoxelFutureValue<FVoxelTerminalBufferView>> CpuView;

	mutable FThreadSafeBool bHasGpuBuffer = false;
	mutable TSharedPtr<FGpuBuffer> GpuBuffer;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelTerminalBufferView : public FVoxelBufferView
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelTerminalBufferView() = default;
	FVoxelTerminalBufferView(const FVoxelPinType& InnerType, const TSharedRef<const TVoxelArray<uint8>>& Array)
		: PrivateArray(Array)
	{
		PrivateInnerType = InnerType;

		const int32 TypeSize = InnerType.GetTypeSize();
		ensure(Array->Num() % TypeSize == 0);
		PrivateNum = Array->Num() / TypeSize;
	}

	FORCEINLINE const void* RESTRICT GetData() const
	{
		checkVoxelSlow(PrivateArray);
		return PrivateArray.Get()->GetData();
	}

	FORCEINLINE TConstVoxelArrayView<uint8> MakeByteView() const
	{
		return *PrivateArray;
	}
	FORCEINLINE TSharedRef<const TVoxelArray<uint8>> GetByteArray() const
	{
		return PrivateArray.ToSharedRef();
	}

	virtual void ForeachBufferView(TVoxelFunctionRef<void(const FVoxelTerminalBufferView&)> Lambda) const final override;

private:
	TSharedPtr<const TVoxelArray<uint8>> PrivateArray;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelTerminalBuffer : public FVoxelBuffer
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelTerminalBuffer& operator=(const TSharedRef<const FVoxelBufferData>& NewData)
	{
		Data = NewData;
		return *this;
	}

	FORCEINLINE const void* GetDataPtr() const
	{
		return Data.Get();
	}

	FVoxelRDGBuffer GetGpuBuffer() const;
	TVoxelFutureValue<FVoxelTerminalBufferView> MakeView() const;

	TOptional<FFloatInterval> GetInterval() const;
	void SetInterval(const FFloatInterval& Interval);

	virtual EPixelFormat GetFormat() const VOXEL_PURE_VIRTUAL({});

	virtual TVoxelFutureValue<FVoxelBufferView> MakeGenericView() const final override;
	virtual int32 Num() const final override;
	virtual bool IsValid() const final override { return true; }
	virtual bool Identical(const FVoxelBuffer& Other) const final override;
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) final override;

	virtual void ForeachBuffer(TVoxelFunctionRef<void(FVoxelTerminalBuffer&)> Lambda) override;
	virtual void ForeachBufferPair(const FVoxelBuffer& Other, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, const FVoxelTerminalBuffer&)> Lambda) final override;
	virtual void ForeachBufferArray(TConstVoxelArrayView<const FVoxelBuffer*> Others, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, TConstVoxelArrayView<const FVoxelTerminalBuffer*>)> Lambda) final override;

	virtual uint64 GetHash() const final override;
	virtual int64 GetAllocatedSize() const final override;

private:
	TSharedPtr<const FVoxelBufferData> Data;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelContainerBufferView : public FVoxelBufferView
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelContainerBufferView() = default;

	virtual void ForeachBufferView(TVoxelFunctionRef<void(const FVoxelTerminalBufferView&)> Lambda) const final override;

protected:
	FORCEINLINE int32 NumBuffers() const
	{
		checkVoxelSlow(PrivateBuffers.Data.Num() > 0);
		return PrivateBuffers.Data.Num();
	}
	void ComputeBuffers() const;

	mutable TVoxelRelativePointerArray<FVoxelTerminalBufferView> PrivateBuffers;

	friend struct FVoxelContainerBuffer;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelContainerBuffer : public FVoxelBuffer
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual int32 Num() const final override;
	virtual bool IsValid() const final override;
	virtual bool Identical(const FVoxelBuffer& Other) const final override;
	virtual int64 GetAllocatedSize() const final override;

	virtual void ForeachBuffer(TVoxelFunctionRef<void(FVoxelTerminalBuffer&)> Lambda) override;
	virtual void ForeachBufferPair(const FVoxelBuffer& Other, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, const FVoxelTerminalBuffer&)> Lambda) final override;
	virtual void ForeachBufferArray(TConstVoxelArrayView<const FVoxelBuffer*> Others, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, TConstVoxelArrayView<const FVoxelTerminalBuffer*>)> Lambda) final override;

protected:
	template<typename T, typename LambdaType, typename... TArgs>
	TVoxelFutureValue<TVoxelBufferView<T>> MakeViewHelper(LambdaType Lambda, TArgs&&... Args) const
	{
		return FVoxelTask::New<TVoxelBufferView<T>>(
			MakeShared<FVoxelTaskStat>(),
			STATIC_FNAME("MakeView"),
			EVoxelTaskThread::AnyThread,
			{ Args... },
			[Num = Num(), InnerType = GetInnerType(), Lambda = ::MoveTemp(Lambda)]
			{
				TVoxelBufferView<T> View;
				View.PrivateNum = Num;
				View.PrivateInnerType = InnerType;
				Lambda(View);
				return View;
			});
	}

protected:
	FORCEINLINE int32 NumBuffers() const
	{
		checkVoxelSlow(PrivateBuffers.Data.Num() > 0);
		return PrivateBuffers.Data.Num();
	}
	void ComputeBuffers() const;

	template<typename LambdaType>
	void ForeachBuffer(LambdaType Lambda) const;
	template<typename LambdaType>
	void ForeachBufferPair(const FVoxelContainerBuffer& Other, LambdaType Lambda) const;

	mutable TVoxelRelativePointerArray<FVoxelTerminalBuffer> PrivateBuffers;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelSwitchTerminalTypeSize
{
	const int32 TypeSize;

	template<typename T>
	FORCEINLINE auto operator+(T&& Lambda) -> decltype(auto)
	{
		switch (TypeSize)
		{
		case 1: return Lambda(TVoxelIntegralConstant<int32, 1>());
		case 2: return Lambda(TVoxelIntegralConstant<int32, 2>());
		case 4: return Lambda(TVoxelIntegralConstant<int32, 4>());
		case 8: return Lambda(TVoxelIntegralConstant<int32, 8>());
		default:
		{
			ensure(false);
			return Lambda(TypeSize);
		}
		}
	}
};

#define VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize) FVoxelSwitchTerminalTypeSize{ TypeSize } + [&](auto Static ## TypeSize INTELLISENSE_ONLY(, struct FDummy& TypeSize))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define DECLARE_VOXEL_CONTAINER_BUFFER(InBufferViewType, InBufferType, InUniformType) \
	struct InBufferType; \
	struct InBufferViewType; \
	\
	template<> \
	struct TVoxelBufferType<InUniformType> \
	{ \
		using Type = InBufferType; \
	}; \
	template<> \
	struct TVoxelBufferViewType<InUniformType> \
	{ \
		using Type = InBufferViewType; \
	}; \
	template<> \
	struct TIsVoxelBuffer<InBufferType> \
	{ \
		static constexpr bool Value = true; \
	};

#define DECLARE_VOXEL_TERMINAL_BUFFER(InBufferViewType, InBufferType, InUniformType, InFormat) \
	checkStatic(GetPixelFormat<InFormat>().BlockBytes == sizeof(InUniformType)); \
	template<> \
	struct TVoxelBufferPixelFormat<InUniformType> \
	{ \
		static constexpr EPixelFormat Value = InFormat; \
	}; \
	DECLARE_VOXEL_CONTAINER_BUFFER(InBufferViewType, InBufferType, InUniformType)

#define GENERATED_VOXEL_TERMINAL_BUFFER_BODY(InType) \
	GENERATED_VIRTUAL_STRUCT_BODY() \
	using UniformType = InType; \
	using BufferType = TVoxelBufferType<InType>::Type; \
	\
	static BufferType MakeCpu(TVoxelArray<UniformType>& Array) \
	{ \
		BufferType Result; \
		Result = FVoxelBufferData::MakeCpu(Array); \
		return Result; \
	} \
	static BufferType MakeCpu(TVoxelArray<UniformType>&& Array) \
	{ \
		return MakeCpu(static_cast<TVoxelArray<UniformType>&>(Array)); \
	} \
	static BufferType MakeGpu(int32 InNum) \
	{ \
		BufferType Result; \
		Result = FVoxelBufferData::MakeGpu<UniformType>(InNum); \
		return Result; \
	} \
	static BufferType Constant(UniformType Constant) \
	{ \
		BufferType Result; \
		Result = FVoxelBufferData::MakeConstant<UniformType>(Constant); \
		return Result; \
	} \
	static TVoxelArray<InType> Allocate(int32 Num) \
	{ \
		return Super::Allocate<UniformType>(Num); \
	} \
	FVoxelTerminalBuffer& operator=(const TSharedRef<const FVoxelBufferData>& NewData) \
	{ \
		Super::operator=(NewData); \
		return *this; \
	} \
	\
	virtual EPixelFormat GetFormat() const final override \
	{ \
		return TVoxelBufferPixelFormat<UniformType>::Value; \
	} \
	virtual FVoxelPinType GetInnerType() const final override \
	{ \
		return FVoxelPinType::Make<UniformType>(); \
	} \
	virtual FVoxelPinType GetViewType() const final override \
	{ \
		return FVoxelPinType::Make<TVoxelBufferViewType<UniformType>::Type>(); \
	} \
	TVoxelFutureValue<TVoxelBufferView<InType>> MakeView() const \
	{ \
		return TVoxelFutureValue<TVoxelBufferView<InType>>(Super::MakeView()); \
	}
		
#define GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(InType) \
	GENERATED_VIRTUAL_STRUCT_BODY() \
	using Type = InType; \
	\
	FORCEINLINE const InType* RESTRICT GetData() const \
	{ \
		return static_cast<const Type*>(FVoxelTerminalBufferView::GetData()); \
	} \
	FORCEINLINE TConstVoxelArrayView<InType> GetRawView() const \
	{ \
		return TConstVoxelArrayView<Type>(GetData(), Num()); \
	} \
	\
	FORCEINLINE const InType& GetConstant() const \
	{ \
		checkVoxelSlow(IsConstant()); \
		return operator[](0); \
	} \
	virtual FVoxelSharedPinValue GetGenericConstant() const override \
	{ \
		return FVoxelSharedPinValue::Make(GetConstant()); \
	} \
	\
	FORCEINLINE const InType& operator[](int32 Index) const \
	{ \
		checkVoxelSlow(IsConstant() || (0 <= Index && Index < Num())); \
		return GetData()[IsConstant() ? 0 : Index]; \
	} \
	\
	FORCEINLINE const InType* RESTRICT begin() const \
	{ \
		return GetData(); \
	} \
	FORCEINLINE const InType* RESTRICT end() const \
	{ \
		return GetData() + Num(); \
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelBoolBufferView, FVoxelBoolBuffer, bool, PF_R8_UINT);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelBoolBufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(bool);
};

USTRUCT(DisplayName = "Boolean Buffer")
struct VOXELMETAGRAPH_API FVoxelBoolBuffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(bool);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelByteBufferView, FVoxelByteBuffer, uint8, PF_R8_UINT);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelByteBufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(uint8);
};

USTRUCT(DisplayName = "Byte Buffer")
struct VOXELMETAGRAPH_API FVoxelByteBuffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(uint8);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelFloatBufferView, FVoxelFloatBuffer, float, PF_R32_FLOAT);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelFloatBufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(float);
};

USTRUCT(DisplayName = "Float Buffer")
struct VOXELMETAGRAPH_API FVoxelFloatBuffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(float);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelInt32BufferView, FVoxelInt32Buffer, int32, PF_R32_SINT);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelInt32BufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(int32);
};

USTRUCT(DisplayName = "Integer Buffer")
struct VOXELMETAGRAPH_API FVoxelInt32Buffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(int32);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GENERATED_VOXEL_CONTAINER_BUFFER_BODY(InThisType, InType) \
	GENERATED_VIRTUAL_STRUCT_BODY() \
	using UniformType = InType; \
	\
	virtual FVoxelPinType GetInnerType() const final override \
	{ \
		return FVoxelPinType::Make<UniformType>(); \
	} \
	virtual FVoxelPinType GetViewType() const final override \
	{ \
		return FVoxelPinType::Make<TVoxelBufferViewType<UniformType>::Type>(); \
	} \
	virtual TVoxelFutureValue<FVoxelBufferView> MakeGenericView() const final override \
	{ \
		return MakeView(); \
	} \
	InThisType() \
	{ \
		ComputeBuffers(); \
	}

#define GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(InThisType, InType) \
	GENERATED_VIRTUAL_STRUCT_BODY() \
	using UniformType = InType; \
	\
	friend FVoxelContainerBuffer; \
	\
	FORCEINLINE auto GetConstant() const -> decltype(auto) \
	{ \
		checkVoxelSlow(IsConstant()); \
		return operator[](0); \
	} \
	virtual FVoxelSharedPinValue GetGenericConstant() const override \
	{ \
		return FVoxelSharedPinValue::Make(UniformType(GetConstant())); \
	} \
	InThisType() \
	{ \
		ComputeBuffers(); \
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelVector2DBufferView, FVoxelVector2DBuffer, FVector2D);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelVector2DBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelVector2DBufferView, FVector2D);

	FORCEINLINE FVector2f operator[](int32 Index) const
	{
		return FVector2f(X[Index], Y[Index]);
	}
		
	UPROPERTY()
	FVoxelFloatBufferView X;

	UPROPERTY()
	FVoxelFloatBufferView Y;
};

USTRUCT(DisplayName = "Vector2D Buffer")
struct VOXELMETAGRAPH_API FVoxelVector2DBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelVector2DBuffer, FVector2D);
		
	UPROPERTY()
	FVoxelFloatBuffer X;

	UPROPERTY()
	FVoxelFloatBuffer Y;

	static FVoxelVector2DBuffer MakeCpu(TVoxelArray<float>& InX, TVoxelArray<float>& InY)
	{
		FVoxelVector2DBuffer Result;
		Result.X = FVoxelFloatBuffer::MakeCpu(InX);
		Result.Y = FVoxelFloatBuffer::MakeCpu(InY);
		return Result;
	}
	TVoxelFutureValue<TVoxelBufferView<FVector2D>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewX = X.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewY = Y.MakeView();

		return MakeViewHelper<FVector2D>([=](TVoxelBufferView<FVector2D>& View)
		{
			View.X = ViewX.Get_CheckCompleted();
			View.Y = ViewY.Get_CheckCompleted();
		}, ViewX, ViewY);
	}
	virtual uint64 GetHash() const final override
	{
		return X.GetHash() ^ FVoxelUtilities::MurmurHash64(Y.GetHash());
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		X = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().X);
		Y = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().Y);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelVectorBufferView, FVoxelVectorBuffer, FVector);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelVectorBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelVectorBufferView, FVector);

	FORCEINLINE FVector3f operator[](int32 Index) const
	{
		return FVector3f(X[Index], Y[Index], Z[Index]);
	}
		
	UPROPERTY()
	FVoxelFloatBufferView X;

	UPROPERTY()
	FVoxelFloatBufferView Y;

	UPROPERTY()
	FVoxelFloatBufferView Z;
};

USTRUCT(DisplayName = "Vector Buffer")
struct VOXELMETAGRAPH_API FVoxelVectorBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelVectorBuffer, FVector);
		
	UPROPERTY()
	FVoxelFloatBuffer X;

	UPROPERTY()
	FVoxelFloatBuffer Y;

	UPROPERTY()
	FVoxelFloatBuffer Z;

	static FVoxelVectorBuffer MakeCpu(TVoxelArray<float>& InX, TVoxelArray<float>& InY, TVoxelArray<float>& InZ)
	{
		FVoxelVectorBuffer Result;
		Result.X = FVoxelFloatBuffer::MakeCpu(InX);
		Result.Y = FVoxelFloatBuffer::MakeCpu(InY);
		Result.Z = FVoxelFloatBuffer::MakeCpu(InZ);
		return Result;
	}
	TVoxelFutureValue<TVoxelBufferView<FVector>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewX = X.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewY = Y.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewZ = Z.MakeView();

		return MakeViewHelper<FVector>([=](TVoxelBufferView<FVector>& View)
		{
			View.X = ViewX.Get_CheckCompleted();
			View.Y = ViewY.Get_CheckCompleted();
			View.Z = ViewZ.Get_CheckCompleted();
		}, ViewX, ViewY, ViewZ);
	}
	virtual uint64 GetHash() const final override
	{
		return X.GetHash() ^ FVoxelUtilities::MurmurHash64(Y.GetHash() ^ FVoxelUtilities::MurmurHash64(Z.GetHash()));
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		X = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().X);
		Y = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().Y);
		Z = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().Z);
	}

	void SetBounds(const FVoxelBox& Bounds);
	TOptional<FVoxelBox> GetBounds() const;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelQuaternionBufferView, FVoxelQuaternionBuffer, FQuat);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelQuaternionBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelQuaternionBufferView, FQuat);

	FORCEINLINE FQuat4f operator[](int32 Index) const
	{
		return FQuat4f(X[Index], Y[Index], Z[Index], W[Index]);
	}
		
	UPROPERTY()
	FVoxelFloatBufferView X;

	UPROPERTY()
	FVoxelFloatBufferView Y;

	UPROPERTY()
	FVoxelFloatBufferView Z;

	UPROPERTY()
	FVoxelFloatBufferView W;
};

USTRUCT(DisplayName = "Quaternion Buffer")
struct VOXELMETAGRAPH_API FVoxelQuaternionBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelQuaternionBuffer, FQuat);
		
	UPROPERTY()
	FVoxelFloatBuffer X;

	UPROPERTY()
	FVoxelFloatBuffer Y;

	UPROPERTY()
	FVoxelFloatBuffer Z;

	UPROPERTY()
	FVoxelFloatBuffer W;

	static FVoxelQuaternionBuffer MakeCpu(TVoxelArray<float>& InX, TVoxelArray<float>& InY, TVoxelArray<float>& InZ, TVoxelArray<float>& InW)
	{
		FVoxelQuaternionBuffer Result;
		Result.X = FVoxelFloatBuffer::MakeCpu(InX);
		Result.Y = FVoxelFloatBuffer::MakeCpu(InY);
		Result.Z = FVoxelFloatBuffer::MakeCpu(InZ);
		Result.W = FVoxelFloatBuffer::MakeCpu(InW);
		return Result;
	}
	TVoxelFutureValue<TVoxelBufferView<FQuat>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewX = X.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewY = Y.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewZ = Z.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewW = W.MakeView();

		return MakeViewHelper<FQuat>([=](TVoxelBufferView<FQuat>& View)
		{
			View.X = ViewX.Get_CheckCompleted();
			View.Y = ViewY.Get_CheckCompleted();
			View.Z = ViewZ.Get_CheckCompleted();
			View.W = ViewW.Get_CheckCompleted();
		}, ViewX, ViewY, ViewZ, ViewW);
	}
	virtual uint64 GetHash() const final override
	{
		return X.GetHash() ^ FVoxelUtilities::MurmurHash64(Y.GetHash() ^ FVoxelUtilities::MurmurHash64(Z.GetHash() ^ FVoxelUtilities::MurmurHash64(W.GetHash())));
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		X = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().X);
		Y = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().Y);
		Z = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().Z);
		W = FVoxelBufferData::MakeConstant<float>(Constant.Get<UniformType>().W);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelTransformBufferView, FVoxelTransformBuffer, FTransform);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelTransformBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelTransformBufferView, FTransform);

	FORCEINLINE FTransform3f operator[](int32 Index) const
	{
		return FTransform3f(Rotation[Index], Translation[Index], Scale[Index]);
	}
		
	UPROPERTY()
	FVoxelQuaternionBufferView Rotation;

	UPROPERTY()
	FVoxelVectorBufferView Translation;

	UPROPERTY()
	FVoxelVectorBufferView Scale;
};

USTRUCT(DisplayName = "Transform Buffer")
struct VOXELMETAGRAPH_API FVoxelTransformBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelTransformBuffer, FTransform);
		
	UPROPERTY()
	FVoxelQuaternionBuffer Rotation;

	UPROPERTY()
	FVoxelVectorBuffer Translation;

	UPROPERTY()
	FVoxelVectorBuffer Scale;

	TVoxelFutureValue<TVoxelBufferView<FTransform>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<FQuat>> RotationView = Rotation.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<FVector>> TranslationView = Translation.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<FVector>> ScaleView = Scale.MakeView();

		return MakeViewHelper<FTransform>([=](TVoxelBufferView<FTransform>& View)
		{
			View.Rotation = RotationView.Get_CheckCompleted();
			View.Translation = TranslationView.Get_CheckCompleted();
			View.Scale = ScaleView.Get_CheckCompleted();
		}, RotationView, TranslationView, ScaleView);
	}
	virtual uint64 GetHash() const final override
	{
		return Rotation.GetHash() ^ FVoxelUtilities::MurmurHash64(Translation.GetHash() ^ FVoxelUtilities::MurmurHash64(Scale.GetHash()));
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		Rotation.InitializeFromConstant(FVoxelSharedPinValue::Make(Constant.Get<UniformType>().GetRotation()));
		Translation.InitializeFromConstant(FVoxelSharedPinValue::Make(Constant.Get<UniformType>().GetTranslation()));
		Scale.InitializeFromConstant(FVoxelSharedPinValue::Make(Constant.Get<UniformType>().GetScale3D()));
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelLinearColorBufferView, FVoxelLinearColorBuffer, FLinearColor);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelLinearColorBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelLinearColorBufferView, FLinearColor);

	FORCEINLINE FLinearColor operator[](int32 Index) const
	{
		return FLinearColor(R[Index], G[Index], B[Index], A[Index]);
	}
		
	UPROPERTY()
	FVoxelFloatBufferView R;

	UPROPERTY()
	FVoxelFloatBufferView G;

	UPROPERTY()
	FVoxelFloatBufferView B;

	UPROPERTY()
	FVoxelFloatBufferView A;
};

USTRUCT(DisplayName = "Linear Color Buffer")
struct VOXELMETAGRAPH_API FVoxelLinearColorBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelLinearColorBuffer, FLinearColor);

	UPROPERTY()
	FVoxelFloatBuffer R;

	UPROPERTY()
	FVoxelFloatBuffer G;

	UPROPERTY()
	FVoxelFloatBuffer B;

	UPROPERTY()
	FVoxelFloatBuffer A;

	static FVoxelLinearColorBuffer MakeCpu(TVoxelArray<float>& InR, TVoxelArray<float>& InG, TVoxelArray<float>& InB, TVoxelArray<float>& InA)
	{
		FVoxelLinearColorBuffer Result;
		Result.R = FVoxelFloatBuffer::MakeCpu(InR);
		Result.G = FVoxelFloatBuffer::MakeCpu(InG);
		Result.B = FVoxelFloatBuffer::MakeCpu(InB);
		Result.A = FVoxelFloatBuffer::MakeCpu(InA);
		return Result;
	}
	TVoxelFutureValue<TVoxelBufferView<FLinearColor>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewR = R.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewG = G.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewB = B.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<float>> ViewA = A.MakeView();

		return MakeViewHelper<FLinearColor>([=](TVoxelBufferView<FLinearColor>& View)
		{
			View.R = ViewR.Get_CheckCompleted();
			View.G = ViewG.Get_CheckCompleted();
			View.B = ViewB.Get_CheckCompleted();
			View.A = ViewA.Get_CheckCompleted();
		}, ViewR, ViewG, ViewB, ViewA);
	}
	virtual uint64 GetHash() const final override
	{
		return R.GetHash() ^ FVoxelUtilities::MurmurHash64(G.GetHash() ^ FVoxelUtilities::MurmurHash64(B.GetHash() ^ FVoxelUtilities::MurmurHash64(A.GetHash())));
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		R = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().R);
		G = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().G);
		B = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().B);
		A = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().A);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelIntPointBufferView, FVoxelIntPointBuffer, FIntPoint);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelIntPointBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelIntPointBufferView, FIntPoint);

	FORCEINLINE FIntPoint operator[](int32 Index) const
	{
		return FIntPoint(X[Index], Y[Index]);
	}
		
	UPROPERTY()
	FVoxelInt32BufferView X;

	UPROPERTY()
	FVoxelInt32BufferView Y;
};

USTRUCT(DisplayName = "Int Point Buffer")
struct VOXELMETAGRAPH_API FVoxelIntPointBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelIntPointBuffer, FIntPoint);
		
	UPROPERTY()
	FVoxelInt32Buffer X;

	UPROPERTY()
	FVoxelInt32Buffer Y;

	static FVoxelIntPointBuffer MakeCpu(TVoxelArray<int32>& InX, TVoxelArray<int32>& InY, TVoxelArray<int32>& InZ)
	{
		FVoxelIntPointBuffer Result;
		Result.X = FVoxelInt32Buffer::MakeCpu(InX);
		Result.Y = FVoxelInt32Buffer::MakeCpu(InY);
		return Result;
	}
	TVoxelFutureValue<TVoxelBufferView<FIntPoint>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<int32>> ViewX = X.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<int32>> ViewY = Y.MakeView();

		return MakeViewHelper<FIntPoint>([=](TVoxelBufferView<FIntPoint>& View)
		{
			View.X = ViewX.Get_CheckCompleted();
			View.Y = ViewY.Get_CheckCompleted();
		}, ViewX, ViewY);
	}
	virtual uint64 GetHash() const final override
	{
		return X.GetHash() ^ FVoxelUtilities::MurmurHash64(Y.GetHash());
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		X = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().X);
		Y = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().Y);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_CONTAINER_BUFFER(FVoxelIntVectorBufferView, FVoxelIntVectorBuffer, FIntVector);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelIntVectorBufferView : public FVoxelContainerBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_VIEW_BODY(FVoxelIntVectorBufferView, FIntVector);

	FORCEINLINE FIntVector operator[](int32 Index) const
	{
		return FIntVector(X[Index], Y[Index], Z[Index]);
	}
		
	UPROPERTY()
	FVoxelInt32BufferView X;

	UPROPERTY()
	FVoxelInt32BufferView Y;

	UPROPERTY()
	FVoxelInt32BufferView Z;
};

USTRUCT(DisplayName = "Int Vector Buffer")
struct VOXELMETAGRAPH_API FVoxelIntVectorBuffer : public FVoxelContainerBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_CONTAINER_BUFFER_BODY(FVoxelIntVectorBuffer, FIntVector);
		
	UPROPERTY()
	FVoxelInt32Buffer X;

	UPROPERTY()
	FVoxelInt32Buffer Y;

	UPROPERTY()
	FVoxelInt32Buffer Z;

	static FVoxelIntVectorBuffer MakeCpu(TVoxelArray<int32>& InX, TVoxelArray<int32>& InY, TVoxelArray<int32>& InZ)
	{
		FVoxelIntVectorBuffer Result;
		Result.X = FVoxelInt32Buffer::MakeCpu(InX);
		Result.Y = FVoxelInt32Buffer::MakeCpu(InY);
		Result.Z = FVoxelInt32Buffer::MakeCpu(InZ);
		return Result;
	}
	TVoxelFutureValue<TVoxelBufferView<FIntVector>> MakeView() const
	{
		const TVoxelFutureValue<TVoxelBufferView<int32>> ViewX = X.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<int32>> ViewY = Y.MakeView();
		const TVoxelFutureValue<TVoxelBufferView<int32>> ViewZ = Z.MakeView();

		return MakeViewHelper<FIntVector>([=](TVoxelBufferView<FIntVector>& View)
		{
			View.X = ViewX.Get_CheckCompleted();
			View.Y = ViewY.Get_CheckCompleted();
			View.Z = ViewZ.Get_CheckCompleted();
		}, ViewX, ViewY, ViewZ);
	}
	virtual uint64 GetHash() const final override
	{
		return X.GetHash() ^ FVoxelUtilities::MurmurHash64(Y.GetHash() ^ FVoxelUtilities::MurmurHash64(Z.GetHash()));
	}
	virtual void InitializeFromConstant(const FVoxelSharedPinValue& Constant) override
	{
		X = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().X);
		Y = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().Y);
		Z = FVoxelBufferData::MakeConstant(Constant.Get<UniformType>().Z);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(DisplayName = "Float4")
struct VOXELMETAGRAPH_API FVoxelFloat4
{
	GENERATED_BODY()

	float X = 0;
	float Y = 0;
	float Z = 0;
	float W = 0;
};

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelFloat4BufferView, FVoxelFloat4Buffer, FVoxelFloat4, PF_A32B32G32R32F);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelFloat4BufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(FVoxelFloat4);
};

USTRUCT(DisplayName = "Float4 Buffer")
struct VOXELMETAGRAPH_API FVoxelFloat4Buffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelFloat4);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(DisplayName = "Int4")
struct VOXELMETAGRAPH_API FVoxelInt4
{
	GENERATED_BODY()

	int32 X = 0;
	int32 Y = 0;
	int32 Z = 0;
	int32 W = 0;
};

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelInt4BufferView, FVoxelInt4Buffer, FVoxelInt4, PF_R32G32B32A32_UINT);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelInt4BufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(FVoxelInt4);
};

USTRUCT(DisplayName = "Int4 Buffer")
struct VOXELMETAGRAPH_API FVoxelInt4Buffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelInt4);
};