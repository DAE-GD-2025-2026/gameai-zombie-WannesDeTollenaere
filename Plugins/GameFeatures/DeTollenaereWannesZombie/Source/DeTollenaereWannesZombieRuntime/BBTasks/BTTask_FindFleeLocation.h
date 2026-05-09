#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindFleeLocation.generated.h"

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTTask_FindFleeLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindFleeLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FleeDistance = 2000.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float LookAheadTime = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector ThreatKey;
};