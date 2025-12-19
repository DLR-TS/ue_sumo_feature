#pragma once

#ifdef _WIN32
#ifdef SUMO_WRAPPER_EXPORTS
#define SUMO_API __declspec(dllexport)
#else
#define SUMO_API __declspec(dllimport)
#endif
#else
#define SUMO_API __attribute__((visibility("default")))
#endif
#define STRING_LEN 64
struct vehicle_info
{
	char id[STRING_LEN];
	char type[STRING_LEN];
	char vclass[STRING_LEN];
	double x;
	double y;
	double z;
	double angle;
	double accel;
	double speed;
	int signals;
};

struct person_info
{
	char id[STRING_LEN];
	double x;
	double y;
	double z;
	double angle;
	double speed;
};

struct Point3D
{
	double x;
	double y;
	double z;
};

#ifdef __cplusplus
#include <vector>
#include <string>

static std::vector<const char*> vehicle_ids_c_str;
static std::vector<std::string> vehicle_ids;

static std::vector<const char*> cstrpersons;
static std::vector<std::string> stdpersons;

static std::vector<const char*> cstrtls;
static std::vector<std::string> stdtls;

static std::vector<const char*> cstrtlstates;
static std::vector<std::string> stdtlstates;

static std::vector<const char*> cstrlanes;
static std::vector<std::string> stdlanes;

static std::vector<Point3D> lanePoints;

extern "C"
{
#endif
	SUMO_API int sumo_load_with_paramenters(const char* params[]);
	SUMO_API int sumo_load_with_step_length(const char* config_file, float step_length);
	SUMO_API int sumo_load(const char* config_file);
	SUMO_API int sumo_close(void);

	SUMO_API int sumo_step(const float target_time);
	SUMO_API double sumo_get_time(void);

	SUMO_API int sumo_get_vehicle_ids(const char*** vehicleIds);
	SUMO_API int sumo_get_vehicle_info(const char* vid, struct vehicle_info* vinfo);

	SUMO_API int sumo_get_vehicle_dimension(const char* vid, double* l, double* w, double* h);

	SUMO_API int sumo_get_persons(const char*** persons);
	SUMO_API int sumo_get_person_info(const char* pid, struct person_info* pinfo);
	SUMO_API int sumo_get_trafficlights(const char*** tlIds, const char*** tlstates);
	SUMO_API int sumo_get_lanes(const char*** lanes);
	// SUMO_API void sumo_get_lane_shape(const char* laneID, struct Point3D*** outPoints, int* outCount);
	SUMO_API int sumo_get_is_done(void);


	SUMO_API const char* sumo_get_last_error(void);


#ifdef __cplusplus
}
#endif
