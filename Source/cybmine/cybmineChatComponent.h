#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "cybmineTypes.h"
#include "cybmineChatComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CYBMINE_API UcybmineChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UcybmineChatComponent();

	// Invia un messaggio dal Client al Server
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Chat")
	void Server_SendChatMessage(const FString& Message, EChatChannel Channel);

	// Il Server instrada il messaggio a questo Client
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Chat")
	void Client_ReceiveChatMessage(const FString& SenderName, const FString& Message, EChatChannel Channel);

	// Evento per aggiornare l'HUD in Blueprint o C++
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChatMessageReceived, const FString&, SenderName, const FString&, Message, EChatChannel, Channel);
	
	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FOnChatMessageReceived OnChatMessageReceived;

protected:
	UPROPERTY(EditAnywhere, Category = "Chat|Settings")
	float LocalChatRadius = 3000.0f; // Raggio in unità (30 metri) per la chat locale
};