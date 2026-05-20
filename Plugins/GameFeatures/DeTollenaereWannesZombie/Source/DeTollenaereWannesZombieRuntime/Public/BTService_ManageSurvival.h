#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_ManageSurvival.generated.h"

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTService_ManageSurvival : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_ManageSurvival();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	                      float DeltaSeconds) override;

public:
	// Use a medkit when health drops below this fraction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Survival")
	float HealthThreshold = 0.75f;

	// Use food when stamina drops below this fraction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Survival")
	float StaminaThreshold = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Survival")
	float CriticalHealthThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Survival")
	float WalkStaminaThreshold = 0.45f;
};
