// Fill out your copyright notice in the Description page of Project Settings.


#include "SUMOManager.h"

FString GetAbsolutePathOfGameFeatureFile(const FString& RelativeFilePath, const FString& PluginName = TEXT("ue_sumo_feature"))
{
	// Get the plugin directory
	FString PluginBaseDir = IPluginManager::Get().FindPlugin(PluginName)->GetBaseDir();

	// Combine with relative path inside the GameFeature
	FString FullPath = FPaths::Combine(PluginBaseDir, RelativeFilePath);

	// Convert to absolute path
	return FPaths::ConvertRelativePathToFull(FullPath);
}

// Sets default values
ASUMOManager::ASUMOManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Status = 0;
	Time = 0;
	Mode = ESumoMode::Unset;
}

// Called when the game starts or when spawned
void ASUMOManager::BeginPlay()
{
	Super::BeginPlay();
	if (bUseBPConfiguration) {
		FString p = GetAbsolutePathOfGameFeatureFile(ConfigFilePath);
		UE_LOG(LogTemp, Log, TEXT("sumo: %s "), *p);
		Parameters = p;
		Mode = ESumoMode::Configured;
	}
}

// Called every frame
void ASUMOManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	int _state = 0;
	switch (Mode)
	{
	case ESumoMode::Unset:
		break;
	case ESumoMode::Configured:
		LoadScenario();
	case ESumoMode::Running:
		Time += DeltaTime;
		_state = sumo_step(Time);
		if (_state != 0) {
			UE_LOG(LogTemp, Log, TEXT("sumo: Error during simulation step, returns %d"), _state);
			return;
		}
		sendVehicleUpdates();
		sendTrafficLightUpdates();
		if (sumo_get_is_done() < 1) {
			Mode = ESumoMode::Done;
			UE_LOG(LogTemp, Log, TEXT("sumo: no vehicles left to simulate or time past"));
		}
		break;
	case ESumoMode::Done:
		if (GIsEditor)
		{
			// Running inside Unreal Editor
			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("Exit"));
		}
		else
		{
			// Running as packaged game or standalone
			UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
		}

		break;
	default:
		break;
	}
}

void ASUMOManager::Configure(TMap<FString, FInnerConfig> Configurations) {
	if (Configurations.Num() > 1) {
		UE_LOG(LogTemp, Error, TEXT("sumo: Got multiple configurations."));
		return;
	}
	for (const TPair<FName, FInnerConfig>& Pair : Configurations)
	{
		const FString* valueParameters = Pair.Value.Entries.Find("Parameters");
		if (valueParameters != nullptr) {
			Parameters = *valueParameters;
		}
		Mode = ESumoMode::Configured;
	}
	bUseBPConfiguration = false;
}

void ASUMOManager::sendVehicleUpdates() {
	int vehicle_count = sumo_get_vehicle_ids(&vehIDs);
	for (int i = 0; i < vehicle_count; ++i) {
		FString objName(vehIDs[i]);
		vehicle_info vi;
		sumo_get_vehicle_info(vehIDs[i], &vi);
		FString typeName(vi.vclass);
		FTransform t = TranslateToUnrealWorldPosition(vi);
		double  l, w, h;
		sumo_get_vehicle_dimension(vi.id, &l, &w, &h);
		FVehicleDataUpdateData newMessage;
		newMessage.signals = vi.signals;
		newMessage.speed = vi.speed;
		newMessage.accel = vi.accel;
		newMessage.BoundingBox.Set(l, w, h);
		newMessage.Name = FName(objName);
		newMessage.VehicleClass = FName(vi.vclass);
		newMessage.Position = t;
		newMessage.Provider = PROVIDER_NAME;
		for (auto& j : VehicleDataActorsToInform) {
			IVehicleDataUpdate::Execute_VehicleDataUpdate(j, newMessage);
		}
	}
}

void ASUMOManager::sendTrafficLightUpdates() {
	int tl_count = sumo_get_trafficlights(&tlIds, &tlStates);
	for (int i = 0; i < tl_count; ++i) {
		FTrafficLightData trafficLightData;
		trafficLightData.id = tlIds[i];
		trafficLightData.lightState = tlStates[i];

		FString* lastTrafficLightState = LastTrafficLightStates.Find(trafficLightData.id);
		if (lastTrafficLightState != nullptr) {
			if (*lastTrafficLightState == trafficLightData.lightState) {
				//PROBLEM: some traffic light plugins spawn the lightbulbs during the first simulation step and are not available before
				//FIX: send the traffic light data for additional timesteps even without changing states
				if (sendTrafficLightsForExtraStartTimeSteps != trafficLightTimeSteps) {
					trafficLightTimeSteps++;
				}
				else {
					//no updates, skip interface calls
					return;
				}
			}
		}
		LastTrafficLightStates.Add(trafficLightData.id, trafficLightData.lightState);

		//UE_LOG(LogTemp, Log, TEXT("Traffic light: %s State: %s"), *trafficLightData.id.ToString(), *trafficLightData.lightState.ToString());
		for (auto& j : TrafficLightActorsToInform) {
			ITrafficLightDataUpdate::Execute_TrafficLightDataUpdate(j, trafficLightData);
		}
	}
}

