#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "cybmineWorldItem.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UcybmineItemData;
struct FStreamableHandle;

UENUM(BlueprintType)
enum class EItemState : uint8
{
    Dropped,
    Equipped
};

UCLASS()
class CYBMINE_API AcybmineWorldItem : public AActor
{
    GENERATED_BODY()

public:	
    AcybmineWorldItem();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Inizializza l'oggetto (Chiamato dal Server quando spawna l'item)
    void InitializeItem(UcybmineItemData* InItemData, EItemState InitialState);

    // Cambia lo stato fisico dell'oggetto
    void SetItemState(EItemState NewState);

protected:
    // Componenti
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* InteractVolume;

    // Dati replicati
    UPROPERTY(ReplicatedUsing = OnRep_ItemData)
    UcybmineItemData* ItemData;

    UPROPERTY(ReplicatedUsing = OnRep_ItemState)
    EItemState CurrentState;

    // Funzioni di RepNotify (eseguite sui Client quando il server cambia i dati)
    UFUNCTION()
    void OnRep_ItemData();

    UFUNCTION()
    void OnRep_ItemState();

    // Logica di caricamento asincrono della mesh
    void LoadMeshAsync();
    void OnMeshLoaded();

    // Gestione fisica in base allo stato
    void UpdatePhysicalState();

    // Handle per tenere in memoria la richiesta di caricamento
    TSharedPtr<FStreamableHandle> MeshLoadHandle;
};