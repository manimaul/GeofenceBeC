#include <microhttpd.h>
#include <jansson.h>
#include <string.h>
#include <libmongoc-1.0/mongoc.h>
#include "database.h"
#include "location.h"

#define PORT 8181
//#define TEXT_HTML "text/html"
#define APPLICATION_JSON "application/json"
#define CONTENT_TYPE "Content-type"
#define METHOD_GET "GET"
#define METHOD_POST "POST"
#define METHOD_DELETE "DELETE"

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct MA_HandlerData {
    mongoc_client_pool_t *pool;
};

struct MA_ConnectionInfo {
    size_t sz;
    char *body;
};

//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The handler function for all http requests that conform to the MHD_AccessHandlerCallback protocol in microhttpd.h
 */
int _answerConnection(void *pCls,
                      struct MHD_Connection *pConn,
                      char const *pUrl,
                      char const *pMethod,
                      char const *version,
                      char const *upload_data,
                      size_t *upload_data_size,
                      void **con_cls);

/**
 * Request handler for / endpoint
 *
 * param pConn - the connection to queue a response to
 */
int _handleRoot(struct MHD_Connection *pConn);

/**
 * Generic Request handler for requests where body is to be inserted into the database
 *
 * param pConn - the connection to enqueue a response to
 * param pData - data to retrieve a MongoDb client from
 * param pConnInfo - connection info to retrieve the request body
 * param fPtr - function pointer to insert json record into database
 */
int _handlePostWithDbInsertBodyJson(struct MHD_Connection *pConn, struct MA_HandlerData *pData,
                                    struct MA_ConnectionInfo *pConnInfo,
                                    DB_insertFunction fPtr);

/**
 * Request handler for /fence_entry POST endpoint
 *
 * param pConn - the connection to enqueue a response to
 * param pData - data to retrieve a MongoDb client from
 * param pConnInfo - connection info to retrieve the request body
 */
int _handlePostFenceEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData, struct MA_ConnectionInfo *pConnInfo);

/**
 * Request handler for POST /gps_log endpoint
 *
 * param pConn - the connection to enqueue a response to
 * param pData - data to retrieve a MongoDb client from
 * param pConnInfo - connection info to retrieve the request body
 */
int _handlePostGpsLog(struct MHD_Connection *pConn, struct MA_HandlerData *pData, struct MA_ConnectionInfo *pConnInfo);

/**
 * Request handler for /fence_entry endpoint
 *
 * param pConn - the connection to enqueue a response to
 * param pData - data to retrieve a MongoDb client from
 * param pId - the geofence id (i request param)
 */
int _handleGetFenceEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData, char const *pId);

/**
 * Request handler for /gps_log endpoint
 *
 * param pConn - the connection to enqueue a response to
 * param pData - data to retrieve a MongoDb client from
 * param epoch - a time within the gps log time frame (t request param)
 */
int _handleGetGpsLogEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData, long epoch);

/**
 * Request handler for /gps_log_list endpoint
 *
 * param pConn - the connection to enqueue a response to
 * param pData - data to retrieve a MongoDb client from
 */
int _handleGetGpsLogEntryList(struct MHD_Connection *pConn, struct MA_HandlerData *pData);

/**
 * Request handler for 404 - resource not found
 *
 * param pConn - the connection to enqueue a response to
 */
int _handleNotFound(struct MHD_Connection *pConn);

int _handleError(struct MHD_Connection *pConn);

int _handleDeleteFenceEntry(struct MHD_Connection *pConnection, struct MA_HandlerData *pData, const char *pId);

int _handleDeleteGpsLog(struct MHD_Connection *pConnection, struct MA_HandlerData *pData, const char *pId);

//endregion

//region STATIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static struct MA_ConnectionInfo *__createConnectionInfo() {
    struct MA_ConnectionInfo *info = malloc(sizeof(struct MA_ConnectionInfo));
    info->body = NULL;
    info->sz = 0;
    return info;
}

