#pragma once

#include "CoreMinimal.h"

#include "MultiUserPing.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMultiUserPing, Log, All);

class IConcertClientSession;
struct FConcertSessionContext;

USTRUCT()
struct FMultiUserPingEvent
{
	GENERATED_BODY()

	UPROPERTY()
	FString Message;

	UPROPERTY()
	FString Sender;
};

class FMultiUserPingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void OnConcertSessionStartup(TSharedRef<IConcertClientSession> ConcertClientSession);

	void HandlePingEvent(const FConcertSessionContext&, const FMultiUserPingEvent& Event);

	FDelegateHandle StartupHandle;
};