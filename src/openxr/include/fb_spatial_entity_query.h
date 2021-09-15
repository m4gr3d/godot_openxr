/************************************************************************************

Filename    :   fb_spatial_entity_query.h
Content     :   spatial entity interface.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#include <openxr/include/fb_spatial_entity.h>

/*
  157 XR_FB_spatial_entity_query
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_spatial_entity_query

#ifndef XR_FB_spatial_entity
#error "This extension depends XR_FB_spatial_entity which has not been defined"
#endif

#if defined(XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION)
#if XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION != 1
#error "unknown experimental version number for XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION"
#endif

#define XR_FB_spatial_entity_query 1
#define XR_FB_spatial_entity_query_SPEC_VERSION 1
#define XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME "XR_FBX1_spatial_entity_query"

// This extension allows an application to query the spaces that have been previously shared
// or persisted onto the device
//
// There are several types of Query actions that can be performed.
//
// - XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB
//      Performs a simple query that returns a XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB for
//      each XrSpace that was found during the query.  There will also be a final
//      XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB event that is returned when all XrSpaces
//      have been returned.  This query type can be used when looking for a list of all
//      spaces that match a filter criteria.  Using this query does an implicit load on the
//      spsAnchorHandle for the spatial anchor


// Example Usage:
//    XrSpatialEntityStorageLocationInfoFB storageLocationInfo = {
//        XR_TYPE_SPATIAL_ENTITY_STORAGE_LOCATION_INFO_FB,
//        nullptr,
//        XR_SPATIAL_ENTITY_STORAGE_LOCATION_LOCAL_FB};
//
//    XrSpatialEntityQueryFilterSpaceTypeFB filterInfo = {
//        XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FB,
//        &storageLocationInfo,
//        XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FB};
//
//    XrSpatialEntityQueryInfoActionQueryFB queryInfo = {
//        XR_TYPE_SPATIAL_ENTITY_QUERY_INFO_ACTION_QUERY_FB,
//        nullptr,
//        MAX_PERSISTENT_SPACES,
//        0,
//        XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB,
//        (XrSpatialEntityQueryFilterBaseHeaderFB*)&filterInfo,
//        nullptr};
//
//    XrAsyncRequestIdFB requestId;
//    XrResult result = xrQuerySpatialEntityFB(
//        app.Session, (XrSpatialEntityQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
//

// Type of query being performed.
typedef enum XrSpatialEntityQueryPredicateFB {
    XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB = 0, // returns XrSpaces
        XR_SPATIAL_ENTITY_QUERY_PREDICATE_MAX_ENUM_FB = 0x7FFFFFFF
} XrSpatialEntityQueryPredicateFB;

// Query Info Structs
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_INFO_ACTION_QUERY_FB =
    (XrStructureType)1000156000;

// Query Filters Structs
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FB =
    (XrStructureType)1000156050;
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FB =
    (XrStructureType)1000156051;

// Events
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB =
    (XrStructureType)1000156100;
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB =
    (XrStructureType)1000156101;

// Query Filters
typedef struct XR_MAY_ALIAS XrSpatialEntityQueryFilterBaseHeaderFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
} XrSpatialEntityQueryFilterBaseHeaderFB;

// May be used to query the system to find all spaces of a particular type
typedef struct XrSpatialEntityQueryFilterSpaceTypeFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrSpatialEntityTypeFB spaceType;
} XrSpatialEntityQueryFilterSpaceTypeFB;

// May be used to query the system to find all spaces that match the uuids provided
// in the filter info
typedef struct XrSpatialEntityQueryFilterIdsFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrSpatialEntityUuidFB* uuids;
    uint32_t numIds;
} XrSpatialEntityQueryFilterIdsFB;

// Query Info
typedef struct XR_MAY_ALIAS XrSpatialEntityQueryInfoBaseHeaderFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
} XrSpatialEntityQueryInfoBaseHeaderFB;

// May be used to query for spaces and perform a specific action on the spaces returned.
// The available actions can be found in XrSpatialEntityQueryPredicateFB.
// The filter info provided to the filter member of the struct will be used as an inclusive
// filter.  All spaces that match this criteria will be included in the results returned.
// The filter info provided to the excludeFilter member of the struct will be used to exclude
// spaces from the results returned from the filter.  This is to allow for a more selective
// style query
typedef struct XrSpatialEntityQueryInfoActionQueryFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next; // string multiple queries together using next
    int32_t maxQuerySpaces;
    XrDuration timeout;
    XrSpatialEntityQueryPredicateFB queryAction; // What -> type of query to be performed
    const XrSpatialEntityQueryFilterBaseHeaderFB* filter; // Which -> which info we are querying for
    const XrSpatialEntityQueryFilterBaseHeaderFB*
        excludeFilter; // exclude specific results from query
} XrSpatialEntityQueryInfoActionQueryFB;

// Returned via XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB when a XrSpace is found matching the
// data provided by the filter info of the query
typedef struct XrEventSpatialEntityQueryResultFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrAsyncRequestIdFB request;
    XrSpace space;
    XrSpatialEntityUuidFB uuid;
} XrEventSpatialEntityQueryResultFB;

// When a query has completely finished this will be returned via
// XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB
typedef struct XrEventSpatialEntityQueryCompleteFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
    XrResult result;
    int32_t numSpacesFound;
    XrAsyncRequestIdFB request;
} XrEventSpatialEntityQueryCompleteFB;

typedef XrResult(XRAPI_PTR* PFN_xrQuerySpatialEntityFB)(
    XrSession session,
    const XrSpatialEntityQueryInfoBaseHeaderFB* info,
    XrAsyncRequestIdFB* request);

typedef XrResult(XRAPI_PTR* PFN_xrTerminateSpatialEntityQueryFB)(
    XrSession session,
    const XrAsyncRequestIdFB* request);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

XRAPI_ATTR XrResult XRAPI_CALL xrQuerySpatialEntityFB(
    XrSession session,
    const XrSpatialEntityQueryInfoBaseHeaderFB* info,
    XrAsyncRequestIdFB* request);

XRAPI_ATTR XrResult XRAPI_CALL
xrTerminateSpatialEntityQueryFB(XrSession session, const XrAsyncRequestIdFB* request);

#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

#endif // defined(XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION)

#endif // XR_FB_spatial_entity_query

#ifdef __cplusplus
}
#endif
