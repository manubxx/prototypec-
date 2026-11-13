#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "cybmineTypes.h"
#include "cybmineFactionSubsystem.generated.h"

UCLASS()
class CYBMINE_API UcybmineFactionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- GESTIONE FAZIONI ---
	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool CreateFaction(const FString& FactionName, const FString& CreatorPlayerId, const FString& CreatorName, FGuid& OutFactionID);

	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool JoinFaction(const FGuid& FactionID, const FString& PlayerId, const FString& PlayerName);

	UFUNCTION(BlueprintCallable, Category = "Factions")
	FGuid GetPlayerFaction(const FString& PlayerId) const;

	// --- GESTIONE TERRITORI (CLAIM) ---
	// La dimensione di un singolo chunk/claim (es. 2000 unità = 20 metri)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factions|Claim")
	float ChunkSize = 2000.0f;

	UFUNCTION(BlueprintCallable, Category = "Factions|Claim")
	bool ClaimTerritory(const FVector& WorldLocation, const FGuid& FactionID);

	UFUNCTION(BlueprintCallable, Category = "Factions|Claim")
	bool UnclaimTerritory(const FVector& WorldLocation, const FGuid& FactionID);

	// Verifica se un giocatore può interagire (costruire/distruggere) in una specifica location
	UFUNCTION(BlueprintCallable, Category = "Factions|Permissions")
	bool CanPlayerInteractAtLocation(const FString& PlayerId, const FVector& WorldLocation) const;

	// Converte una coordinata del mondo nelle coordinate della griglia chunk
	UFUNCTION(BlueprintPure, Category = "Factions|Claim")
	FIntPoint GetChunkCoordsFromLocation(const FVector& Location) const;

protected:
	// Database in RAM di tutte le fazioni attive sul server
	UPROPERTY()
	TMap<FGuid, FFactionData> ActiveFactions;

	// Mappa dei territori claimati: Chiave = Coordinate Chunk, Valore = FactionID
	UPROPERTY()
	TMap<FIntPoint, FGuid> ClaimedTerritories;

	// Mappa rapida per trovare a quale fazione appartiene un giocatore
	UPROPERTY()
	TMap<FString, FGuid> PlayerToFactionMap;
};