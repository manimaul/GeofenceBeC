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

struct Record {
    json_t* record;
    char* message;
};

json_t* validateFenceRecord(const char* pJson);

/**
 * Inserts a fence record when the record is valid.
 *
 * returns json string of inserted record which you must deleteRecord()
 */
struct Record* insertFenceRecord(const char* pJson, mongoc_client_t *pClient);

/**
 *
 */
void deleteRecord(struct Record* pRecord);

/**
 * Inserts a gps_log record when the record is valid.
 *
 * returns json string of inserted record which you must free()
 */
//char* insertGpsLogRecord(const char* pJson, mongoc_client_t *pClient);

#endif //GEOFENCEBEC_DATABASE_H
