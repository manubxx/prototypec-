#include "cybminePlayerState.h"
#include "Net/UnrealNetwork.h"

AcybminePlayerState::AcybminePlayerState()
{
    // Valori di default alla nascita del giocatore
    Money = 0;
    CurrentJob = EPlayerJob::None;

    // Assicurati che replichi (il PlayerState lo fa di default, ma è buona norma)
    bReplicates = true;
}

void AcybminePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Registra le variabili per la sincronizzazione Rete Server -> Client
    DOREPLIFETIME(AcybminePlayerState, Money);
    DOREPLIFETIME(AcybminePlayerState, CurrentJob);
}

void AcybminePlayerState::AddMoney(int64 Amount)
{
    // Misura di sicurezza: esegui solo sul Server
    if (HasAuthority())
    {
        Money += Amount;

        // Evitiamo che i soldi scendano sotto lo 0 se si sottraggono somme (es. comprando un item)
        if (Money < 0) 
        {
            Money = 0;
        }

        // Il server chiama esplicitamente OnRep se ha bisogno di aggiornare la sua UI locale
        // (Sui client dedicati non serve, ma sui Listen Server è comodo)
        OnRep_Money();
    }
}

void AcybminePlayerState::SetCurrentJob(EPlayerJob NewJob)
{
    if (HasAuthority())
    {
        CurrentJob = NewJob;
        OnRep_CurrentJob();
    }
}

void AcybminePlayerState::OnRep_Money()
{
    // Qui puoi chiamare un delegate (Event Dispatcher) per dire all'HUD di aggiornare la UI
    // Es: OnMoneyChangedDelegate.Broadcast(Money);
}

void AcybminePlayerState::OnRep_CurrentJob()
{
    // Qui puoi aggiornare la UI per mostrare il nuovo lavoro
}