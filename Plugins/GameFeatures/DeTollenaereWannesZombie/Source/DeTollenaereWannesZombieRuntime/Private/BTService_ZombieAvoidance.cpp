#include "BTService_ZombieAvoidance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Common/InventoryComponent.h"
#include "Items/Weapon.h"
#include "Zombies/BaseZombie.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

static const FName BBK_HasZombieInSight("HasZombieInSight");

UBTService_ZombieAvoidance::UBTService_ZombieAvoidance()
{
	NodeName        = "Zombie Avoidance";
	bNotifyTick     = true;
	Interval        = 0.f;
	RandomDeviation = 0.f;
}


void UBTService_ZombieAvoidance::TickNode(UBehaviorTreeComponent& OwnerComp,
                                           uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FZombieAvoidanceMemory* Mem = reinterpret_cast<FZombieAvoidanceMemory*>(NodeMemory);

	// Initialise randomised peek interval on first tick
	if (Mem->PeekInterval <= 0.f)
		Mem->PeekInterval = FMath::FRandRange(PeekIntervalMin, PeekIntervalMax);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	// peek behind
	Mem->PeekTimer += DeltaSeconds;
	if (Mem->PeekTimer >= Mem->PeekInterval)
	{
		Mem->PeekTimer    = 0.f;
		Mem->PeekInterval = FMath::FRandRange(PeekIntervalMin, PeekIntervalMax);
		TryPeekBehind(OwnerComp, Pawn, BB, Mem);
	}

	// avoidance and panic fire
	const bool bZombieActive = BB->GetValueAsBool(BBK_HasZombieInSight);
	AActor* Threat = bZombieActive
	    ? Cast<AActor>(BB->GetValueAsObject(ThreatKey.SelectedKeyName))
	    : nullptr;

	if (!Threat)
	{
		Mem->FireCooldown = FMath::Max(0.f, Mem->FireCooldown - DeltaSeconds);
		Mem->FireArmTimer = 0.f; 
		return;
	}

	const FVector SurvivorPos = Pawn->GetActorLocation();
	const FVector ThreatPos   = Threat->GetActorLocation();
	const float   ThreatDist  = FVector::Dist2D(SurvivorPos, ThreatPos);

	Mem->FireCooldown  = FMath::Max(0.f, Mem->FireCooldown  - DeltaSeconds);
	Mem->FireArmTimer += DeltaSeconds;

	// panic fire when:
	if (ThreatDist    <= PanicFireRange &&
	    Mem->FireCooldown <= 0.f &&
	    Mem->FireArmTimer >= PanicFireArmDelay)
	{
		TryPanicFire(Pawn, AIC, ThreatPos, Mem);
	}

	// Steering avoidance
	if (ThreatDist < AvoidanceRadius && ThreatDist > KINDA_SMALL_NUMBER)
		ApplyAvoidance(Pawn, ThreatPos, ThreatDist);
}


// avoidance steering

void UBTService_ZombieAvoidance::ApplyAvoidance(APawn* Pawn, const FVector& ThreatPos,float ThreatDist) const
{
	const FVector SurvivorPos = Pawn->GetActorLocation();
	const FVector ToThreatDir = (ThreatPos - SurvivorPos) / ThreatDist;

	const float Urgency = FMath::Clamp(1.f - ThreatDist / AvoidanceRadius, 0.f, 1.f);

	// Away direction
	const FVector AwayDir = -ToThreatDir;

	// Sidestep direction — pick the side that matches current velocity
	const FVector MoveDir = Pawn->GetVelocity().GetSafeNormal2D();
	const FVector Perp1   = FVector(-ToThreatDir.Y,  ToThreatDir.X, 0.f);
	const FVector Perp2   = -Perp1;
	const FVector SideDir = (FVector::DotProduct(Perp1, MoveDir) >= 0.f) ? Perp1 : Perp2;

	// Ramp sidestep bias toward 0.9 when heading directly into the zombie so
	// the survivor ducks sideways instead of fighting the path-follow force.
	const float DotHeadOn        = FMath::Max(0.f, FVector::DotProduct(MoveDir, ToThreatDir));
	const float EffectiveSidestep = FMath::Lerp(SidestepBias, 0.9f, DotHeadOn);

	const FVector AvoidDir = (AwayDir * (1.f - EffectiveSidestep)
	                        + SideDir * EffectiveSidestep).GetSafeNormal2D();

	Pawn->AddMovementInput(AvoidDir, Urgency * RepulsionStrength);
}

