#include "cybmineWorldItem.h"
#include "cybmineItemData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/AssetManager.h"

AcybmineWorldItem::AcybmineWorldItem()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    // Setup Mesh (Root)
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    
    // Setup Volume di Interazione (per raccoglierlo)
    InteractVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractVolume"));
    InteractVolume->SetupAttachment(RootComponent);
    InteractVolume->SetSphereRadius(100.f);

    CurrentState = EItemState::Dropped;
}

void AcybmineWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AcybmineWorldItem, ItemData);
    DOREPLIFETIME(AcybmineWorldItem, CurrentState);
}

void AcybmineWorldItem::InitializeItem(UcybmineItemData* InItemData, EItemState InitialState)
{
    if (HasAuthority())
    {
        ItemData = InItemData;
        CurrentState = InitialState;
        
        // Il server forza l'aggiornamento locale
        OnRep_ItemData();
        OnRep_ItemState();
    }
}

void AcybmineWorldItem::SetItemState(EItemState NewState)
{
    if (HasAuthority() && CurrentState != NewState)
    {
        CurrentState = NewState;
        OnRep_ItemState(); // Aggiorna sul server
    }
}

void AcybmineWorldItem::OnRep_ItemData()
{
    // Quando i client ricevono i dati dell'oggetto, avviano il caricamento della grafica
    LoadMeshAsync();
}

void AcybmineWorldItem::OnRep_ItemState()
{
    // Aggiorna collisioni e fisica ogni volta che lo stato cambia
    UpdatePhysicalState();
}

void AcybmineWorldItem::UpdatePhysicalState()
{
    if (CurrentState == EItemState::Dropped)
    {
        // A terra: Attiva fisica e collisioni
        MeshComponent->SetSimulatePhysics(true);
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        InteractVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    else if (CurrentState == EItemState::Equipped)
    {
        // Equipaggiato: Disattiva fisica, lo controlla l'animazione del giocatore
        MeshComponent->SetSimulatePhysics(false);
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        InteractVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AcybmineWorldItem::LoadMeshAsync()
{
    if (!ItemData || ItemData->WorldStaticMesh.IsNull()) return;

    FSoftObjectPath MeshPath = ItemData->WorldStaticMesh.ToSoftObjectPath();

    if (MeshPath.IsValid())
    {
        if (UStaticMesh* LoadedMesh = ItemData->WorldStaticMesh.Get())
        {
            MeshComponent->SetStaticMesh(LoadedMesh);
            return;
        }

        MeshLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
            MeshPath,
            FStreamableDelegate::CreateUObject(this, &AcybmineWorldItem::OnMeshLoaded)
        );
    }
}

void AcybmineWorldItem::OnMeshLoaded()
{
    if (ItemData && ItemData->WorldStaticMesh.Get())
    {
        MeshComponent->SetStaticMesh(ItemData->WorldStaticMesh.Get());
    }

    if (MeshLoadHandle.IsValid())
    {
        MeshLoadHandle->CancelHandle();
        MeshLoadHandle.Reset();
    }
}