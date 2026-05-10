#include "BTTask_GrabItem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_GrabItem::UBTTask_GrabItem()
{
	NodeName = "Grab Item";
}

EBTNodeResult::Type UBTTask_GrabItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	UInventoryComponent* Inventory = AIPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return EBTNodeResult::Failed;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return EBTNodeResult::Failed;

	ABaseItem* TargetItem = Cast<ABaseItem>(Blackboard->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!TargetItem) return EBTNodeResult::Failed;

	float DistanceToItem = FVector::Dist(AIPawn->GetActorLocation(), TargetItem->GetActorLocation());
	if (DistanceToItem > (Inventory->GetPickupRange() + 50.0f))
	{
		return EBTNodeResult::Failed;
	}

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();
	for (int i = 0; i < Items.Num(); ++i)
	{
		if (Items[i] == nullptr)
		{
			if (Inventory->GrabItem(i, TargetItem))
			{
				Blackboard->ClearValue(GetSelectedBlackboardKey());
				return EBTNodeResult::Succeeded;
			}
		}
	}

	Blackboard->ClearValue(GetSelectedBlackboardKey());
	return EBTNodeResult::Failed;
}