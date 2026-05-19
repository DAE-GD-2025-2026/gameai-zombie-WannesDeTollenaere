// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "Zombies/BaseZombie.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Items/Weapon.h"
#include "PurgeZones/PurgeZone.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"

// returns item urgency
static float ScoreItem(ABaseItem* Item, APawn* OwnerPawn)
{
	if (!Item || !OwnerPawn) return 0.0f;

	UHealthComponent*    HealthComp  = OwnerPawn->FindComponentByClass<UHealthComponent>();
	UStaminaComponent*   StaminaComp = OwnerPawn->FindComponentByClass<UStaminaComponent>();
	UInventoryComponent* Inventory   = OwnerPawn->FindComponentByClass<UInventoryComponent>();

	const float HealthPct  = HealthComp  ? static_cast<float>(HealthComp->GetHealth()) / HealthComp->GetMaxHealth() : 1.0f;
	const float StaminaPct = StaminaComp ? StaminaComp->GetCurrentStamina() / StaminaComp->GetMaxStamina()          : 1.0f;

	switch (Item->GetItemType())
	{
	case EItemType::Medkit:
		if (HealthPct > 9.0f) return 0.0f;     
		return (1.0f - HealthPct) * 100.0f;         

	case EItemType::Food:
		if (StaminaPct > 8.5f) return 0.0f;         
		return (1.0f - StaminaPct) * 60.0f;

	case EItemType::Pistol:
	case EItemType::Shotgun:
	{
		int32 BestOwnedDamage = 0;
		if (Inventory)
		{
			for (ABaseItem* Slot : Inventory->GetInventory())
				if (AWeapon* W = Cast<AWeapon>(Slot))
					BestOwnedDamage = FMath::Max(BestOwnedDamage, W->GetDamage());
		}
		const int32 ItemDamage = Cast<AWeapon>(Item) ? Cast<AWeapon>(Item)->GetDamage() : 0;
		if (BestOwnedDamage == 0)          return 75.0f; 
		if (ItemDamage > BestOwnedDamage)  return 45.0f; 
		return 5.0f;     
	}

	default:
		return 0.0f;
	}
}

const FName BBK_HasZombieInSight = FName("HasZombieInSight");
const FName BBK_ClosestZombie = FName("ClosestZombie");
const FName BBK_TargetHouse = FName("TargetHouse");
const FName BBK_TargetItem = FName("TargetItem");

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();

	if (UAIPerceptionComponent* PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
}

UBlackboardComponent* UStudentPerceptor::GetBlackboard()
{
	if (CachedBlackboard) return CachedBlackboard;

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController()))
		{
			CachedBlackboard = AIController->GetBlackboardComponent();
		}
	}

	return CachedBlackboard;
}

void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UBlackboardComponent* Blackboard = GetBlackboard();
	if (!Blackboard) return;

	if (Blackboard->GetValueAsBool(BBK_HasZombieInSight))
	{
		AActor* Zombie = Cast<AActor>(Blackboard->GetValueAsObject(BBK_ClosestZombie));

		if (Zombie && FVector::Dist(GetOwner()->GetActorLocation(), Zombie->GetActorLocation()) <= SafetyDistance) return; 


		Blackboard->SetValueAsBool(BBK_HasZombieInSight, false);
		Blackboard->ClearValue(BBK_ClosestZombie);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Reached safe distance!"));
	}
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	UBlackboardComponent* Blackboard = GetBlackboard();
	if (!Blackboard) return;

	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed()) return;


		AActor* CurrentZombie = Cast<AActor>(Blackboard->GetValueAsObject(BBK_ClosestZombie));
		const float NewDist = FVector::Dist(GetOwner()->GetActorLocation(), Zombie->GetActorLocation());
		const float CurDist = CurrentZombie
			? FVector::Dist(GetOwner()->GetActorLocation(), CurrentZombie->GetActorLocation())
			: FLT_MAX;

		if (NewDist < CurDist)
		{
			Blackboard->SetValueAsBool(BBK_HasZombieInSight, true);
			Blackboard->SetValueAsObject(BBK_ClosestZombie, Zombie);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Zombie spotted!"));
		}
	}
	else if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed()) return;
		if (Item->GetItemType() == EItemType::Garbage) return;

		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (!OwnerPawn) return;

		if (UInventoryComponent* Inventory = OwnerPawn->FindComponentByClass<UInventoryComponent>())
		{
			bool bHasEmptySlot = false;
			for (ABaseItem* Slot : Inventory->GetInventory())
				if (Slot == nullptr) { bHasEmptySlot = true; break; }
			if (!bHasEmptySlot) return;
		}

		const float NewScore = ScoreItem(Item, OwnerPawn);
		if (NewScore <= 0.0f) return; 

		ABaseItem* CurrentTarget = Cast<ABaseItem>(Blackboard->GetValueAsObject(BBK_TargetItem));
		if (CurrentTarget && IsValid(CurrentTarget))
		{
			if (NewScore <= ScoreItem(CurrentTarget, OwnerPawn)) return;
		}

		Blackboard->SetValueAsObject(BBK_TargetItem, Item);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Item targeted (score %.0f): %s"), NewScore, *Item->GetName()));
	}
	else if (AHouse* House = Cast<AHouse>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed()) return;
		Blackboard->SetValueAsObject(BBK_TargetHouse, House);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("House spotted!"));
	}
}