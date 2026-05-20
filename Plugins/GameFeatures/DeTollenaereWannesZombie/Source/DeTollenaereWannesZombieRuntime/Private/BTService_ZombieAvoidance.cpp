#include "BTService_ZombieAvoidance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "Zombies/BaseZombie.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "SurvivorBlackboardKeys.h"


UBTService_ZombieAvoidance::UBTService_ZombieAvoidance()
{
	NodeName = "Weighted Steering & Avoidance";
	bNotifyTick = true;
	Interval = 0.f;
	RandomDeviation = 0.f;
}

void UBTService_ZombieAvoidance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

}

void UBTService_ZombieAvoidance::TryPanicFire(APawn* Pawn, AAIController* AIC, const FVector& ThreatPos, FZombieAvoidanceMemory* Mem) const
{
}

void UBTService_ZombieAvoidance::TryPeekBehind(UBehaviorTreeComponent& OwnerComp, APawn* Pawn, UBlackboardComponent* BB, FZombieAvoidanceMemory* Mem) const
{
}