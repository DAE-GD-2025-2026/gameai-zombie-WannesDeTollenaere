#include "BTService_ManageSurvival.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Items/BaseItem.h"
#include "Items/Medkit.h"
#include "Items/Food.h"
#include "Items/Weapon.h"
#include "Survivor/SurvivorPawn.h"

UBTService_ManageSurvival::UBTService_ManageSurvival()
{
	NodeName = "Manage Survival Stats";
	bNotifyTick = true;
}

void UBTService_ManageSurvival::TickNode(UBehaviorTreeComponent& OwnerComp,
                                         uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	APawn* AIPawn = AIC->GetPawn();
	if (!AIPawn) return;

	UInventoryComponent* Inventory = AIPawn->FindComponentByClass<UInventoryComponent>();
	UHealthComponent* HealthComp = AIPawn->FindComponentByClass<UHealthComponent>();
	UStaminaComponent* StaminaComp = AIPawn->FindComponentByClass<UStaminaComponent>();

	if (!Inventory || !HealthComp || !StaminaComp) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();
	const float HealthPct = static_cast<float>(HealthComp->GetHealth()) / HealthComp->GetMaxHealth();
	const float StaminaPct = StaminaComp->GetCurrentStamina() / StaminaComp->GetMaxStamina();
	const bool  bChased = BB && BB->GetValueAsBool(FName("HasZombieInSight"));

	if (BB)
		BB->SetValueAsBool(FName("HasCriticalHealth"), HealthPct < CriticalHealthThreshold);

	const bool bEmergencyHeal = bChased && (HealthPct < HealthThreshold * 0.6f);
	if (HealthPct <= HealthThreshold || bEmergencyHeal)
	{
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			if (Cast<AMedkit>(Items[i]) && Inventory->UseItem(i))
			{
				Inventory->RemoveItem(i);
				return;
			}
		}
	}

	if (StaminaPct <= StaminaThreshold)
	{
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			if (Cast<AFood>(Items[i]) && Inventory->UseItem(i))
			{
				Inventory->RemoveItem(i);
				return;
			}
		}
	}

	if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(AIPawn))
	{
		if (bChased)
		{
			if (!Survivor->IsRunning() && StaminaPct > 0.05f)
				Survivor->StartRunning();
			else if (Survivor->IsRunning() && StaminaPct <= 0.05f)
				Survivor->StopRunning();
		}
		else
		{
			if (Survivor->IsRunning())
				Survivor->StopRunning();
		}
	}
}
