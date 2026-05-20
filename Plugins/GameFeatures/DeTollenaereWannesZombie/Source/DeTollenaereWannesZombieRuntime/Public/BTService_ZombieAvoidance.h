#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_ZombieAvoidance.generated.h"


// UNUSED

struct FZombieAvoidanceMemory
{
	float FireCooldown = 0.f;
	float FireArmTimer = 0.f;
	float PeekTimer = 0.f;
	float PeekInterval = 0.f;

	float PathRecalculateTimer = 0.f;
	FVector CachedNextWaypoint = FVector::ZeroVector;

	bool bIsPanicTurning = false;
	FVector PanicTurnTargetPos = FVector::ZeroVector;

	bool bIsPeekTurning = false;
	float PeekDurationTimer = 0.f;
	FVector PeekLookAtDirection = FVector::ZeroVector;
};

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTService_ZombieAvoidance : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_ZombieAvoidance();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ThreatKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float PathFollowingWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float ZombieRepulsionWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float SidestepWeight = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Path")
	float PathRecalculateInterval = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Path")
	float WaypointToleranceRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
	float AvoidanceRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SidestepBias = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PanicFire")
	float PanicFireRange = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PanicFire")
	float PanicFireArmDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PanicFire")
	float FireCooldownTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PeekBehind")
	float PeekRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PeekBehind")
	float PeekIntervalMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PeekBehind")
	float PeekIntervalMax = 2.0f;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override
	{
		return sizeof(FZombieAvoidanceMemory);
	}

private:
	void TryPanicFire(APawn* Pawn, AAIController* AIC, const FVector& ThreatPos, FZombieAvoidanceMemory* Mem) const;
	void TryPeekBehind(UBehaviorTreeComponent& OwnerComp, APawn* Pawn, UBlackboardComponent* BB, FZombieAvoidanceMemory* Mem) const;
};