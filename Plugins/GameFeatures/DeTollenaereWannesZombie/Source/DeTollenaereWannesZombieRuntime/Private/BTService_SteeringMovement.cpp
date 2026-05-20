#include "BTService_SteeringMovement.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "SurvivorBlackboardKeys.h"
#include "DrawDebugHelpers.h"

UBTService_SteeringMovement::UBTService_SteeringMovement()
{
	NodeName = "Steering Locomotion Driver";
	bNotifyTick = true;
	Interval = 0.f;
	RandomDeviation = 0.f;
}

void UBTService_SteeringMovement::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FSteeringMovementMemory* Mem = reinterpret_cast<FSteeringMovementMemory*>(NodeMemory);
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!AIC || !Pawn || !BB) return;

	const FVector SurvivorPos = Pawn->GetActorLocation();


	FVector TargetLocation = FVector::ZeroVector;
	bool bHasValidTarget = false;

	AActor* ActiveTarget = Cast<AActor>(BB->GetValueAsObject(BBK_TargetItem));
	if (!ActiveTarget)
	{
		ActiveTarget = Cast<AActor>(BB->GetValueAsObject(BBK_TargetHouse));
	}

	if (ActiveTarget && ::IsValid(ActiveTarget))
	{
		TargetLocation = ActiveTarget->GetActorLocation();
		bHasValidTarget = true;
	}
	else
	{
		if (BB->IsVectorValueSet(BBK_TargetLocation))
		{
			TargetLocation = BB->GetValueAsVector(BBK_TargetLocation);
			bHasValidTarget = true;
		}
	}

	// Seek
	FVector PathDriveForce = FVector::ZeroVector;
	if (bHasValidTarget)
	{
		Mem->PathRecalculateTimer += DeltaSeconds;
		if (Mem->PathRecalculateTimer >= PathRecalculateInterval || Mem->CachedNextWaypoint.IsZero())
		{
			Mem->PathRecalculateTimer = 0.f;
			if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld()))
			{
				UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(Pawn->GetWorld(), SurvivorPos, TargetLocation);
				if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 1)
				{
					Mem->CachedNextWaypoint = TargetLocation;
					for (int32 i = 1; i < NavPath->PathPoints.Num(); ++i)
					{
						if (FVector::Dist2D(SurvivorPos, NavPath->PathPoints[i]) > WaypointToleranceRadius)
						{
							Mem->CachedNextWaypoint = NavPath->PathPoints[i];
							break;
						}
					}
				}
				else
				{
					Mem->CachedNextWaypoint = TargetLocation;
				}
			}
		}

		if (!Mem->CachedNextWaypoint.IsZero())
		{
			PathDriveForce = (Mem->CachedNextWaypoint - SurvivorPos).GetSafeNormal2D();
		}
	}
	else
	{
		Mem->CachedNextWaypoint = FVector::ZeroVector;
	}

	FVector RepulsionForce = FVector::ZeroVector;
	FVector SidestepForce = FVector::ZeroVector;
	FVector FleeForce = FVector::ZeroVector; // NEW

	const bool bZombieActive = BB->GetValueAsBool(BBK_HasZombieInSight);
	AActor* Threat = bZombieActive ? Cast<AActor>(BB->GetValueAsObject(ThreatKey.SelectedKeyName)) : nullptr;

	// rotation lock when in combat
	const bool bLockRotationKey = BB->GetValueAsBool(FName("IsExecutingCombatRotation"));

	if (Threat && ::IsValid(Threat))
	{
		const FVector ThreatPos = Threat->GetActorLocation();
		const float ThreatDist = FVector::Dist2D(SurvivorPos, ThreatPos);
		const FVector ToThreatDir = (ThreatPos - SurvivorPos).GetSafeNormal2D(); // Calculated early

		FleeForce = -ToThreatDir;

		if (ThreatDist < AvoidanceRadius && ThreatDist > KINDA_SMALL_NUMBER)
		{
			const float Urgency = FMath::Clamp(1.f - (ThreatDist / AvoidanceRadius), 0.f, 1.f);

			RepulsionForce = -ToThreatDir * Urgency;

			if (!PathDriveForce.IsNearlyZero())
			{
				const FVector Perp1 = FVector(-ToThreatDir.Y, ToThreatDir.X, 0.f);
				const FVector Perp2 = -Perp1;
				FVector ChosenSide = (FVector::DotProduct(Perp1, PathDriveForce) >= 0.f) ? Perp1 : Perp2;

				const float DotHeadOn = FMath::Max(0.f, FVector::DotProduct(PathDriveForce, ToThreatDir));
				const float EffectiveSidestepBias = FMath::Lerp(SidestepBias, 0.9f, DotHeadOn);

				SidestepForce = ChosenSide * EffectiveSidestepBias * Urgency;
			}
		}
	}

	// Apply steering
	FVector BlendedSteeringVector = (PathDriveForce * PathFollowingWeight) +
		(RepulsionForce * ZombieRepulsionWeight) +
		(SidestepForce * SidestepWeight) +
		(FleeForce * FleeWeight);

	UWorld* World = Pawn->GetWorld();
	if (World)
	{
		FVector DebugOrigin = SurvivorPos + FVector(0.f, 0.f, 50.f);

		//full navigation path array and render every segment
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld()))
		{
			UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(Pawn->GetWorld(), SurvivorPos, TargetLocation);
			if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 1)
			{
				for (int32 i = 0; i < NavPath->PathPoints.Num() - 1; ++i)
				{
					FVector CurrentNode = NavPath->PathPoints[i] + FVector(0.f, 0.f, 50.f);
					FVector NextNode = NavPath->PathPoints[i + 1] + FVector(0.f, 0.f, 50.f);

					DrawDebugLine(World, CurrentNode, NextNode, FColor::Cyan, false, -1.f, 0, 3.0f);

					DrawDebugSphere(World, NextNode, 12.f, 6, FColor::Cyan, false, -1.f, 0, 1.0f);
				}
			}

			else if (!TargetLocation.IsZero())
			{
				DrawDebugLine(World, DebugOrigin, TargetLocation + FVector(0.f, 0.f, 50.f), FColor::Cyan, false, -1.f, 0, 2.0f);
			}
		}

		if (!PathDriveForce.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(World, DebugOrigin, DebugOrigin + (PathDriveForce * 120.f), 25.f, FColor::Green, false, -1.f, 0, 3.5f);
		}

		if (!RepulsionForce.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(World, DebugOrigin, DebugOrigin + (RepulsionForce * 150.f), 25.f, FColor::Red, false, -1.f, 0, 4.0f);
		}

		if (!SidestepForce.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(World, DebugOrigin, DebugOrigin + (SidestepForce * 120.f), 25.f, FColor::Yellow, false, -1.f, 0, 3.5f);
		}

		if (!BlendedSteeringVector.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(World, DebugOrigin, DebugOrigin + (BlendedSteeringVector.GetSafeNormal2D() * 180.f), 35.f, FColor::Blue, false, -1.f, 0, 6.0f);
		}
	}

	if (!BlendedSteeringVector.IsNearlyZero())
	{
		BlendedSteeringVector = BlendedSteeringVector.GetSafeNormal2D();
		Pawn->AddMovementInput(BlendedSteeringVector, 1.0f);
	}

	if (!BB->GetValueAsBool(FName("IsExecutingCombatRotation")))
	{
		FRotator TargetRotation = BlendedSteeringVector.IsNearlyZero() ? Pawn->GetActorRotation() : BlendedSteeringVector.Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		// turning
		FRotator NewRotation = FMath::RInterpConstantTo(Pawn->GetActorRotation(), TargetRotation, DeltaSeconds, 1800);

		Pawn->SetActorRotation(NewRotation);
		AIC->SetControlRotation(NewRotation);
	}

}