static void __destroyConnectionInfo(struct MA_ConnectionInfo *pInfo) {
    if (NULL == pInfo) {
        return;
    }

    if (NULL != pInfo->body) {
        printf("__destroyConnectionInfo() free(free(MA_ConnectionInfo->body)\n");
        free(pInfo->body);
    }
    printf("__destroyConnectionInfo() free(MA_ConnectionInfo)\n");
    free(pInfo);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/**
 * Request completion callback to perform cleanup after each request.
 */
static void __requestCompleted(void *pCls, struct MHD_Connection *pConn, void **pConnCls,
                               enum MHD_RequestTerminationCode pTermCode) {

    struct MA_ConnectionInfo *info = *pConnCls;
    __destroyConnectionInfo(info);
    *pConnCls = NULL;
}

#pragma clang diagnostic pop

//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int _appendData(struct MHD_Connection *pConn, size_t *pUploadDataSize, char const *pUploadData,
                struct MA_ConnectionInfo *connectionInfo) {

    size_t len = *pUploadDataSize;
    printf("_answerConnection CHUNK size:%lu\n", len);
    if (connectionInfo->body == NULL) {
        printf("_answerConnection CUMULATIVE body size:%lu\n", len);
        char *body = malloc(len);
        memcpy(body, pUploadData, len);
        connectionInfo->body = body;
        connectionInfo->sz = len;
    } else {
        size_t newSize = connectionInfo->sz + len;
        printf("_answerConnection CUMULATIVE body size:%lu\n", newSize);
        char *temp = realloc(connectionInfo->body, newSize);
        if (temp != NULL) {
            memcpy(&temp[connectionInfo->sz], pUploadData, len);
            connectionInfo->body = temp;
            connectionInfo->sz = newSize;
        } else {
            return _handleError(pConn);
        }
    }
    *pUploadDataSize = 0;
    return MHD_YES;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

int _answerConnection(void *pCls,
                      struct MHD_Connection *pConn,
                      char const *pUrl,
                      char const *pMethod,
                      char const *pVersion,
                      char const *pUploadData,
                      size_t *pUploadDataSize,
                      void **pConnCls) {

    /*
     * Fetch the request body
     */
    struct MA_ConnectionInfo *connectionInfo = *pConnCls;
    if (NULL == *pConnCls) {
        connectionInfo = __createConnectionInfo();
        *pConnCls = (void *) connectionInfo;
        return MHD_YES;
    }
    if (*pUploadDataSize) {
        return _appendData(pConn, pUploadDataSize, pUploadData, connectionInfo);
    }

    /*
     * Answer GET requests
     */
    if (0 == strcmp(pMethod, METHOD_GET)) {
        /*
         * Answer / endpoint
         */
        if (0 == strcmp(pUrl, "/")) {
            return _handleRoot(pConn);
        }

        /*
         * Answer /fence_entry endpoint
         */
        if (0 == strcmp(pUrl, "/fence_entry")) {
            char const *val = MHD_lookup_connection_value(pConn, MHD_GET_ARGUMENT_KIND, "i");
            if (val) {
                return _handleGetFenceEntry(pConn, pCls, val);
            }
        }

        /*
         * Answer gps_log endpoint
         */
        if (0 == strcmp(pUrl, "/gps_log")) {
            char const *val = MHD_lookup_connection_value(pConn, MHD_GET_ARGUMENT_KIND, "t");
            if (val) {
                long time = strtol(val, NULL, 10);
                return _handleGetGpsLogEntry(pConn, pCls, time);
            }
        }

        /*
         * Answer gps_log endpoint
         */
        if (0 == strcmp(pUrl, "/gps_log_list")) {
            return _handleGetGpsLogEntryList(pConn, pCls);
        }
    }

   /*
    * Answer POST requests
    */
    else if (0 == strcmp(pMethod, METHOD_POST)) {
        /*
         * Answer /fence_entry endpoint
         */
        if (0 == strcmp(pUrl, "/fence_entry")) {
            return _handlePostFenceEntry(pConn, pCls, connectionInfo);
        }

        /*
         * Answer /fence_entry endpoint
         */
        if (0 == strcmp(pUrl, "/gps_log")) {
            return _handlePostGpsLog(pConn, pCls, connectionInfo);
        }
    }

   /*
    * Answer POST requests
    */
    else if (0 == strcmp(pMethod, METHOD_DELETE)) {
        /*
         * Answer /fence_entry endpoint
         */
        if (0 == strcmp(pUrl, "/fence_entry")) {
            char const *val = MHD_lookup_connection_value(pConn, MHD_GET_ARGUMENT_KIND, "id");
            if (val) {
                return _handleDeleteFenceEntry(pConn, pCls, val);
            }
        }

        /*
         * Answer /fence_entry endpoint
         */
        if (0 == strcmp(pUrl, "/gps_log")) {
            char const *val = MHD_lookup_connection_value(pConn, MHD_GET_ARGUMENT_KIND, "id");
            if (val) {
                return _handleDeleteGpsLog(pConn, pCls, val);
            }
        }
    }

    /*
     * Answer with 404 not found
     */
    *pUploadDataSize = 0;
    return _handleNotFound(pConn);
}

int _handleDeleteGpsLog(struct MHD_Connection *pConn, struct MA_HandlerData *pData, const char *pId) {
    /*
     * Craft json response
     */
    bson_t *json_response = bson_new();
    BSON_APPEND_UTF8(json_response, "message", "not implemented");
    char *responseBody = bson_as_json(json_response, NULL);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, MHD_HTTP_OK, response);

    /*
     * Cleanup
     */
    MHD_destroy_response(response);
    bson_destroy(json_response);
    printf("_handleRoot() free(responseBody)\n");
    bson_free(responseBody);

    return ret;
}

