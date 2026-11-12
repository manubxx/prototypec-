#include "cybmineInventoryComponent.h"
#include "cybmineItemData.h"
#include "cybmineInventoryOwnerInterface.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

// ============================================================================
// CALLBACKS DI FFastArraySerializerItem
// ============================================================================

void FCybmineInventorySlotEntry::PreReplicatedRemove(const FCybmineInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifySlotReplicated(SlotIndex, FInventoryItem(), InArraySerializer.bIsArmorList);
	}
}

void FCybmineInventorySlotEntry::PostReplicatedAdd(const FCybmineInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifySlotReplicated(SlotIndex, Item, InArraySerializer.bIsArmorList);
	}
}

void FCybmineInventorySlotEntry::PostReplicatedChange(const FCybmineInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		InArraySerializer.OwnerComponent->NotifySlotReplicated(SlotIndex, Item, InArraySerializer.bIsArmorList);
	}
}

// ============================================================================
// UcybmineInventoryComponent
// ============================================================================

UcybmineInventoryComponent::UcybmineInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	StorageList.OwnerComponent = this;
	StorageList.bIsArmorList = false;

	ArmorList.OwnerComponent = this;
	ArmorList.bIsArmorList = true;
}

void UcybmineInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UcybmineInventoryComponent, SelectedHotbarSlot);
	DOREPLIFETIME(UcybmineInventoryComponent, StorageList);
	DOREPLIFETIME(UcybmineInventoryComponent, ArmorList);
}

void UcybmineInventoryComponent::InitializeSlotArrays()
{
	StorageList.OwnerComponent = this;
	StorageList.bIsArmorList = false;

	ArmorList.OwnerComponent = this;
	ArmorList.bIsArmorList = true;

	// Se non è l'inventario di un giocatore, disabilita automaticamente hotbar e armatura
	if (ContainerType != ECybmineContainerType::PlayerInventory)
	{
		bHasArmorSlots = false;
		bHasHotbar = false;
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const int32 DesiredSlots = FMath::Max(1, MaxSlotCount);
		if (StorageList.Entries.Num() != DesiredSlots)
		{
			StorageList.Entries.SetNum(DesiredSlots);
			for (int32 i = 0; i < DesiredSlots; ++i)
			{
				StorageList.Entries[i].SlotIndex = i;
				StorageList.Entries[i].Item.Clear();
			}
			StorageList.MarkArrayDirty();
		}

		const int32 DesiredArmor = bHasArmorSlots ? CybmineConstants::ArmorSlotCount : 0;
		if (ArmorList.Entries.Num() != DesiredArmor)
		{
			ArmorList.Entries.SetNum(DesiredArmor);
			for (int32 i = 0; i < DesiredArmor; ++i)
			{
				ArmorList.Entries[i].SlotIndex = i;
				ArmorList.Entries[i].Item.Clear();
			}
			ArmorList.MarkArrayDirty();
		}
	}
}

void UcybmineInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeSlotArrays();

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		Manager->LoadPrimaryAssetsWithType(FPrimaryAssetType("ItemData"));
	}

	if (bHasHotbar)
	{
		BroadcastEquippedItemChanged();
	}
}

void UcybmineInventoryComponent::NotifySlotReplicated(int32 SlotIndex, const FInventoryItem& UpdatedItem, bool bIsArmor)
{
	OnInventorySlotUpdated.Broadcast(SlotIndex, UpdatedItem, bIsArmor);

	if (bHasHotbar && !bIsArmor && SlotIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}
}

FInventoryItem UcybmineInventoryComponent::GetStorageItem(int32 Index) const
{
	if (StorageList.Entries.IsValidIndex(Index))
	{
		return StorageList.Entries[Index].Item;
	}

	for (const FCybmineInventorySlotEntry& Entry : StorageList.Entries)
	{
		if (Entry.SlotIndex == Index)
		{
			return Entry.Item;
		}
	}
	return FInventoryItem();
}

FInventoryItem UcybmineInventoryComponent::GetArmorItem(int32 Index) const
{
	if (!bHasArmorSlots) return FInventoryItem();

	if (ArmorList.Entries.IsValidIndex(Index))
	{
		return ArmorList.Entries[Index].Item;
	}

	for (const FCybmineInventorySlotEntry& Entry : ArmorList.Entries)
	{
		if (Entry.SlotIndex == Index)
		{
			return Entry.Item;
		}
	}
	return FInventoryItem();
}

