// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelEditorStyle.h"
#include "DetailLayoutBuilder.h"
#include "Tools/LegacyEdModeInterfaces.h"
#include "Framework/Commands/Commands.h"

template<typename T>
class TVoxelCommands : public TCommands<T>
{
public:
	static FString Name;

	TVoxelCommands()
		: TCommands<T>(
			*Name,
			FText::FromString(Name),
			NAME_None,
			FVoxelEditorStyle::GetStyleSetName())
	{
	}
};

VOXELCOREEDITOR_API FString Voxel_GetCommandsName(FString Name);

#define DEFINE_VOXEL_COMMANDS(InName) \
	template<> \
	FString TVoxelCommands<InName>::Name = Voxel_GetCommandsName(#InName); \
	VOXEL_RUN_ON_STARTUP_EDITOR(Register ## InName) \
	{ \
		InName::Register(); \
	}

#define VOXEL_UI_COMMAND( CommandId, FriendlyName, InDescription, CommandType, InDefaultChord, ... ) \
	MakeUICommand_InternalUseOnly( this, CommandId, *Name, TEXT(#CommandId), TEXT(#CommandId) TEXT("_ToolTip"), "." #CommandId, TEXT(FriendlyName), TEXT(InDescription), CommandType, InDefaultChord, ## __VA_ARGS__ );

#define GetChildHandleStatic(Class, Property) GetChildHandle(GET_MEMBER_NAME_STATIC(Class, Property), false).ToSharedRef()

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXELCOREEDITOR_API FVoxelEditorUtilities
{
public:
	static void EnableRealtime();

	static FSlateFontInfo Font();
	
	static FSimpleDelegate MakeRefreshDelegate(const IPropertyTypeCustomizationUtils& CustomizationUtils);
	static FSimpleDelegate MakeRefreshDelegate(const IDetailLayoutBuilder& DetailLayout);
	static FSimpleDelegate MakeRefreshDelegate(const IDetailCategoryBuilder& CategoryBuilder);
	static FSimpleDelegate MakeRefreshDelegate(const IDetailChildrenBuilder& ChildrenBuilder);
	static FSimpleDelegate MakeRefreshDelegate(const TSharedPtr<IPropertyUtilities>& PropertyUtilities, const IDetailsView* DetailsView);

	static void SetSortOrder(IDetailLayoutBuilder& DetailLayout, FName Name, ECategoryPriority::Type Priority, int32 PriorityOffset);

public:
	static void RegisterAssetContextMenu(UClass* Class, const FText& Label, const FText& ToolTip, TFunction<void(UObject*)> Lambda);

	template<typename T>
	static void RegisterAssetContextMenu(const FText& Label, const FText& ToolTip, TFunction<void(T*)> Lambda)
	{
		FVoxelEditorUtilities::RegisterAssetContextMenu(T::StaticClass(), Label, ToolTip, [=](UObject* Object)
		{
			Lambda(CastChecked<T>(Object));
		});
	}

public:
	static bool IsSingleValue(const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!ensure(Handle))
		{
			return false;
		}

		void* Address = nullptr;
		return Handle->GetValueData(Address) == FPropertyAccess::Success;
	}

	template<typename T>
	static bool GetPropertyValue(const TSharedPtr<IPropertyHandle>& Handle, T*& OutValue)
	{
		checkStatic(!TIsDerivedFrom<T, UObject>::Value);

		OutValue = nullptr;
		
		if (!ensure(Handle))
		{
			return false;
		}
		
		void* Address = nullptr;
		if (Handle->GetValueData(Address) != FPropertyAccess::Success ||
			!ensure(Address))
		{
			return false;
		}

		OutValue = static_cast<T*>(Address);

		return true;
	}
	template<typename T>
	static T GetPropertyValue(const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!ensure(Handle))
		{
			return {};
		}
		
		T Value;
		if (!ensure(Handle->GetValue(Value) == FPropertyAccess::Success))
		{
			return {};
		}

		return Value;
	}

public:
	template<typename T>
	static void ForeachData(const TSharedPtr<IPropertyHandle>& PropertyHandle, TFunctionRef<void(T&)> Lambda)
	{
		if (!ensure(PropertyHandle) ||
			!ensure(PropertyHandle->GetProperty()) ||
			!ensure(FVoxelObjectUtilities::MatchesProperty<T>(*PropertyHandle->GetProperty())))
		{
			return;
		}

		PropertyHandle->EnumerateRawData([&](void* RawData, const int32 DataIndex, const int32 NumDatas)
		{
			if (!ensure(RawData))
			{
				return true;
			}

			Lambda(*static_cast<T*>(RawData));
			return true;
		});
	}

public:
	template<typename T>
	static const T& GetStructPropertyValue(const TSharedPtr<IPropertyHandle>& Handle)
	{
		static T Default;
		if (!ensure(Handle))
		{
			return Default;
		}

		FProperty* Property = Handle->GetProperty();
		if (!ensure(Property) ||
			!ensure(Property->IsA<FStructProperty>()) ||
			!ensure(CastFieldChecked<FStructProperty>(Property)->Struct == T::StaticStruct()))
		{
			return Default;
		}

		void* Address = nullptr;
		if (!ensure(Handle->GetValueData(Address) == FPropertyAccess::Success) || !ensure(Address))
		{
			return Default;
		}

		return *static_cast<T*>(Address);
	}
	template<typename T>
	static void SetStructPropertyValue(const TSharedPtr<IPropertyHandle>& Handle, const T& Value)
	{
		if (!ensure(Handle))
		{
			return;
		}

		FProperty* Property = Handle->GetProperty();
		if (!ensure(Property) ||
			!ensure(Property->IsA<FStructProperty>()) ||
			!ensure(CastFieldChecked<FStructProperty>(Property)->Struct == T::StaticStruct()))
		{
			return;
		}

		const FString StringValue = FVoxelObjectUtilities::PropertyToText_Direct(*Property, static_cast<const void*>(&Value), nullptr);
		ensure(Handle->SetValueFromFormattedString(StringValue) == FPropertyAccess::Success);
	}

public:
	template<typename T>
	static T GetEnumPropertyValue(const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!ensure(Handle))
		{
			return {};
		}

		uint8 Value = 0;
		if (!ensure(Handle->GetValue(Value) == FPropertyAccess::Success))
		{
			return {};
		}

		return T(Value);
	}
	template<typename T>
	static T* GetUObjectProperty(const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!Handle)
		{
			return nullptr;
		}

		UObject* Object = nullptr;
		Handle->GetValue(Object);
		return Cast<T>(Object);
	}

	static TArray<FName> GetPropertyOptions(const TSharedRef<IPropertyHandle>& PropertyHandle);
	static TArray<TSharedPtr<IPropertyHandle>> GetChildHandlesRecursive(const TSharedPtr<IPropertyHandle>& PropertyHandle);

public:
	static void RegisterClassLayout(const UClass* Class, FOnGetDetailCustomizationInstance Delegate);
	static void RegisterStructLayout(const UScriptStruct* Struct, FOnGetPropertyTypeCustomizationInstance Delegate, bool bRecursive);
	static void RegisterStructLayout(const UScriptStruct* Struct, FOnGetPropertyTypeCustomizationInstance Delegate, bool bRecursive, const TSharedPtr<IPropertyTypeIdentifier>& Identifier);

public:
	static bool GetRayInfo(FEditorViewportClient* ViewportClient, FVector& OutStart, FVector& OutEnd);

public:
	static TSharedRef<FAssetThumbnailPool> GetThumbnailPool();
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename Class, typename Customization>
void RegisterVoxelClassLayout()
{
	VOXEL_LLM_SCOPE();

	FVoxelEditorUtilities::RegisterClassLayout(Class::StaticClass(), FOnGetDetailCustomizationInstance::CreateLambda([]
	{
		return MakeShared<Customization>();
	}));
}

template<typename Struct, typename Customization, bool bRecursive>
void RegisterVoxelStructLayout()
{
	VOXEL_LLM_SCOPE();

	FVoxelEditorUtilities::RegisterStructLayout(Struct::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateLambda([]
	{
		return MakeShared<Customization>();
	}), bRecursive);
}

template<typename Struct, typename Customization, bool bRecursive, typename Identifier>
void RegisterVoxelStructLayout()
{
	VOXEL_LLM_SCOPE();

	FVoxelEditorUtilities::RegisterStructLayout(Struct::StaticStruct(), FOnGetPropertyTypeCustomizationInstance::CreateLambda([]
	{
		return MakeShared<Customization>();
	}), bRecursive, MakeShared<Identifier>());
}

#define DEFINE_VOXEL_CLASS_LAYOUT(Class, Customization) \
	VOXEL_RUN_ON_STARTUP_EDITOR(ANONYMOUS_VARIABLE(RegisterVoxelClassLayout)) \
	{ \
		RegisterVoxelClassLayout<Class, Customization>(); \
	}

#define DEFINE_VOXEL_STRUCT_LAYOUT(Struct, Customization) \
	VOXEL_RUN_ON_STARTUP_EDITOR(ANONYMOUS_VARIABLE(RegisterVoxelStructLayout)) \
	{ \
		RegisterVoxelStructLayout<Struct, Customization, false>(); \
	}

#define DEFINE_VOXEL_STRUCT_LAYOUT_IDENTIFIER(Struct, Customization, Identifier) \
	VOXEL_RUN_ON_STARTUP_EDITOR(ANONYMOUS_VARIABLE(RegisterVoxelStructLayout)) \
	{ \
		RegisterVoxelStructLayout<Struct, Customization, false, Identifier>(); \
	}

#define DEFINE_VOXEL_STRUCT_LAYOUT_RECURSIVE(Struct, Customization) \
	VOXEL_RUN_ON_STARTUP_EDITOR(ANONYMOUS_VARIABLE(RegisterVoxelStructLayout)) \
	{ \
		RegisterVoxelStructLayout<Struct, Customization, true>(); \
	}

#define DEFINE_VOXEL_STRUCT_LAYOUT_RECURSIVE_IDENTIFIER(Struct, Customization, Identifier) \
	VOXEL_RUN_ON_STARTUP_EDITOR(ANONYMOUS_VARIABLE(RegisterVoxelStructLayout)) \
	{ \
		RegisterVoxelStructLayout<Struct, Customization, true, Identifier>(); \
	}

#define VOXEL_CUSTOMIZE_CLASS(Class) \
	class F ## Class ## Customization : public IDetailCustomization \
	{ \
	public: \
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override; \
	}; \
	DEFINE_VOXEL_CLASS_LAYOUT(Class, F ## Class ## Customization); \
	void F ## Class ## Customization::CustomizeDetails

#define VOXEL_CUSTOMIZE_STRUCT_HEADER(Struct) \
	class F ## Struct ## Customization : public IPropertyTypeCustomization \
	{ \
	public: \
		virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override; \
		virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {} \
	}; \
	DEFINE_VOXEL_STRUCT_LAYOUT(Struct, F ## Struct ## Customization); \
	void F ## Struct ## Customization::CustomizeHeader

#define VOXEL_CUSTOMIZE_STRUCT_CHILDREN(Struct) \
	class F ## Struct ## Customization : public IPropertyTypeCustomization \
	{ \
	public: \
		virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override {} \
		virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override; \
	}; \
	DEFINE_VOXEL_STRUCT_LAYOUT(Struct, F ## Struct ## Customization); \
	void F ## Struct ## Customization::CustomizeChildren