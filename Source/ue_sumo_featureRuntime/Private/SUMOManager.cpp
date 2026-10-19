/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */


#include "SUMOManager.h"
#include "Interfaces/Weather/WeatherDataUpdate.h"
#include "SUMOLog.h"
#include "Utils/StandaloneWeatherParser.h"
#include "SUMOBridge.h"

#include <cmath>
#include <string>
#include <vector>

ASUMOManager::ASUMOManager()
{
	PrimaryActorTick.bCanEverTick = true;

	auto Wire = [this](TArray<FDataTarget> *List)
	{
		return [this, List](FDataTarget &Target, const FString &Producer)
		{
			// Only requests addressed to this producer
			if (!Producer.Equals(PROVIDER_NAME.ToString(), ESearchCase::IgnoreCase))
			{
				return false;
			}
			List->AddUnique(Target);
			Target.Actor->AddTickPrerequisiteActor(this);
			return true;
		};
	};
	auto Unwire = [this](TArray<FDataTarget> *List)
	{
		return [this, List](FDataTarget &Target, const FString &)
		{
			const int32 Removed = List->Remove(Target);
			if (Removed > 0 && Target.Actor)
			{
				Target.Actor->RemoveTickPrerequisiteActor(this);
			}
			return Removed > 0;
		};
	};

	AcceptChannel(GemstarInterfaceType::TRAFFICUPDATE, Wire(&VehicleActorsToInform), Unwire(&VehicleActorsToInform));
	AcceptChannel(GemstarInterfaceType::TRAFFICLIGHTUPDATE, Wire(&TrafficLightActorsToInform), Unwire(&TrafficLightActorsToInform));
	AcceptChannel(GemstarInterfaceType::VRUUPDATE, Wire(&VRUActorsToInform), Unwire(&VRUActorsToInform));
	AcceptChannel(GemstarInterfaceType::WEATHERDATA, Wire(&WeatherEffectActorsToInform), Unwire(&WeatherEffectActorsToInform));
}

// Called when the game starts or when spawned
void ASUMOManager::BeginPlay()
{
	Super::BeginPlay();
	if (bUseBPConfiguration) {
		FString p = GemstarHelper::GetAbsolutePathOfGameFeatureFile(ConfigFilePath, "ue_sumo_feature");
		UE_LOG(Sumo, Log, TEXT("Set %s as sumocfg"), *p);
		Parameters = p;
		Mode = ESumoMode::Configured;
	}
}

void ASUMOManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(GemstarSimulationSubsystem)) {
		GemstarSimulationSubsystem->UnregisterManager(this);
		GemstarSimulationSubsystem = nullptr;
	}

	for (FDataTarget DataTarget : VehicleActorsToInform)
	{
		AActor *Consumer = DataTarget.Actor;

		if (IsValid(Consumer))
		{
			Consumer->RemoveTickPrerequisiteActor(this);
		}
	}

	for (FDataTarget DataTarget : TrafficLightActorsToInform)
	{
		AActor *Consumer = DataTarget.Actor;

		if (IsValid(Consumer))
		{
			Consumer->RemoveTickPrerequisiteActor(this);
		}
	}
	VehicleActorsToInform.Reset();
	TrafficLightActorsToInform.Reset();
	LastTrafficLightStates.Reset();
	VRUActorsToInform.Reset();
	if (Mode == ESumoMode::Running || Mode == ESumoMode::Done)
	{
		SumoBridge::Close();
	}
	Mode = ESumoMode::Unset;

	Super::EndPlay(EndPlayReason);
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
		if (SumoBridge::Step(Time) != 0)
		{
			UE_LOG(Sumo, Log, TEXT("Error during simulation step: %s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
			Mode = ESumoMode::Done;
			return;
		}
		SendVehicleUpdates();
		SendPersonUpdates();
		SendTrafficLightUpdates();
		if (SumoBridge::IsDone() != 0)
		{
			Mode = ESumoMode::Done;
			UE_LOG(Sumo, Log, TEXT("No vehicles left to simulate or time past."));
		}
		break;
	case ESumoMode::Done:
		if (IsValid(GemstarSimulationSubsystem) && GemstarSimulationSubsystem->ReportScenarioFinished(this)) {
			SumoBridge::Close();
			Mode = ESumoMode::Unset;
			return;
		}
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


void ASUMOManager::Configure(TMap<FString, FInnerConfig> Configurations, UGemstarSimulationSubsystem* SimulationSubsystem) {
	GemstarSimulationSubsystem = SimulationSubsystem;

	if (Configurations.Num() > 1)
	{
		UE_LOG(Sumo, Error, TEXT("Got multiple configurations."));
		return;
	}
	for (const TPair<FString, FInnerConfig> &Pair : Configurations)
	{
		const FString *valueParameters = Pair.Value.Entries.Find("Parameters");
		if (valueParameters != nullptr)
		{
			Parameters = *valueParameters;
		}
		const FString *Debug = Pair.Value.Entries.Find("Debug");
		if (Debug != nullptr)
		{
			bDebugLogging = *Debug == "true" ? true : false;
			Sumo.SetVerbosity(bDebugLogging ? ELogVerbosity::Verbose : ELogVerbosity::Log);
		}
		Mode = ESumoMode::Configured;
	}
	bUseBPConfiguration = false;
}

void ASUMOManager::SendVehicleUpdates()
{
	const char **VehicleIds = nullptr;
	const int VehicleCount = SumoBridge::GetVehicleIds(&VehicleIds);
	if (VehicleCount < 0)
	{
		UE_LOG(Sumo, Warning, TEXT("%s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
		return;
	}

	for (int Index = 0; Index < VehicleCount; ++Index)
	{
		const char *VehicleId = VehicleIds[Index];
		if (VehicleId == nullptr)
		{
			continue;
		}

		FSumoVehicleInfo VehicleInfo;
		if (SumoBridge::GetVehicleInfo(VehicleId, &VehicleInfo) != 0)
		{
			UE_LOG(Sumo, Warning, TEXT("%s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
			continue;
		}

		FVehicleDataUpdateData newMessage;
		newMessage.signals = VehicleInfo.Signals;
		newMessage.speed = VehicleInfo.Speed;
		newMessage.accel = VehicleInfo.Accel;

		double Length = 0.0;
		double Width = 0.0;
		double Height = 0.0;
		if (SumoBridge::GetVehicleDimension(VehicleId, &Length, &Width, &Height) == 0)
		{
			newMessage.BoundingBox.Set(Length, Width, Height);
		}
		newMessage.Name = FName(UTF8_TO_TCHAR(VehicleInfo.Id));
		newMessage.VehicleClass = FName(UTF8_TO_TCHAR(VehicleInfo.VehicleClass));
		newMessage.Position = TranslateToUnrealWorldPosition(VehicleInfo.X, VehicleInfo.Y, VehicleInfo.Z, VehicleInfo.Angle);
		newMessage.Provider = PROVIDER_NAME;
		for (auto &VehicleActor : VehicleActorsToInform)
		{
			IVehicleDataUpdate::Execute_VehicleDataUpdate(VehicleActor.Actor, newMessage);
		}
	}
}

void ASUMOManager::SendPersonUpdates()
{
	const char **PersonIds = nullptr;
	const int PersonCount = SumoBridge::GetPersons(&PersonIds);
	if (PersonCount < 0)
	{
		UE_LOG(Sumo, Warning, TEXT("%s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
		return;
	}

	for (int Index = 0; Index < PersonCount; ++Index)
	{
		const char *PersonId = PersonIds[Index];
		if (PersonId == nullptr)
		{
			continue;
		}

		FSumoPersonInfo PersonInfo;
		if (SumoBridge::GetPersonInfo(PersonId, &PersonInfo) != 0)
		{
			UE_LOG(Sumo, Warning, TEXT("%s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
			continue;
		}

		FVRUDataUpdateData VRUData;
		VRUData.Name = UTF8_TO_TCHAR(PersonInfo.Id);
		VRUData.Position = TranslateToUnrealWorldPosition(PersonInfo.X, PersonInfo.Y, PersonInfo.Z, PersonInfo.Angle);
		VRUData.Provider = PROVIDER_NAME;
		VRUData.speed = PersonInfo.Speed;
		VRUData.BoundingBox = FVector(30.0, 50.0, 175.0);
		for (auto &VRUActors : VRUActorsToInform)
		{
			IVRUDataUpdate::Execute_VRUDataUpdate(VRUActors.Actor, VRUData);
		}
	}
}

void ASUMOManager::SendTrafficLightUpdates()
{
	const char **TrafficLightIds = nullptr;
	const char **TrafficLightStates = nullptr;
	const int TrafficLightCount = SumoBridge::GetTrafficLights(&TrafficLightIds, &TrafficLightStates);
	if (TrafficLightCount < 0)
	{
		UE_LOG(Sumo, Warning, TEXT("%s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
		return;
	}

	for (int Index = 0; Index < TrafficLightCount; ++Index)
	{
		if (TrafficLightIds[Index] == nullptr || TrafficLightStates[Index] == nullptr)
		{
			continue;
		}

		FTrafficLightData trafficLightData;
		trafficLightData.id = UTF8_TO_TCHAR(TrafficLightIds[Index]);
		trafficLightData.lightState = UTF8_TO_TCHAR(TrafficLightStates[Index]);

		FString *lastTrafficLightState = LastTrafficLightStates.Find(trafficLightData.id);
		if (lastTrafficLightState != nullptr)
		{
			if (*lastTrafficLightState == trafficLightData.lightState)
			{
				if (sendTrafficLightsForExtraStartTimeSteps != trafficLightTimeSteps)
				{
					trafficLightTimeSteps++;
				}
				else
				{
					return;
				}
			}
		}
		LastTrafficLightStates.Add(trafficLightData.id, trafficLightData.lightState);

		for (auto &TrafficLightActors : TrafficLightActorsToInform)
		{
			ITrafficLightDataUpdate::Execute_TrafficLightDataUpdate(TrafficLightActors.Actor, trafficLightData);
		}
	}
}

void ASUMOManager::SendWeatherUpdate()
{
	FString WeatherPath = UStandaloneWeatherParser::FindMatchingWeatherFile(FullPath);
	if (!WeatherPath.IsEmpty())
	{
		FStandaloneWeatherData Weather;
		FString Error;
		if (UStandaloneWeatherParser::ParseEnvironmentFile(WeatherPath, Weather, Error))
		{
			UE_LOG(Sumo, Verbose, TEXT("Temperature: %.1f C | "
									   "Pressure: %.1f hPa | "
									   "Visibility: %.0f m | "
									   "Rain/Snow: %s %.1f mm/h"),
				   Weather.TemperatureCelsius, Weather.AtmosphericPressureHectoPascal,
				   Weather.VisualRangeMeters, *Weather.PrecipitationType,
				   Weather.PrecipitationIntensityMmPerHour);

			FDateTime Dt;
			if (!FDateTime::ParseIso8601(*Weather.TimeOfDayDateTime, Dt))
			{
				Dt = -1;
				UE_LOG(Sumo, Warning,
					   TEXT("Cant convret %s to FDateTime"),
					   *Weather.TimeOfDayDateTime);
			}

			FWeatherData Data;
			// TimeOfDay
			Data.TimeOfDayInfo.bHasTimeOfDay = Weather.bHasTimeOfDay;
			if (Weather.bHasTimeOfDay)
			{
				// OpenSCENARIO dateTime as string
				Data.TimeOfDayInfo.ScenarioDateTime = Weather.TimeOfDayDateTime;
				// DateTime in seconds
				Data.TimeOfDayInfo.TimeOfDayInSeconds = Weather.ParsedTimeOfDay.GetHour() * 3600.0 +
														Weather.ParsedTimeOfDay.GetMinute() * 60.0 +
														Weather.ParsedTimeOfDay.GetSecond();
			}
			// Fog Visibility/ Meteorological Optical Range in meters
			Data.WeatherState.bHasMOR = Weather.bHasFog;
			if (Weather.bHasFog)
			{
				Data.WeatherState.MOR = Weather.VisualRangeMeters;
			}
			// Precipitation intensity in mm/h
			Data.WeatherState.bHasPrecipitation = Weather.bHasPrecipitation;
			if (Weather.bHasPrecipitation)
			{
				Data.WeatherState.PrecipitationIntensity = Weather.PrecipitationIntensityMmPerHour;
				if (Weather.PrecipitationType.Equals(TEXT("Rain"), ESearchCase::IgnoreCase)) {
					Data.WeatherState.PrecipitationType = GemstarPrecipitationType::Rain;

				}
				else if (Weather.PrecipitationType.Equals(TEXT("Snow"), ESearchCase::IgnoreCase)) {
					Data.WeatherState.PrecipitationType = GemstarPrecipitationType::Snow;

				}
				else if (Weather.PrecipitationType.Equals(TEXT("Dry"), ESearchCase::IgnoreCase)) {
					Data.WeatherState.PrecipitationType = GemstarPrecipitationType::Dry;
				}
			}
			// Pressure in Pascal
			Data.WeatherState.bHasPressure = Weather.bHasAtmosphericPressure;
			if (Weather.bHasAtmosphericPressure)
			{
				Data.WeatherState.AtmosphericPressure = Weather.AtmosphericPressurePascal;
			}
			// Temperature in Kelvin
			Data.WeatherState.bHasTemperature = Weather.bHasTemperature;
			if (Weather.bHasTemperature)
			{
				Data.WeatherState.Temperature = Weather.TemperatureKelvin;
			}
			// Wind speed in m/s and direction in radians
			Data.WeatherState.bHasWind = Weather.bHasWind;
			if (Weather.bHasWind)
			{
				Data.WeatherState.WindSpeed = Weather.WindSpeedMetersPerSecond;
				Data.WeatherState.WindDirection = Weather.WindDirectionRadians;
			}
			// FractionalCloudCover
			Data.WeatherState.bHasFractionalCloudCover = Weather.bHasFractionalCloudCover;
			if (Weather.bHasFractionalCloudCover)
			{
				Data.WeatherState.FractionalCloudCover = Weather.FractionalCloudCover_normalized * 100;
			}
			for (auto &WeatherEffectActor : WeatherEffectActorsToInform)
			{
				IWeatherDataUpdate::Execute_WeatherDataUpdate(WeatherEffectActor.Actor, Data, 0);
			}
		}
		else
		{
			UE_LOG(Sumo, Warning, TEXT("Weather parsing failed: %s"), *Error);
		}
	}
}

void ASUMOManager::RegisterVehicleDataReceiver_Implementation(AActor *vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>())
	{
		VehicleActorsToInform.AddUnique({vehicleDataReceiver, 0});
	}
	else
	{
		UE_LOG(Sumo, Log, TEXT("Tried to register an actor without UVehicleDataUpdate."));
	}
}

void ASUMOManager::UnregisterVehicleDataReceiver_Implementation(AActor *vehicleDataReceiver)
{
	if (vehicleDataReceiver && vehicleDataReceiver->Implements<UVehicleDataUpdate>())
	{
		VehicleActorsToInform.Remove({vehicleDataReceiver, 0});
	}
}

void ASUMOManager::RegisterTrafficLight_Implementation(AActor *trafficLight)
{
	if (trafficLight && trafficLight->Implements<UTrafficLightDataUpdate>())
	{
		TrafficLightActorsToInform.AddUnique({trafficLight, 0});
	}
	else
	{
		UE_LOG(Sumo, Log, TEXT("Tried to register an actor without UEsminiVehicleDataUpdate."));
	}
}

void ASUMOManager::UnregisterTrafficLight_Implementation(AActor *trafficLight)
{
	if (trafficLight && trafficLight->Implements<UTrafficLightDataUpdate>())
	{
		TrafficLightActorsToInform.Remove({trafficLight, 0});
	}
}

void ASUMOManager::LoadScenario()
{
	TArray<FString> ParameterArray;
	if (bUseBPConfiguration)
	{
		if (ConfigFilePath.IsEmpty())
		{
			UE_LOG(Sumo, Warning, TEXT("config file path is empty."));
			UE_LOG(Sumo, Warning, TEXT("take ExamplePath1(%s)."), *ExamplePath1);
			ConfigFilePath = ExamplePath1;
		}
		FullPath = ConfigFilePath;
		if (!FPaths::FileExists(ConfigFilePath))
		{
			FString SumoPath = GetSumoRootDirectory();
			FullPath = FPaths::Combine(SumoPath, ConfigFilePath);
		}
		UE_LOG(
			Sumo,
			Verbose,
			TEXT("Path %s: %s"),
			*FullPath,
			FPaths::FileExists(FullPath) ? TEXT("found") : TEXT("not found"));
		TArray<FString> LoadParameters;
		ParameterArray.Add(TEXT("-c"));
		ParameterArray.Add(FullPath);
	}
	else
	{
		UE_LOG(Sumo, Verbose, TEXT("Use Parameters from DefaultGemstar.ini: %s"), *Parameters);
		ParameterArray = ParseOptions(Parameters);

		FString ParamPath;
		if (GetOptionValue(ParameterArray, "-c", ParamPath))
		{
			if (!FPaths::FileExists(ParamPath))
			{
				FString SumoPath = GetSumoRootDirectory();
				UE_LOG(Sumo, Verbose, TEXT("File %s not exists, look in %s"), *ParamPath, *SumoPath);
				FullPath = FPaths::Combine(SumoPath, ParamPath);
				UE_LOG(
					Sumo,
					Verbose,
					TEXT("Path %s: %s"),
					*FullPath,
					FPaths::FileExists(FullPath) ? TEXT("found") : TEXT("not found"));
				SetOptionValue(ParameterArray, "-c", FullPath);
			}
		}
	}
	ParameterArray.Add(TEXT("--step-length"));
	ParameterArray.Add(FString::SanitizeFloat(StepLength));
	AddSumoLoggingParameters(ParameterArray);
	if (LoadSumoWithParameters(ParameterArray) != 0)
	{
		Status = 1;
		UE_LOG(Sumo, Warning, TEXT("%s"), UTF8_TO_TCHAR(SumoBridge::GetLastError()));
		Mode = ESumoMode::Done;
		return;
	}
	SendWeatherUpdate();
	Status = 0;
	UE_LOG(Sumo, Display, TEXT("Load sumocfg succeeded."));
	Mode = ESumoMode::Running;
}

void ASUMOManager::VehicleDataUpdate_Implementation(FVehicleDataUpdateData &data)
{
	if (data.Provider.IsEqual(PROVIDER_NAME))
	{
		// skip, use for testing
		UE_LOG(Sumo, Warning, TEXT("should not receive own data %s "), *data.Name.ToString());
	}
	else
	{
		// todo for co-simulation purposes
	}
}

FTransform ASUMOManager::TranslateToUnrealWorldPosition(double X, double Y, double Z, double Angle)
{
	FTransform transform;
	const double SafeZ = !std::isfinite(Z) || Z < -1000000.0 ? 0.0 : Z;
	transform.SetLocation(CoordinateSystemOffset.InverseTransformPosition(FVector(Y * 100, X * 100, SafeZ * 100)));
	transform.SetRotation(CoordinateSystemOffset.InverseTransformRotation(FRotator(0, Angle, 0).Quaternion()));
	return transform;
}

FString ASUMOManager::GetSumoRootDirectory()
{
	const FString ProjectDir = FPaths::ProjectDir();
	FString SumoDir = FPaths::Combine(
		ProjectDir,
		TEXT("Simulation"),
		TEXT("SUMO"));
	SumoDir = FPaths::ConvertRelativePathToFull(SumoDir);
	FPaths::NormalizeDirectoryName(SumoDir);
	return SumoDir;
}

int ASUMOManager::LoadSumoWithParameters(const TArray<FString> &Parsed)
{
	std::vector<std::string> Params;
	Params.reserve(Parsed.Num());
	for (const FString &Param : Parsed)
	{
		Params.emplace_back(TCHAR_TO_UTF8(*Param));
		UE_LOG(Sumo, Display, TEXT("Token: %s"), *Param);
	}

	std::vector<const char *> ParamPointers;
	ParamPointers.reserve(Params.size() + 1);
	for (const std::string &Param : Params)
	{
		ParamPointers.push_back(Param.c_str());
	}
	ParamPointers.push_back(nullptr);

	return SumoBridge::LoadWithParameters(ParamPointers.data());
}

bool ASUMOManager::HasSumoOption(const TArray<FString> &Params, const TCHAR *OptionName)
{
	const FString Option(OptionName);
	const FString OptionWithValuePrefix = Option + TEXT("=");

	for (const FString &Param : Params)
	{
		if (Param == Option || Param.StartsWith(OptionWithValuePrefix))
		{
			return true;
		}
	}

	return false;
}

void ASUMOManager::AddSumoLoggingParameters(TArray<FString> &Params)
{
	const FString SumoLogFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectLogDir(), TEXT("sumo.log")));
	const FString SumoErrorLogFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectLogDir(), TEXT("sumo-error.log")));

	if (!HasSumoOption(Params, TEXT("--log")) && !HasSumoOption(Params, TEXT("--log-file")) && !HasSumoOption(Params, TEXT("-l")))
	{
		Params.Add(TEXT("--log"));
		Params.Add(SumoLogFile);
	}

	if (!HasSumoOption(Params, TEXT("--error-log")))
	{
		Params.Add(TEXT("--error-log"));
		Params.Add(SumoErrorLogFile);
	}

	UE_LOG(Sumo, Log, TEXT("sumo: log=%s error-log=%s"), *SumoLogFile, *SumoErrorLogFile);
}

bool ASUMOManager::GetOptionValue(const TArray<FString> &Arguments, const FString &Parameter, FString &OutValue)
{
	const int32 Index = Arguments.Find(Parameter);
	if (Index != INDEX_NONE && Arguments.IsValidIndex(Index + 1))
	{
		OutValue = Arguments[Index + 1];
		return true;
	}

	return false;
}

bool ASUMOManager::SetOptionValue(TArray<FString> &Arguments, const FString &Parameter, const FString &InValue)
{
	const int32 Index = Arguments.Find(Parameter);
	if (Index != INDEX_NONE && Arguments.IsValidIndex(Index + 1))
	{
		Arguments[Index + 1] = InValue;
		return true;
	}
	return false;
}

TArray<FString> ASUMOManager::ParseOptions(const FString &Options)
{
	TArray<FString> ArgumentArray;
	const TCHAR *Cmd = *Options;
	FString Token;
	while (FParse::Token(Cmd, Token, true))
	{
		ArgumentArray.Add(Token);
	}
	return ArgumentArray;
}