UcybmineItemData* UcybmineInventoryComponent::GetItemData(const FPrimaryAssetId& ItemID)
{
	if (!ItemID.IsValid()) return nullptr;

	if (UcybmineItemData* CodeItem = UcybmineItemData::FindCodeItem(ItemID))
	{
		return CodeItem;
	}

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		return Cast<UcybmineItemData>(Manager->GetPrimaryAssetObject(ItemID));
	}

	return nullptr;
}

void UcybmineInventoryComponent::GetItemDataAsync(const FPrimaryAssetId& ItemID, FOnItemDataLoaded OnLoaded)
{
	if (!ItemID.IsValid())
	{
		OnLoaded.ExecuteIfBound(nullptr);
		return;
	}

	if (UcybmineItemData* AlreadyLoaded = GetItemData(ItemID))
	{
		OnLoaded.ExecuteIfBound(AlreadyLoaded);
		return;
	}

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		Manager->LoadPrimaryAsset(ItemID, TArray<FName>(), FStreamableDelegate::CreateLambda([ItemID, OnLoaded]()
			{
				OnLoaded.ExecuteIfBound(GetItemData(ItemID));
			}));
	}
	else
	{
		OnLoaded.ExecuteIfBound(nullptr);
	}
}

void UcybmineInventoryComponent::OnRep_SelectedHotbarSlot()
{
	BroadcastEquippedItemChanged();
}

void UcybmineInventoryComponent::BroadcastEquippedItemChanged()
{
	if (bHasHotbar)
	{
		OnEquippedItemChanged.Broadcast(GetSelectedHotbarItem());
	}
}

// ============================================================================
// VALIDAZIONE SERVER-AUTHORITATIVE
// ============================================================================

bool UcybmineInventoryComponent::ValidateServerAccess(AActor* RequestingActor) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !RequestingActor)
	{
		return false;
	}

	// Risolvi il Pawn del giocatore che ha effettuato la richiesta
	APawn* RequestingPawn = Cast<APawn>(RequestingActor);
	if (!RequestingPawn)
	{
		if (AController* PC = Cast<AController>(RequestingActor))
		{
			RequestingPawn = PC->GetPawn();
		}
		else if (APlayerState* PS = Cast<APlayerState>(RequestingActor))
		{
			RequestingPawn = PS->GetPawn();
		}
	}

	// 1. Caso PlayerInventory: solo il giocatore stesso può toccare il proprio zaino
	if (ContainerType == ECybmineContainerType::PlayerInventory)
	{
		if (OwnerActor != RequestingPawn && OwnerActor != RequestingActor)
		{
			return false;
		}
	}
	// 2. Caso AuctionStorage (attaccato a PlayerState): solo il proprietario del PlayerState può accedervi
	else if (ContainerType == ECybmineContainerType::AuctionStorage)
	{
		APlayerState* OwnerPS = Cast<APlayerState>(OwnerActor);
		if (!OwnerPS || (RequestingPawn && OwnerPS->GetPawn() != RequestingPawn))
		{
			return false;
		}
	}
	// 3. Caso FactionChest / LootBag nel mondo: controllo distanza fisica (anti-teleport/reach hack)
	else
	{
		if (!RequestingPawn)
		{
			return false;
		}

		const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), RequestingPawn->GetActorLocation());
		if (DistSq > FMath::Square(MaxInteractionDistance))
		{
			return false;
		}
	}

	// 4. Controllo permessi personalizzati (es. fazione proprietaria della cassa) tramite Interfaccia
	if (OwnerActor->GetClass()->ImplementsInterface(UcybmineInventoryOwnerInterface::StaticClass()))
	{
		if (IcybmineInventoryOwnerInterface* OwnerInterface = Cast<IcybmineInventoryOwnerInterface>(OwnerActor))
		{
			if (!OwnerInterface->CanActorAccessInventory(RequestingPawn ? static_cast<AActor*>(RequestingPawn) : RequestingActor))
			{
				return false;
			}
		}
	}

	return true;
}

// ============================================================================
// CONTROLLO HOTBAR (Server Authoritative)
// ============================================================================

void UcybmineInventoryComponent::SelectHotbarSlot(int32 SlotIndex)
{
	if (!bHasHotbar || bIsInventoryOpen) return;

	const int32 Clamped = FMath::Clamp(SlotIndex, 0, CybmineConstants::HotbarSlotCount - 1);
	if (SelectedHotbarSlot == Clamped) return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SelectedHotbarSlot = Clamped;
		BroadcastEquippedItemChanged();
	}
	else
	{
		// Il client non modifica lo stato autoritativo, chiede al server
		Server_SelectHotbarSlot(Clamped);
	}
}

