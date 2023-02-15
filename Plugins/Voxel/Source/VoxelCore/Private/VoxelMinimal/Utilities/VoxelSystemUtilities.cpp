// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#endif
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/COMPointer.h"
#include <commdlg.h>
#include <shlobj.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

void FVoxelSystemUtilities::DelayedCall(TFunction<void()> Call, float Delay)
{
	if (!IsInGameThread())
	{
		// Delay will be inaccurate but w/e
		AsyncTask(ENamedThreads::GameThread, [=]
		{
			DelayedCall(Call, Delay);
		});
		return;
	}
	check(IsInGameThread());
	
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=](float)
	{
		VOXEL_LLM_SCOPE();
		Call();
		return false;
	}), Delay);
}

IPlugin& FVoxelSystemUtilities::GetPlugin()
{
	return *IPluginManager::Get().FindPlugin(VOXEL_PLUGIN_NAME).ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelSystemUtilities::FileDialog(
	bool bSave, 
	const FString& DialogTitle, 
	const FString& DefaultPath, 
	const FString& DefaultFile, 
	const FString& FileTypes, 
	bool bAllowMultiple, 
	TArray<FString>& OutFilenames)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!bSave || !bAllowMultiple);

#if WITH_EDITOR
	{
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (DesktopPlatform)
		{
			if (bSave)
			{
				return DesktopPlatform->SaveFileDialog(
					ParentWindowHandle,
					DialogTitle,
					DefaultPath,
					DefaultFile,
					FileTypes,
					EFileDialogFlags::None,
					OutFilenames);
			}
			else
			{
				return DesktopPlatform->OpenFileDialog(
					ParentWindowHandle,
					DialogTitle,
					DefaultPath,
					DefaultFile,
					FileTypes,
					bAllowMultiple ? EFileDialogFlags::Multiple : EFileDialogFlags::None,
					OutFilenames);
			}
		}
	}
#endif

