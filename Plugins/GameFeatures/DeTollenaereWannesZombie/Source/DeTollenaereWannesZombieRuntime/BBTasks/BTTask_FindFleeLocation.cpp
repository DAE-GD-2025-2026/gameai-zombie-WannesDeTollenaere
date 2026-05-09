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

	FVector TargetFleeLocation = AIPawn->GetActorLocation() + (RunDirection * FleeDistance);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	FNavLocation SafeLocation;

	if (NavSys->ProjectPointToNavigation(TargetFleeLocation, SafeLocation, FVector::ZeroVector))
	{
		Blackboard->SetValueAsVector(GetSelectedBlackboardKey(), SafeLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}