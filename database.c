//
// Created by William Kamp on 6/15/16.
//

#include "database.h"
#include <jansson.h>

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool validateFenceRecord(json_t *pJson);

//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//void mongoFind() {
//    mongoc_client_t *client;
//    mongoc_collection_t *collection;
//    mongoc_cursor_t *cursor;
//    const bson_t *doc;
//    bson_t *query;
//    char *str;
//
//    client = mongoc_client_new("mongodb://localhost:27017/");
//    collection = mongoc_client_get_collection(client, "mydb", "mycoll");
//    query = bson_new();
//    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
//
//    while (mongoc_cursor_next(cursor, &doc)) {
//        str = bson_as_json(doc, NULL);
//        printf("%s\n", str);
//        bson_free(str);
//    }
//
//    bson_destroy(query);
//    mongoc_cursor_destroy(cursor);
//    mongoc_collection_destroy(collection);
//    mongoc_client_destroy(client);
//}
//
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
//
//void mongoInsert() {
//    mongoc_client_t *client;
//    mongoc_collection_t *collection;
//    bson_error_t error;
//    bson_oid_t oid;
//    bson_t *doc;
//
//    mongoc_init();
//
//    client = mongoc_client_new("mongodb://localhost:27017/");
//    collection = mongoc_client_get_collection(client, "mydb", "mycoll");
//
//    doc = bson_new();
//    bson_oid_init(&oid, NULL);
//    BSON_APPEND_OID(doc, "_id", &oid);
//    BSON_APPEND_UTF8(doc, "hello", "world");
//
//    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
//        fprintf(stderr, "%s\n", error.message);
//    }
//
//    bson_destroy(doc);
//    mongoc_collection_destroy(collection);
//}

bool validateFenceRecord(json_t *pJson) {
    if (pJson != NULL) {
        json_t *obj = json_object_get(pJson, "identifier");
        if (!json_is_string(obj)) {
            return false;
        }
        obj = json_object_get(pJson, "latitude");
        if (!json_is_real(obj)) {
            return false;
        }
        obj = json_object_get(pJson, "longitude");
        if (!json_is_real(obj)) {
            return false;
        }
        obj = json_object_get(pJson, "entry_time");
        if (!json_is_number(obj)) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

//endregion

//region PUBLIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int insertFenceRecord(const char* pJson, mongoc_client_t *client) {
    int retVal = 1;
    json_error_t error;
    json_t *json = json_loads(pJson, strlen(pJson), &error);

    if (validateFenceRecord(json)) {
        mongoc_collection_t *collection;
        bson_error_t bsonError;
        bson_t *doc;
        collection = mongoc_client_get_collection(client, DB, COLLECTION_FENCES);
        doc = bson_new_from_json((const uint8_t *)pJson, -1, &bsonError);
        if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &bsonError)) {
            fprintf(stderr, "%s\n", bsonError.message);
        } else {
            retVal = 0;
        }

        bson_destroy(doc);
        mongoc_collection_destroy(collection);
    }
    return retVal;
}

//endregion
