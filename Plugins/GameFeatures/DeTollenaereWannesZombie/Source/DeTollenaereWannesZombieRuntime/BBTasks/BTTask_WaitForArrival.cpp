#include "BTTask_WaitForArrival.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Village/House/House.h"

UBTTask_WaitForArrival::UBTTask_WaitForArrival()
{
	NodeName = "Wait For Steering Arrival";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_WaitForArrival::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_WaitForArrival::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIC || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FName SelectedKeyName = GetSelectedBlackboardKey();
	bool bHasArrived = false;
	bool bHasValidTarget = false;

	// check if we reached target
	if (UObject* TargetObj = BB->GetValueAsObject(SelectedKeyName))
	{
		if (AActor* TargetActor = Cast<AActor>(TargetObj))
		{
			bHasValidTarget = true;
			FVector PawnLoc = Pawn->GetActorLocation();

			if (TargetActor->IsA<AHouse>())
			{
				FBox ActorBounds = TargetActor->GetComponentsBoundingBox(true);
				ActorBounds = ActorBounds.ExpandBy(ArrivalRadius);

				if (PawnLoc.X >= ActorBounds.Min.X && PawnLoc.X <= ActorBounds.Max.X &&
					PawnLoc.Y >= ActorBounds.Min.Y && PawnLoc.Y <= ActorBounds.Max.Y)
				{
					bHasArrived = true;
				}
			}
			else
			{
				float DistToTarget = FVector::Dist2D(PawnLoc, TargetActor->GetActorLocation());
				if (DistToTarget <= ArrivalRadius)
				{
					bHasArrived = true;
				}
			}
		}
	}
	else if (BB->IsVectorValueSet(SelectedKeyName))
	{
		bHasValidTarget = true;
		FVector TargetPos = BB->GetValueAsVector(SelectedKeyName);

		if (FVector::Dist2D(Pawn->GetActorLocation(), TargetPos) <= ArrivalRadius)
		{
			bHasArrived = true;
		}
	}

	if (!bHasValidTarget)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Complete task on arrival
	if (bHasArrived)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}