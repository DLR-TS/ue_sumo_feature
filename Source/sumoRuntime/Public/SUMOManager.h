// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "sumo_wrapper.h"
#include "GEMSTARRoot.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "VehicleDataDispatcher.h"
#include "TrafficLightDataDispatcher.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "SUMOManager.generated.h"


UENUM(BlueprintType)
enum class ESumoMode : uint8
{
	Unset UMETA(DisplayName = "Configure was not called"),
	Configured UMETA(DisplayName = "Configure was called"),
	Running UMETA(DisplayName = "libsumo::load was called"),
	Done UMETA(DisplayName = "sumo is done")
};

UCLASS()
class SUMORUNTIME_API ASUMOManager : public AActor,
	public IVehicleDataDispatcher,
	public IVehicleDataUpdate,
	public ITrafficLightDataDispatcher,
	public IGEMSTARManager
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASUMOManager();

	void RegisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver) override;
	void UnregisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver) override;
	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;
	void RegisterTrafficLight_Implementation(AActor* trafficLight) override;
	void UnregisterTrafficLight_Implementation(AActor* trafficLight) override;
	void Configure(TMap<FString, FInnerConfig> Configurations);
	virtual bool RegisterDataflow_Implementation(AActor* consumer, FString& producer, GEMSTARInterfaceType interfaceName) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	void sendVehicleUpdates();
	void sendTrafficLightUpdates();

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	FString ConfigFilePath = "Resources/simple-intersection/simple-intersection.sumocfg";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	bool bUseBPConfiguration = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	FString ExamplePath1 = "Resources/simple-intersection/simple-intersection.sumocfg";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	FString ExamplePath2 = "Resources/sl-intersection/sl-intersection.sumocfg";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	FString ExamplePath3 = "Resources/square/square.sumocfg";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	int Status;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	ESumoMode Mode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	FString Parameters;

	UFUNCTION(BlueprintCallable, Category = "sumo")
	void LoadScenario();

	UPROPERTY(EditAnywhere, Category = "sumo")
	FTransform CoordinateSystemOffset;
private:
	FName PROVIDER_NAME{ "sumo" };
	float Time = 0;
	float StepLength = 0.05;
	const char** tlIds = nullptr;
	const char** tlStates = nullptr;
	const char** vehIDs = nullptr;
	int sendTrafficLightsForExtraStartTimeSteps = 2;
	int trafficLightTimeSteps = 0;

	TMap<FName, FString> LastTrafficLightStates;
	TArray<AActor*> VehicleDataActorsToInform;
	TArray<AActor*> TrafficLightActorsToInform;
	FTransform TranslateToUnrealWorldPosition(const vehicle_info& obj);
};
