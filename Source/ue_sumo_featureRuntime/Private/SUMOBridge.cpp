/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "SUMOBridge.h"

#include <cstdio>
#include <string>
#include <vector>

static std::string LastSumoError;
static std::vector<std::string> CachedVehicleIds;
static std::vector<const char*> CachedVehicleIdPointers;
static std::vector<std::string> CachedPersonIds;
static std::vector<const char*> CachedPersonIdPointers;
static std::vector<std::string> CachedTrafficLightIds;
static std::vector<const char*> CachedTrafficLightIdPointers;
static std::vector<std::string> CachedTrafficLightStates;
static std::vector<const char*> CachedTrafficLightStatePointers;

static void CopyString(char* Destination, const char* Source)
{
	std::snprintf(Destination, SUMO_BRIDGE_STRING_LEN, "%s", Source != nullptr ? Source : "");
}

static bool CapturePendingSumoException()
{
	libsumoc_SWIG_CException* Exception = libsumoc_SWIG_CException_get_pending();
	if (Exception == nullptr) {
		return false;
	}

	const char* Message = libsumoc_SWIG_CException_msg_get(Exception);
	LastSumoError = Message != nullptr ? Message : "SUMO C binding call failed.";
	libsumoc_SWIG_CException_delete(Exception);
	libsumoc_SWIG_CException_reset_pending();
	return true;
}

static int FinishSumoCall()
{
	if (CapturePendingSumoException()) {
		return -1;
	}
	LastSumoError.clear();
	return 0;
}

static int CopyStringVector(libsumoc_StringVector* Source, std::vector<std::string>& Values, std::vector<const char*>& Pointers)
{
	if (Source == nullptr) {
		return FinishSumoCall();
	}

	Values.clear();
	Pointers.clear();
	const size_t Count = libsumoc_StringVector_size(Source);
	Values.reserve(Count);
	Pointers.reserve(Count);
	for (size_t Index = 0; Index < Count; ++Index) {
		Values.emplace_back(libsumoc_StringVector_get(Source, static_cast<int>(Index)));
		if (CapturePendingSumoException()) {
			libsumoc_StringVector_delete(Source);
			return -1;
		}
	}
	libsumoc_StringVector_delete(Source);

	for (const std::string& Value : Values) {
		Pointers.push_back(Value.c_str());
	}
	return FinishSumoCall();
}

static int CopyPosition(libsumoc_TraCIPosition* Position, double& X, double& Y, double& Z)
{
	if (Position == nullptr) {
		return FinishSumoCall();
	}

	X = libsumoc_TraCIPosition_x_get(Position);
	Y = libsumoc_TraCIPosition_y_get(Position);
	Z = libsumoc_TraCIPosition_z_get(Position);
	libsumoc_TraCIPosition_delete(Position);
	return FinishSumoCall();
}

int SumoBridge::LoadWithParameters(const char* Params[])
{
	libsumoc_StringVector* Args = libsumoc_StringVector_new();
	if (CapturePendingSumoException()) {
		return -1;
	}

	for (int Index = 0; Params[Index] != nullptr; ++Index) {
		libsumoc_StringVector_push_back(Args, Params[Index]);
		if (CapturePendingSumoException()) {
			libsumoc_StringVector_delete(Args);
			return -1;
		}
	}

	libsumoc_Simulation_load(Args);
	libsumoc_StringVector_delete(Args);
	return FinishSumoCall();
}

void SumoBridge::Close()
{
	libsumoc_Simulation_close();
	CapturePendingSumoException();
}

int SumoBridge::LoadWithStepLength(const char* ConfigFile, float StepLength)
{
	const std::string StepLengthString = std::to_string(StepLength);
	const char* Params[] = { "-c", ConfigFile, "--step-length", StepLengthString.c_str(), nullptr };
	return LoadWithParameters(Params);
}

int SumoBridge::Step(float TargetTime)
{
	libsumoc_Simulation_step_cd(TargetTime);
	return FinishSumoCall();
}

int SumoBridge::GetVehicleIds(const char*** VehicleIds)
{
	const int Result = CopyStringVector(libsumoc_Vehicle_getIDList(), CachedVehicleIds, CachedVehicleIdPointers);
	*VehicleIds = CachedVehicleIdPointers.data();
	return Result == 0 ? static_cast<int>(CachedVehicleIdPointers.size()) : Result;
}

