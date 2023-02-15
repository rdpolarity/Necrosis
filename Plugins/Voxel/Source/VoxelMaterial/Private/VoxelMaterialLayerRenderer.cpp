// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialLayerRenderer.h"
#include "Misc/UObjectToken.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelMaterialLayerRenderer);

void FVoxelMaterialLayerRenderer::AddReferencedObjects(FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Collector);

	for (const auto& ClassDataIt : ClassDatas)
	{
		for (auto& ParameterIt : ClassDataIt.Value->Parameters)
		{
			ParameterIt.Value.AddStructReferencedObjects(Collector);
		}
	}
}

void FVoxelMaterialLayerRenderer::Tick()
{
	Super::Tick();

	if (!bHasPendingChanges)
	{
		return;
	}
	bHasPendingChanges = false;

	VOXEL_SCOPE_LOCK(CriticalSection);

	for (auto& ClassDataIt : ClassDatas)
	{
		FClassData& ClassData = *ClassDataIt.Value;

		UMaterialInstanceDynamic* MaterialInstance = ClassData.MaterialInstance->GetMaterialInstance();
		if (!ensure(MaterialInstance))
		{
			continue;
		}

		for (auto& ParameterIt : ClassData.Parameters)
		{
			FVoxelMaterialParameterData& Parameter = *ParameterIt.Value;
			Parameter.FlushChanges();
			Parameter.SetupMaterial(*MaterialInstance);
		}
	}
}

FVoxelMaterialLayer FVoxelMaterialLayerRenderer::RegisterLayer_GameThread(const UVoxelMaterialLayerAsset* Layer)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(Layer);

	VOXEL_SCOPE_LOCK(CriticalSection);

	UClass* Class = Layer->GetClass();
	TSharedPtr<FClassData>& ClassData = ClassDatas.FindOrAdd(Class);

	if (!ClassData)
	{
		uint8 ClassIndex = 0;
		if (ClassIndexCounter < 256)
		{
			ClassIndex = ClassIndexCounter++;
		}
		else
		{
			VOXEL_MESSAGE(Error, "{0}: More than 256 materials being rendered", GetActor());
		}
		ClassToClassIndex.Add(Class, ClassIndex);
		ClassIndexToClass.Add(ClassIndex, Class);

		UMaterialInterface* Material = Class->GetDefaultObject<UVoxelMaterialLayerAsset>()->Material;
		if (!Material)
		{
			VOXEL_MESSAGE(Error, "{0}: Material is null", Class);
		}
		ClassData = MakeShared<FClassData>(Class, FVoxelMaterialRef::MakeInstance(Material));

		for (const FProperty& Property : GetClassProperties(Class))
		{
			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			if (!StructProperty ||
				!StructProperty->Struct->IsChildOf(FVoxelMaterialParameter::StaticStruct()))
			{
				continue;
			}

			UScriptStruct* ParameterDataStruct = FindObjectChecked<UScriptStruct>(nullptr, *(StructProperty->Struct->GetPathName() + "Data"));

			TVoxelInstancedStruct<FVoxelMaterialParameterData> ParameterData(ParameterDataStruct);
			ParameterData->Initialize(
				Property.GetFName(),
				*Property.ContainerPtrToValuePtr<FVoxelMaterialParameter>(Class->GetDefaultObject()));

			ClassData->Parameters.Add(Property.GetFName(), MoveTemp(ParameterData));
		}
	}

	if (!ClassData->LayerIndices.Contains(Layer))
	{
		bHasPendingChanges = true;

		for (auto& It : ClassData->Parameters)
		{
			const FProperty* Property = Class->FindPropertyByName(It.Key);
			check(Property);
			
			FVoxelScopedMessageConsumer ScopedMessageConsumer([&](const TSharedRef<FTokenizedMessage>& Message)
			{
				VOXEL_CONST_CAST(Message->GetMessageTokens()).Insert(FUObjectToken::Create(Layer), 1);
				VOXEL_CONST_CAST(Message->GetMessageTokens()).Insert(FTextToken::Create(FText::FromString("." + Property->GetName() + ": ")), 2);

				FVoxelMessages::LogMessage(Message);
			});

			It.Value->AddParameter(*Property->ContainerPtrToValuePtr<FVoxelMaterialParameter>(Layer));
		}

		uint8 LayerIndex = 0;
		if (ClassData->LayerIndexCounter < 256)
		{
			LayerIndex = ClassData->LayerIndexCounter++;
		}
		else
		{
			VOXEL_MESSAGE(Error, "{0}: More than 256 layers being rendered as {1}", GetActor(), Class);
		}
		ClassData->LayerIndices.Add(Layer, LayerIndex);
	}

	return { ClassToClassIndex[Class], ClassData->LayerIndices[Layer]};
}

TSharedRef<FVoxelMaterialRef> FVoxelMaterialLayerRenderer::GetMaterialInstance_AnyThread(const uint8 ClassIndex) const
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	const TSubclassOf<UVoxelMaterialLayerAsset> Class = ClassIndexToClass.FindRef(ClassIndex);
	if (!Class)
	{
		return FVoxelMaterialRef::Default();
	}

	return ClassDatas[Class]->MaterialInstance;
}