// panic fire

void UBTService_ZombieAvoidance::TryPanicFire(APawn* Pawn, AAIController* AIC,
                                               const FVector& ThreatPos,
                                               FZombieAvoidanceMemory* Mem) const
{
	UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	int32   BestSlot   = -1;
	int32   BestDamage = -1;
	AWeapon* BestWeapon = nullptr;

	const TArray<ABaseItem*>& Items = Inv->GetInventory();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		AWeapon* W = Cast<AWeapon>(Items[i]);
		if (!W || W->GetValue() <= 0) continue;

		if (W->GetDamage() > BestDamage)
		{
			BestDamage = W->GetDamage();
			BestSlot   = i;
			BestWeapon = W;
		}
	}

	if (BestSlot == -1) return; // no ammo

	// Face the zombie before firing
	FVector LookDir = (ThreatPos - Pawn->GetActorLocation());
	LookDir.Z = 0.f;
	if (!LookDir.IsNearlyZero())
	{
		const FRotator Rot = LookDir.Rotation();
		Pawn->SetActorRotation(Rot); // needs to be fixed
		AIC->SetControlRotation(Rot);
	}

	if (Inv->UseItem(BestSlot))
	{
		Mem->FireCooldown = FireCooldownTime;

		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Orange,
			FString::Printf(TEXT("Panic fire! (%s)"), *BestWeapon->GetName()));

		// drop weapon if no ammo left
		if (BestWeapon->GetValue() <= 0)
		{
			Inv->RemoveItem(BestSlot);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("Dropped empty weapon."));
		}
	}
}

// Peek behind

void UBTService_ZombieAvoidance::TryPeekBehind(UBehaviorTreeComponent& OwnerComp, APawn* Pawn, UBlackboardComponent* BB,FZombieAvoidanceMemory* Mem) const
{
	const FVector SurvivorPos = Pawn->GetActorLocation();

	// fix rotate character first

	UAIPerceptionComponent* Perc = Pawn->FindComponentByClass<UAIPerceptionComponent>();
	if (!Perc) return;

	TArray<AActor*> KnownActors;
	Perc->GetKnownPerceivedActors(UAISense_Sight::StaticClass(), KnownActors);

	ABaseZombie* NearestZombie = nullptr;
	float NearestDist   = PeekRadius; 

	for (AActor* Actor : KnownActors)
	{
		ABaseZombie* Z = Cast<ABaseZombie>(Actor);
		if (!Z) continue;

		const float Dist = FVector::Dist2D(SurvivorPos, Z->GetActorLocation());
		if (Dist < NearestDist)
		{
			NearestDist   = Dist;
			NearestZombie = Z;
		}
	}

	if (!NearestZombie) return;

	AActor* CurrentThreat = Cast<AActor>(BB->GetValueAsObject(ThreatKey.SelectedKeyName));
	const float CurrentDist = CurrentThreat
	    ? FVector::Dist2D(SurvivorPos, CurrentThreat->GetActorLocation())
	    : FLT_MAX;

	if (NearestDist < CurrentDist)
	{
		BB->SetValueAsBool  (BBK_HasZombieInSight, true);
		BB->SetValueAsObject(ThreatKey.SelectedKeyName, NearestZombie);

		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red,
			FString::Printf(TEXT("Peek: zombie spotted (%.0f UU)"), NearestDist));
	}
}
