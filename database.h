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
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct DB_Record {
    json_t *record;
    char *message;
};

/*
 * Defines a function that inserts json into the database
 */
typedef struct DB_Record* (*DB_insertFunction) (char const *pJson, mongoc_client_t *pClient);

/**
 * Inserts a gps log record when the record is valid
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_insertGpsLogRecord(char const *pJson, mongoc_client_t *pClient);

/**
 * Inserts a fence record when the record is valid
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_insertFenceRecord(char const *pJson, mongoc_client_t *pClient);

/**
 * Retrieve a fence record with an identifier
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_getFenceRecord(char const *pIdentifier, mongoc_client_t *pClient);

/**
 * Retrieve a list of log record sub-sets (id, time_window, bounding_box)
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_getGpsLogRecordList(mongoc_client_t *pClient);

/**
 * Retrieve a list of fence entry records
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_getFenceRecordList(mongoc_client_t *pClient);

/**
 * Retrieve a gps log record that spans a specified time
 *
 * returns struct DB_Record which you must later DB_deleteRecord()
 */
struct DB_Record *DB_getGpsLogRecord(long pEpochTime, mongoc_client_t *pClient);

/**
 * Delete a gps log record with an id.
 */
void DB_deleteGpsLogRecord(char const *pIdentifier, mongoc_client_t *pClient);

/**
 * Delete a gps log record with an id.
 */
void DB_deleteFenceRecord(char const *pIdentifier, mongoc_client_t *pClient);

/**
 * Deallocate a record that has been retrieved
 */
void DB_freeRecord(struct DB_Record *pRecord);

#endif //GEOFENCEBEC_DATABASE_H