int SumoBridge::GetVehicleInfo(const char* VehicleId, FSumoVehicleInfo* VehicleInfo)
{
	libsumoc_TraCIPosition* Position = libsumoc_Vehicle_getPosition3D(VehicleId);
	if (CapturePendingSumoException()) {
		return -1;
	}

	const char* TypeId = libsumoc_Vehicle_getTypeID(VehicleId);
	if (CapturePendingSumoException()) {
		libsumoc_TraCIPosition_delete(Position);
		return -1;
	}
	const std::string TypeIdString = TypeId != nullptr ? TypeId : "";

	CopyString(VehicleInfo->Id, VehicleId);
	CopyString(VehicleInfo->Type, TypeIdString.c_str());
	CopyString(VehicleInfo->VehicleClass, libsumoc_VehicleType_getVehicleClass(TypeIdString.c_str()));
	if (CapturePendingSumoException()) {
		libsumoc_TraCIPosition_delete(Position);
		return -1;
	}

	if (CopyPosition(Position, VehicleInfo->X, VehicleInfo->Y, VehicleInfo->Z) != 0) {
		return -1;
	}
	VehicleInfo->Angle = libsumoc_Vehicle_getAngle(VehicleId);
	VehicleInfo->Accel = libsumoc_Vehicle_getAcceleration(VehicleId);
	VehicleInfo->Speed = libsumoc_Vehicle_getSpeed(VehicleId);
	VehicleInfo->Signals = libsumoc_Vehicle_getSignals(VehicleId);
	return FinishSumoCall();
}

int SumoBridge::GetVehicleDimension(const char* VehicleId, double* Length, double* Width, double* Height)
{
	const char* TypeId = libsumoc_Vehicle_getTypeID(VehicleId);
	if (CapturePendingSumoException()) {
		return -1;
	}
	const std::string TypeIdString = TypeId != nullptr ? TypeId : "";

	*Length = libsumoc_VehicleType_getLength(TypeIdString.c_str());
	*Width = libsumoc_VehicleType_getWidth(TypeIdString.c_str());
	*Height = libsumoc_VehicleType_getHeight(TypeIdString.c_str());
	return FinishSumoCall();
}

int SumoBridge::GetPersons(const char*** Persons)
{
	const int Result = CopyStringVector(libsumoc_Person_getIDList(), CachedPersonIds, CachedPersonIdPointers);
	*Persons = CachedPersonIdPointers.data();
	return Result == 0 ? static_cast<int>(CachedPersonIdPointers.size()) : Result;
}

int SumoBridge::GetPersonInfo(const char* PersonId, FSumoPersonInfo* PersonInfo)
{
	libsumoc_TraCIPosition* Position = libsumoc_Person_getPosition3D(PersonId);
	if (CapturePendingSumoException()) {
		return -1;
	}

	CopyString(PersonInfo->Id, PersonId);
	PersonInfo->Type[0] = 0;
	if (CopyPosition(Position, PersonInfo->X, PersonInfo->Y, PersonInfo->Z) != 0) {
		return -1;
	}
	PersonInfo->Angle = libsumoc_Person_getAngle(PersonId);
	PersonInfo->Speed = libsumoc_Person_getSpeed(PersonId);
	PersonInfo->Slope = 0.0;
	return FinishSumoCall();
}

int SumoBridge::GetTrafficLights(const char*** TrafficLightIds, const char*** TrafficLightStates)
{
	const int IdResult = CopyStringVector(libsumoc_TrafficLight_getIDList(), CachedTrafficLightIds, CachedTrafficLightIdPointers);
	if (IdResult < 0) {
		return IdResult;
	}

	CachedTrafficLightStates.clear();
	CachedTrafficLightStatePointers.clear();
	CachedTrafficLightStates.reserve(CachedTrafficLightIds.size());
	CachedTrafficLightStatePointers.reserve(CachedTrafficLightIds.size());
	for (const std::string& TrafficLightId : CachedTrafficLightIds) {
		CachedTrafficLightStates.emplace_back(libsumoc_TrafficLight_getRedYellowGreenState(TrafficLightId.c_str()));
		if (CapturePendingSumoException()) {
			return -1;
		}
	}
	for (const std::string& TrafficLightState : CachedTrafficLightStates) {
		CachedTrafficLightStatePointers.push_back(TrafficLightState.c_str());
	}
	*TrafficLightIds = CachedTrafficLightIdPointers.data();
	*TrafficLightStates = CachedTrafficLightStatePointers.data();
	return static_cast<int>(CachedTrafficLightIdPointers.size());
}

int SumoBridge::IsDone()
{
	const int MinExpectedNumber = libsumoc_Simulation_getMinExpectedNumber();
	if (CapturePendingSumoException()) {
		return -1;
	}
	return MinExpectedNumber < 1 ? 1 : 0;
}

const char* SumoBridge::GetLastError()
{
	return LastSumoError.c_str();
}

