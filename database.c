//
// Created by William Kamp on 6/15/16.
//

#include "database.h"

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef bson_t *(*_insertFunction)(char const *pJson);

/**
 * Insert a record into the database
 *
 * param pJson - the json post body
 */
struct DB_Record *_insertRecord(char const *pJson, mongoc_client_t *pClient, char const *pCollection,
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
bson_t *_validateFenceRecord(char const *pJson);

/**
 * Validate a json string as a valid gps_log
 *
 * returns a json_t object when valid NULL otherwise
 */
bson_t *_validateGpsLogRecord(char const *pJson);

/**
 * Create a message that must be freed with free()
 */
char *_createMessage(char const *const msg);

//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct DB_Record *_insertRecord(char const *pJson, mongoc_client_t *pClient, char const *pCollection,
                                _insertFunction fPtr) {
    struct DB_Record *retVal = _allocateRecord();
    bson_t *record = fPtr(pJson);
    if (record) {
        mongoc_collection_t *collection;
        bson_error_t bsonError;
        collection = mongoc_client_get_collection(pClient, DB, pCollection);
        if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, record, NULL, &bsonError)) {
            retVal->message = _createMessage(bsonError.message);
            retVal->record = NULL;
        } else {
            retVal->record = record;
            retVal->message = _createMessage("ok");
        }
        mongoc_collection_destroy(collection);
    } else {
        retVal->message = _createMessage("validation error");
    }
    return retVal;
}

bson_t *_validateGpsLogRecord(char const *pJson) {
    double minLatitude = 90.0;
    double maxLatitude = -90.0;
    double minLongitude = 180.0;
    double maxLongitude = -180.0;
    int64_t startTime = INT64_MAX;
    int64_t endTime = 0;

    bson_error_t error;
    bson_t *bson = NULL;

    bson_value_t const *value;
    bson_iter_t iter;

    bson = bson_new_from_json((uint8_t const *) pJson, strlen(pJson), &error);

    bool result = bson_iter_init(&iter, bson) &&
                  bson_iter_find(&iter, "log");

    if (result) {
        value = bson_iter_value(&iter);
        if (value->value_type == BSON_TYPE_ARRAY) {
            bson_iter_t logItr;
            bson_iter_t logEntryItr;
            if (bson_iter_recurse(&iter, &logItr)) {
                while (bson_iter_next(&logItr)) {
                    value = bson_iter_value(&logItr);
                    if (value->value_type == BSON_TYPE_DOCUMENT) {
                        if (bson_iter_recurse(&logItr, &logEntryItr)) {
                            // latitude
                            bson_iter_find(&logEntryItr, "latitude");
                            value = bson_iter_value(&logEntryItr);
                            if (DB_bsonTypeIsNumber(&value->value_type)) {
                                minLatitude = MIN(minLatitude, value->value.v_double);
                                maxLatitude = MAX(maxLatitude, value->value.v_double);
                            } else {
                                strncpy(error.message, "log entry missing latitude", 504);
                                result = false;
                                break;
                            }

                            // longitude
                            bson_iter_find(&logEntryItr, "longitude");
                            value = bson_iter_value(&logEntryItr); //todo: crash
                            if (DB_bsonTypeIsNumber(&value->value_type)) {
                                minLongitude = MIN(minLongitude, value->value.v_double);
                                maxLongitude = MAX(maxLongitude, value->value.v_double);
                            } else {
                                strncpy(error.message, "log entry missing longitude", 504);
                                result = false;
                                break;
                            }

                            // time
                            bson_iter_find(&logEntryItr, "time");
                            value = bson_iter_value(&logEntryItr);
                            if (DB_bsonTypeIsNumber(&value->value_type)) {
                                startTime = MIN(startTime, value->value.v_int64);
                                endTime = MAX(endTime, value->value.v_int64);
                            } else {
                                strncpy(error.message, "log entry missing time", 504);
                                result = false;
                                break;
                            }

                            bson_iter_find(&logEntryItr, "latitude");
                            value = bson_iter_value(&logEntryItr);
                            if (DB_bsonTypeIsNumber(&value->value_type)) {

                            } else {
                                strncpy(error.message, "log entry missing latitude", 504);
                                result = false;
                                break;
                            }
                        } else {
                            strncpy(error.message, "log entry missing values", 504);
                            result = false;
                            break;
                        }
                    } else {
                        strncpy(error.message, "log entry not json", 504);
                        result = false;
                        break;
                    }
                }
            }
        } else {
            strncpy(error.message, "log is not an array", 504);
            result = false;
        }

        if (result) {
            bson_t box;
            bson_init(&box);
            bson_append_double(&box, "min_latitude", -1, minLatitude);
            bson_append_double(&box, "max_latitude", -1, maxLatitude);
            bson_append_double(&box, "min_longitude", -1, minLongitude);
            bson_append_double(&box, "max_longitude", -1, maxLongitude);
            bson_append_document(bson, "bounding_box", -1, &box); // contents copied into heap allocated bson
            bson_destroy(&box);

            bson_t timeWindow;
            bson_init(&timeWindow);
            bson_append_int64(&timeWindow, "start_time", -1, startTime);
            bson_append_int64(&timeWindow, "end_time", -1, endTime);
            bson_append_document(bson, "time_window", -1, &timeWindow); // contents copied into heap allocated bson
            bson_destroy(&timeWindow);
        }
    }

    if (result) {
        return bson;
    } else {
        printf("error validating gps log record %s\n", error.message);
        bson_free(bson);
        return NULL;
    }
}