bool UcybmineInventoryComponent::Server_SelectHotbarSlot_Validate(int32 SlotIndex)
{
	return SlotIndex >= 0 && SlotIndex < CybmineConstants::HotbarSlotCount;
}

void UcybmineInventoryComponent::Server_SelectHotbarSlot_Implementation(int32 SlotIndex)
{
	if (!bHasHotbar || !ValidateServerAccess(GetOwner())) return;

	const int32 Clamped = FMath::Clamp(SlotIndex, 0, CybmineConstants::HotbarSlotCount - 1);
	if (SelectedHotbarSlot != Clamped)
	{
		SelectedHotbarSlot = Clamped;
		BroadcastEquippedItemChanged();
	}
}

void UcybmineInventoryComponent::NextHotbarSlot()
{
	if (!bHasHotbar || bIsInventoryOpen) return;
	SelectHotbarSlot((SelectedHotbarSlot + 1) % CybmineConstants::HotbarSlotCount);
}

void UcybmineInventoryComponent::PrevHotbarSlot()
{
	if (!bHasHotbar || bIsInventoryOpen) return;
	SelectHotbarSlot((SelectedHotbarSlot - 1 + CybmineConstants::HotbarSlotCount) % CybmineConstants::HotbarSlotCount);
}

FInventoryItem UcybmineInventoryComponent::GetSelectedHotbarItem() const
{
	if (!bHasHotbar) return FInventoryItem();
	return GetStorageItem(SelectedHotbarSlot);
}

// ============================================================================
// RICHIESTE RPC CLIENT -> SERVER (Spostamento, Drop, Armatura)
// ============================================================================

void UcybmineInventoryComponent::RequestMoveItem(UcybmineInventoryComponent* TargetContainer, int32 SourceSlot, int32 TargetSlot, int32 QuantityToMove)
{
	UcybmineInventoryComponent* Dest = TargetContainer ? TargetContainer : this;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ExecuteAtomicTransfer(GetOwner(), this, SourceSlot, Dest, TargetSlot, QuantityToMove);
	}
	else
	{
		Server_RequestMoveItem(Dest, SourceSlot, TargetSlot, QuantityToMove);
	}
}

bool UcybmineInventoryComponent::Server_RequestMoveItem_Validate(UcybmineInventoryComponent* TargetContainer, int32 SourceSlot, int32 TargetSlot, int32 QuantityToMove)
{
	return SourceSlot >= 0 && TargetSlot >= 0 && (QuantityToMove == -1 || QuantityToMove > 0);
}

void UcybmineInventoryComponent::Server_RequestMoveItem_Implementation(UcybmineInventoryComponent* TargetContainer, int32 SourceSlot, int32 TargetSlot, int32 QuantityToMove)
{
	UcybmineInventoryComponent* Dest = TargetContainer ? TargetContainer : this;
	ExecuteAtomicTransfer(GetOwner(), this, SourceSlot, Dest, TargetSlot, QuantityToMove);
}

void UcybmineInventoryComponent::SwapStorageSlots(int32 IndexA, int32 IndexB)
{
	RequestMoveItem(this, IndexA, IndexB, -1);
}

void UcybmineInventoryComponent::RequestDropItem(int32 SlotIndex, int32 QuantityToDrop)
{
	if (QuantityToDrop <= 0) return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Server_RequestDropItem_Implementation(SlotIndex, QuantityToDrop);
	}
	else
	{
		Server_RequestDropItem(SlotIndex, QuantityToDrop);
	}
}

bool UcybmineInventoryComponent::Server_RequestDropItem_Validate(int32 SlotIndex, int32 QuantityToDrop)
{
	return SlotIndex >= 0 && QuantityToDrop > 0;
}

