//
// Created by William Kamp on 6/15/16.
//

#include "database.h"

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef json_t * (*_insertFunction) (char const *pJson);

/**
 * Insert a record into the database
 *
 * param pJson - the json post body
 */
struct DB_Record *_insertRecord(char const *pJson, mongoc_client_t *pClient, char const* pCollection,
                                _insertFunction fPtr);

/**
 * Create a DB_Record structure that must be freed with void DB_freeRecord(struct DB_Record* pResult)
 */
struct DB_Record *_allocateRecord();

/**
 * Validate a json string as a valid fence_record
 *
 * returns a json_t object when valid NULL otherwise
 */
json_t *_validateFenceRecord(char const *pJson);

/**
 * Validate a json string as a valid gps_log
 *
 * returns a json_t object when valid NULL otherwise
 */
json_t *_validateGpsLogRecord(char const *pJson);

/**
 * Create a message that must be freed with free()
 */
char *_createMessage(char const *const msg);

//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct DB_Record *_insertRecord(char const *pJson, mongoc_client_t *pClient, char const* pCollection,
                                _insertFunction fPtr) {
    struct DB_Record *retVal = _allocateRecord();
    json_t *record = fPtr(pJson);
    if (record) {
        mongoc_collection_t *collection;
        bson_error_t bsonError;
        bson_t *doc;
        collection = mongoc_client_get_collection(pClient, DB, pCollection);
        char *insertJson = json_dumps(record, JSON_COMPACT);
        doc = bson_new_from_json((uint8_t const *) insertJson, -1, &bsonError);
        free(insertJson);
        if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &bsonError)) {
            retVal->message = malloc(strlen(bsonError.message));
            strcpy(retVal->message, bsonError.message);
            retVal->record = NULL;
        } else {
            char *value = bson_as_json(doc, NULL);
            json_error_t error;
            retVal->record = json_loads(value, strlen(value), &error);
            retVal->message = _createMessage("ok");
            bson_free(value);
        }
        bson_destroy(doc);
        mongoc_collection_destroy(collection);
    } else {
        retVal->message = _createMessage("validation error");
    }
    return retVal;
}

json_t *_validateGpsLogRecord(char const *pJson) {
    json_error_t error;
    json_t *json = json_loads(pJson, strlen(pJson), &error);
    double minLatitude = 90.0;
    double maxLatitude = -90.0;
    double minLongitude = 180.0;
    double maxLongitude = -180.0;
    json_int_t startTime = 0;
    json_int_t endTime = 0;
    bool result = true;
    if (json != NULL) {
        json_t *obj = json_object_get(json, "log");
        if (!json_is_array(obj)) {
            result = false;
        } else {
            size_t index;
            json_t *logObj;
            json_array_foreach(obj, index, logObj) {
                json_t *value = json_object_get(logObj, "latitude");
                if (!json_is_number(value)) {
                    result = false;
                    goto resultBlock;
                }
                value = json_object_get(logObj, "longitude");
                if (!json_is_number(value)) {
                    result = false;
                    goto resultBlock;
                }
                double longitude = json_number_value(value);
                if (index == 0) {
                    minLongitude = longitude;
                    maxLongitude = longitude;
                } else {
                    minLongitude = MIN(minLongitude, longitude);
                    maxLongitude = MAX(maxLongitude, longitude);
                }
                value = json_object_get(logObj, "time");
                if (!json_is_number(value)) {
                    result = false;
                    goto resultBlock;
                }
                double latitude = json_number_value(value);
                if (index == 0) {
                    minLatitude= latitude;
                    maxLatitude= latitude;
                } else {
                    minLatitude= MIN(minLatitude, latitude);
                    maxLatitude= MAX(maxLatitude, latitude);
                }
                long t = (long) json_number_value(value);
                if (index == 0) {
                    startTime = t;
                    endTime = t;
                } else {
                    startTime = MIN(startTime, t);
                    endTime = MAX(endTime, t);
                }
            }

            json_t *box = json_object();
            json_object_set_new(box, "min_latitude", json_real(minLatitude));
            json_object_set_new(box, "max_latitude", json_real(maxLatitude));
            json_object_set_new(box, "min_longitude", json_real(minLongitude));
            json_object_set_new(box, "max_longitude", json_real(maxLongitude));
            json_object_set_new(json, "bounding_box", box);

            json_t *timeWindow = json_object();
            json_object_set_new(timeWindow, "start_time", json_integer(startTime));
            json_object_set_new(timeWindow, "end_time", json_integer(endTime));
            json_object_set_new(json, "time_window", timeWindow);
        }
    }

    resultBlock:
    {
        if (result) {
            return json;
        } else {
            json_decref(json);
            return NULL;
        }
    }
}

