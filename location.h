//
// Created by William Kamp on 6/21/16.
//

#ifndef GEOFENCEBEC_LOCATION_H
#define GEOFENCEBEC_LOCATION_H

struct LocationInfo {
    float distanceMeters;
    float initialBearingDegrees;
    float finalBearingDegrees;
};

/**
 * Calculate the distance in meters, initial bearing in degrees and final bearing in degrees between two WGS-84 points
 *
 * Calculations based on http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf using the "Inverse Formula" (section 4)
 */
void LOC_calculateLocationInfo(struct LocationInfo *pInfo, double startLat, double startLng, double endLat,
                               double endLng);

#endif //GEOFENCEBEC_LOCATION_H
