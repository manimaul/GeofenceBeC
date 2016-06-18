//
// Created by William Kamp on 6/15/16.
//

#ifndef GEOFENCEBEC_DATABASE_H
#define GEOFENCEBEC_DATABASE_H

#include <libmongoc-1.0/mongoc.h>
#include <jansson.h>

#define DB_URL "mongodb://localhost:27017/"
#define DB "geofence"
#define COLLECTION_FENCES "fences"
#define COLLECTION_GPS_LOGS "gps_logs"

struct DB_Record {
    json_t *record;
    char *message;
};

/*
 * Defines a function that inserts json into the database
 */
typedef struct DB_Record* (*DB_insertFunction) (const char *pJson, mongoc_client_t *pClient);

/**
 * Inserts a gps log record when the record is valid
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_insertGpsLogRecord(const char *pJson, mongoc_client_t *pClient);

/**
 * Inserts a fence record when the record is valid
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_insertFenceRecord(const char *pJson, mongoc_client_t *pClient);

/**
 * Retrieve a fence record with an identifier
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_getFenceRecord(const char *pIdentifier, mongoc_client_t *pClient);

/**
 * Deallocate a record that has been retrieved
 */
void DB_deleteRecord(struct DB_Record *pRecord);

#endif //GEOFENCEBEC_DATABASE_H
