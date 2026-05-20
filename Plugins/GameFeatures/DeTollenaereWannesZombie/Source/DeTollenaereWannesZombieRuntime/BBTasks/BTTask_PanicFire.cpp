#include "BTTask_PanicFire.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "SurvivorBlackboardKeys.h"

UBTTask_PanicFire::UBTTask_PanicFire()
{
	NodeName = "Execute Panic Fire";
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_PanicFire::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	BB->SetValueAsBool(BBK_IsExecutingCombatRotation, true);
	return EBTNodeResult::InProgress;
}

void UBTTask_PanicFire::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (!AIC || !BB || !Pawn)
	{
		if (BB) BB->SetValueAsBool(BBK_IsExecutingCombatRotation, false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Threat = Cast<AActor>(BB->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!Threat || !::IsValid(Threat))
	{
		BB->SetValueAsBool(BBK_IsExecutingCombatRotation, false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector SurvivorPos = Pawn->GetActorLocation();
	FVector ToThreat = (Threat->GetActorLocation() - SurvivorPos).GetSafeNormal2D();

	FRotator TargetRotation = ToThreat.Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;

	FRotator NewRotation = FMath::RInterpConstantTo(Pawn->GetActorRotation(), TargetRotation, DeltaSeconds, TurnSpeed);
	Pawn->SetActorRotation(NewRotation);
	AIC->SetControlRotation(NewRotation);

	float AlignmentDot = FVector::DotProduct(Pawn->GetActorForwardVector().GetSafeNormal2D(), ToThreat);
	if (AlignmentDot >= 0.98f)
	{
		UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>();
		if (Inv)
		{
			int32 BestSlot = -1;
			int32 BestDamage = -1;
			const TArray<ABaseItem*>& Items = Inv->GetInventory();

			for (int32 i = 0; i < Items.Num(); ++i)
			{
				AWeapon* W = Cast<AWeapon>(Items[i]);
				if (!W || W->GetValue() <= 0) continue;

				if (W->GetDamage() > BestDamage)
				{
					BestDamage = W->GetDamage();
					BestSlot = i;
				}
			}

			if (BestSlot != -1 && Inv->UseItem(BestSlot))
			{
				if (AWeapon* SpentWeapon = Cast<AWeapon>(Items[BestSlot]))
				{
					if (SpentWeapon->GetValue() <= 0)
					{
						Inv->RemoveItem(BestSlot);
					}
				}
				BB->SetValueAsBool(BBK_IsExecutingCombatRotation, false);
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
		}
		BB->SetValueAsBool(BBK_IsExecutingCombatRotation, false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type UBTTask_PanicFire::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsBool(BBK_IsExecutingCombatRotation, false);
	}
	return Super::AbortTask(OwnerComp, NodeMemory);
}