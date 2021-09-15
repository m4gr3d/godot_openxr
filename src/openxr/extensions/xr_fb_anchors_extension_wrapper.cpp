#include "xr_fb_anchors_extension_wrapper.h"

#include "openxr/include/util.h"
#include <string>

using namespace godot;

namespace {
std::string bin2hex(const uint8_t *src, uint32_t size) {
	std::string res;
	res.reserve(size * 2);
	const char hex[] = "0123456789ABCDEF";
	for (uint32_t i = 0; i < size; ++i) {
		uint8_t c = src[i];
		res += hex[c >> 4];
		res += hex[c & 0xf];
	}
	return res;
}

std::string uuidToHexString(const XrSpatialEntityUuidFB &uuid) {
	return bin2hex(reinterpret_cast<const uint8_t *>(uuid.value), XR_UUID_SIZE_FB * 8);
}
} // namespace

XRFbAnchorsExtensionWrapper *XRFbAnchorsExtensionWrapper::singleton = nullptr;

XRFbAnchorsExtensionWrapper *XRFbAnchorsExtensionWrapper::get_singleton() {
	if (!singleton) {
		singleton = new XRFbAnchorsExtensionWrapper();
	}
	return singleton;
}

XRFbAnchorsExtensionWrapper::XRFbAnchorsExtensionWrapper() {
	openxr_api = OpenXRApi::openxr_get_api();
	request_extensions[XR_FB_SPATIAL_ENTITY_EXTENSION_NAME] =
			&fb_spatial_entity_ext;
	request_extensions[XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME] =
			&fb_spatial_entity_storage_ext;
	request_extensions[XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME] =
			&fb_spatial_entity_query_ext;
}

XRFbAnchorsExtensionWrapper::~XRFbAnchorsExtensionWrapper() {
	cleanup();
	OpenXRApi::openxr_release_api();
}

void XRFbAnchorsExtensionWrapper::on_instance_initialized(const XrInstance instance) {
	if (fb_spatial_entity_ext) {
		XrResult result = initialize_xr_fb_spatial_entity_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialize fb_spatial_entity extension")) {
			fb_spatial_entity_ext = false;
		}
	}
	if (fb_spatial_entity_storage_ext) {
		XrResult result = initialize_xr_fb_spatial_entity_storage_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialize fb_spatial_entity_storage extension")) {
			fb_spatial_entity_storage_ext = false;
		}
	}
	if (fb_spatial_entity_query_ext) {
		XrResult result = initialize_xr_fb_spatial_entity_query_extension(instance);
		if (!openxr_api->xr_result(result, "Failed to initialize fb_spatial_entity_query extension")) {
			fb_spatial_entity_query_ext = false;
		}
	}
}

