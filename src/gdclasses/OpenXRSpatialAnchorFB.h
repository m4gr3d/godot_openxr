#ifndef OPENXR_SPATIAL_ANCHOR_FB_H
#define OPENXR_SPATIAL_ANCHOR_FB_H

#include "gdclasses_util.h"
#include "openxr/OpenXRApi.h"
#include "openxr/extensions/xr_fb_anchors_extension_wrapper.h"
#include <Spatial.hpp>

#ifndef XR_FB_spatial_entity
#define XR_FB_spatial_entity_EXPERIMENTAL_VERSION 1
#include <openxr/include/fb_spatial_entity.h>
#endif
#ifndef XR_FB_spatial_entity_storage
#define XR_FB_spatial_entity_storage_EXPERIMENTAL_VERSION 1
#include <openxr/include/fb_spatial_entity_storage.h>
#endif
#ifndef XR_FB_spatial_entity_query
#define XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION 1
#include <openxr/include/fb_spatial_entity_query.h>
#endif

namespace godot {
class OpenXRSpatialAnchorFB : public Spatial {
	GODOT_CLASS(OpenXRSpatialAnchorFB, Spatial);

public:
	static void _register_methods();

	void _init();
	void _ready();
	void _physics_process(float delta);

	OpenXRSpatialAnchorFB();
	~OpenXRSpatialAnchorFB();

	void set_anchor(const Transform pose);
	void load_anchor(const Array uuid);

	// Anchor must be loaded by calling set_anchor or load_anchor before it can
	// be erased.
	void erase_anchor();

private:
	OpenXRApi *openxr_api;
	XRFbAnchorsExtensionWrapper *spatial_anchors_wrapper = nullptr;
	XrSpatialEntityUuidFB uuid;
	XrSpace space;
	bool loaded;

	// TODO: possibly merge listeners.
	class SaveListener : public XRFbAnchorsExtensionWrapper::SpatialEntitySaveResultListener {
	public:
		SaveListener(OpenXRSpatialAnchorFB *anchor, OpenXRApi *openxr_api);
		~SaveListener();

		void on_save_result(const XrEventSpatialEntityStorageSaveResultFB *save_result) const override;

	private:
		OpenXRSpatialAnchorFB *anchor;
		OpenXRApi *openxr_api;
	};

	class QueryListener : public XRFbAnchorsExtensionWrapper::SpatialEntityQueryResultListener {
	public:
		QueryListener(OpenXRSpatialAnchorFB *anchor);
		~QueryListener();

		void on_query_result(const XrEventSpatialEntityQueryResultFB *query_result) const override;

	private:
		OpenXRSpatialAnchorFB *anchor;
	};

	class EraseListener : public XRFbAnchorsExtensionWrapper::SpatialEntityEraseResultListener {
	public:
		EraseListener(OpenXRSpatialAnchorFB *anchor, OpenXRApi *open_xr_api);
		~EraseListener();

		void on_erase_result(const XrEventSpatialEntityStorageEraseResultFB *query_result) const override;

	private:
		OpenXRSpatialAnchorFB *anchor;
		OpenXRApi *openxr_api;
	};

	SaveListener *save_listener;
	QueryListener *query_listener;
	EraseListener *erase_listener;

protected:
	void update_anchor_uuid_and_space(XrSpatialEntityUuidFB uuid, XrSpace space);
	void erase_anchor_uuid_and_space();
};
} // namespace godot

#endif // OPENXR_SPATIAL_ANCHOR_FB_H
