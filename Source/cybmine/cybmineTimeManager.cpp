#include "cybmineTimeManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Math/UnrealMathUtility.h"

AcybmineTimeManager::AcybmineTimeManager()
{
	// Imposta questo Actor per chiamare il Tick() a ogni frame
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true; // Deve esistere per tutti i client ovunque si trovino

	// Valori di default: Partiamo dalle ore 12:00 (Mezzogiorno)
	ServerTimeOfDay = 12.0f;
	LocalTimeOfDay = 12.0f;
	TimeMultiplier = 60.0f; // 1 giorno in game = 24 minuti reali
}

void AcybmineTimeManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AcybmineTimeManager, ServerTimeOfDay);
	DOREPLIFETIME(AcybmineTimeManager, TimeMultiplier);
}

void AcybmineTimeManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Sincronizza l'inizio
	LocalTimeOfDay = ServerTimeOfDay;
}

void AcybmineTimeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Calcolo quanto tempo è passato in ore in questo frame
	// DeltaTime è in secondi. Diviso 3600 diventa ore.
	float TimeToAdvance = (DeltaTime / 3600.0f) * TimeMultiplier;

	// 2. Il Server aggiorna la sua ora ufficiale
	if (HasAuthority())
	{
		ServerTimeOfDay += TimeToAdvance;
		if (ServerTimeOfDay >= 24.0f)
		{
			ServerTimeOfDay -= 24.0f; // Resetta a mezzanotte
		}
	}

	// 3. Tutti (Server e Client) aggiornano la propria ora locale fluidamente
	LocalTimeOfDay += TimeToAdvance;
	if (LocalTimeOfDay >= 24.0f)
	{
		LocalTimeOfDay -= 24.0f;
	}

	// 4. Aggiorniamo la grafica (Il Sole)
	UpdateSunRotation();
}

void AcybmineTimeManager::OnRep_ServerTime()
{
	// Il Client riceve l'ora ufficiale dal Server.
	// Se la differenza tra l'ora locale e quella del server è troppa (es. ha laggato), forziamo la correzione.
	float TimeDifference = FMath::Abs(LocalTimeOfDay - ServerTimeOfDay);
	
	if (TimeDifference > 0.1f && TimeDifference < 23.9f) // Ignoriamo il salto di mezzanotte
	{
		LocalTimeOfDay = FMath::FInterpTo(LocalTimeOfDay, ServerTimeOfDay, GetWorld()->GetDeltaSeconds(), 2.0f);
	}
}

void AcybmineTimeManager::UpdateSunRotation()
{
	if (!SunLight) return;

	// Mappiamo l'ora (0-24) in gradi (0-360)
	// Alle 6:00 (alba) vogliamo Pitch = 0. Alle 12:00 (mezzogiorno) Pitch = -90 (perpendicolare).
	float SunPitch = ((LocalTimeOfDay / 24.0f) * 360.0f) - 90.0f;
	
	// Creiamo la rotazione e la applichiamo al Sole
	FRotator NewSunRotation = FRotator(SunPitch, 0.0f, 0.0f);
	SunLight->SetActorRotation(NewSunRotation);

	// Se usi UE5 SkyAtmosphere, aggiornare la rotazione del Sole DirectionalLight 
	// aggiorna AUTOMATICAMENTE il colore del cielo e l'atmosfera!
}