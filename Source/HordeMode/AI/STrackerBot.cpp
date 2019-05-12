// Fill out your copyright notice in the Description page of Project Settings.

#include "STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include <NavigationSystem/Public/NavigationPath.h>
#include <NavigationSystem/Public/NavigationSystem.h>
#include <Kismet/GameplayStatics.h>
#include <GameFramework/Character.h>
#include <DrawDebugHelpers.h>
#include "HordeMode/Components/SHealthComponent.h"
#include <Particles/ParticleSystemComponent.h>

ASTrackerBot::ASTrackerBot()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetSimulatePhysics(true);
	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));
	RootComponent = MeshComponent;
	bUseVelocityChange = false;
	moveMentForce = 1000.0f;
	requiredDistanceToTarget = 100.0f;
	isDead = false;
	explosionRadius = 300.0f;
	damageAmount = 80.0f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	NextPathVector = getNextLocation();

	HealthComponent->OnHealthChanged.AddDynamic(this, &ASTrackerBot::OnTakeDamage);
}

void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float distanceToTarget = (GetActorLocation() - NextPathVector).Size();

	if (distanceToTarget <= requiredDistanceToTarget) {
		NextPathVector = getNextLocation();

		DrawDebugString(GetWorld(), GetActorLocation(), "TargetReached");
	}
	else {
		FVector forceDirection = NextPathVector - GetActorLocation();
		forceDirection.Normalize(); //get direction
		forceDirection *= moveMentForce; //scale
		MeshComponent->AddForce(forceDirection, NAME_None, bUseVelocityChange);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + forceDirection, 32, FColor::Green, false, 0.0);
	}

	DrawDebugSphere(GetWorld(), NextPathVector, 20, 12, FColor::Yellow, false, 0.0f, 1.0f);
}

void ASTrackerBot::SelfDestruct()
{
	isDead = true;

	if(ExplodeEffect)
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplodeEffect, GetActorLocation(), GetActorRotation());

	TArray<AActor *> IgnoredActors;
	IgnoredActors.Add(this);

	UGameplayStatics::ApplyRadialDamage(this, damageAmount, GetActorLocation(), explosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);
	
	DrawDebugSphere(GetWorld(), GetActorLocation(), explosionRadius, 10, FColor::Green, false, 2.0f, 0, 1.0f);

	Destroy();
}

FVector ASTrackerBot::getNextLocation()
{
	// hack to get player location, won't work in multi player
	ACharacter * playerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);
	UNavigationPath * navPath = UNavigationSystemV1::FindPathToActorSynchronously(GetWorld(), GetActorLocation(), playerPawn);

	if (navPath->PathPoints.Num() > 1) {
		// current is at index 0, next is at 1
		return navPath->PathPoints[1];
	}

	return GetActorLocation();
}

void ASTrackerBot::OnTakeDamage(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	// dynamic instance at runtime, in case of multiple instances
	if(currentMaterialOnMesh == nullptr)
	currentMaterialOnMesh = MeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComponent->GetMaterial(0));

	if (currentMaterialOnMesh != nullptr)
	currentMaterialOnMesh->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);

	if (Health <= 0.0f && !isDead)
		SelfDestruct();

	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(Health), *GetName());
}

