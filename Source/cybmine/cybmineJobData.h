#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "cybmineTypes.h"
#include "cybmineJobData.generated.h"

UCLASS(BlueprintType)
class CYBMINE_API UcybmineJobData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Il tipo di lavoro a cui si riferisce questo DataAsset
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Job Info")
    EPlayerJob JobType;

    // Stipendio base (es. pagato ogni TOT tempo nel gioco)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
    int32 BaseSalary;

    // Moltiplicatore generico (es. x1.5 per le risorse minate se sei Minatore)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float ActionRewardMultiplier;

    // Aggiungi qui in futuro altri array, es. TArray<FGuid> AllowedItems per limitare i craft
};