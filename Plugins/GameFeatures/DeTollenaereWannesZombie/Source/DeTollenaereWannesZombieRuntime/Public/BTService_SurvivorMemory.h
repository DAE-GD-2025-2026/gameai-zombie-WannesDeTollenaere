#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SurvivorMemory.generated.h"

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTService_SurvivorMemory : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SurvivorMemory();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	                      float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory")
	float SpeedMargin = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory")
	float HouseArrivalRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory")
	float UnscoutedHouseBonus = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory")
	float HouseReselectInterval = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory")
	float ItemRecallCooldown = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Memory")
	float ItemStaleTimeout = 12.f;

private:
	float HouseReselectTimer = 0.f;
	float ItemRecallTimer = 0.f;
	float TargetItemAge = 0.f;

	UPROPERTY()
	AActor* LastRecalledItem = nullptr;

	UPROPERTY()
	AActor* TrackedTargetItem = nullptr;

	static float GetMaxSpeed(AActor* Actor);

	float ScoreHouseForVisit(const struct FHouseMemory& HMem, APawn* Pawn) const;
};