bool DB_bsonTypeIsNumber(bson_type_t *pType) {
    bson_type_t type = *pType;
    return (type == BSON_TYPE_INT64 || type == BSON_TYPE_INT32 || type == BSON_TYPE_DOUBLE);
}

bson_t *_validateFenceRecord(char const *pJson) {
    bson_error_t error;
    bson_t *bson = bson_new_from_json((uint8_t const *) pJson, strlen(pJson), &error);

    bson_value_t const *value;
    bson_iter_t iter;

    bool result = true;
    if (bson_iter_init(&iter, bson) && bson_iter_find(&iter, "identifier")) {
        value = bson_iter_value(&iter);
        if (value->value_type != BSON_TYPE_UTF8) {
            return false;
        }
    } else {
        result = false;
    }
    if (bson_iter_init(&iter, bson) && bson_iter_find(&iter, "radius")) {
        value = bson_iter_value(&iter);
        if (!DB_bsonTypeIsNumber(&value->value_type)) {
            return false;
        }
    } else {
        result = false;
    }
    if (bson_iter_init(&iter, bson) && bson_iter_find(&iter, "latitude")) {
        value = bson_iter_value(&iter);
        if (!DB_bsonTypeIsNumber(&value->value_type)) {
            return false;
        }
    } else {
        result = false;
    }
    if (bson_iter_init(&iter, bson) && bson_iter_find(&iter, "longitude")) {
        value = bson_iter_value(&iter);
        if (!DB_bsonTypeIsNumber(&value->value_type)) {
            return false;
        }
    } else {
        result = false;
    }
    if (bson_iter_init(&iter, bson) && bson_iter_find(&iter, "entry_time")) {
        value = bson_iter_value(&iter);
        if (!DB_bsonTypeIsNumber(&value->value_type)) {
            return false;
        }
    } else {
        result = false;
    }

    if (result) {
        return bson;
    } else {
        printf("error validating fence record %s\n", error.message);
        bson_free(bson);
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
        retVal->record = bson_copy(doc);
        retVal->message = _createMessage("ok");
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

    bson_t jsonArray;
    bson_init(&jsonArray);
    bson_t *records = bson_new(); //freed with DB_Record
    uint32_t i = 0;
    char iStr[16];
    char const *key;
    while (mongoc_cursor_next(cursor, &doc)) {
        //initialize jsonArray
        if (NULL == retVal->record) {
            retVal->message = _createMessage("ok");
            retVal->record = records;
        }

        //http://api.mongodb.com/libbson/1.3.5/performance.html#keys
        bson_uint32_to_string(i, &key, iStr, sizeof iStr);
        BSON_APPEND_DOCUMENT(&jsonArray, key, doc);
        ++i;
    }

    if (retVal->message && bson_count_keys(&jsonArray) > 0) {
        BSON_APPEND_ARRAY(records, "records", &jsonArray);
    }

    bson_destroy(query);
    bson_destroy(fields);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    bson_destroy(&jsonArray);

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

    bson_t jsonArray;
    bson_init(&jsonArray);
    bson_t *records = bson_new(); //freed with DB_Record
    uint32_t i = 0;
    char iStr[16];
    char const *key;
    while (mongoc_cursor_next(cursor, &doc)) {
        //todo: add whether there is a corresponding gps_log

        //initialize jsonArray
        if (NULL == retVal->record) {
            retVal->message = _createMessage("ok");
            retVal->record = records;
        }

        //http://api.mongodb.com/libbson/1.3.5/performance.html#keys
        bson_uint32_to_string(i, &key, iStr, sizeof iStr);
        BSON_APPEND_DOCUMENT(&jsonArray, key, doc);
        ++i;
    }

    if (retVal->message && bson_count_keys(&jsonArray) > 0) {
        BSON_APPEND_ARRAY(records, "records", &jsonArray);
    }

    bson_destroy(query);
    bson_destroy(&jsonArray);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return retVal;
}

struct DB_Record *DB_getGpsLogRecord(int64_t pEpochTime, mongoc_client_t *pClient) {
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
        retVal->record = bson_copy(doc);
        retVal->message = _createMessage("ok");
    }

    bson_destroy(&query);
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
    bson_destroy(&selector);
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
    bson_destroy(&selector);
    mongoc_collection_destroy(collection);
}

char *_createMessage(char const *const pMsg) {
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
            bson_free(pResult->record);
        }
        if (NULL != pResult->message) {
            free(pResult->message);
        }
        free(pResult);
    }
}

//endregion
