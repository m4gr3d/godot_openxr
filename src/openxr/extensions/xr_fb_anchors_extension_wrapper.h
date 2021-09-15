#ifndef XR_FB_ANCHORS_EXTENSION_WRAPPER_H
#define XR_FB_ANCHORS_EXTENSION_WRAPPER_H

#include "openxr/OpenXRApi.h"
#include "xr_extension_wrapper.h"

#include <map>

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

class XRFbAnchorsExtensionWrapper : public XRExtensionWrapper {
public:
	static XRFbAnchorsExtensionWrapper *get_singleton();

	void on_instance_initialized(const XrInstance instance) override;
	void on_instance_destroyed() override;
	bool on_event_polled(const XrEventDataBuffer &event) override;

	// TODO: possibly merge listeners
	class SpatialEntitySaveResultListener {
	public:
		virtual void on_save_result(const XrEventSpatialEntityStorageSaveResultFB *save_result) const = 0;
	};

	class SpatialEntityQueryResultListener {
	public:
		virtual void on_query_result(const XrEventSpatialEntityQueryResultFB *query_result) const = 0;
	};

	class SpatialEntityEraseResultListener {
	public:
		virtual void on_erase_result(const XrEventSpatialEntityStorageEraseResultFB *erase_result) const = 0;
	};

	void create_spatial_anchor(const XrPosef pose, const XrTime time, const SpatialEntitySaveResultListener *listener);
	void erase_spatial_anchor(const XrSpace space, const SpatialEntityEraseResultListener *listener);
	void find_spatial_anchor(XrSpatialEntityUuidFB uuid, const SpatialEntityQueryResultListener *listener);

protected:
	XRFbAnchorsExtensionWrapper();
	~XRFbAnchorsExtensionWrapper();

private:
	static XRAPI_ATTR XrResult XRAPI_CALL xrCreateSpatialAnchorFB(
			XrSession session,
			const XrSpatialAnchorCreateInfoFB *info,
			XrSpace *space);

	static XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSupportedComponentsFB(
			XrSpace space,
			uint32_t componentTypesCapacityInput,
			uint32_t *componentTypesCountOutput,
			XrComponentTypeFB *componentTypes);

	static XRAPI_ATTR XrResult XRAPI_CALL xrGetComponentStatusFB(
			XrSpace space,
			XrComponentTypeFB componentType,
			XrComponentStatusFB *status);

	static XRAPI_ATTR XrResult XRAPI_CALL xrSetComponentEnabledFB(
			XrSpace space,
			const XrComponentEnableRequestFB *request,
			XrAsyncRequestIdFB *asyncRequest);

	static XRAPI_ATTR XrResult XRAPI_CALL xrQuerySpatialEntityFB(
			XrSession session,
			const XrSpatialEntityQueryInfoBaseHeaderFB *info,
			XrAsyncRequestIdFB *request);

	static XRAPI_ATTR XrResult XRAPI_CALL xrTerminateSpatialEntityQueryFB(
			XrSession session,
			const XrAsyncRequestIdFB *request);

	static XRAPI_ATTR XrResult XRAPI_CALL xrSpatialEntitySaveSpaceFB(
			XrSession session,
			const XrSpatialEntityStorageSaveInfoFB *info,
			XrAsyncRequestIdFB *request);

	static XRAPI_ATTR XrResult XRAPI_CALL xrSpatialEntityEraseSpaceFB(
			XrSession session,
			const XrSpatialEntityStorageEraseInfoFB *info,
			XrAsyncRequestIdFB *request);

	static XrResult initialize_xr_fb_spatial_entity_extension(XrInstance instance);
	static XrResult initialize_xr_fb_spatial_entity_storage_extension(XrInstance instance);
	static XrResult initialize_xr_fb_spatial_entity_query_extension(XrInstance instance);

	bool is_component_supported(XrSpace space, XrComponentTypeFB type);
	void store_spatial_anchor(XrSpace space, XrAsyncRequestIdFB *request);

	void cleanup();

	static XRFbAnchorsExtensionWrapper *singleton;
	OpenXRApi *openxr_api = nullptr;

	std::map<XrAsyncRequestIdFB, const SpatialEntitySaveResultListener *> save_result_listener_map;
	std::map<XrAsyncRequestIdFB, const SpatialEntityEraseResultListener *> erase_result_listener_map;
	// Map from hex UUIDs to any listeners listening for the result of a query
	// for that UUID.
	std::map<std::string, const SpatialEntityQueryResultListener *> query_result_listener_map;

	bool fb_spatial_entity_ext = false;
	bool fb_spatial_entity_storage_ext = false;
	bool fb_spatial_entity_query_ext = false;
};

#endif // XR_FB_ANCHORS_EXTENSION_WRAPPER_H
