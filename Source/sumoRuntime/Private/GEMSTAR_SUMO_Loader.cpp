// Fill out your copyright notice in the Description page of Project Settings.


#include "GEMSTAR_SUMO_Loader.h"

// Sets default values for this component's properties
UGEMSTAR_SUMO_Loader::UGEMSTAR_SUMO_Loader()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UGEMSTAR_SUMO_Loader::BeginPlay()
{
	Super::BeginPlay();
	AActor* OwnerActor = GetOwner();
	AGEMSTARRoot* GemstarRoot = Cast<AGEMSTARRoot>(OwnerActor);
	if (GemstarRoot)
	{
		// Check Use SUMO
		if (GemstarRoot->UseSUMO)
		{
			FTransform SpawnTransform;

			FActorSpawnParameters SpawnParams;
			ASUMOManager* SpawnedSumoManager = GetWorld()->SpawnActor<ASUMOManager>(ASUMOManager::StaticClass(), SpawnTransform, SpawnParams);

			TMap<FString, FInnerConfig> config = GemstarRoot->RegisterManagerAndGetConfiguration(SpawnedSumoManager);
			SpawnedSumoManager->Configure(config);
			UE_LOG(LogTemp, Log, TEXT("Registered and configured SUMO"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Error: Cast of GEMSTARRoot was not successful"));
	}
}

// Called every frame
void UGEMSTAR_SUMO_Loader::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
