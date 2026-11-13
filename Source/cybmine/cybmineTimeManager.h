#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "cybmineTimeManager.generated.h"

class ADirectionalLight;

UCLASS()
class CYBMINE_API AcybmineTimeManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AcybmineTimeManager();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// --- VARIABILI GRAFICHE (Da assegnare nell'Editor) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time Manager|Environment")
	ADirectionalLight* SunLight;

	// --- PARAMETRI TEMPO ---
	// Moltiplicatore del tempo (es. 1.0 = tempo reale, 60.0 = 1 minuto in gioco è 1 secondo reale)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Manager|Settings", Replicated)
	float TimeMultiplier;

	// L'ora ufficiale dettata dal Server (da 0.0 a 24.0)
	UPROPERTY(ReplicatedUsing = OnRep_ServerTime)
	float ServerTimeOfDay;

	// L'ora locale calcolata sul Client ricalcando il server
	float LocalTimeOfDay;

	// --- FUNZIONI ---
	UFUNCTION()
	void OnRep_ServerTime();

	// Aggiorna l'inclinazione del Sole in base all'ora
	void UpdateSunRotation();
};