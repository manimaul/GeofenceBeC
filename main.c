#include <microhttpd.h>
#include <jansson.h>
#include <string.h>
#include <libmongoc-1.0/mongoc.h>
#include "database.h"

#define PORT 8080
#define TEXT_HTML "text/html"
#define APPLICATION_JSON "application/json"
#define CONTENT_TYPE "text/html"
#define METHOD_GET "GET"
#define METHOD_POST "POST"

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct HandlerData {
    mongoc_client_pool_t *pool;
} HandlerData;

struct ConnectionInfo {
    char *body;
};

//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The handler function for all http requests.
 */
int answerConnection(void *pCls,
                     struct MHD_Connection *pConn,
                     const char *pUrl,
                     const char *pMethod,
                     const char *version,
                     const char *upload_data,
                     size_t *upload_data_size,
                     void **con_cls);

/**
 * Request handler for / endpoint
 */
int handleRoot(struct MHD_Connection *pConn);

/**
 * Request handler for /fence_entry endpoint
 */
int handleFenceEntry(struct MHD_Connection *pConn, struct HandlerData *pData, struct ConnectionInfo *pConnInfo);

/**
 * Request handler for 404 - resource not found
 */
int handleNotFound(struct MHD_Connection *connection);

//endregion

//region STATIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
static void requestCompleted(void *cls, struct MHD_Connection *connection, void **con_cls,
                             enum MHD_RequestTerminationCode toe) {

    struct ConnectionInfo *con_info = *con_cls;

    if (NULL == con_info) {
        return;
    }

    if (NULL != con_info->body) {
        free(con_info->body);
    }
    free(con_info);
    *con_cls = NULL;
}
#pragma clang diagnostic pop

//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
int answerConnection(void *pCls,
                     struct MHD_Connection *pConn,
                     const char *pUrl,
                     const char *pMethod,
                     const char *pVersion,
                     const char *pUploadData,
                     size_t *pUploadDataSize,
                     void **pConnCls) {

    if (0 == strcmp(pMethod, METHOD_GET)) {
        if (0 == strcmp(pUrl, "/")) {
            return handleRoot(pConn);
        }
    } else if (0 == strcmp(pMethod, METHOD_POST)) {
        if (0 == strcmp(pUrl, "/fence_entry")) {
            struct ConnectionInfo *connectionInfo = *pConnCls;
            if (NULL == *pConnCls) {
                connectionInfo = malloc(sizeof(struct ConnectionInfo));
                *pConnCls = (void *) connectionInfo;
                return MHD_YES;
            }
            if (*pUploadDataSize) {
                size_t len = *pUploadDataSize;
                if (len > 0) {
                    char *body = malloc(len);
                    strcpy(body, pUploadData);
                    connectionInfo->body = body;
                }
                *pUploadDataSize = 0;
                return MHD_YES;
            }
            return handleFenceEntry(pConn, pCls, connectionInfo);
        }
    }

    return handleNotFound(pConn);
}
#pragma clang diagnostic pop

int handleRoot(struct MHD_Connection *pConn) {
    const char *page = "<html><body>GeoFence Mark API</body></html>";
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, CONTENT_TYPE, TEXT_HTML);
    ret = MHD_queue_response(pConn, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

int handleFenceEntry(struct MHD_Connection *pConn, struct HandlerData *pData, struct ConnectionInfo *pConnInfo) {
    mongoc_client_pool_t *pool = pData->pool;
    mongoc_client_t *client;
    client = mongoc_client_pool_pop(pool);
    insertFenceRecord(pConnInfo->body, client);
    mongoc_client_pool_push(pool, client);

    json_t *json_response = json_object();
    json_object_set(json_response, "message", json_string("OK"));
    char *page = json_dumps(json_response, JSON_COMPACT);
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATION_JSON);
    int ret = MHD_queue_response(pConn, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    free(page); //https://jansson.readthedocs.io/en/2.7/apiref.html#encoding
    return ret;
}

int handleNotFound(struct MHD_Connection *pConn) {
    const char *page = "<html><body>Whoops! 404</body></html>";
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, CONTENT_TYPE, TEXT_HTML);
    ret = MHD_queue_response(pConn, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

//endregion

//region PUBLIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main() {
    /**
     * Initialize mongo-c
     */
    mongoc_init();

    /**
     * Initialize mongo-c client pool
     */
    mongoc_client_pool_t *pool;
    mongoc_uri_t *uri;
    uri = mongoc_uri_new(DB_URL);
    pool = mongoc_client_pool_new(uri);

    struct HandlerData *data = malloc(sizeof(HandlerData));
    data->pool = pool;

    /*
     * Start http daemon
     */
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                                                 &answerConnection, data,
                                                 MHD_OPTION_NOTIFY_COMPLETED, requestCompleted,
                                                 NULL, MHD_OPTION_END);

    if (NULL == daemon) {
        return 1;
    }

    //wait for key to quit
    getchar();

    /**
     * Cleanup mongo-c
     */
    mongoc_client_pool_destroy(pool);
    mongoc_uri_destroy(uri);
    mongoc_cleanup();

    free(data);

    /**
     * Stop http daemon
     */
    MHD_stop_daemon(daemon);
    return 0;
}

//endregion