#if PLATFORM_WINDOWS
	TComPtr<IFileDialog> FileDialog;
	if (!SUCCEEDED(::CoCreateInstance(
		bSave ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		bSave ? IID_IFileSaveDialog : IID_IFileOpenDialog,
		IID_PPV_ARGS_Helper(&FileDialog))))
	{
		return false;
	}

	if (bSave)
	{
		// Set the default "filename"
		if (!DefaultFile.IsEmpty())
		{
			FileDialog->SetFileName(*FPaths::GetCleanFilename(DefaultFile));
		}
	}
	else
	{
		// Set this up as a multi-select picker
		if (bAllowMultiple)
		{
			DWORD dwFlags = 0;
			FileDialog->GetOptions(&dwFlags);
			FileDialog->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
		}
	}

	// Set up common settings
	FileDialog->SetTitle(*DialogTitle);
	if (!DefaultPath.IsEmpty())
	{
		// SHCreateItemFromParsingName requires the given path be absolute and use \ rather than / as our normalized paths do
		FString DefaultWindowsPath = FPaths::ConvertRelativePathToFull(DefaultPath);
		DefaultWindowsPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::CaseSensitive);

		TComPtr<IShellItem> DefaultPathItem;
		if (SUCCEEDED(::SHCreateItemFromParsingName(*DefaultWindowsPath, nullptr, IID_PPV_ARGS(&DefaultPathItem))))
		{
			FileDialog->SetFolder(DefaultPathItem);
		}
	}

	// Set-up the file type filters
	TArray<FString> UnformattedExtensions;
	TArray<COMDLG_FILTERSPEC> FileDialogFilters;
	{
		// Split the given filter string (formatted as "Pair1String1|Pair1String2|Pair2String1|Pair2String2") into the Windows specific filter struct
		FileTypes.ParseIntoArray(UnformattedExtensions, TEXT("|"), true);

		if (UnformattedExtensions.Num() % 2 == 0)
		{
			FileDialogFilters.Reserve(UnformattedExtensions.Num() / 2);
			for (int32 ExtensionIndex = 0; ExtensionIndex < UnformattedExtensions.Num();)
			{
				COMDLG_FILTERSPEC& NewFilterSpec = FileDialogFilters[FileDialogFilters.AddDefaulted()];
				NewFilterSpec.pszName = *UnformattedExtensions[ExtensionIndex++];
				NewFilterSpec.pszSpec = *UnformattedExtensions[ExtensionIndex++];
			}
		}
	}
	FileDialog->SetFileTypes(FileDialogFilters.Num(), FileDialogFilters.GetData());

	// Show the picker
	if (!SUCCEEDED(FileDialog->Show(NULL)))
	{
		return false;
	}

	int32 OutFilterIndex = 0;
	if (SUCCEEDED(FileDialog->GetFileTypeIndex((UINT*)&OutFilterIndex)))
	{
		OutFilterIndex -= 1; // GetFileTypeIndex returns a 1-based index
	}

	TFunction<void(const FString&)> AddOutFilename = [&OutFilenames](const FString& InFilename)
	{
		FString& OutFilename = OutFilenames.Add_GetRef(InFilename);
		OutFilename = IFileManager::Get().ConvertToRelativePath(*OutFilename);
		FPaths::NormalizeFilename(OutFilename);
	};

	if (bSave)
	{
		TComPtr<IShellItem> Result;
		if (!SUCCEEDED(FileDialog->GetResult(&Result)))
		{
			return false;
		}

		PWSTR pFilePath = nullptr;
		if (!SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
		{
			return false;
		}

		// Apply the selected extension if the given filename doesn't already have one
		FString SaveFilePath = pFilePath;
		if (FileDialogFilters.IsValidIndex(OutFilterIndex))
		{
			// May have multiple semi-colon separated extensions in the pattern
			const FString ExtensionPattern = FileDialogFilters[OutFilterIndex].pszSpec;
			TArray<FString> SaveExtensions;
			ExtensionPattern.ParseIntoArray(SaveExtensions, TEXT(";"));

			// Build a "clean" version of the selected extension (without the wildcard)
			FString CleanExtension = SaveExtensions[0];
			if (CleanExtension == TEXT("*.*"))
			{
				CleanExtension.Reset();
			}
			else
			{
				int32 WildCardIndex = INDEX_NONE;
				if (CleanExtension.FindChar(TEXT('*'), WildCardIndex))
				{
					CleanExtension.RightChopInline(WildCardIndex + 1, false);
				}
			}

			// We need to split these before testing the extension to avoid anything within the path being treated as a file extension
			FString SaveFileName = FPaths::GetCleanFilename(SaveFilePath);
			SaveFilePath = FPaths::GetPath(SaveFilePath);

			// Apply the extension if the file name doesn't already have one
			if (FPaths::GetExtension(SaveFileName).IsEmpty() && !CleanExtension.IsEmpty())
			{
				SaveFileName = FPaths::SetExtension(SaveFileName, CleanExtension);
			}

			SaveFilePath /= SaveFileName;
		}
		AddOutFilename(SaveFilePath);

		::CoTaskMemFree(pFilePath);

		return true;
	}
	else
	{
		IFileOpenDialog* FileOpenDialog = static_cast<IFileOpenDialog*>(FileDialog.Get());

		TComPtr<IShellItemArray> Results;
		if (!SUCCEEDED(FileOpenDialog->GetResults(&Results)))
		{
			return false;
		}

		DWORD NumResults = 0;
		Results->GetCount(&NumResults);
		for (DWORD ResultIndex = 0; ResultIndex < NumResults; ++ResultIndex)
		{
			TComPtr<IShellItem> Result;
			if (SUCCEEDED(Results->GetItemAt(ResultIndex, &Result)))
			{
				PWSTR pFilePath = nullptr;
				if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
				{
					AddOutFilename(pFilePath);
					::CoTaskMemFree(pFilePath);
				}
			}
		}

		return true;
	}
#endif

	ensure(false);
	return false;
}

FString FVoxelSystemUtilities::SaveFileDialog(
	const FString& DialogTitle, 
	const FString& DefaultPath, 
	const FString& DefaultFile, 
	const FString& FileTypes)
{
	TArray<FString> Filenames;
	if (!FileDialog(true, DialogTitle, DefaultPath, DefaultFile, FileTypes, false, Filenames))
	{
		return {};
	}

	if (!ensure(Filenames.Num() == 1))
	{
		return {};
	}

	return Filenames[0];
}

bool FVoxelSystemUtilities::OpenFileDialog(
	const FString& DialogTitle,
	const FString& DefaultPath,
	const FString& DefaultFile,
	const FString& FileTypes,
	bool bAllowMultiple,
	TArray<FString>& OutFilenames)
{
	return FileDialog(false, DialogTitle, DefaultPath, DefaultFile, FileTypes, bAllowMultiple, OutFilenames);
}