bool XRFbAnchorsExtensionWrapper::on_event_polled(const XrEventDataBuffer &event) {
	switch (event.type) {
		case XR_TYPE_EVENT_DATA_SET_COMPONENT_ENABLE_RESULT_FB: {
			Godot::print("XRFbAnchorsExtensionWrapper: received XR_TYPE_EVENT_DATA_SET_COMPONENT_ENABLE_RESULT_FB");
			const XrEventDataSetComponentEnableResultFB *enable_result =
					(XrEventDataSetComponentEnableResultFB *)(&event);

			if (enable_result->result == XR_SUCCESS) {
				if (enable_result->componentType == XR_COMPONENT_TYPE_STORABLE_FB) {
					XrAsyncRequestIdFB save_anchor_request_id;
					store_spatial_anchor(
							enable_result->space,
							&save_anchor_request_id);
					if (save_result_listener_map.count(enable_result->requestId)) {
						save_result_listener_map[save_anchor_request_id] = save_result_listener_map[enable_result->requestId];
						save_result_listener_map.erase(enable_result->requestId);
					}
				}
			}
		} break;
		case XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FB: {
			Godot::print("XRFbAnchorsExtensionWrapper: received XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_SAVE_RESULT_FB");
			const XrEventSpatialEntityStorageSaveResultFB *save_result =
					(XrEventSpatialEntityStorageSaveResultFB *)(&event);
			if (save_result_listener_map.count(save_result->request)) {
				auto *listener = (SpatialEntitySaveResultListener *)(save_result_listener_map[save_result->request]);
				listener->on_save_result(save_result);
				save_result_listener_map.erase(save_result->request);
			}
		} break;
		case XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FB: {
			Godot::print("XRFbAnchorsExtensionWrapper: received XR_TYPE_EVENT_SPATIAL_ENTITY_STORAGE_ERASE_RESULT_FB");
			const XrEventSpatialEntityStorageEraseResultFB *erase_result =
					(XrEventSpatialEntityStorageEraseResultFB *)(&event);
			if (erase_result_listener_map.count(erase_result->request)) {
				auto *listener = (SpatialEntityEraseResultListener *)(erase_result_listener_map[erase_result->request]);
				listener->on_erase_result(erase_result);
				erase_result_listener_map.erase(erase_result->request);
			}
		} break;
		case XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB: {
			Godot::print("XRFbAnchorsExtensionWrapper: received XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB");
			const XrEventSpatialEntityQueryResultFB *query_result =
					(XrEventSpatialEntityQueryResultFB *)(&event);
			if (is_component_supported(query_result->space, XR_COMPONENT_TYPE_LOCATABLE_FB)) {
				// TODO: Locatable seems to be on by default, check if we can skip enabling here.
				XrComponentEnableRequestFB enable_locatable_request = {
					.type = XR_TYPE_COMPONENT_ENABLE_REQUEST_FB,
					.next = nullptr,
					.componentType = XR_COMPONENT_TYPE_LOCATABLE_FB,
					.enable = true,
					.timeout = 0
				};
				XrAsyncRequestIdFB enable_locatable_request_id;
				xrSetComponentEnabledFB(
						query_result->space,
						&enable_locatable_request,
						&enable_locatable_request_id);
				std::string uuid_string = uuidToHexString(query_result->uuid);
				// TODO: should probably wait to return query success after setting locatable instead of fire and forget.
				// This means call listener below only if result is XR_ERROR_SET_COMPONENT_ENABLE_ALREADY_ENABLED_FB.
				if (query_result_listener_map.count(uuid_string)) {
					auto *listener = (SpatialEntityQueryResultListener *)(query_result_listener_map[uuid_string]);
					listener->on_query_result(query_result);
					query_result_listener_map.erase(uuid_string);
				}
			}
		} break;
		default: {
			Godot::print("No events for XRFbAnchorsExtensionWrapper to handle.");
		} break;
	}

	return true;
}

void XRFbAnchorsExtensionWrapper::on_instance_destroyed() {
	cleanup();
}

void XRFbAnchorsExtensionWrapper::create_spatial_anchor(const XrPosef pose, const XrTime time, const SpatialEntitySaveResultListener *listener) {
	if (fb_spatial_entity_ext && fb_spatial_entity_storage_ext) {
		XrSpace space;
		XrSpatialAnchorCreateInfoFB anchor_create_info = {
			.type = XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_FB,
			.next = nullptr,
			.space = openxr_api->get_play_space(),
			.poseInSpace = pose,
			.time = time,
		};

		XrResult result = xrCreateSpatialAnchorFB(openxr_api->get_session(), &anchor_create_info, &space);
		if (!openxr_api->xr_result(result, "Failed to create spatial anchor.")) {
			return;
		}

		if (is_component_supported(space, XR_COMPONENT_TYPE_STORABLE_FB)) {
			XrComponentEnableRequestFB enable_component_request = {
				.type = XR_TYPE_COMPONENT_ENABLE_REQUEST_FB,
				.next = nullptr,
				.componentType = XR_COMPONENT_TYPE_STORABLE_FB,
				.enable = true,
				.timeout = 0
			};

			XrAsyncRequestIdFB enable_component_request_id;
			result = xrSetComponentEnabledFB(space,
					&enable_component_request,
					&enable_component_request_id);
			if (result == XR_ERROR_SET_COMPONENT_ENABLE_ALREADY_ENABLED_FB) {
				XrAsyncRequestIdFB save_request_id;
				store_spatial_anchor(space, &save_request_id);
				save_result_listener_map[save_request_id] = listener;
			} else {
				save_result_listener_map[enable_component_request_id] = listener;
			}
		}
	}
}

void XRFbAnchorsExtensionWrapper::erase_spatial_anchor(XrSpace space, const SpatialEntityEraseResultListener *listener) {
	XrSpatialEntityStorageEraseInfoFB eraseInfo = {
		.type = XR_TYPE_SPATIAL_ENTITY_STORAGE_ERASE_INFO_FB,
		.next = nullptr,
		.space = space,
		.location = XR_SPATIAL_ENTITY_STORAGE_LOCATION_LOCAL_FB
	};

	XrAsyncRequestIdFB requestId;
	XrResult result = xrSpatialEntityEraseSpaceFB(openxr_api->get_session(), &eraseInfo, &requestId);
	if (!openxr_api->xr_result(result, "Failed to erase spatial anchor")) {
		// TODO: Possibly inform listener of failure here.
		return;
	}
	erase_result_listener_map[requestId] = listener;
}