json_t *_validateFenceRecord(char const *pJson) {
    json_error_t error;
    json_t *json = json_loads(pJson, strlen(pJson), &error);
    bool result = true;
    if (json != NULL) {
        json_t *obj = json_object_get(json, "identifier");
        if (!json_is_string(obj)) {
            result = false;
        }
        obj = json_object_get(json, "radius");
        if (result && !json_is_number(obj)) {
            result = false;
        }
        obj = json_object_get(json, "latitude");
        if (result && !json_is_real(obj)) {
            result = false;
        }
        obj = json_object_get(json, "longitude");
        if (result && !json_is_real(obj)) {
            result = false;
        }
        obj = json_object_get(json, "entry_time");
        if (result && !json_is_number(obj)) {
            result = false;
        }
    }
    if (result) {
        return json;
    } else {
        json_decref(json);
        return NULL;
    }
}

//endregion

//region PUBLIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct DB_Record *DB_insertGpsLogRecord(char const *pJson, mongoc_client_t *pClient) {
    return _insertRecord(pJson, pClient, COLLECTION_GPS_LOGS, &_validateGpsLogRecord);
}

struct DB_Record *DB_insertFenceRecord(char const *pJson, mongoc_client_t *pClient) {
    return _insertRecord(pJson, pClient, COLLECTION_FENCES, &_validateFenceRecord);
}

struct DB_Record *DB_getFenceRecord(char const *pIdentifier, mongoc_client_t *pClient) {
    struct DB_Record *retVal = _allocateRecord();

    mongoc_collection_t *collection;
    mongoc_cursor_t *cursor;
    bson_t const *doc;
    bson_t *query;

    collection = mongoc_client_get_collection(pClient, DB, COLLECTION_FENCES);
    query = bson_new();
    BSON_APPEND_UTF8(query, "identifier", pIdentifier);
    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, query, NULL, NULL);

    if (mongoc_cursor_next(cursor, &doc)) {
        json_error_t error;
        char *value = bson_as_json(doc, NULL);
        retVal->record = json_loads(value, strlen(value), &error);
        retVal->message = _createMessage("ok");
        bson_free(value);
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return retVal;
}

struct DB_Record *DB_getGpsLogRecordList(mongoc_client_t *pClient) {
    struct DB_Record *retVal = _allocateRecord();

    mongoc_collection_t *collection = mongoc_client_get_collection(pClient, DB, COLLECTION_GPS_LOGS);
    mongoc_cursor_t *cursor;
    bson_t const *doc;

    /*
     * Build the query
     */
    bson_t *query = bson_new();
    bson_t *fields = bson_new();

    BSON_APPEND_INT32(fields, "_id", 1);
    BSON_APPEND_INT32(fields, "time_window", 1);
    BSON_APPEND_INT32(fields, "bounding_box", 1);

    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1000, 0, query, fields, NULL);

    json_t *jsonArray = NULL;
    while (mongoc_cursor_next(cursor, &doc)) {
        json_error_t error;
        char *value = bson_as_json(doc, NULL);
        if (NULL == retVal->record) {
            retVal->message = _createMessage("ok");
            retVal->record = json_object();
            jsonArray = json_array();
            json_object_set_new(retVal->record, "records", jsonArray);
        }
        json_array_append_new(jsonArray, json_loads(value, strlen(value), &error));
        bson_free(value);
    }

    bson_destroy(query);
    bson_destroy(fields);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return retVal;
}

struct DB_Record *DB_getFenceRecordList(mongoc_client_t *pClient) {
    struct DB_Record *retVal = _allocateRecord();

