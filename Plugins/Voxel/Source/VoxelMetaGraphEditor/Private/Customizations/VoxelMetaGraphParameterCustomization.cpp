// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphParameterCustomization.h"
#include "VoxelMetaGraph.h"
#include "VoxelNodeLibrary.h"
#include "VoxelMetaGraphSeed.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphToolkit.h"
#include "Widgets/SVoxelMetaGraphPinTypeComboBox.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FParameterObjectCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const TArray<TWeakObjectPtr<UObject>> SelectedObjects = DetailLayout.GetSelectedObjects();

	if (SelectedObjects.Num() != 1)
	{
		return;
	}

	UVoxelMetaGraph* MetaGraph = Cast<UVoxelMetaGraph>(SelectedObjects[0]);
	if (!ensure(MetaGraph))
	{
		return;
	}

	DetailLayout.HideCategory("Config");

	const TSharedPtr<IPropertyHandle>& ParametersHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelMetaGraph, Parameters), MetaGraph->GetClass());
	uint32 ParametersCount = 0;
	ParametersHandle->GetNumChildren(ParametersCount);

	TSharedPtr<IPropertyHandle> ParameterHandle;
	for (uint32 Index = 0; Index < ParametersCount; Index++)
	{
		const TSharedPtr<IPropertyHandle>& ChildParameterHandle = ParametersHandle->GetChildHandle(Index);

		FVoxelMetaGraphParameter ChildParameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMetaGraphParameter>(ChildParameterHandle);
		if (ChildParameter.Guid != TargetParameterId)
		{
			continue;
		}

		ParameterHandle = ChildParameterHandle;
		break;
	}

	if (!ParameterHandle)
	{
		return;
	}

	const FVoxelMetaGraphParameter TargetParameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMetaGraphParameter>(ParameterHandle);

	const TSharedRef<IPropertyHandle> TypeHandle = ParameterHandle->GetChildHandleStatic(FVoxelMetaGraphParameter, Type);
	const TSharedRef<IPropertyHandle> CategoryHandle = ParameterHandle->GetChildHandleStatic(FVoxelMetaGraphParameter, Category);
	DetailLayout.HideProperty(TypeHandle);
	DetailLayout.HideProperty(CategoryHandle);
	
	IDetailCategoryBuilder& ParameterBuilder = DetailLayout.EditCategory("Parameter", VOXEL_LOCTEXT("Parameter"));

	const TSharedRef<IPropertyHandle> NameHandle = ParameterHandle->GetChildHandleStatic(FVoxelMetaGraphParameter, Name);
	ParameterBuilder.AddProperty(NameHandle);

	const TSharedRef<IPropertyHandle>& DefaultValueHandle = ParameterHandle->GetChildHandleStatic(FVoxelMetaGraphParameter, DefaultValue);
	DetailLayout.HideProperty(DefaultValueHandle);

	ParameterBuilder.AddCustomRow(VOXEL_LOCTEXT("Type"))
	.NameContent()
	[
		TypeHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SPinTypeComboBox)
		.PinTypes_Lambda([]
		{
			return FVoxelNodeLibrary::GetParameterTypes();
		})
		.OnTypeChanged_Lambda([=](FVoxelPinType Type)
		{
			FVoxelEditorUtilities::SetStructPropertyValue(TypeHandle, Type);

			const FVoxelPinType ExposedType = Type.GetExposedType();

			FVoxelPinValue DefaultValue(ExposedType);
			if (ExposedType.Is<FVoxelMetaGraphSeed>())
			{
				DefaultValue.Get<FVoxelMetaGraphSeed>().Randomize();
			}
			FVoxelEditorUtilities::SetStructPropertyValue(DefaultValueHandle, DefaultValue);

			if (const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin())
			{
				Toolkit->SelectParameter(TargetParameterId, true);
			}
		})
		.CurrentType(TargetParameter.Type)
		.WithContainerType(TargetParameter.ParameterType != EVoxelMetaGraphParameterType::Parameter)
	];

	ParameterBuilder.AddProperty(ParameterHandle->GetChildHandleStatic(FVoxelMetaGraphParameter, Description));

	// Create category selection
	{
		FString Category;
		CategoryHandle->GetValue(Category);
	
		ParameterBuilder.AddCustomRow(VOXEL_LOCTEXT("Category"))
		.NameContent()
		[
			CategoryHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SBox)
			.MinDesiredWidth(125.f)
			[
				SNew(SVoxelDetailComboBox<FString>)
				.RefreshDelegate(ParameterBuilder)
				.RefreshOptionsOnChange()
				.Options_Lambda([=]()
				{
					TArray<FString> Categories = MetaGraph->GetCategories(TargetParameter.ParameterType);
					Categories.AddUnique("Default");
					return Categories;
				})
				.CurrentOption(Category.IsEmpty() ? "Default" : Category)
				.CanEnterCustomOption(true)
				.OptionText(MakeLambdaDelegate([](FString Option)
				{
					return Option;
				}))
				.OnSelection_Lambda([CategoryHandle](FString NewValue)
				{
					CategoryHandle->SetValue(NewValue == "Default" ? "" : NewValue);
				})
			]
		];
	}

	if (TargetParameter.ParameterType == EVoxelMetaGraphParameterType::Parameter ||
		TargetParameter.ParameterType == EVoxelMetaGraphParameterType::LocalVariable)
	{
		IDetailCategoryBuilder& DefaultValueBuilder = DetailLayout.EditCategory("Default Value", VOXEL_LOCTEXT("Default Value"));
		AddPinValueCustomization(
			DefaultValueBuilder,
			DefaultValueHandle,
			TargetParameter.DefaultValue,
			"Default Value",
			"",
			[](FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)
			{
				Row
				.NameContent()
				[
					NameWidget
				]
				.ValueContent()
				[
					ValueWidget
				];
			});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FPinValueCustomization> FParameterObjectCustomization::GetSelfSharedPtr()
{
	return StaticCastSharedRef<FParameterObjectCustomization>(AsShared());
}

END_VOXEL_NAMESPACE(MetaGraph)