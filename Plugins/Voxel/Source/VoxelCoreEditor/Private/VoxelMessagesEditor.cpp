// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "MessageLogModule.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

struct FVoxelMessagesEditor
{
	static void AddButton(
		FNotificationInfo& Info,
		const FSimpleDelegate& OnClick,
		const FText& Text,
		const FText& Tooltip,
		bool bCloseOnClick,
		const TSharedRef<TWeakPtr<SNotificationItem>>& PtrToPtr)
	{
		const auto Callback = FSimpleDelegate::CreateLambda([=]()
		{
			OnClick.ExecuteIfBound();

			if (bCloseOnClick)
			{
				auto Ptr = PtrToPtr->Pin();
				if (Ptr.IsValid())
				{
					Ptr->SetFadeOutDuration(0);
					Ptr->Fadeout();
				}
			}
		});
		
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			Text,
			Tooltip,
			Callback,
			SNotificationItem::CS_None));
	}

	static void LogMessage(const TSharedRef<FTokenizedMessage>& Message, bool bShowNotification)
	{
		const FText MessageText = Message->ToText();

		if (GEditor->PlayWorld || GIsPlayInEditorWorld)
		{
			FMessageLog("PIE")
			.AddMessage(Message);
		}
		else
		{
			FMessageLog("Voxel")
			.AddMessage(Message);

			if (bShowNotification)
			{
				struct FLastNotification
				{
					TWeakPtr<SNotificationItem> Ptr;
					FText Text;
					int32 Count;
				};
				static TArray<FLastNotification> LastNotifications;
				
				LastNotifications.RemoveAll([](auto& Notification) { return !Notification.Ptr.IsValid(); });
				
				for (FLastNotification& LastNotification : LastNotifications)
				{
					TSharedPtr<SNotificationItem> LastNotificationPtr = LastNotification.Ptr.Pin();
					if (ensure(LastNotificationPtr.IsValid()) && LastNotification.Text.EqualToCaseIgnored(MessageText))
					{
						LastNotification.Text = MessageText;
						LastNotification.Count++;

						LastNotificationPtr->SetText(FText::Format(VOXEL_LOCTEXT("{0} (x{1})"), MessageText, FText::AsNumber(LastNotification.Count)));
						LastNotificationPtr->ExpireAndFadeout();

						return;
					}
				}

				FNotificationInfo Info = FNotificationInfo(MessageText);
				Info.CheckBoxState = ECheckBoxState::Unchecked;
				Info.ExpireDuration = 10;
		
				const TSharedRef<TWeakPtr<SNotificationItem>> PtrToPtr = MakeShared<TWeakPtr<SNotificationItem>>();
				AddButton(Info, {}, VOXEL_LOCTEXT("Close"), VOXEL_LOCTEXT("Close"), true, PtrToPtr);
				const TSharedPtr<SNotificationItem> Ptr = FSlateNotificationManager::Get().AddNotification(Info);
				*PtrToPtr = Ptr;

				LastNotifications.Emplace(FLastNotification{ Ptr, MessageText, 1 });
			}
		}
	}

	static void ShowNotification(const FVoxelMessages::FNotification& Notification)
	{
		struct FLastNotification
		{
			TWeakPtr<SNotificationItem> Ptr;
			uint64 UniqueId;
		};
		static TArray<FLastNotification> LastNotifications;

		LastNotifications.RemoveAll([](auto& It) { return !It.Ptr.IsValid(); });
		if (const FLastNotification* LastNotification = LastNotifications.FindByPredicate([&](auto& It) { return It.UniqueId == Notification.UniqueId; }))
		{
			if (const TSharedPtr<SNotificationItem> Item = LastNotification->Ptr.Pin())
			{
				Item->SetText(FText::FromString(Notification.Message));
				Item->SetExpireDuration(Notification.Duration);
				Item->ExpireAndFadeout();
			}

			return;
		}

		FNotificationInfo Info(FText::FromString(Notification.Message));
		Info.CheckBoxState = ECheckBoxState::Unchecked;
		Info.ExpireDuration = Notification.Duration;
		
		const TSharedRef<TWeakPtr<SNotificationItem>> PtrToPtr = MakeShared<TWeakPtr<SNotificationItem>>();

		for (auto& Button : Notification.Buttons)
		{
			AddButton(Info, Button.OnClick, FText::FromString(Button.Text), FText::FromString(Button.Tooltip), Button.bCloseOnClick, PtrToPtr);
		}
		AddButton(Info, Notification.OnClose, VOXEL_LOCTEXT("Close"), VOXEL_LOCTEXT("Close"), true, PtrToPtr);

		const auto Ptr = FSlateNotificationManager::Get().AddNotification(Info);
		*PtrToPtr = Ptr;

		LastNotifications.Add({ Ptr, Notification.UniqueId });
	}
};

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterMessageEditor)
{
	FVoxelMessages::OnMessageLogged.AddLambda([](const TSharedRef<FTokenizedMessage>& Message, bool bShowNotification)
	{
		FVoxelMessagesEditor::LogMessage(Message, bShowNotification);
	});
	FVoxelMessages::OnShowNotification.AddStatic([](const FVoxelMessages::FNotification& Notification)
	{
		// Delay by one frame to work on startup
		FVoxelSystemUtilities::DelayedCall([=]()
		{
			FVoxelMessagesEditor::ShowNotification(Notification);
		});
	});

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	FMessageLogInitializationOptions InitOptions;
	InitOptions.bShowFilters = true;
	InitOptions.bShowPages = false;
	InitOptions.bAllowClear = true;
	MessageLogModule.RegisterLogListing("Voxel", VOXEL_LOCTEXT("Voxel"), InitOptions);
}