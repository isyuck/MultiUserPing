#include "MultiUserPing.h"

#include "IConcertSyncClient.h"
#include "IConcertSyncClientModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/IConsoleManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY(LogMultiUserPing);

#define LOCTEXT_NAMESPACE "FMultiUserPingModule"

void FMultiUserPingModule::StartupModule()
{
	if (TSharedPtr<IConcertSyncClient> ConcertSyncClient = IConcertSyncClientModule::Get().GetClient(TEXT("MultiUser")))
	{
		const IConcertClientRef ConcertClient = ConcertSyncClient->GetConcertClient();
		StartupHandle = ConcertClient->OnSessionStartup().AddRaw(this, &FMultiUserPingModule::OnConcertSessionStartup);

		if (const TSharedPtr<IConcertClientSession> Session = ConcertClient->GetCurrentSession())
		{
			OnConcertSessionStartup(Session.ToSharedRef());
		}
	}
}

void FMultiUserPingModule::ShutdownModule()
{
	if (TSharedPtr<IConcertSyncClient> ConcertSyncClient = IConcertSyncClientModule::Get().GetClient(TEXT("MultiUser")))
	{
		ConcertSyncClient->GetConcertClient()->OnSessionStartup().Remove(StartupHandle);
	}
}

void FMultiUserPingModule::OnConcertSessionStartup(TSharedRef<IConcertClientSession> ConcertClientSession)
{
	if (TSharedPtr<IConcertClientSession> PinnedSession = ConcertClientSession.ToSharedPtr())
	{
		ConcertClientSession->RegisterCustomEventHandler<FMultiUserPingEvent>(
			this, &FMultiUserPingModule::HandlePingEvent);
	}
}

void FMultiUserPingModule::HandlePingEvent(const FConcertSessionContext&, const FMultiUserPingEvent& Event)
{
	FNotificationInfo Info(FText::FromString(Event.Message));
	Info.SubText = FText::FromString(Event.Sender);
	FSlateNotificationManager::Get().AddNotification(Info);
}

static FAutoConsoleCommandWithWorldAndArgs GMultiUserPing(
	TEXT("muping"),
	TEXT("Send messages to other clients in a multi-user session"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
	{
		TSharedPtr<IConcertSyncClient> ConcertSyncClient = IConcertSyncClientModule::Get().GetClient(TEXT("MultiUser"));
		if (!ConcertSyncClient)
		{
			UE_LOG(LogMultiUserPing, Error, TEXT("No multi-user client"));
			return;
		}

		const TSharedPtr<IConcertClientSession> Session = ConcertSyncClient->GetConcertClient()->GetCurrentSession();
		if (!Session)
		{
			UE_LOG(LogMultiUserPing, Error, TEXT("Not in a multi-user session"));
			return;
		}

		if (Args.Num() <= 1)
		{
			UE_LOG(LogMultiUserPing, Error, TEXT("No user or no message. Format: \"muping user message\""));
			UE_LOG(LogMultiUserPing, Display, TEXT("(muping expects the display name of the user.)"));
			return;
		}

		const FString& Username = Args[0];

		// everything after the first arg is part of the message
		FString Message;
		for (int32 i = 1; i < Args.Num(); i++)
		{
			Message += Args[i] + ' ';
		}
		Message.RemoveAt(Message.Len() - 1);

		// find the destination client based on display name of the client
		for (const FConcertSessionClientInfo& Client : Session->GetSessionClients())
		{
			if (Client.ClientInfo.DisplayName.Equals(Username, ESearchCase::IgnoreCase))
			{
				// ^ found the desired client, now construct and send the event
				FMultiUserPingEvent Event;
				Event.Message = Message;
				Event.Sender = Session->GetLocalClientInfo().DisplayName;

				UE_LOG(LogMultiUserPing, Verbose, TEXT("sending message %s to %s"),
					*Event.Message, *Client.ClientEndpointId.ToString());

				Session->SendCustomEvent(Event, Client.ClientEndpointId, EConcertMessageFlags::None);

				return;
			}
		}

		UE_LOG(LogMultiUserPing, Error, TEXT("could not find client (user) with name %s"), *Username);
	})
);

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMultiUserPingModule, MultiUserPing)