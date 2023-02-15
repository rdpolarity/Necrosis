// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "UObject/ScriptInterface.h"
#include "Logging/TokenizedMessage.h"
#include "Containers/VoxelArray.h"

class IVoxelMessageConsumer
{
public:
	virtual ~IVoxelMessageConsumer() = default;

	virtual void LogMessage(const TSharedRef<FTokenizedMessage>& Message) = 0;
};

class VOXELCORE_API FVoxelScopedMessageConsumer
{
public:
	explicit FVoxelScopedMessageConsumer(TWeakPtr<IVoxelMessageConsumer> MessageConsumer);
	explicit FVoxelScopedMessageConsumer(TFunction<void(const TSharedRef<FTokenizedMessage>&)> LogMessage);
	~FVoxelScopedMessageConsumer();

private:
	TSharedPtr<IVoxelMessageConsumer> TempConsumer;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELCORE_API FVoxelMessageBuilder
{
public:
	using FArg = TFunction<void(FTokenizedMessage& Message, TSet<const UEdGraph*>& OutGraphs)>;

	const EMessageSeverity::Type Severity;
	const FString Format;
	const TSharedPtr<IVoxelMessageConsumer> MessageConsumer;

	explicit FVoxelMessageBuilder(EMessageSeverity::Type Severity, const FString& Format);

	void AddArg(FArg Arg)
	{
		Args.Add(Arg);
	}

	void Silence()
	{
		bIsSilenced = true;
	}
	bool IsSilenced() const
	{
		return bIsSilenced;
	}

	TSharedRef<FTokenizedMessage> BuildMessage(TSet<const UEdGraph*>& OutGraphs) const;

private:
	TArray<FArg> Args;
	bool bIsSilenced = false;
};

template<typename T, typename = void>
struct TVoxelMessageArgProcessor;

template<typename T>
using TVoxelMessageArgProcessorType = TVoxelMessageArgProcessor<typename TRemoveConst<typename TRemovePointer<typename TDecay<T>::Type>::Type>::Type>;

struct VOXELCORE_API FVoxelMessageArgProcessor
{
	template<typename T>
	static void ProcessArg(FVoxelMessageBuilder& Builder, const T& Value)
	{
		TVoxelMessageArgProcessorType<T>::ProcessArg(Builder, Value);
	}

	static void ProcessArgs(FVoxelMessageBuilder& Builder) {}

	template<typename ArgType, typename... ArgTypes>
	static void ProcessArgs(FVoxelMessageBuilder& Builder, ArgType&& Arg, ArgTypes&&... Args)
	{
		ProcessArg(Builder, Arg);
		ProcessArgs(Builder, Forward<ArgTypes>(Args)...);
	}

	static void ProcessTextArg(FVoxelMessageBuilder& Builder, const FText& Text);
	static void ProcessPinArg(FVoxelMessageBuilder& Builder, const UEdGraphPin* Pin);
	static void ProcessObjectArg(FVoxelMessageBuilder& Builder, TWeakObjectPtr<const UObject> Object);
	static void ProcessTokenArg(FVoxelMessageBuilder& Builder, const TSharedRef<IMessageToken>& Token);
	static void ProcessChildArg(FVoxelMessageBuilder& Builder, const TSharedRef<FVoxelMessageBuilder>& ChildBuilder);
};

template<>
struct TVoxelMessageArgProcessor<TSharedRef<IMessageToken>>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const TSharedRef<IMessageToken>& Token)
	{
		FVoxelMessageArgProcessor::ProcessTokenArg(Builder, Token);
	}
};

template<>
struct TVoxelMessageArgProcessor<TSharedRef<FVoxelMessageBuilder>>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const TSharedRef<FVoxelMessageBuilder>& ChildBuilder)
	{
		FVoxelMessageArgProcessor::ProcessChildArg(Builder, ChildBuilder);
	}
};

template<>
struct TVoxelMessageArgProcessor<FText>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FText& Text)
	{
		FVoxelMessageArgProcessor::ProcessTextArg(Builder, Text);
	}
};
template<>
struct TVoxelMessageArgProcessor<char>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const char* Text)
	{
		FVoxelMessageArgProcessor::ProcessTextArg(Builder, FText::FromString(Text));
	}
};
template<>
struct TVoxelMessageArgProcessor<TCHAR>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const TCHAR* Text)
	{
		FVoxelMessageArgProcessor::ProcessTextArg(Builder, FText::FromString(Text));
	}
};
template<>
struct TVoxelMessageArgProcessor<FName>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, FName Text)
	{
		FVoxelMessageArgProcessor::ProcessTextArg(Builder, FText::FromName(Text));
	}
};
template<>
struct TVoxelMessageArgProcessor<FString>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FString& Text)
	{
		FVoxelMessageArgProcessor::ProcessTextArg(Builder, FText::FromString(Text));
	}
};

#define PROCESS_NUMBER(Type) \
	template<> \
	struct TVoxelMessageArgProcessor<Type> \
	{ \
		static void ProcessArg(FVoxelMessageBuilder& Builder, Type Value) \
		{ \
			FVoxelMessageArgProcessor::ProcessTextArg(Builder, FText::AsNumber(Value, &FNumberFormattingOptions::DefaultNoGrouping())); \
		} \
	};

	PROCESS_NUMBER(int8);
	PROCESS_NUMBER(int16);
	PROCESS_NUMBER(int32);
	PROCESS_NUMBER(int64);

	PROCESS_NUMBER(uint8);
	PROCESS_NUMBER(uint16);
	PROCESS_NUMBER(uint32);
	PROCESS_NUMBER(uint64);

	PROCESS_NUMBER(float);
	PROCESS_NUMBER(double);