int _handleDeleteFenceEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData, const char *pId) {
    return 0;
}

#pragma clang diagnostic pop

int _handleRoot(struct MHD_Connection *pConn) {
    /*
     * Craft json response
     */
    bson_t *json_response = bson_new();
    BSON_APPEND_UTF8(json_response, "message", "GeoFenceMark");
    char *responseBody = bson_as_json(json_response, NULL);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, MHD_HTTP_OK, response);

    /*
     * Cleanup
     */
    MHD_destroy_response(response);
    bson_destroy(json_response);
    printf("_handleRoot() free(responseBody)\n");
    bson_free(responseBody);

    return ret;
}

int _handleGetFenceEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData, char const *pId) {
    /*
     * Fetch the record from the database
     */
    mongoc_client_pool_t *pool = pData->pool;
    mongoc_client_t *client;
    client = mongoc_client_pool_pop(pool);
    struct DB_Record *record = DB_getFenceRecord(pId, client);
    struct DB_Record *logRecord = NULL;
    json_t *actualEntryPoint = NULL;
    if (record->record) { // Find corresponding log entry
        json_t *entryTime = json_object_get(record->record, "entry_time");
        if (json_is_number(entryTime)) {
            double et = json_real_value(entryTime); //todo: fix loss of precision (jansson - json_loads)
            logRecord = DB_getGpsLogRecord((long) et, client);
            struct LocationInfo locationInfo;
            double fenceLat = json_real_value(json_object_get(record->record, "latitude"));
            double fenceLng = json_real_value(json_object_get(record->record, "longitude"));
            double radius = json_real_value(json_object_get(record->record, "radius"));

            size_t index;
            json_t *logObj;
            json_array_foreach(json_object_get(logRecord->record, "log"), index, logObj) {
                double ptLat = json_real_value(json_object_get(logObj, "latitude"));
                double ptLng = json_real_value(json_object_get(logObj, "longitude"));
                LOC_calculateLocationInfo(&locationInfo, fenceLat, fenceLng, ptLat, ptLng);
                json_object_set_new(logObj, "distance", json_real(locationInfo.distanceMeters));
                if (locationInfo.distanceMeters <= radius) {
                    double actualEntryTime = json_real_value(json_object_get(logObj, "time"));
                    double entryTimeDelta = et - actualEntryTime;
                    printf("reported entry time %lf", et);
                    printf("actual entry time %lf", actualEntryTime);
                    printf("delta entry time %lf", entryTimeDelta);
                    json_object_set_new(actualEntryPoint, "entry_delta", json_real(entryTimeDelta));
                    if (actualEntryPoint == NULL) {
                        actualEntryPoint = logObj;
                    }
                }
            }
        }
    }
    mongoc_client_pool_push(pool, client);

    /*
     * Craft json response
     */
    json_t *json_response = json_object();
    unsigned int statusCode;
    if (record->record) {
        json_object_set_new(json_response, "message", json_string(record->message));
        json_object_set(json_response, "record", record->record); // not set_new because we free in cleanup
        if (logRecord != NULL) {
            json_object_set(json_response, "corresponding_log", logRecord->record);
            json_object_set(json_response, "actual_entry", actualEntryPoint);
        }
        statusCode = MHD_HTTP_OK;
    } else {
        json_object_set_new(json_response, "message", json_string("DB_Record not found"));
        json_object_set(json_response, "record", NULL);
        statusCode = MHD_HTTP_NOT_FOUND;
    }
    char *responseBody = json_dumps(json_response, JSON_COMPACT);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, statusCode, response);

    /**
     * Cleanup
     */
    MHD_destroy_response(response);
    DB_deleteRecord(record);
    DB_deleteRecord(logRecord);
    printf("_handleGetFenceEntry() json_decref(json_response)\n");
    json_decref(json_response);
    printf("_handleGetFenceEntry() free(responseBody)\n");
    free(responseBody);

    return ret;
}

