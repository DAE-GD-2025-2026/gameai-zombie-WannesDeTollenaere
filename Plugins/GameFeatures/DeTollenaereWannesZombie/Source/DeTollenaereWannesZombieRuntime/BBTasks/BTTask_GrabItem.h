#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_GrabItem.generated.h"

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTTask_GrabItem : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_GrabItem();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};