#undef PROCESS_NUMBER

template<typename T>
struct TVoxelMessageArgProcessor<TWeakObjectPtr<T>, typename TEnableIf<TIsDerivedFrom<T, UObject>::Value>::Type>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, TWeakObjectPtr<const UObject> Object)
	{
		FVoxelMessageArgProcessor::ProcessObjectArg(Builder, Object);
	}
};

template<typename T>
struct TVoxelMessageArgProcessor<T, typename TEnableIf<TIsDerivedFrom<T, UObject>::Value>::Type>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const UObject* Object)
	{
		ensure(IsInGameThread());
		FVoxelMessageArgProcessor::ProcessObjectArg(Builder, Object);
	}
	static void ProcessArg(FVoxelMessageBuilder& Builder, const UObject& Object)
	{
		ensure(IsInGameThread());
		FVoxelMessageArgProcessor::ProcessObjectArg(Builder, &Object);
	}
};

template<typename T>
struct TVoxelMessageArgProcessor<TObjectPtr<T>, typename TEnableIf<TIsDerivedFrom<T, UObject>::Value>::Type>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const TObjectPtr<T>& Object)
	{
		ensure(IsInGameThread());
		FVoxelMessageArgProcessor::ProcessObjectArg(Builder, Object.Get());
	}
};

template<>
struct TVoxelMessageArgProcessor<FScriptInterface>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FScriptInterface& ScriptInterface)
	{
		ensure(IsInGameThread());
		FVoxelMessageArgProcessor::ProcessObjectArg(Builder, ScriptInterface.GetObject());
	}
};

template<>
struct TVoxelMessageArgProcessor<UEdGraphPin>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const UEdGraphPin* Pin)
	{
		FVoxelMessageArgProcessor::ProcessPinArg(Builder, Pin);
	}
	static void ProcessArg(FVoxelMessageBuilder& Builder, const UEdGraphPin& Pin)
	{
		FVoxelMessageArgProcessor::ProcessPinArg(Builder, &Pin);
	}
};

template<typename T, typename Allocator>
struct TVoxelMessageArgProcessor<TArray<T, Allocator>>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const TArray<T, Allocator>& Array)
	{
		if (Array.Num() == 0)
		{
			FVoxelMessageArgProcessor::ProcessArg(Builder, "Empty");
			return;
		}

		FString Format = "{0}";
		for (int32 Index = 1; Index < Array.Num(); Index++)
		{
			Format += FString::Printf(TEXT(", {%d}"), Index);
		}

		const TSharedRef<FVoxelMessageBuilder> ChildBuilder = MakeShared<FVoxelMessageBuilder>(Builder.Severity, Format);
		for (const T& Token : Array)
		{
			FVoxelMessageArgProcessor::ProcessArg(*ChildBuilder, Token);
		}
		FVoxelMessageArgProcessor::ProcessArg(Builder, ChildBuilder);
	}
};

template<typename T, typename Allocator>
struct TVoxelMessageArgProcessor<TVoxelArray<T, Allocator>>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const TVoxelArray<T, Allocator>& Array)
	{
		TVoxelMessageArgProcessor<TArray<T, Allocator>>::ProcessArg(Builder, Array);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXELCORE_API FVoxelMessages
{
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMessageLogged, const TSharedRef<FTokenizedMessage>&, bool bShowNotification);
	static FOnMessageLogged OnMessageLogged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFocusNode, const UEdGraphNode& Node);
	static FOnFocusNode OnFocusNode;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNodeMessageLogged, const UEdGraph* Graph, const TSharedRef<FTokenizedMessage>& Message);
	static FOnNodeMessageLogged OnNodeMessageLogged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnClearNodeMessages, const UEdGraph* Graph);
	static FOnClearNodeMessages OnClearNodeMessages;

	struct FButton
	{
		FString Text;
		FString Tooltip;
		FSimpleDelegate OnClick;
		bool bCloseOnClick = true;
	};
	struct FNotification
	{
		uint64 UniqueId = 0;
		FString Message;
		FSimpleDelegate OnClose;		
		float Duration = 10.f;

		TArray<FButton> Buttons;
	};
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowNotification, const FNotification&);
	static FOnShowNotification OnShowNotification;
	
public:
	static void LogMessage(const TSharedRef<FTokenizedMessage>& Message);

	static void ShowNotification(const FNotification& Notification);
	static void ShowNotification(const FString& Text);
	
public:
	template<typename... ArgTypes>
	static void LogMessage(EMessageSeverity::Type Severity, const TCHAR* Format, ArgTypes&&... Args)
	{
		const TSharedRef<FVoxelMessageBuilder> Builder = MakeShared<FVoxelMessageBuilder>(Severity, Format);
		FVoxelMessageArgProcessor::ProcessArgs(*Builder, Forward<ArgTypes>(Args)...);
		InternalLogMessage(Builder);
	}

	static void InternalLogMessage(const TSharedRef<FVoxelMessageBuilder>& Builder);
};

#define INTERNAL_CHECK_ARG(Name) INTELLISENSE_ONLY((void)((TVoxelMessageArgProcessorType<VOXEL_GET_TYPE(Name)>*)nullptr);)

#define VOXEL_MESSAGE(Severity, Message, ...) \
	FVoxelMessages::LogMessage(EMessageSeverity::Severity, TEXT(Message), ##__VA_ARGS__); \
	VOXEL_FOREACH(INTERNAL_CHECK_ARG, 0, ##__VA_ARGS__)