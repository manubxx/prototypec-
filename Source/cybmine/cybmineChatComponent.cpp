#include "cybmineChatComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "cybmineFactionSubsystem.h" // Per controllare la fazione

UcybmineChatComponent::UcybmineChatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UcybmineChatComponent::Server_SendChatMessage_Validate(const FString& Message, EChatChannel Channel)
{
	// Anti-spam e limiti base
	return Message.Len() > 0 && Message.Len() <= 256;
}

void UcybmineChatComponent::Server_SendChatMessage_Implementation(const FString& Message, EChatChannel Channel)
{
	APlayerController* MyPC = Cast<APlayerController>(GetOwner());
	if (!MyPC || !MyPC->PlayerState) return;

	FString SenderName = MyPC->PlayerState->GetPlayerName();
	FString SenderID = MyPC->PlayerState->GetUniqueId().ToString();

	UWorld* World = GetWorld();
	if (!World) return;

	// Smistamento basato sul canale
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* TargetPC = Iterator->Get();
		if (!TargetPC) continue;

		UcybmineChatComponent* TargetChatComp = TargetPC->FindComponentByClass<UcybmineChatComponent>();
		if (!TargetChatComp) continue;

		bool bShouldReceive = false;

		switch (Channel)
		{
			case EChatChannel::Global:
				bShouldReceive = true;
				break;

			case EChatChannel::Local:
			{
				if (MyPC->GetPawn() && TargetPC->GetPawn())
				{
					float Dist = FVector::Dist(MyPC->GetPawn()->GetActorLocation(), TargetPC->GetPawn()->GetActorLocation());
					if (Dist <= LocalChatRadius) bShouldReceive = true;
				}
				break;
			}

			case EChatChannel::Faction:
			{
				if (UcybmineFactionSubsystem* FactionSys = World->GetSubsystem<UcybmineFactionSubsystem>())
				{
					FGuid MyFaction = FactionSys->GetPlayerFaction(SenderID);
					FGuid TargetFaction = FactionSys->GetPlayerFaction(TargetPC->PlayerState->GetUniqueId().ToString());
					
					if (MyFaction.IsValid() && MyFaction == TargetFaction)
					{
						bShouldReceive = true;
					}
				}
				break;
			}
			default: break;
		}

		if (bShouldReceive)
		{
			TargetChatComp->Client_ReceiveChatMessage(SenderName, Message, Channel);
		}
	}
}

void UcybmineChatComponent::Client_ReceiveChatMessage_Implementation(const FString& SenderName, const FString& Message, EChatChannel Channel)
{
	// Notifica l'HUD (che si iscriverà a questo evento)
	OnChatMessageReceived.Broadcast(SenderName, Message, Channel);
}