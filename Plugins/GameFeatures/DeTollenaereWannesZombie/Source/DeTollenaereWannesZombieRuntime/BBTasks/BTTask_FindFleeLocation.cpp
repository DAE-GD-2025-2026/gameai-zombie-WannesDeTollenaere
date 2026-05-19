#include "BTTask_FindFleeLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"

UBTTask_FindFleeLocation::UBTTask_FindFleeLocation()
{
	NodeName = "Find Flee (Evade) Location";
}

EBTNodeResult::Type UBTTask_FindFleeLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	AActor* ThreatActor = Cast<AActor>(Blackboard->GetValueAsObject(ThreatKey.SelectedKeyName));
	if (!ThreatActor) return EBTNodeResult::Failed;

	FVector CurrentThreatLocation = ThreatActor->GetActorLocation();
	FVector ThreatVelocity = ThreatActor->GetVelocity();
	
	FVector PredictedThreatLocation = CurrentThreatLocation + (ThreatVelocity * LookAheadTime);

	FVector RunDirection = AIPawn->GetActorLocation() - PredictedThreatLocation;
	RunDirection.Z = 0.0f;
	RunDirection.Normalize();

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	const float AngleStepDeg = 30.0f;
	const int32 MaxAttempts = 12;
	const FVector NavExtent(500.0f, 500.0f, 250.0f);

	FNavLocation SafeLocation;
	bool bFound = false;

	for (int32 i = 0; i <= MaxAttempts && !bFound; ++i)
	{
		const float Sign = (i % 2 == 0) ? 1.0f : -1.0f;
		const float AngleDeg = Sign * (i / 2) * AngleStepDeg;
		const FVector AttemptDir = RunDirection.RotateAngleAxis(AngleDeg, FVector::UpVector);
		const FVector AttemptTarget = AIPawn->GetActorLocation() + AttemptDir * FleeDistance;

		if (NavSys->ProjectPointToNavigation(AttemptTarget, SafeLocation, NavExtent))
			bFound = true;
	}

	if (!bFound)
	{
		if (!NavSys->GetRandomReachablePointInRadius(AIPawn->GetActorLocation(), FleeDistance, SafeLocation))
			return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(GetSelectedBlackboardKey(), SafeLocation.Location);
	return EBTNodeResult::Succeeded;
}