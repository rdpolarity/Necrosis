// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class VOXELMETAGRAPH_API IVoxelNodeDefinition
{
public:
	enum class ENodeState
	{
		Pin,
		ArrayCategory,
		Category
	};

	struct FNode
	{
		const ENodeState NodeState;
		const FName Name;
		const TArray<FName> Path;
		TArray<TSharedRef<FNode>> Children;

	private:
		explicit FNode(ENodeState NodeState, const FName Name, const TArray<FName>& Path)
			: NodeState(NodeState)
			, Name(Name)
			, Path(Path)
		{
		}

	public:
		FName GetFullPath() const
		{
			if (Path.Num() == 0)
			{
				return {};
			}

			FString ResultPath = Path[0].ToString();
			for (int32 Index = 1; Index < Path.Num(); Index++)
			{
				ResultPath += "|" + Path[Index].ToString();
			}

			return FName(ResultPath);
		}

		static TSharedRef<FNode> MakePin(const FName Name, const TArray<FName>& Path)
		{
			return MakeShareable(new FNode(ENodeState::Pin, Name, Path));
		}
		static TSharedRef<FNode> MakeArrayCategory(const FName Name, const TArray<FName>& Path)
		{
			return MakeShareable(new FNode(ENodeState::ArrayCategory, Name, Path));
		}
		static TSharedRef<FNode> MakeCategory(const FName Name, const TArray<FName>& Path)
		{
			return MakeShareable(new FNode(ENodeState::Category, Name, Path));
		}

		static TArray<FName> MakePath(const FString& RawPath)
		{
			TArray<FString> StringsPath;
			RawPath.ParseIntoArray(StringsPath, TEXT("|"));

			TArray<FName> Result;
			for (const FString& Element : StringsPath)
			{
				Result.Add(FName(Element));
			}

			return Result;
		}
		static TArray<FName> MakePath(const FString& RawPath, const FName ArrayName)
		{
			TArray<FString> StringsPath;
			RawPath.ParseIntoArray(StringsPath, TEXT("|"));
			StringsPath.Add(ArrayName.ToString());

			TArray<FName> Result;
			for (const FString& Element : StringsPath)
			{
				Result.Add(FName(Element));
			}

			return Result;
		}
	};

	IVoxelNodeDefinition() = default;
	virtual ~IVoxelNodeDefinition() = default;

	virtual TSharedPtr<const FNode> GetInputs() const { return nullptr; }
	virtual TSharedPtr<const FNode> GetOutputs() const { return nullptr; }
	virtual bool OverridePinsOrder() const { return false; }

	virtual FString GetAddPinLabel() const { ensure(!CanAddInputPin()); return {}; }
	virtual FString GetAddPinTooltip() const { ensure(!CanAddInputPin()); return {}; }
	virtual FString GetRemovePinTooltip() const { ensure(!CanRemoveInputPin()); return {}; }

	virtual bool CanAddInputPin() const { return false; }
	virtual void AddInputPin() VOXEL_PURE_VIRTUAL();

	virtual bool CanRemoveInputPin() const { return false; }
	virtual void RemoveInputPin() VOXEL_PURE_VIRTUAL();

	virtual bool CanAddToCategory(FName Category) const { return false; }
	virtual void AddToCategory(FName Category) VOXEL_PURE_VIRTUAL();

	virtual bool CanRemoveFromCategory(FName Category) const { return false; }
	virtual void RemoveFromCategory(FName Category) VOXEL_PURE_VIRTUAL();

	virtual bool CanRemoveSelectedPin(FName PinName) const { return false; }
	virtual void RemoveSelectedPin(FName PinName) VOXEL_PURE_VIRTUAL();

	virtual void InsertPinBefore(FName PinName) VOXEL_PURE_VIRTUAL();
	virtual void DuplicatePin(FName PinName) VOXEL_PURE_VIRTUAL();

#if WITH_EDITOR
	virtual FString GetPinTooltip(FName PinName) const { return {}; }
	virtual FString GetCategoryTooltip(FName CategoryName) const { return {}; }
#endif
};