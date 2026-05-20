// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "Zombies/BaseZombie.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Items/Weapon.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "SurvivorBlackboardKeys.h"



bool FHouseMemory::IsValidHouse() const
{
	return ::IsValid(HouseActor);
}

FVector FHouseMemory::GetLocation() const
{
	return HouseActor ? HouseActor->GetActorLocation() : FVector::ZeroVector;
}

void FHouseMemory::PurgeStaleItems()
{
	KnownItems.RemoveAll([](ABaseItem* I){ return !::IsValid(I); });
}


float UStudentPerceptor::ScoreItem(ABaseItem* Item, APawn* OwnerPawn)
{
	if (!Item || !OwnerPawn) return 0.f;

	UHealthComponent*    HP  = OwnerPawn->FindComponentByClass<UHealthComponent>();
	UStaminaComponent*   ST  = OwnerPawn->FindComponentByClass<UStaminaComponent>();
	UInventoryComponent* Inv = OwnerPawn->FindComponentByClass<UInventoryComponent>();

	const float HealthPct  = HP ? static_cast<float>(HP->GetHealth()) / HP->GetMaxHealth() : 1.f;
	const float StaminaPct = ST ? ST->GetCurrentStamina() / ST->GetMaxStamina()            : 1.f;

	switch (Item->GetItemType())
	{
	case EItemType::Medkit:
		if (HealthPct > 0.9f) return 0.f;
		return (1.f - HealthPct) * 100.f;

	case EItemType::Food:
		if (StaminaPct > 0.85f) return 0.f;
		return (1.f - StaminaPct) * 60.f;

	case EItemType::Pistol:
	case EItemType::Shotgun:
	{
		int32 BestOwned = 0;
		if (Inv)
			for (ABaseItem* Slot : Inv->GetInventory())
				if (AWeapon* W = Cast<AWeapon>(Slot))
					BestOwned = FMath::Max(BestOwned, W->GetDamage());

		const int32 ItemDmg = Cast<AWeapon>(Item) ? Cast<AWeapon>(Item)->GetDamage() : 0;
		if (BestOwned == 0)      return 75.f;
		if (ItemDmg > BestOwned) return 45.f;
		return 5.f;
	}
	default: return 0.f;
	}
}


UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();

	if (UAIPerceptionComponent* Perc = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
		Perc->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
}

UBlackboardComponent* UStudentPerceptor::GetBlackboard()
{
	if (CachedBlackboard) return CachedBlackboard;

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
		if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
			CachedBlackboard = AIC->GetBlackboardComponent();

	return CachedBlackboard;
}


FHouseMemory* UStudentPerceptor::FindNearestHouseMemory(const FVector& WorldPos, float MaxRadius)
{
	FHouseMemory* Best     = nullptr;
	float         BestDist = MaxRadius;

	for (FHouseMemory& HMem : KnownHouses)
	{
		if (!HMem.IsValidHouse()) continue;
		const float D = FVector::Dist(HMem.GetLocation(), WorldPos);
		if (D < BestDist) { BestDist = D; Best = &HMem; }
	}

	return Best;
}

void UStudentPerceptor::MarkHouseScouted(AActor* HouseActor)
{
	for (FHouseMemory& HMem : KnownHouses)
	{
		if (HMem.HouseActor == HouseActor)
		{
			HMem.bScouted = true;
			return;
		}
	}
}


void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UBlackboardComponent* BB = GetBlackboard();
	if (!BB) return;

	UAIPerceptionComponent* Perc = GetOwner()->GetComponentByClass<UAIPerceptionComponent>();
	if (!Perc) return;

	TArray<AActor*> KnownActors;
	Perc->GetKnownPerceivedActors(UAISense_Sight::StaticClass(), KnownActors);

	AActor* ClosestZombie = nullptr;
	float MinDistance = SafetyDistance;

	// find closest zombie
	for (AActor* Actor : KnownActors)
	{
		if (ABaseZombie* Zombie = Cast<ABaseZombie>(Actor))
		{
			float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Zombie->GetActorLocation());
			if (Dist < MinDistance)
			{
				MinDistance = Dist;
				ClosestZombie = Zombie;
			}
		}
	}

	// Update the Blackboard based on real-time proximity
	if (ClosestZombie)
	{
		BB->SetValueAsBool(BBK_HasZombieInSight, true);
		BB->SetValueAsObject(BBK_ClosestZombie, ClosestZombie);
	}
	else
	{
		BB->SetValueAsBool(BBK_HasZombieInSight, false);
		BB->ClearValue(BBK_ClosestZombie);
	}
}


void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	UBlackboardComponent* BB = GetBlackboard();
	if (!BB) return;

	// zombie
	// zombie
	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Zombie spotted!"));
		}
		return;
	}

	//  Item
	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed() || Item->GetItemType() == EItemType::Garbage)
		{
			return;
		}

		// Grab the current target item from the blackboard
		AActor* CurrentTargetItem = BB ? Cast<AActor>(BB->GetValueAsObject(BBK_TargetItem)) : nullptr;

		// Switch to new item if no item
		if (!CurrentTargetItem)
		{
			BB->SetValueAsObject(BBK_TargetItem, Item);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("New item spotted! Heading over instantly."));
		}
		else
		{
			float DistToNewItem = FVector::Dist2D(GetOwner()->GetActorLocation(), Item->GetActorLocation());
			float DistToCurrentTarget = FVector::Dist2D(GetOwner()->GetActorLocation(), CurrentTargetItem->GetActorLocation());

			if (DistToNewItem < DistToCurrentTarget)
			{
				BB->SetValueAsObject(BBK_TargetItem, Item);
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Spotted a closer item! Diverting course."));
			}
		}
		return;
	}

	//House
	if (AHouse* House = Cast<AHouse>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed()) return;

		bool bAlreadyKnown = false;
		for (const FHouseMemory& HMem : KnownHouses)
			if (HMem.HouseActor == House) { bAlreadyKnown = true; break; }

		if (!bAlreadyKnown)
		{
			FHouseMemory NewEntry;
			NewEntry.HouseActor = House;
			NewEntry.bScouted = false;
			KnownHouses.Add(NewEntry);

			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
				FString::Printf(TEXT("New house discovered (%d total)"), KnownHouses.Num()));

			if (BB)
			{
				BB->SetValueAsObject(BBK_TargetHouse, House);
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Heading to new house!"));
			}
		}
	}
}
