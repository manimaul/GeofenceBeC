//
// Created by William Kamp on 6/15/16.
//

#include "database.h"

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * Create a DB_Record structure that must be freed with void DB_deleteRecord(struct DB_Record* pResult)
 */
struct DB_Record *createRecord();

/**
 * Validate a json string as a valid fence_record
 *
 * returns a json_t object when valid NULL otherwise
 */
json_t *_validateFenceRecord(const char *pJson);

/**
 * Validate a json string as a valid gps_log
 *
 * returns a json_t object when valid NULL otherwise
 */
json_t *_validateGpsLogRecord(const char *pJson);

/**
 * Create a message that must bee freed with free()
 */
char *_createMessage(char const *const msg);

//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//void mongoUpsert() {
//    mongoc_collection_t *collection;
//    mongoc_client_t *client;
//    bson_error_t error;
//    bson_oid_t oid;
//    bson_t *doc = NULL;
//    bson_t *update = NULL;
//    bson_t *query = NULL;
//
//    client = mongoc_client_new("mongodb://localhost:27017/");
//    collection = mongoc_client_get_collection(client, "mydb", "mycoll");
//
//    bson_oid_init(&oid, NULL);
//    doc = BCON_NEW("_id", BCON_OID(&oid),
//                   "key", BCON_UTF8("old_value"));
//
//    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
//        fprintf(stderr, "%s\n", error.message);
//        goto fail;
//    }
//
//    query = BCON_NEW("_id", BCON_OID(&oid));
//    update = BCON_NEW("$set", "{",
//                      "key", BCON_UTF8("new_value"),
//                      "updated", BCON_BOOL(true),
//                      "}");
//
//    if (!mongoc_collection_update(collection, MONGOC_UPDATE_NONE, query, update, NULL, &error)) {
//        fprintf(stderr, "%s\n", error.message);
//        goto fail;
//    }
//
//    fail:
//    if (doc)
//        bson_destroy(doc);
//    if (query)
//        bson_destroy(query);
//    if (update)
//        bson_destroy(update);
//
//    mongoc_collection_destroy(collection);
//    mongoc_client_destroy(client);
//}

json_t *_validateGpsLogRecord(const char *pJson) {
    json_error_t error;
    json_t *json = json_loads(pJson, strlen(pJson), &error);
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
                value = json_object_get(logObj, "time");
                if (!json_is_number(value)) {
                    result = false;
                    goto resultBlock;
                }
            }
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

json_t *_validateFenceRecord(const char *pJson) {
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

struct DB_Record *DB_insertGpsLogRecord(const char *pJson, mongoc_client_t *pClient) {
    struct DB_Record *retVal = createRecord();
    json_t *record = _validateGpsLogRecord(pJson);
    if (record) {
        mongoc_collection_t *collection;
        bson_error_t bsonError;
        bson_t *doc;
        collection = mongoc_client_get_collection(pClient, DB, COLLECTION_GPS_LOGS);
        doc = bson_new_from_json((const uint8_t *) pJson, -1, &bsonError);
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

struct DB_Record *DB_insertFenceRecord(const char *pJson, mongoc_client_t *pClient) {
    struct DB_Record *retVal = createRecord();
    json_t *record = _validateFenceRecord(pJson);
    if (record) {
        mongoc_collection_t *collection;
        bson_error_t bsonError;
        bson_t *doc;
        collection = mongoc_client_get_collection(pClient, DB, COLLECTION_FENCES);
        doc = bson_new_from_json((const uint8_t *) pJson, -1, &bsonError);
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

struct DB_Record *DB_getFenceRecord(const char *pIdentifier, mongoc_client_t *pClient) {
    struct DB_Record *retVal = createRecord();

    mongoc_collection_t *collection;
    mongoc_cursor_t *cursor;
    const bson_t *doc;
    bson_t *query;

    collection = mongoc_client_get_collection(pClient, DB, COLLECTION_FENCES);
    query = bson_new();
    BSON_APPEND_UTF8(query, "identifier", pIdentifier);
    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, query, NULL, NULL);

    while (mongoc_cursor_next(cursor, &doc)) {
        json_error_t error;
        char *value = bson_as_json(doc, NULL);
        retVal->record = json_loads(value, strlen(value), &error);
        retVal->message = _createMessage("ok");
        bson_free(value);
        break;
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);

    return retVal;
}

char *_createMessage(char const *const pMsg) {
    char *retVal = malloc(sizeof(char) * strlen(pMsg));
    strcpy(retVal, pMsg);
    return retVal;
}

struct DB_Record *createRecord() {
    struct DB_Record *retVal = malloc(sizeof(struct DB_Record));
    retVal->message = NULL;
    retVal->record = NULL;
    return retVal;
}

void DB_deleteRecord(struct DB_Record *pResult) {
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
