#include "BTService_SurvivorMemory.h"
#include "StudentPerceptor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "SurvivorBlackboardKeys.h"

float UBTService_SurvivorMemory::GetMaxSpeed(AActor* Actor)
{
	if (!Actor) return 0.f;

	if (UFloatingPawnMovement* FPM = Actor->FindComponentByClass<UFloatingPawnMovement>())
		return FPM->MaxSpeed;

	if (UCharacterMovementComponent* CMC = Actor->FindComponentByClass<UCharacterMovementComponent>())
		return CMC->MaxWalkSpeed;

	return Actor->GetVelocity().Size();
}

float UBTService_SurvivorMemory::ScoreHouseForVisit(const FHouseMemory& HMem, APawn* Pawn) const
{
	if (!HMem.IsValidHouse()) return -1.f;

	// unscouted houses always visit
	float Score = HMem.bScouted ? 0.f : UnscoutedHouseBonus;

	for (ABaseItem* Item : HMem.KnownItems)
	{
		if (!IsValid(Item)) continue;
		Score += UStudentPerceptor::ScoreItem(Item, Pawn);
	}

	if (Score <= 0.f) return 0.f; 

	// prefer closer houses 
	const float Dist = FVector::Dist2D(Pawn->GetActorLocation(), HMem.GetLocation());
	return Score / (1.f + Dist / 3000.f);
}


UBTService_SurvivorMemory::UBTService_SurvivorMemory()
{
	NodeName        = "Survivor Memory";
	bNotifyTick     = true;
	Interval        = 0.f;
	RandomDeviation = 0.f;
}

void UBTService_SurvivorMemory::TickNode(UBehaviorTreeComponent& OwnerComp,
                                         uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
	if (!Perceptor) return;


	// hasweapon
	{
		bool bArmed = false;
		if (UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>())
			for (ABaseItem* Slot : Inv->GetInventory())
				if (AWeapon* W = Cast<AWeapon>(Slot))
					if (W->GetValue() > 0) { bArmed = true; break; }
		BB->SetValueAsBool(BBK_HasWeapon, bArmed);
	}

	// zombieIsfaster
	{
		AActor* Zombie  = Cast<AActor>(BB->GetValueAsObject(BBK_ClosestZombie));
		bool    bFaster = false;
		if (Zombie && BB->GetValueAsBool(BBK_HasZombieInSight))
			bFaster = (GetMaxSpeed(Zombie) > GetMaxSpeed(Pawn) + SpeedMargin);
		BB->SetValueAsBool(BBK_ZombieIsFaster, bFaster);
	}

	const bool bZombieActive = BB->GetValueAsBool(BBK_HasZombieInSight);

	if (bZombieActive)
	{
		HouseReselectTimer = 0.f;         
		ItemRecallTimer = ItemRecallCooldown; 
		LastRecalledItem = nullptr;
		return;
	}

	// House arrival check 
	{
		AActor* HouseTarget = Cast<AActor>(BB->GetValueAsObject(BBK_TargetHouse));
		if (HouseTarget && IsValid(HouseTarget))
		{
			const float Dist = FVector::Dist2D(Pawn->GetActorLocation(),
			                                   HouseTarget->GetActorLocation());
			if (Dist < HouseArrivalRadius)
			{
				Perceptor->MarkHouseScouted(HouseTarget);
				BB->ClearValue(BBK_TargetHouse);
				HouseReselectTimer = HouseReselectInterval; 
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
					TEXT("House scouted."));
			}
		}
	}

	// Stale-item
	AActor* CurrentItem = Cast<AActor>(BB->GetValueAsObject(BBK_TargetItem));
	if (CurrentItem && IsValid(CurrentItem))
	{
		if (CurrentItem != TrackedTargetItem)
		{
			TrackedTargetItem = CurrentItem;
			TargetItemAge     = 0.f;
		}
		else
		{
			TargetItemAge += DeltaSeconds;
			if (TargetItemAge >= ItemStaleTimeout)
			{
				Perceptor->RemoveKnownItem(CurrentItem);
				BB->ClearValue(BBK_TargetItem);
				TrackedTargetItem = nullptr;
				TargetItemAge     = 0.f;
				LastRecalledItem  = nullptr;
				ItemRecallTimer   = 0.f;
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
					TEXT("Item stale — giving up."));
			}
		}
		return; 
	}

	TrackedTargetItem = nullptr;
	TargetItemAge     = 0.f;

	// House selection
	HouseReselectTimer += DeltaSeconds;
	if (HouseReselectTimer >= HouseReselectInterval)
	{
		HouseReselectTimer = 0.f;

		AActor* CurrentHouseTarget = Cast<AActor>(BB->GetValueAsObject(BBK_TargetHouse));

		// dont interrupt current house visit unless we find a significantly better option, to avoid erratic behavior
		const float CurrentHouseScore = CurrentHouseTarget
		    ? [&]() -> float {
		          for (const FHouseMemory& H : Perceptor->GetKnownHousesConst())
		              if (H.HouseActor == CurrentHouseTarget) return ScoreHouseForVisit(H, Pawn);
		          return 0.f;
		      }()
		    : 0.f;

		AActor* BestHouse = nullptr;
		float BestHouseScore = CurrentHouseScore * 1.25f;

		for (FHouseMemory& HMem : Perceptor->GetKnownHouses())
		{
			HMem.PurgeStaleItems(); 
			const float Score = ScoreHouseForVisit(HMem, Pawn);
			if (Score > BestHouseScore)
			{
				BestHouseScore = Score;
				BestHouse      = HMem.HouseActor;
			}
		}

		if (BestHouse)
		{
			BB->SetValueAsObject(BBK_TargetHouse, BestHouse);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
				FString::Printf(TEXT("Heading to house (score %.0f)"), BestHouseScore));
		}
	}

	if (BB->GetValueAsObject(BBK_TargetHouse)) return; 

	ItemRecallTimer += DeltaSeconds;
	if (ItemRecallTimer < ItemRecallCooldown) return;

	UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	bool bHasSlot = false;
	for (ABaseItem* Slot : Inv->GetInventory())
		if (!Slot) { bHasSlot = true; break; }
	if (!bHasSlot) return;

	Perceptor->PurgeInvalidItems();

	ABaseItem* BestItem  = nullptr;
	float      BestScore = 0.f;
	for (AActor* Actor : Perceptor->GetKnownItems())
	{
		ABaseItem* Item = Cast<ABaseItem>(Actor);
		if (!Item) continue;
		const float Score = UStudentPerceptor::ScoreItem(Item, Pawn);
		if (Score > BestScore) { BestScore = Score; BestItem = Item; }
	}

	if (!BestItem) { ItemRecallTimer = 0.f; return; }

	if (BestItem == LastRecalledItem)
	{
		Perceptor->RemoveKnownItem(BestItem);
		LastRecalledItem = nullptr;
		ItemRecallTimer  = 0.f;
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
			FString::Printf(TEXT("Unreachable item forgotten: %s"), *BestItem->GetName()));
		return;
	}

	LastRecalledItem = BestItem;
	ItemRecallTimer  = 0.f;
	BB->SetValueAsObject(BBK_TargetItem, BestItem);
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
		FString::Printf(TEXT("Recalling item (%.0f): %s"), BestScore, *BestItem->GetName()));
}
