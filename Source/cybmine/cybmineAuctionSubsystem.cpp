#include "cybmineAuctionSubsystem.h"
#include "cybmineInventoryComponent.h"
#include "cybminePlayerState.h" // Presuppone che la valuta sia qui o nel JobData

void UcybmineAuctionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

bool UcybmineAuctionSubsystem::CreateAuction(UcybmineInventoryComponent* SellerInventory, const FString& SellerID, const FString& SellerName, FGuid ItemInstanceID, int32 Price)
{
	if (!SellerInventory || Price <= 0) return false;

	// Trova l'oggetto
	FInventoryItem ItemToSell;
	bool bFound = false;

	// Simula la ricerca e rimozione atomica (richiede una funzione RemoveItemByInstanceID nel tuo InventoryComponent)
	// if (SellerInventory->RemoveItemByInstanceID(ItemInstanceID, ItemToSell)) bFound = true;
	// PER ORA: Placeholder finché non colleghiamo l'InventoryComponent
	
	if (!bFound) return false;

	FAuctionListing NewListing;
	NewListing.AuctionID = FGuid::NewGuid();
	NewListing.SellerPlayerID = SellerID;
	NewListing.SellerName = SellerName;
	NewListing.ItemToSell = ItemToSell;
	NewListing.BuyoutPrice = Price;
	NewListing.ExpirationTime = FDateTime::UtcNow() + FTimespan::FromDays(7); // Scade in 7 giorni

	ActiveAuctions.Add(NewListing.AuctionID, NewListing);
	return true;
}

bool UcybmineAuctionSubsystem::BuyoutAuction(const FGuid& AuctionID, UcybmineInventoryComponent* BuyerInventory, const FString& BuyerID)
{
	if (!ActiveAuctions.Contains(AuctionID) || !BuyerInventory) return false;

	FAuctionListing Listing = ActiveAuctions[AuctionID];

	// Non puoi comprare la tua stessa asta
	if (Listing.SellerPlayerID == BuyerID) return false;

	// TODO: Controllare che il Buyer abbia i soldi (tramite cybminePlayerState/JobData)
	bool bHasEnoughMoney = true; 

	if (bHasEnoughMoney)
	{
		// TODO: Scalare i soldi al compratore e inviarli via Mail/Subsystem al venditore
		
		// Aggiunge l'oggetto all'inventario del compratore
		// BuyerInventory->Server_RequestMoveItem(...); (adattare alla tua logica)

		ActiveAuctions.Remove(AuctionID);
		return true;
	}

	return false;
}

TArray<FAuctionListing> UcybmineAuctionSubsystem::GetActiveAuctions() const
{
	TArray<FAuctionListing> Listings;
	ActiveAuctions.GenerateValueArray(Listings);
	return Listings;
}