int _handleGetGpsLogEntryList(struct MHD_Connection *pConn, struct MA_HandlerData *pData) {
    /*
     * Fetch the record from the database
     */
    mongoc_client_pool_t *pool = pData->pool;
    mongoc_client_t *client;
    client = mongoc_client_pool_pop(pool);
    struct DB_Record *record = DB_getGpsLogRecordList(client);
    mongoc_client_pool_push(pool, client);

    /*
     * Craft json response
     */
    json_t *json_response = json_object();
    unsigned int statusCode;
    if (record->record) {
        json_object_set_new(json_response, "message", json_string(record->message));
        json_object_set(json_response, "record", record->record); // not set_new because we free in cleanup
        statusCode = MHD_HTTP_OK;
    } else {
        json_object_set_new(json_response, "message", json_string("DB_Record not found"));
        json_object_set(json_response, "record", NULL);
        statusCode = MHD_HTTP_NOT_FOUND;
    }
    char *responseBody = json_dumps(json_response, JSON_COMPACT);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, statusCode, response);

    /**
     * Cleanup
     */
    MHD_destroy_response(response);
    DB_deleteRecord(record);
    printf("_handleGetGpsLogEntry() json_decref(json_response)\n");
    json_decref(json_response);
    printf("_handleGetGpsLogEntry() free(responseBody)\n");
    free(responseBody);

    return ret;
}

int _handleGetGpsLogEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData, long epoch) {
    /*
     * Fetch the record from the database
     */
    mongoc_client_pool_t *pool = pData->pool;
    mongoc_client_t *client;
    client = mongoc_client_pool_pop(pool);
    struct DB_Record *record = DB_getGpsLogRecord(epoch, client);
    mongoc_client_pool_push(pool, client);

    /*
     * Craft json response
     */
    json_t *json_response = json_object();
    unsigned int statusCode;
    if (record->record) {
        json_object_set_new(json_response, "message", json_string(record->message));
        json_object_set(json_response, "record", record->record); // not set_new because we free in cleanup
        statusCode = MHD_HTTP_OK;
    } else {
        json_object_set_new(json_response, "message", json_string("DB_Record not found"));
        json_object_set(json_response, "record", NULL);
        statusCode = MHD_HTTP_NOT_FOUND;
    }
    char *responseBody = json_dumps(json_response, JSON_COMPACT);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, statusCode, response);

    /**
     * Cleanup
     */
    MHD_destroy_response(response);
    DB_deleteRecord(record);
    printf("_handleGetGpsLogEntry() json_decref(json_response)\n");
    json_decref(json_response);
    printf("_handleGetGpsLogEntry() free(responseBody)\n");
    free(responseBody);

    return ret;
}

