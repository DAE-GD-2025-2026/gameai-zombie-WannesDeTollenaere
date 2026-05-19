#include "BTTask_AttackZombie.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "Zombies/BaseZombie.h"

UBTTask_AttackZombie::UBTTask_AttackZombie()
{
	NodeName = "Attack Zombie";
}

EBTNodeResult::Type UBTTask_AttackZombie::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	UInventoryComponent* Inventory = AIPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return EBTNodeResult::Failed;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return EBTNodeResult::Failed;

	AActor* ZombieTarget = Cast<AActor>(Blackboard->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!ZombieTarget) return EBTNodeResult::Failed;

	const float DistToTarget = FVector::Dist(AIPawn->GetActorLocation(), ZombieTarget->GetActorLocation());
	if (DistToTarget > MaxAttackRange)
		return EBTNodeResult::Failed;

	// Pick the weapon with the most damage that still has ammo
	int32 SelectedWeaponSlot = -1;
	int32 BestDamage = -1;
	AWeapon* SelectedWeapon = nullptr;
	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (AWeapon* Weapon = Cast<AWeapon>(Items[i]))
		{
			if (Weapon->GetValue() > 0 && Weapon->GetDamage() > BestDamage)
			{
				BestDamage = Weapon->GetDamage();
				SelectedWeaponSlot = i;
				SelectedWeapon = Weapon;
			}
		}
	}

	if (SelectedWeaponSlot == -1)
		return EBTNodeResult::Failed;

	// Aim
	FVector LookDirection = ZombieTarget->GetActorLocation() - AIPawn->GetActorLocation();
	LookDirection.Z = 0.0f;

	if (!LookDirection.IsNearlyZero())
	{
		FRotator TargetRotation = LookDirection.Rotation();
		AIPawn->SetActorRotation(TargetRotation);
		AIController->SetControlRotation(TargetRotation);
	}

	// shoot
	if (Inventory->UseItem(SelectedWeaponSlot))
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("Firing Weapon!"));

		if (SelectedWeapon->GetValue() <= 0)
		{
			Inventory->RemoveItem(SelectedWeaponSlot);
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Dropped empty weapon!"));
		}

		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}