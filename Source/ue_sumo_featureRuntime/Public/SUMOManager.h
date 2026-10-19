/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#pragma once

#include "Subsystems/GemstarSimulationSubsystem.h"
#include "Interfaces/DataTarget.h"
#include "Interfaces/Movement/VRUDataUpdate.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/Movement/VehicleDataDispatcher.h"
#include "Interfaces/TrafficLight/TrafficLightDataDispatcher.h"
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
class UE_SUMO_FEATURERUNTIME_API ASUMOManager : public AActor,
	public IVehicleDataDispatcher,
	public IVehicleDataUpdate,
	public ITrafficLightDataDispatcher,
	public IGemstarManager
{
	GENERATED_BODY()

public:
	ASUMOManager();

	void RegisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver) override;
	void UnregisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver) override;
	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;
	void RegisterTrafficLight_Implementation(AActor* trafficLight) override;
	void UnregisterTrafficLight_Implementation(AActor* trafficLight) override;
	void Configure(TMap<FString, FInnerConfig> Configurations, UGemstarSimulationSubsystem* SimulationSubsystem);
    // virtual bool UnregisterDataflow_Implementation(FDataTarget& dataTarget, FName interfaceName) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	FString ConfigFilePath = "simple-intersection/simple-intersection.sumocfg";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	bool bUseBPConfiguration = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	FString ExamplePath1 = "simple-intersection/simple-intersection.sumocfg";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	FString ExamplePath2 = "sl-intersection/sl-intersection.sumocfg";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	FString ExamplePath3 = "square/square.sumocfg";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	int Status = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "sumo")
	ESumoMode Mode = ESumoMode::Unset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	FString Parameters;

	UFUNCTION(BlueprintCallable, Category = "sumo")
	void LoadScenario();

	UPROPERTY(EditAnywhere, Category = "sumo")
	FTransform CoordinateSystemOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "sumo")
	bool bDebugLogging = false;

private:
	FName PROVIDER_NAME{"ue_sumo_feature"};
	float Time = 0.0f;
	float StepLength = 0.05f;
	int sendTrafficLightsForExtraStartTimeSteps = 2;
	int trafficLightTimeSteps = 0;
	FString FullPath;

    UPROPERTY()
	TObjectPtr<UGemstarSimulationSubsystem> GemstarSimulationSubsystem;

	TMap<FName, FString> LastTrafficLightStates;

	UPROPERTY()
	TArray<FDataTarget> VehicleActorsToInform;

	TArray<FDataTarget> VRUActorsToInform;

	UPROPERTY()
	TArray<FDataTarget> TrafficLightActorsToInform;

	TArray<FDataTarget> WeatherEffectActorsToInform;

	FTransform TranslateToUnrealWorldPosition(double X, double Y, double Z, double Angle);

	FString GetSumoRootDirectory();

	void SendVehicleUpdates();
	void SendPersonUpdates();
	void SendTrafficLightUpdates();
	void SendWeatherUpdate();

	void AddSumoLoggingParameters(TArray<FString> &Params);
	bool HasSumoOption(const TArray<FString> &Params, const TCHAR *OptionName);

	bool GetOptionValue(const TArray<FString> &Arguments, const FString &Parameter, FString &OutValue);
	bool SetOptionValue(TArray<FString> &Arguments, const FString &Parameter, const FString &InValue);
	TArray<FString> ParseOptions(const FString &Options);

	int LoadSumoWithParameters(const TArray<FString> &Parsed);
};
