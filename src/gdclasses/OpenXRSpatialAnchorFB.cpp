#include "OpenXRSpatialAnchorFB.h"

#include "openxr/openxr.h"

namespace {
const char *kAnchorReadyEvent = "anchor_ready_event";
const char *kAnchorErasedEvent = "anchor_erased_event";
const XrSpaceLocationFlags is_valid_location =
		XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
} // namespace

OpenXRSpatialAnchorFB::OpenXRSpatialAnchorFB() {
	openxr_api = OpenXRApi::openxr_get_api();
	spatial_anchors_wrapper = XRFbAnchorsExtensionWrapper::get_singleton();
	save_listener = new SaveListener(this, openxr_api);
	erase_listener = new EraseListener(this, openxr_api);
	query_listener = new QueryListener(this);
	loaded = false;
}

OpenXRSpatialAnchorFB::~OpenXRSpatialAnchorFB() {
	if (openxr_api != nullptr) {
		OpenXRApi::openxr_release_api();
	}
	spatial_anchors_wrapper = nullptr;
}

void OpenXRSpatialAnchorFB::set_anchor(const Transform pose) {
	spatial_anchors_wrapper->create_spatial_anchor(
			TransformToXrPosef(&pose),
			openxr_api->get_frame_state().predictedDisplayTime,
			save_listener);
}

void OpenXRSpatialAnchorFB::load_anchor(const Array uuid) {
	XrSpatialEntityUuidFB uuid_fb = GodotArrayToXrSpatialEntityUuidFB(uuid);
	spatial_anchors_wrapper->find_spatial_anchor(uuid_fb, query_listener);
}

void OpenXRSpatialAnchorFB::erase_anchor() {
	if (!loaded) {
		Godot::print_warning(
				"Failed to erase anchor - no anchor loaded.",
				__FUNCTION__, __FILE_NAME__, __LINE__);
		return;
	}
	spatial_anchors_wrapper->erase_spatial_anchor(space, erase_listener);
}

void OpenXRSpatialAnchorFB::_register_methods() {
	register_method("_ready", &OpenXRSpatialAnchorFB::_ready);
	register_method("_physics_process", &OpenXRSpatialAnchorFB::_physics_process);

	register_method("set_anchor", &OpenXRSpatialAnchorFB::set_anchor);
	register_method("erase_anchor", &OpenXRSpatialAnchorFB::erase_anchor);
	register_method("load_anchor", &OpenXRSpatialAnchorFB::load_anchor);

	Dictionary common_event_args;
	common_event_args[Variant("uuid")] = Variant(Variant::ARRAY);
	register_signal<OpenXRSpatialAnchorFB>(kAnchorReadyEvent, common_event_args);
	register_signal<OpenXRSpatialAnchorFB>(kAnchorErasedEvent, common_event_args);
}

void OpenXRSpatialAnchorFB::_init() {
	// Nothing to do here
}

void OpenXRSpatialAnchorFB::_ready() {
	// Nothing to do here
}

void OpenXRSpatialAnchorFB::_physics_process(float delta) {
	if (!loaded) {
		return;
	}

	// Update the pose of this anchor.
	XrSpaceLocation xr_space_location = {
		.type = XR_TYPE_SPACE_LOCATION
	};
	XrResult result = xrLocateSpace(
			this->space,
			openxr_api->get_play_space(),
			openxr_api->get_frame_state().predictedDisplayTime,
			&xr_space_location);
	if (!openxr_api->xr_result(result, "failed to locate space!")) {
		return;
	}

	if (xr_space_location.locationFlags & is_valid_location) {
		set_transform(XrPosefToTransform(&xr_space_location.pose));
	}
}

void OpenXRSpatialAnchorFB::update_anchor_uuid_and_space(XrSpatialEntityUuidFB uuid,
		XrSpace space) {
	this->uuid = uuid;
	this->space = space;
	loaded = true;
	emit_signal(
			kAnchorReadyEvent, XrSpatialEntityUuidFBToGodotArray(&uuid));
}

void OpenXRSpatialAnchorFB::erase_anchor_uuid_and_space() {
	space = XR_NULL_HANDLE;
	loaded = false;
	emit_signal(
			kAnchorErasedEvent,
			XrSpatialEntityUuidFBToGodotArray(&uuid));
	uuid = XrSpatialEntityUuidFB();
}

// SaveListener
OpenXRSpatialAnchorFB::SaveListener::SaveListener(OpenXRSpatialAnchorFB *anchor, OpenXRApi *openxr_api) {
	this->anchor = anchor;
	this->openxr_api = openxr_api;
}

OpenXRSpatialAnchorFB::SaveListener::~SaveListener() {
	this->anchor = nullptr;
}

void OpenXRSpatialAnchorFB::SaveListener::on_save_result(const XrEventSpatialEntityStorageSaveResultFB *save_result) const {
	if (!openxr_api->xr_result(save_result->result, "Failed to store anchor associated with async request ID {0}", save_result->request)) {
		return;
	}

	anchor->update_anchor_uuid_and_space(save_result->uuid, save_result->space);
}

// QueryListener
OpenXRSpatialAnchorFB::QueryListener::QueryListener(OpenXRSpatialAnchorFB *anchor) {
	this->anchor = anchor;
}

OpenXRSpatialAnchorFB::QueryListener::~QueryListener() {
	this->anchor = nullptr;
}

void OpenXRSpatialAnchorFB::QueryListener::on_query_result(const XrEventSpatialEntityQueryResultFB *query_result) const {
	anchor->update_anchor_uuid_and_space(query_result->uuid, query_result->space);
}

// EraseListener
OpenXRSpatialAnchorFB::EraseListener::EraseListener(OpenXRSpatialAnchorFB *anchor, OpenXRApi *openxr_api) {
	this->anchor = anchor;
	this->openxr_api = openxr_api;
}

OpenXRSpatialAnchorFB::EraseListener::~EraseListener() {
	this->anchor = nullptr;
}

void OpenXRSpatialAnchorFB::EraseListener::on_erase_result(const XrEventSpatialEntityStorageEraseResultFB *query_result) const {
	if (!openxr_api->xr_result(query_result->result, "Failed to erase anchor associated with async request ID {0}", query_result->request)) {
		return;
	}

	anchor->erase_anchor_uuid_and_space();
}