void UcybmineInventoryComponent::Server_RequestDropItem_Implementation(int32 SlotIndex, int32 QuantityToDrop)
{
	if (bTransactionInProgress || !ValidateServerAccess(GetOwner())) return;
	if (!StorageList.Entries.IsValidIndex(SlotIndex)) return;

	bTransactionInProgress = true;

	FCybmineInventorySlotEntry& Entry = StorageList.Entries[SlotIndex];
	if (Entry.Item.IsEmpty())
	{
		bTransactionInProgress = false;
		return;
	}

	const int32 ActualDropQty = FMath::Min(Entry.Item.Quantity, QuantityToDrop);

	FInventoryItem DroppedItem = Entry.Item;
	DroppedItem.Quantity = ActualDropQty;

	Entry.Item.Quantity -= ActualDropQty;
	if (Entry.Item.Quantity <= 0)
	{
		Entry.Item.Clear();
	}

	StorageList.MarkItemDirty(Entry);
	OnInventorySlotUpdated.Broadcast(SlotIndex, Entry.Item, false);

	if (bHasHotbar && SlotIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}

	bTransactionInProgress = false;

	// Calcola posizione di drop davanti al proprietario
	AActor* OwnerActor = GetOwner();
	const FVector DropLoc = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * 110.0f) + FVector(0.0f, 0.0f, 30.0f);
	const FRotator DropRot = OwnerActor->GetActorRotation();

	OnItemDroppedWorld.Broadcast(DroppedItem, DropLoc, DropRot);

	if (OwnerActor->GetClass()->ImplementsInterface(UcybmineInventoryOwnerInterface::StaticClass()))
	{
		if (IcybmineInventoryOwnerInterface* OwnerInterface = Cast<IcybmineInventoryOwnerInterface>(OwnerActor))
		{
			OwnerInterface->OnItemDroppedFromInventory(DroppedItem, DropLoc, DropRot);
		}
	}
}

bool UcybmineInventoryComponent::EquipArmorFromStorage(int32 StorageIndex, int32 ArmorIndex)
{
	if (!bHasArmorSlots) return false;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Server_RequestEquipArmor_Implementation(StorageIndex, ArmorIndex);
		return true;
	}
	else
	{
		Server_RequestEquipArmor(StorageIndex, ArmorIndex);
		return true;
	}
}

bool UcybmineInventoryComponent::Server_RequestEquipArmor_Validate(int32 StorageIndex, int32 ArmorIndex)
{
	return StorageIndex >= 0 && ArmorIndex >= 0 && ArmorIndex < CybmineConstants::ArmorSlotCount;
}

void UcybmineInventoryComponent::Server_RequestEquipArmor_Implementation(int32 StorageIndex, int32 ArmorIndex)
{
	if (!bHasArmorSlots || bTransactionInProgress || !ValidateServerAccess(GetOwner())) return;
	if (!StorageList.Entries.IsValidIndex(StorageIndex) || !ArmorList.Entries.IsValidIndex(ArmorIndex)) return;

	bTransactionInProgress = true;

	const FInventoryItem& Item = StorageList.Entries[StorageIndex].Item;
	if (!Item.IsEmpty())
	{
		UcybmineItemData* StaticData = GetItemData(Item.ItemID);
		if (!StaticData)
		{
			bTransactionInProgress = false;
			return;
		}

		const bool bValidMatch =
			(ArmorIndex == 0 && StaticData->Category == EItemCategory::Helmet) ||
			(ArmorIndex == 1 && StaticData->Category == EItemCategory::Chestplate) ||
			(ArmorIndex == 2 && StaticData->Category == EItemCategory::Leggings) ||
			(ArmorIndex == 3 && StaticData->Category == EItemCategory::Boots);

		if (!bValidMatch)
		{
			bTransactionInProgress = false;
			return;
		}
	}

	FInventoryItem Temp = ArmorList.Entries[ArmorIndex].Item;
	ArmorList.Entries[ArmorIndex].Item = StorageList.Entries[StorageIndex].Item;
	StorageList.Entries[StorageIndex].Item = Temp;

	StorageList.MarkItemDirty(StorageList.Entries[StorageIndex]);
	ArmorList.MarkItemDirty(ArmorList.Entries[ArmorIndex]);

	OnInventorySlotUpdated.Broadcast(StorageIndex, StorageList.Entries[StorageIndex].Item, false);
	OnInventorySlotUpdated.Broadcast(ArmorIndex, ArmorList.Entries[ArmorIndex].Item, true);

	if (bHasHotbar && StorageIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}

	bTransactionInProgress = false;
}

// ============================================================================
// TRANSAZIONE ATOMICA UNIVERSALE (Anti-Dupe tra due Contenitori)
// ============================================================================

