//
// Created by William Kamp on 6/15/16.
//

#ifndef GEOFENCEBEC_DATABASE_H
#define GEOFENCEBEC_DATABASE_H

#include <libmongoc-1.0/mongoc.h>

#define DB_URL "mongodb://localhost:27017/"
#define DB "geofence"
#define COLLECTION_FENCES "fences"
#define COLLECTION_GPS_LOGS "gps_logs"

/**
 * return 0 when inserted : 1 when NOT inserted (invalid json)
 */
int insertFenceRecord(const char* pJson, mongoc_client_t *client);

#endif //GEOFENCEBEC_DATABASE_H
