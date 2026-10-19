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

#include "libsumo/c/libsumoC_wrap.h"

#define SUMO_BRIDGE_STRING_LEN 64

struct FSumoVehicleInfo
{
	char Id[SUMO_BRIDGE_STRING_LEN];
	char Type[SUMO_BRIDGE_STRING_LEN];
	char VehicleClass[SUMO_BRIDGE_STRING_LEN];
	double X;
	double Y;
	double Z;
	double Angle;
	double Accel;
	double Speed;
	int Signals;
};

struct FSumoPersonInfo
{
	char Id[SUMO_BRIDGE_STRING_LEN];
	char Type[SUMO_BRIDGE_STRING_LEN];
	double X;
	double Y;
	double Z;
	double Angle;
	double Speed;
	double Slope;
};

class SumoBridge
{
public:
	static int LoadWithParameters(const char* Params[]);
	static void Close();
	static int LoadWithStepLength(const char* ConfigFile, float StepLength);
	static int Step(float TargetTime);
	static int GetVehicleIds(const char*** VehicleIds);
	static int GetVehicleInfo(const char* VehicleId, FSumoVehicleInfo* VehicleInfo);
	static int GetVehicleDimension(const char* VehicleId, double* Length, double* Width, double* Height);
	static int GetPersons(const char*** Persons);
	static int GetPersonInfo(const char* PersonId, FSumoPersonInfo* PersonInfo);
	static int GetTrafficLights(const char*** TrafficLightIds, const char*** TrafficLightStates);
	static int IsDone();
	static const char* GetLastError();
};
