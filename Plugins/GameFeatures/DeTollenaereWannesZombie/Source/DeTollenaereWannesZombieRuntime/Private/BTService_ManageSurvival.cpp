#include "BTService_ManageSurvival.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Items/Medkit.h"
#include "Items/Food.h"

UBTService_ManageSurvival::UBTService_ManageSurvival()
{
	NodeName = "Manage Survival Stats";
	bNotifyTick = true; 
}

void UBTService_ManageSurvival::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return;

	UInventoryComponent* Inventory = AIPawn->FindComponentByClass<UInventoryComponent>();
	UHealthComponent* HealthComp = AIPawn->FindComponentByClass<UHealthComponent>();
	UStaminaComponent* StaminaComp = AIPawn->FindComponentByClass<UStaminaComponent>();

	if (!Inventory || !HealthComp || !StaminaComp) return;

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	float HealthPct = static_cast<float>(HealthComp->GetHealth()) / HealthComp->GetMaxHealth();
	if (HealthPct <= HealthThreshold)
	{
		for (int i = 0; i < Items.Num(); ++i)
		{
			if (Cast<AMedkit>(Items[i])) // Found a medkit
			{
				Inventory->UseItem(i); 
				return; 
			}
		}
	}

	float StaminaPct = StaminaComp->GetCurrentStamina() / StaminaComp->GetMaxStamina();
	if (StaminaPct <= StaminaThreshold)
	{
		for (int i = 0; i < Items.Num(); ++i)
		{
			if (Cast<AFood>(Items[i])) // Found food
			{
				Inventory->UseItem(i);  
				return;
			}
		}
	}
}