    mongoc_collection_t *collection = mongoc_client_get_collection(pClient, DB, COLLECTION_FENCES);
    mongoc_cursor_t *cursor;
    bson_t const *doc;

    /*
     * Build the query
     */
    bson_t *query = bson_new();
    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1000, 0, query, NULL, NULL);

    json_t *jsonArray = NULL;
    while (mongoc_cursor_next(cursor, &doc)) {
        json_error_t error;
        char *value = bson_as_json(doc, NULL);
        if (NULL == retVal->record) {
            retVal->message = _createMessage("ok");
            retVal->record = json_object();
            jsonArray = json_array();
            json_object_set_new(retVal->record, "records", jsonArray);
        }
        json_array_append_new(jsonArray, json_loads(value, strlen(value), &error));
        bson_free(value);
    }

    bson_destroy(query);

    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return retVal;
}

struct DB_Record *DB_getGpsLogRecord(long pEpochTime, mongoc_client_t *pClient) {
    struct DB_Record *retVal = _allocateRecord();

    mongoc_collection_t *collection = mongoc_client_get_collection(pClient, DB, COLLECTION_GPS_LOGS);
    mongoc_cursor_t *cursor;
    bson_t const *doc;

    /*
     * Build the query
     */
    bson_t query;
    bson_init(&query);
    bson_t queryChildEndTime;
    bson_t queryChildStartTime;

    BSON_APPEND_DOCUMENT_BEGIN(&query, "time_window.end_time", &queryChildEndTime);
    BSON_APPEND_INT64(&queryChildEndTime, "$gte", pEpochTime);
    bson_append_document_end(&query, &queryChildEndTime);

    BSON_APPEND_DOCUMENT_BEGIN(&query, "time_window.start_time", &queryChildStartTime);
    BSON_APPEND_INT64(&queryChildStartTime, "$lte", pEpochTime);
    bson_append_document_end(&query, &queryChildStartTime);

    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, &query, NULL, NULL);

    if (mongoc_cursor_next(cursor, &doc)) {
        json_error_t error;
        char *value = bson_as_json(doc, NULL);
        retVal->record = json_loads(value, strlen(value), &error);
        retVal->message = _createMessage("ok");
        bson_free(value);
    }

    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return retVal;
}

void DB_deleteGpsLogRecord(char const *pIdentifier, mongoc_client_t *pClient) {
    mongoc_collection_t *collection = mongoc_client_get_collection(pClient, DB, COLLECTION_GPS_LOGS);
    bson_t selector;
    bson_init(&selector);

    bson_oid_t oid;
    bson_oid_init_from_string(&oid, pIdentifier);
    BSON_APPEND_OID(&selector, "_id", &oid);
    bson_error_t error;

    mongoc_collection_remove(collection, MONGOC_REMOVE_SINGLE_REMOVE, &selector, NULL, &error);
    printf("error %s\n", error.message);
    mongoc_collection_destroy(collection);
}

void DB_deleteFenceRecord(char const *pIdentifier, mongoc_client_t *pClient) {
    mongoc_collection_t *collection = mongoc_client_get_collection(pClient, DB, COLLECTION_FENCES);
    bson_t selector;
    bson_init(&selector);

    bson_oid_t oid;
    bson_oid_init_from_string(&oid, pIdentifier);
    BSON_APPEND_OID(&selector, "_id", &oid);
    bson_error_t error;

    mongoc_collection_remove(collection, MONGOC_REMOVE_SINGLE_REMOVE, &selector, NULL, &error);
    printf("error %s\n", error.message);
    mongoc_collection_destroy(collection);
}

char *_createMessage(char const * const pMsg) {
    char *retVal = malloc(strlen(pMsg));
    strcpy(retVal, pMsg);
    return retVal;
}

struct DB_Record *_allocateRecord() {
    struct DB_Record *retVal = malloc(sizeof(struct DB_Record));
    retVal->message = NULL;
    retVal->record = NULL;
    return retVal;
}

void DB_freeRecord(struct DB_Record *pResult) {
    if (NULL != pResult) {
        if (NULL != pResult->record) {
            json_decref(pResult->record);
        }
        if (NULL != pResult->message) {
            free(pResult->message);
        }
        free(pResult);
    }
}

//endregion
