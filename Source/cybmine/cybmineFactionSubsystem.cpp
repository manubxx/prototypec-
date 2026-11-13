#include "cybmineFactionSubsystem.h"

void UcybmineFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Qui potrai in futuro caricare i dati dal Database o dai SaveGame
}

void UcybmineFactionSubsystem::Deinitialize()
{
	// Qui salverai i dati al momento dello spegnimento del server
	Super::Deinitialize();
}

bool UcybmineFactionSubsystem::CreateFaction(const FString& FactionName, const FString& CreatorPlayerId, const FString& CreatorName, FGuid& OutFactionID)
{
	// Controllo se il giocatore è già in una fazione
	if (PlayerToFactionMap.Contains(CreatorPlayerId))
	{
		return false; 
	}

	OutFactionID = FGuid::NewGuid();

	FFactionData NewFaction;
	NewFaction.FactionID = OutFactionID;
	NewFaction.FactionName = FactionName;
	NewFaction.BankBalance = 0;

	// Aggiunge il creatore come Leader
	NewFaction.Members.Add(CreatorPlayerId, FFactionMember(CreatorName, EFactionRank::Leader));

	ActiveFactions.Add(OutFactionID, NewFaction);
	PlayerToFactionMap.Add(CreatorPlayerId, OutFactionID);

	return true;
}

bool UcybmineFactionSubsystem::JoinFaction(const FGuid& FactionID, const FString& PlayerId, const FString& PlayerName)
{
	if (PlayerToFactionMap.Contains(PlayerId) || !ActiveFactions.Contains(FactionID))
	{
		return false; // Già in una fazione o fazione inesistente
	}

	ActiveFactions[FactionID].Members.Add(PlayerId, FFactionMember(PlayerName, EFactionRank::Recruit));
	PlayerToFactionMap.Add(PlayerId, FactionID);

	return true;
}

FGuid UcybmineFactionSubsystem::GetPlayerFaction(const FString& PlayerId) const
{
	if (const FGuid* FoundID = PlayerToFactionMap.Find(PlayerId))
	{
		return *FoundID;
	}
	return FGuid(); // Guid vuoto se non ha fazione
}

FIntPoint UcybmineFactionSubsystem::GetChunkCoordsFromLocation(const FVector& Location) const
{
	// Divide le coordinate del mondo per la grandezza del chunk per ottenere la griglia
	return FIntPoint(
		FMath::FloorToInt(Location.X / ChunkSize),
		FMath::FloorToInt(Location.Y / ChunkSize)
	);
}

bool UcybmineFactionSubsystem::ClaimTerritory(const FVector& WorldLocation, const FGuid& FactionID)
{
	if (!ActiveFactions.Contains(FactionID)) return false;

	FIntPoint Chunk = GetChunkCoordsFromLocation(WorldLocation);

	// Se è già claimato da qualcun altro, fallisce
	if (ClaimedTerritories.Contains(Chunk))
	{
		return false;
	}

	ClaimedTerritories.Add(Chunk, FactionID);
	return true;
}

bool UcybmineFactionSubsystem::UnclaimTerritory(const FVector& WorldLocation, const FGuid& FactionID)
{
	FIntPoint Chunk = GetChunkCoordsFromLocation(WorldLocation);

	if (const FGuid* OwnerFaction = ClaimedTerritories.Find(Chunk))
	{
		if (*OwnerFaction == FactionID)
		{
			ClaimedTerritories.Remove(Chunk);
			return true;
		}
	}
	return false;
}

bool UcybmineFactionSubsystem::CanPlayerInteractAtLocation(const FString& PlayerId, const FVector& WorldLocation) const
{
	FIntPoint Chunk = GetChunkCoordsFromLocation(WorldLocation);

	// Se il territorio non è di nessuno (Wilderness), tutti possono interagire
	if (!ClaimedTerritories.Contains(Chunk))
	{
		return true; 
	}

	FGuid TerritoryOwnerID = ClaimedTerritories[Chunk];
	FGuid PlayerFactionID = GetPlayerFaction(PlayerId);

	// Se la fazione del giocatore coincide con la fazione del territorio, l'interazione è consentita
	return (TerritoryOwnerID == PlayerFactionID);
}