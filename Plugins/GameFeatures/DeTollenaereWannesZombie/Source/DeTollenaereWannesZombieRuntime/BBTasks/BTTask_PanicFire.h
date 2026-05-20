#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_PanicFire.generated.h"

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTTask_PanicFire : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_PanicFire();

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireCooldownTime = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float TurnSpeed = 1800.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};