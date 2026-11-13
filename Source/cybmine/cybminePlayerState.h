#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "cybmineTypes.h"
#include "cybminePlayerState.generated.h"

UCLASS()
class CYBMINE_API AcybminePlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AcybminePlayerState();

    // Funzione obbligatoria per registrare le variabili replicate
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- GETTERS ---
    UFUNCTION(BlueprintPure, Category = "Economy")
    int64 GetMoney() const { return Money; }

    UFUNCTION(BlueprintPure, Category = "Economy")
    EPlayerJob GetCurrentJob() const { return CurrentJob; }

    // --- SETTERS (Solo Server) ---
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Economy")
    void AddMoney(int64 Amount);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Economy")
    void SetCurrentJob(EPlayerJob NewJob);

protected:
    // Variabili Replicate (RepNotify per aggiornare la UI sui client)
    UPROPERTY(ReplicatedUsing = OnRep_Money)
    int64 Money;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentJob)
    EPlayerJob CurrentJob;

    // --- FUNZIONI DI REP NOTIFY ---
    UFUNCTION()
    void OnRep_Money();

    UFUNCTION()
    void OnRep_CurrentJob();
};