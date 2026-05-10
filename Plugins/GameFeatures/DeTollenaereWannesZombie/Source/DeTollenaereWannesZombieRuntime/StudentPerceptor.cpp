// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "Zombies/BaseZombie.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"
#include "PurgeZones/PurgeZone.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

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
	if (!Actor || !Stimulus.WasSuccessfullySensed()) return;

	UBlackboardComponent* Blackboard = GetBlackboard();
	if (!Blackboard) return;

	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Actor))
	{
		Blackboard->SetValueAsBool(BBK_HasZombieInSight, true);
		Blackboard->SetValueAsObject(BBK_ClosestZombie, Zombie);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Zombie spotted!"));
	}
	else if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		Blackboard->SetValueAsObject(BBK_TargetItem, Item);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Item spotted!"));
	}
	else if (AHouse* House = Cast<AHouse>(Actor))
	{
		Blackboard->SetValueAsObject(BBK_TargetHouse, House);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("House spotted!"));
	}
}