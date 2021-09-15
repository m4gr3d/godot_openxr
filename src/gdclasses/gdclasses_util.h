#ifndef GDCLASSES_UTIL_H
#define GDCLASSES_UTIL_H

#include "openxr/OpenXRApi.h"
#include <string>

static XrPosef TransformToXrPosef(const Transform *transform) {
	Quat orientation = Quat(transform->basis);
	XrQuaternionf xr_quaternionf;
	xr_quaternionf.x = orientation.x;
	xr_quaternionf.y = orientation.y;
	xr_quaternionf.z = orientation.z;
	xr_quaternionf.w = orientation.w;
	XrVector3f xr_vector_3_f;
	xr_vector_3_f.x = transform->origin.x;
	xr_vector_3_f.y = transform->origin.y;
	xr_vector_3_f.z = transform->origin.z;
	return XrPosef_Create(xr_quaternionf, xr_vector_3_f);
}

static Transform XrPosefToTransform(const XrPosef *pose) {
	return Transform(
			Basis(Quat(pose->orientation.x, pose->orientation.y, pose->orientation.z, pose->orientation.w)),
			Vector3(pose->position.x, pose->position.y, pose->position.z));
}

static Array XrSpatialEntityUuidFBToGodotArray(const XrSpatialEntityUuidFB *uuid) {
	Array result = Array();
	result.resize(XR_UUID_SIZE_FB);
	for (int i = 0; i < XR_UUID_SIZE_FB; i++) {
		result[i] = std::to_string(uuid->value[i]).c_str();
	}
	return result;
}

// Godot array uuid must be a String array of size XR_UUID_SIZE_FB.
static XrSpatialEntityUuidFB GodotArrayToXrSpatialEntityUuidFB(const Array uuid) {
	XrSpatialEntityUuidFB result;
	result.uuidType = XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FB;
	for (int i = 0; i < XR_UUID_SIZE_FB; i++) {
		String uuid_i_str = uuid[i].operator String();
		result.value[i] = std::strtoull(uuid_i_str.alloc_c_string(), nullptr, 10);
	}
	return result;
}

#endif // GDCLASSES_UTIL_H
