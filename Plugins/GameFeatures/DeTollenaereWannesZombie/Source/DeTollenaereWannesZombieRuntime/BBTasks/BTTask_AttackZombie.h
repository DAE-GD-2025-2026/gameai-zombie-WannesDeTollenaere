#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_AttackZombie.generated.h"

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTTask_AttackZombie : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_AttackZombie();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};