void XRFbAnchorsExtensionWrapper::find_spatial_anchor(XrSpatialEntityUuidFB uuid, const SpatialEntityQueryResultListener *listener) {
	if (fb_spatial_entity_query_ext) {
		XrSpatialEntityStorageLocationInfoFB storage_location_info = {
			.type = XR_TYPE_SPATIAL_ENTITY_STORAGE_LOCATION_INFO_FB,
			.next = nullptr,
			.location = XR_SPATIAL_ENTITY_STORAGE_LOCATION_LOCAL_FB
		};

		XrSpatialEntityQueryFilterSpaceTypeFB location_filter = {
			.type = XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FB,
			.next = &storage_location_info,
			.spaceType = XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FB
		};

		XrSpatialEntityQueryFilterIdsFB uuid_filter = {
			.type = XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FB,
			.next = &location_filter,
			.uuids = &uuid,
			.numIds = 1
		};

		XrSpatialEntityQueryInfoActionQueryFB uuid_query_info = {
			.type = XR_TYPE_SPATIAL_ENTITY_QUERY_INFO_ACTION_QUERY_FB,
			.next = nullptr,
			.maxQuerySpaces = 1,
			.timeout = 0,
			.queryAction = XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB,
			.filter = (XrSpatialEntityQueryFilterBaseHeaderFB *)&uuid_filter,
			.excludeFilter = nullptr
		};

		XrAsyncRequestIdFB query_request_id;
		query_result_listener_map[uuidToHexString(uuid)] = listener;
		XrResult result = xrQuerySpatialEntityFB(
				openxr_api->get_session(),
				(XrSpatialEntityQueryInfoBaseHeaderFB *)&uuid_query_info,
				&query_request_id);
		if (!openxr_api->xr_result(result, "Failed to submit query for spatial anchor")) {
			// TODO: Possibly inform listener of failure here.
			return;
		}
	}
}

bool XRFbAnchorsExtensionWrapper::is_component_supported(XrSpace space, XrComponentTypeFB type) {
	uint32_t numComponents = 0;
	XrResult result = xrEnumerateSupportedComponentsFB(space, 0, &numComponents, nullptr);
	if (!openxr_api->xr_result(result, "Failed to get number of supported components.")) {
		return false;
	}
	XrComponentTypeFB components[numComponents];
	result = xrEnumerateSupportedComponentsFB(space, numComponents, &numComponents, components);
	if (!openxr_api->xr_result(result, "Failed to enumerate supported components.")) {
		return false;
	}
	bool supported = false;
	for (uint32_t c = 0; c < numComponents; ++c) {
		if (components[c] == type) {
			supported = true;
			break;
		}
	}
	return supported;
}

void XRFbAnchorsExtensionWrapper::store_spatial_anchor(XrSpace space, XrAsyncRequestIdFB *request) {
	XrSpatialEntityStorageSaveInfoFB save_info = {
		.type = XR_TYPE_SPATIAL_ENTITY_STORAGE_SAVE_INFO_FB,
		.next = nullptr,
		.space = space,
		.location =
				XR_SPATIAL_ENTITY_STORAGE_LOCATION_LOCAL_FB,
		.persistenceMode =
				XR_SPATIAL_ENTITY_STORAGE_PERSISTENCE_MODE_INDEFINITE_HIGH_PRI_FB
	};
	XrResult result = xrSpatialEntitySaveSpaceFB(openxr_api->get_session(), &save_info, request);
	if (!openxr_api->xr_result(result, "Failed to save spatial anchor")) {
		// TODO: Possibly inform listener of failure here.
		return;
	}
}

PFN_xrCreateSpatialAnchorFB xrCreateSpatialAnchorFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrCreateSpatialAnchorFB(
		XrSession session,
		const XrSpatialAnchorCreateInfoFB *info,
		XrSpace *space) {
	if (xrCreateSpatialAnchorFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrCreateSpatialAnchorFB_ptr)(session, info, space);
}

PFN_xrEnumerateSupportedComponentsFB xrEnumerateSupportedComponentsFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrEnumerateSupportedComponentsFB(
		XrSpace space,
		uint32_t componentTypesCapacityInput,
		uint32_t *componentTypesCountOutput,
		XrComponentTypeFB *componentTypes) {
	if (xrEnumerateSupportedComponentsFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrEnumerateSupportedComponentsFB_ptr)(
			space,
			componentTypesCapacityInput,
			componentTypesCountOutput,
			componentTypes);
}