int _handlePostWithDbInsertBodyJson(struct MHD_Connection *pConn, struct MA_HandlerData *pData,
                                    struct MA_ConnectionInfo *pConnInfo,
                                    DB_insertFunction fPtr) {
    /*
     * Insert the record in the db
     */
    mongoc_client_pool_t *pool = pData->pool;
    mongoc_client_t *client;
    client = mongoc_client_pool_pop(pool);
    struct DB_Record *record = fPtr(pConnInfo->body, client);
    mongoc_client_pool_push(pool, client);

    /*
     * Craft json response
     */
    json_t *json_response = json_object();
    unsigned int statusCode = MHD_HTTP_BAD_REQUEST;
    if (record) {
        json_object_set_new(json_response, "message", json_string(record->message));
        json_object_set(json_response, "record", record->record); // not set_new because we free in cleanup
        statusCode = MHD_HTTP_OK;
    }
    char *responseBody = json_dumps(json_response, JSON_COMPACT);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, statusCode, response);

    /*
     * Cleanup
     */
    MHD_destroy_response(response);
    DB_deleteRecord(record);
    printf("_handlePostWithDbInsertBodyJson() json_decref(json_response)\n");
    json_decref(json_response);
    printf("_handlePostWithDbInsertBodyJson() free(responseBody)\n");
    free(responseBody);

    return ret;
}

int _handlePostGpsLog(struct MHD_Connection *pConn, struct MA_HandlerData *pData, struct MA_ConnectionInfo *pConnInfo) {
    return _handlePostWithDbInsertBodyJson(pConn, pData, pConnInfo, &DB_insertGpsLogRecord);
}

int _handlePostFenceEntry(struct MHD_Connection *pConn, struct MA_HandlerData *pData,
                          struct MA_ConnectionInfo *pConnInfo) {
    return _handlePostWithDbInsertBodyJson(pConn, pData, pConnInfo, &DB_insertFenceRecord);
}

int _handleError(struct MHD_Connection *pConn) {
    /*
     * Craft json response
     */
    json_t *json_response = json_object();
    json_object_set_new(json_response, "message", json_string("Error"));
    char *responseBody = json_dumps(json_response, JSON_COMPACT);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, MHD_HTTP_INTERNAL_SERVER_ERROR, response);

    /*
     * Cleanup
     */
    MHD_destroy_response(response);
    printf("_handleError() json_decref(json_response)\n");
    json_decref(json_response);
    printf("_handleError() free(responseBody)\n");
    free(responseBody);

    return ret;
}


int _handleNotFound(struct MHD_Connection *pConn) {
    /*
     * Craft json response
     */
    json_t *json_response = json_object();
    json_object_set_new(json_response, "message", json_string("Not Found"));
    char *responseBody = json_dumps(json_response, JSON_COMPACT);

    /*
     * Queue a json response
     */
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(responseBody), (void *) responseBody, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, MHD_HTTP_NOT_FOUND, response);

    /*
     * Cleanup
     */
    MHD_destroy_response(response);
    printf("_handleNotFound() json_decref(json_response)\n");
    json_decref(json_response);
    printf("_handleNotFound() free(responseBody)\n");
    free(responseBody);

    return ret;
}

//endregion

//region PUBLIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main() {
    /**
     * Initialize mongo-c
     */
    mongoc_init();

    /*
     * Initialize mongo-c client pool
     */
    mongoc_client_pool_t *pool;
    mongoc_uri_t *uri;
    uri = mongoc_uri_new(DB_URL);
    pool = mongoc_client_pool_new(uri);

    /*
     * Setup the handler data to have access to the mongo-c client pool.
     */
    struct MA_HandlerData *data = malloc(sizeof(struct MA_HandlerData));
    data->pool = pool;

    /*
     * Start http daemon
     */
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                                                 &_answerConnection, data,
                                                 MHD_OPTION_NOTIFY_COMPLETED, __requestCompleted,
                                                 NULL, MHD_OPTION_END);

    /*
     * Wait for the 'q' key if the daemon was started
     */
    if (NULL != daemon) {
        printf("GeoFence Http daemon running - press 'q' to quit.\n");
        waitChar:
        {
            int c = getchar();
            if (c != 'q') {
                goto waitChar;
            }
        };
    }


    /**
     * Cleanup mongo-c
     */
    mongoc_client_pool_destroy(pool);
    mongoc_uri_destroy(uri);
    mongoc_cleanup();

    printf("main() free(MA_HandlerData)\n");
    free(data);

    /**
     * Stop http daemon
     */
    MHD_stop_daemon(daemon);
    return 0;
}

//endregion