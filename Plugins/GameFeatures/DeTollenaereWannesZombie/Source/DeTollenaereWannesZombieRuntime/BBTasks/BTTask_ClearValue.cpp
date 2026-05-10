#include "BTTask_ClearValue.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearValue::UBTTask_ClearValue()
{
	NodeName = "Clear Blackboard Value";
}

EBTNodeResult::Type UBTTask_ClearValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (Blackboard)
	{
		Blackboard->ClearValue(GetSelectedBlackboardKey());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}