PFN_xrGetComponentStatusFB xrGetComponentStatusFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrGetComponentStatusFB(
		XrSpace space,
		XrComponentTypeFB componentType,
		XrComponentStatusFB *status) {
	if (xrGetComponentStatusFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrGetComponentStatusFB_ptr)(space, componentType, status);
}

PFN_xrSetComponentEnabledFB xrSetComponentEnabledFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrSetComponentEnabledFB(
		XrSpace space,
		const XrComponentEnableRequestFB *request,
		XrAsyncRequestIdFB *asyncRequest) {
	if (xrSetComponentEnabledFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrSetComponentEnabledFB_ptr)(space, request, asyncRequest);
}

PFN_xrQuerySpatialEntityFB xrQuerySpatialEntityFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrQuerySpatialEntityFB(
		XrSession session,
		const XrSpatialEntityQueryInfoBaseHeaderFB *info,
		XrAsyncRequestIdFB *request) {
	if (xrQuerySpatialEntityFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrQuerySpatialEntityFB_ptr)(session, info, request);
}

PFN_xrTerminateSpatialEntityQueryFB xrTerminateSpatialEntityQueryFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrTerminateSpatialEntityQueryFB(
		XrSession session,
		const XrAsyncRequestIdFB *request) {
	if (xrTerminateSpatialEntityQueryFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrTerminateSpatialEntityQueryFB_ptr)(session, request);
}

PFN_xrSpatialEntitySaveSpaceFB xrSpatialEntitySaveSpaceFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrSpatialEntitySaveSpaceFB(
		XrSession session,
		const XrSpatialEntityStorageSaveInfoFB *info,
		XrAsyncRequestIdFB *request) {
	if (xrSpatialEntitySaveSpaceFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrSpatialEntitySaveSpaceFB_ptr)(session, info, request);
}

PFN_xrSpatialEntityEraseSpaceFB xrSpatialEntityEraseSpaceFB_ptr = nullptr;

XRAPI_ATTR XrResult XRAPI_CALL XRFbAnchorsExtensionWrapper::xrSpatialEntityEraseSpaceFB(
		XrSession session,
		const XrSpatialEntityStorageEraseInfoFB *info,
		XrAsyncRequestIdFB *request) {
	if (xrSpatialEntityEraseSpaceFB_ptr == nullptr) {
		return XR_ERROR_HANDLE_INVALID;
	}

	return (*xrSpatialEntityEraseSpaceFB_ptr)(session, info, request);
}

XrResult XRFbAnchorsExtensionWrapper::initialize_xr_fb_spatial_entity_extension(XrInstance instance) {
	XrResult result;

	result = xrGetInstanceProcAddr(instance, "xrCreateSpatialAnchorFB",
			(PFN_xrVoidFunction *)&xrCreateSpatialAnchorFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrEnumerateSupportedComponentsFB",
			(PFN_xrVoidFunction *)&xrEnumerateSupportedComponentsFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrGetComponentStatusFB",
			(PFN_xrVoidFunction *)&xrGetComponentStatusFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrSetComponentEnabledFB",
			(PFN_xrVoidFunction *)&xrSetComponentEnabledFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}

XrResult XRFbAnchorsExtensionWrapper::initialize_xr_fb_spatial_entity_storage_extension(XrInstance instance) {
	XrResult result;

	result = xrGetInstanceProcAddr(instance, "xrSpatialEntitySaveSpaceFB",
			(PFN_xrVoidFunction *)&xrSpatialEntitySaveSpaceFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrSpatialEntityEraseSpaceFB",
			(PFN_xrVoidFunction *)&xrSpatialEntityEraseSpaceFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}

XrResult XRFbAnchorsExtensionWrapper::initialize_xr_fb_spatial_entity_query_extension(XrInstance instance) {
	XrResult result;

	result = xrGetInstanceProcAddr(instance, "xrQuerySpatialEntityFB",
			(PFN_xrVoidFunction *)&xrQuerySpatialEntityFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = xrGetInstanceProcAddr(instance, "xrTerminateSpatialEntityQueryFB",
			(PFN_xrVoidFunction *)&xrTerminateSpatialEntityQueryFB_ptr);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}

void XRFbAnchorsExtensionWrapper::cleanup() {
	fb_spatial_entity_ext = false;
	fb_spatial_entity_storage_ext = false;
	fb_spatial_entity_query_ext = false;
}
