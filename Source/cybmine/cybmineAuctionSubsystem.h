#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "cybmineTypes.h"
#include "cybmineAuctionSubsystem.generated.h"

UCLASS()
class CYBMINE_API UcybmineAuctionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Crea una nuova asta, prelevando l'item dall'inventario
	UFUNCTION(BlueprintCallable, Category = "Economy|Auction")
	bool CreateAuction(class UcybmineInventoryComponent* SellerInventory, const FString& SellerID, const FString& SellerName, FGuid ItemInstanceID, int32 Price);

	// Un compratore acquista un oggetto all'asta
	UFUNCTION(BlueprintCallable, Category = "Economy|Auction")
	bool BuyoutAuction(const FGuid& AuctionID, class UcybmineInventoryComponent* BuyerInventory, const FString& BuyerID);

	// Restituisce tutte le aste attive
	UFUNCTION(BlueprintPure, Category = "Economy|Auction")
	TArray<FAuctionListing> GetActiveAuctions() const;

protected:
	// Database in RAM delle aste (solo Server)
	UPROPERTY()
	TMap<FGuid, FAuctionListing> ActiveAuctions;
};