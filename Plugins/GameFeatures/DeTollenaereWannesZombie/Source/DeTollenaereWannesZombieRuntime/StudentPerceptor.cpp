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

static const FName BBK_HasZombieInSight("HasZombieInSight");
static const FName BBK_ClosestZombie   ("ClosestZombie");
static const FName BBK_TargetItem      ("TargetItem");


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


void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UBlackboardComponent* BB = GetBlackboard();
	if (!BB || !BB->GetValueAsBool(BBK_HasZombieInSight)) return;

	AActor* Zombie = Cast<AActor>(BB->GetValueAsObject(BBK_ClosestZombie));
	if (!Zombie) return;

	if (FVector::Dist(GetOwner()->GetActorLocation(), Zombie->GetActorLocation()) > SafetyDistance)
	{
		BB->SetValueAsBool(BBK_HasZombieInSight, false);
		BB->ClearValue(BBK_ClosestZombie);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Reached safe distance."));
	}
}


void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	UBlackboardComponent* BB = GetBlackboard();
	if (!BB) return;

	// zombie
	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed()) return;

		AActor*     Current = Cast<AActor>(BB->GetValueAsObject(BBK_ClosestZombie));
		const float NewDist = FVector::Dist(GetOwner()->GetActorLocation(), Zombie->GetActorLocation());
		const float CurDist = Current
		                    ? FVector::Dist(GetOwner()->GetActorLocation(), Current->GetActorLocation())
		                    : FLT_MAX;

		if (NewDist < CurDist)
		{
			BB->SetValueAsBool  (BBK_HasZombieInSight, true);
			BB->SetValueAsObject(BBK_ClosestZombie, Zombie);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Zombie spotted!"));
		}
		return;
	}

	//  Item
	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (!Stimulus.WasSuccessfullySensed()) return;
		if (Item->GetItemType() == EItemType::Garbage) return;

		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (!OwnerPawn) return;

		KnownItems.AddUnique(Item);


		if (FHouseMemory* NearHouse = FindNearestHouseMemory(Item->GetActorLocation(), ItemToHouseRadius))
			NearHouse->KnownItems.AddUnique(Item);

		UInventoryComponent* Inv = OwnerPawn->FindComponentByClass<UInventoryComponent>();
		bool bHasSlot   = false;
		bool bHasWeapon = false;
		if (Inv)
		{
			for (ABaseItem* Slot : Inv->GetInventory())
			{
				if (!Slot) bHasSlot = true;
				else if (AWeapon* W = Cast<AWeapon>(Slot))
					if (W->GetValue() > 0) bHasWeapon = true;
			}
		}
		if (!bHasSlot) return; // inventory full

		const bool bIsWeapon = (Item->GetItemType() == EItemType::Pistol ||
		                        Item->GetItemType() == EItemType::Shotgun);

	
		if (bIsWeapon && !bHasWeapon && BB->GetValueAsBool(BBK_HasZombieInSight))
		{
			const float Dist = FVector::Dist2D(GetOwner()->GetActorLocation(), Item->GetActorLocation());
			if (Dist < WeaponGrabRadius)
			{
				BB->SetValueAsObject(BBK_TargetItem, Item);
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
					FString::Printf(TEXT("Grabbing weapon: %s"), *Item->GetName()));
				return;
			}
		}

		// Normal priority scoring
		const float NewScore = ScoreItem(Item, OwnerPawn);
		if (NewScore <= 0.f) return;

		ABaseItem* Current = Cast<ABaseItem>(BB->GetValueAsObject(BBK_TargetItem));
		if (Current && IsValid(Current) && NewScore <= ScoreItem(Current, OwnerPawn)) return;

		BB->SetValueAsObject(BBK_TargetItem, Item);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Item targeted (%.0f): %s"), NewScore, *Item->GetName()));
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
			NewEntry.bScouted   = false;
			KnownHouses.Add(NewEntry);

			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
				FString::Printf(TEXT("New house discovered (%d total)"), KnownHouses.Num()));
		}

	}
}