bool UcybmineInventoryComponent::ExecuteAtomicTransfer(
	AActor* RequestingActor,
	UcybmineInventoryComponent* SourceComp,
	int32 SourceSlot,
	UcybmineInventoryComponent* DestComp,
	int32 DestSlot,
	int32 QuantityToMove)
{
	if (!SourceComp || !DestComp || !RequestingActor)
	{
		return false;
	}

	// Evita auto-scambio sullo stesso identico slot
	if (SourceComp == DestComp && SourceSlot == DestSlot)
	{
		return false;
	}

	// Lock atomico anti-duplicazione su entrambi i contenitori
	if (SourceComp->bTransactionInProgress || DestComp->bTransactionInProgress)
	{
		return false;
	}

	// Validazione permessi, proprietà e distanza su ENTRAMBI i contenitori
	if (!SourceComp->ValidateServerAccess(RequestingActor) || !DestComp->ValidateServerAccess(RequestingActor))
	{
		return false;
	}

	if (!SourceComp->StorageList.Entries.IsValidIndex(SourceSlot) || !DestComp->StorageList.Entries.IsValidIndex(DestSlot))
	{
		return false;
	}

	SourceComp->bTransactionInProgress = true;
	if (DestComp != SourceComp)
	{
		DestComp->bTransactionInProgress = true;
	}

	auto ReleaseLocks = [SourceComp, DestComp]()
		{
			SourceComp->bTransactionInProgress = false;
			if (DestComp != SourceComp)
			{
				DestComp->bTransactionInProgress = false;
			}
		};

	FCybmineInventorySlotEntry& SrcEntry = SourceComp->StorageList.Entries[SourceSlot];
	FCybmineInventorySlotEntry& DstEntry = DestComp->StorageList.Entries[DestSlot];

	if (SrcEntry.Item.IsEmpty())
	{
		ReleaseLocks();
		return false;
	}

	UcybmineItemData* SrcStaticData = GetItemData(SrcEntry.Item.ItemID);
	if (!SrcStaticData)
	{
		ReleaseLocks();
		return false;
	}

	const int32 MaxStack = FMath::Max(1, SrcStaticData->MaxStackSize);
	const int32 MoveAmount = (QuantityToMove < 0) ? SrcEntry.Item.Quantity : FMath::Min(QuantityToMove, SrcEntry.Item.Quantity);

	if (MoveAmount <= 0)
	{
		ReleaseLocks();
		return false;
	}

	// CASO A: Destinazione vuota -> sposta MoveAmount (o tutto lo stack)
	if (DstEntry.Item.IsEmpty())
	{
		DstEntry.Item = SrcEntry.Item;
		DstEntry.Item.Quantity = MoveAmount;

		SrcEntry.Item.Quantity -= MoveAmount;
		if (SrcEntry.Item.Quantity <= 0)
		{
			SrcEntry.Item.Clear();
		}
	}
	// CASO B: Stesso oggetto impilabile (e non unico con GUID) -> fondi nello stack destinazione
	else if (DstEntry.Item.ItemID == SrcEntry.Item.ItemID && MaxStack > 1 && !SrcEntry.Item.UniqueInstanceID.IsValid())
	{
		const int32 SpaceAvailable = MaxStack - DstEntry.Item.Quantity;
		if (SpaceAvailable <= 0)
		{
			ReleaseLocks();
			return false;
		}

		const int32 ActualMoved = FMath::Min(SpaceAvailable, MoveAmount);
		DstEntry.Item.Quantity += ActualMoved;
		SrcEntry.Item.Quantity -= ActualMoved;

		if (SrcEntry.Item.Quantity <= 0)
		{
			SrcEntry.Item.Clear();
		}
	}
	// CASO C: Oggetti diversi (o non impilabili) -> scambio completo solo se si sta spostando l'intero stack
	else if (MoveAmount == SrcEntry.Item.Quantity)
	{
		FInventoryItem Temp = DstEntry.Item;
		DstEntry.Item = SrcEntry.Item;
		SrcEntry.Item = Temp;
	}
	else
	{
		// Impossibile dividere uno stack sopra uno slot occupato da un oggetto diverso
		ReleaseLocks();
		return false;
	}

	// Replica delta solo sui 2 slot coinvolti
	SourceComp->StorageList.MarkItemDirty(SrcEntry);
	DestComp->StorageList.MarkItemDirty(DstEntry);

	SourceComp->OnInventorySlotUpdated.Broadcast(SourceSlot, SrcEntry.Item, false);
	DestComp->OnInventorySlotUpdated.Broadcast(DestSlot, DstEntry.Item, false);

	if (SourceComp->bHasHotbar && SourceSlot == SourceComp->SelectedHotbarSlot)
	{
		SourceComp->BroadcastEquippedItemChanged();
	}
	if (DestComp->bHasHotbar && DestSlot == DestComp->SelectedHotbarSlot)
	{
		DestComp->BroadcastEquippedItemChanged();
	}

	ReleaseLocks();
	return true;
}