void ASUMOManager::RegisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>()) {
		VehicleDataActorsToInform.AddUnique(vehicleDataReceiver);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Tried to register an actor without UEsminiVehicleDataUpdate."));
	}
}

void ASUMOManager::UnregisterVehicleDataReceiver_Implementation(AActor* vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>()) {
		VehicleDataActorsToInform.Remove(vehicleDataReceiver);
	}
}

void ASUMOManager::RegisterTrafficLight_Implementation(AActor* trafficLight)
{
	if (trafficLight && trafficLight->Implements<UTrafficLightDataUpdate>()) {
		TrafficLightActorsToInform.AddUnique(trafficLight);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Tried to register an actor without UEsminiVehicleDataUpdate."));
	}
}

void ASUMOManager::UnregisterTrafficLight_Implementation(AActor* trafficLight)
{
	if (trafficLight && trafficLight->Implements<UTrafficLightDataUpdate>()) {
		TrafficLightActorsToInform.Remove(trafficLight);
	}
}

void ASUMOManager::LoadScenario() {
	if (!bUseBPConfiguration) {
		if (ConfigFilePath.IsEmpty()) {
			UE_LOG(LogTemp, Warning, TEXT("sumo: config file path is empty."));
			UE_LOG(LogTemp, Warning, TEXT("sumo: take ExamplePath1(%s)."), *ExamplePath1);
			ConfigFilePath = ExamplePath1;
			//Mode = ESumoMode::Unset;
			//bUseCfgFile = false;
			//return;
		}
		FString fullPath = GetAbsolutePathOfGameFeatureFile(ConfigFilePath);
		UE_LOG(LogTemp, Log, TEXT("sumo: file: %s"), *fullPath);
		std::string StdFilename(TCHAR_TO_UTF8(*fullPath));
		std::string _StepLength = std::to_string(StepLength);
		const char* CParam[] = { "-c", StdFilename.c_str(), "--step-length", _StepLength.c_str(), NULL };
		Status = sumo_load_with_paramenters(CParam);
		if (Status != 0) {
			FString E(sumo_get_last_error());
			UE_LOG(LogTemp, Warning, TEXT("%s"), *E);
			Mode = ESumoMode::Done;
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("sumo: load returns: %d"), Status);
		Mode = ESumoMode::Running;
		return;
	}
	TArray<std::string> Storage;      // owns the bytes
	TArray<const char*> Params;       // raw const char* pointers
	Storage.Add(std::string(TCHAR_TO_UTF8(*Parameters)));

	Params.Add(Storage.Last().c_str());
	Params.Add(NULL);

	Status = sumo_load_with_paramenters(Params.GetData());
	if (Status != 0) {
		FString E(sumo_get_last_error());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *E);
		Mode = ESumoMode::Done;
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("sumo: load returns: %d"), Status);

	Mode = ESumoMode::Running;
}

void ASUMOManager::VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) {
	if (data.Provider.IsEqual(PROVIDER_NAME)) {
		//skip, use for testing
		UE_LOG(LogTemp, Warning, TEXT("sumo: should not receive own data %s "), *data.Name.ToString());
	}
	else {
		//todo
	}
}

FTransform ASUMOManager::TranslateToUnrealWorldPosition(const vehicle_info& obj)
{
	FTransform transform;
	transform.SetLocation(CoordinateSystemOffset.InverseTransformPosition(FVector(obj.y * 100, obj.x * 100, obj.z * 100)));
	transform.SetRotation(CoordinateSystemOffset.InverseTransformRotation(FRotator(0, obj.angle, 0).Quaternion()));
	return transform;
}

bool ASUMOManager::RegisterDataflow_Implementation(AActor* consumer, FString& producer, GEMSTARInterfaceType interfaceName) {
	switch (interfaceName) {
	case GEMSTARInterfaceType::TRAFFICUPDATE:
		VehicleDataActorsToInform.AddUnique(consumer);
		consumer->AddTickPrerequisiteActor(this);
		return true;
	case GEMSTARInterfaceType::TRAFFICLIGHTUPDATE:
		TrafficLightActorsToInform.AddUnique(consumer);
		consumer->AddTickPrerequisiteActor(this);
		return true;
	default:
		return false;
	}
}
