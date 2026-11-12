#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "cybmineTypes.h"
#include "cybmineInventoryOwnerInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UcybmineInventoryOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class CYBMINE_API IcybmineInventoryOwnerInterface
{
	GENERATED_BODY()

public:
	// Chiamato sul Server per verificare se l'Actor richiedente (es. un PlayerController o Character) può aprire/modificare questo contenitore
	virtual bool CanActorAccessInventory(AActor* RequestingActor) const { return true; }

	// Chiamato sul Server quando un oggetto viene droppato nel mondo dal contenitore
	virtual void OnItemDroppedFromInventory(const FInventoryItem& DroppedItem, const FVector& DropLocation, const FRotator& DropRotation) {}
};