// ============================================================================
// FUNZIONI SERVER-ONLY (Generazione e Consumo)
// ============================================================================

bool UcybmineInventoryComponent::AddItemByID(FPrimaryAssetId ItemID, int32 Quantity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	UcybmineItemData* StaticData = GetItemData(ItemID);
	if (!StaticData || Quantity <= 0) return false;

	FInventoryItem NewInstance;
	NewInstance.ItemID = ItemID;
	NewInstance.Quantity = Quantity;
	NewInstance.CurrentDurability = StaticData->MaxDurability;

	if (StaticData->MaxStackSize <= 1)
	{
		NewInstance.UniqueInstanceID = FGuid::NewGuid();
	}

	return AddItem(NewInstance);
}

bool UcybmineInventoryComponent::AddItem(const FInventoryItem& NewItem)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || NewItem.IsEmpty() || bTransactionInProgress)
	{
		return false;
	}

	InitializeSlotArrays();

	UcybmineItemData* StaticData = GetItemData(NewItem.ItemID);
	if (!StaticData) return false;

	bTransactionInProgress = true;

	const int32 MaxStack = FMath::Max(1, StaticData->MaxStackSize);
	int32 Remaining = NewItem.Quantity;
	bool bModifiedActiveSlot = false;

	if (MaxStack > 1)
	{
		for (int32 i = 0; i < StorageList.Entries.Num(); ++i)
		{
			FCybmineInventorySlotEntry& Entry = StorageList.Entries[i];
			if (!Entry.Item.IsEmpty() && Entry.Item.ItemID == NewItem.ItemID && Entry.Item.Quantity < MaxStack)
			{
				const int32 Space = MaxStack - Entry.Item.Quantity;
				const int32 ToAdd = FMath::Min(Space, Remaining);
				Entry.Item.Quantity += ToAdd;
				Remaining -= ToAdd;

				StorageList.MarkItemDirty(Entry);
				OnInventorySlotUpdated.Broadcast(Entry.SlotIndex, Entry.Item, false);

				if (bHasHotbar && Entry.SlotIndex == SelectedHotbarSlot) bModifiedActiveSlot = true;
				if (Remaining <= 0) break;
			}
		}
	}

	if (Remaining > 0)
	{
		for (int32 i = 0; i < StorageList.Entries.Num(); ++i)
		{
			FCybmineInventorySlotEntry& Entry = StorageList.Entries[i];
			if (Entry.Item.IsEmpty())
			{
				Entry.Item = NewItem;
				Entry.Item.Quantity = FMath::Min(MaxStack, Remaining);

				if (MaxStack <= 1 && !Entry.Item.UniqueInstanceID.IsValid())
				{
					Entry.Item.UniqueInstanceID = FGuid::NewGuid();
				}

				Remaining -= Entry.Item.Quantity;

				StorageList.MarkItemDirty(Entry);
				OnInventorySlotUpdated.Broadcast(Entry.SlotIndex, Entry.Item, false);

				if (bHasHotbar && Entry.SlotIndex == SelectedHotbarSlot) bModifiedActiveSlot = true;
				if (Remaining <= 0) break;
			}
		}
	}

	bTransactionInProgress = false;

	if (bModifiedActiveSlot)
	{
		BroadcastEquippedItemChanged();
	}

	return (Remaining < NewItem.Quantity);
}

bool UcybmineInventoryComponent::ConsumeItemFromSlot(int32 SlotIndex, int32 QuantityToConsume)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || QuantityToConsume <= 0 || bTransactionInProgress)
	{
		return false;
	}

	if (!StorageList.Entries.IsValidIndex(SlotIndex)) return false;

	FCybmineInventorySlotEntry& Entry = StorageList.Entries[SlotIndex];
	if (Entry.Item.IsEmpty() || Entry.Item.Quantity < QuantityToConsume)
	{
		return false;
	}

	bTransactionInProgress = true;

	Entry.Item.Quantity -= QuantityToConsume;
	if (Entry.Item.Quantity <= 0)
	{
		Entry.Item.Clear();
	}

	StorageList.MarkItemDirty(Entry);
	OnInventorySlotUpdated.Broadcast(SlotIndex, Entry.Item, false);

	if (bHasHotbar && SlotIndex == SelectedHotbarSlot)
	{
		BroadcastEquippedItemChanged();
	}

	bTransactionInProgress = false;
	return true;
}