#include <microhttpd.h>
#include <jansson.h>
#include <string.h>
#include <libbson-1.0/bson.h>
#include <libmongoc-1.0/mongoc.h>

#define PORT 8080
#define TEXT_HTML "text/html"
#define APPLICATON_JSON "application/json"
#define CONTENT_TYPE "text/html"
#define GET "GET"
#define POST "POST"


#ifndef GEOFENCEBEC_MAIN
#define GEOFENCEBEC_MAIN

//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct HandlerData {
    mongoc_client_pool_t *pool;
} HandlerData;

//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * The handler function for all http requests.
 */
int accessHandler(void *cls,
                  struct MHD_Connection *connection,
                  const char *url,
                  const char *method,
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
int handleFenceEntry(struct MHD_Connection *pConn, struct HandlerData *pData);

/**
 * Request handler for 404 - resource not found
 */
int handleNotFound(struct MHD_Connection *connection);

//endregion

#endif //GEOFENCEBEC_MAIN

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int accessHandler(void *cls,
                  struct MHD_Connection *connection,
                  const char *url,
                  const char *method,
                  const char *version,
                  const char *upload_data,
                  size_t *upload_data_size,
                  void **con_cls) {

    if (strcmp(method, GET) == 0) {
        if (strcmp(url, "/") == 0) {
            return handleRoot(connection);
        }
    } else if (strcmp(method, POST) == 0) {
        if (strcmp(url, "/fence_entry") == 0) {
            return handleFenceEntry(connection, cls);
        }
    }

    return handleNotFound(connection);
}

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

int handleFenceEntry(struct MHD_Connection *pConn, struct HandlerData *pData) {
    mongoc_client_pool_t *pool = pData->pool;
    mongoc_client_t      *client;
    client = mongoc_client_pool_pop(pool);
    /* Do something... */
    mongoc_client_pool_push(pool, client);

    json_t *json_response = json_object();
    json_object_set(json_response, "message", json_string("OK"));
    char *page = json_dumps(json_response, JSON_COMPACT);
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, CONTENT_TYPE, APPLICATON_JSON);
    ret = MHD_queue_response(pConn, MHD_HTTP_OK, response);
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

void mongoFind() {
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    mongoc_cursor_t *cursor;
    const bson_t *doc;
    bson_t *query;
    char *str;

    client = mongoc_client_new("mongodb://localhost:27017/");
    collection = mongoc_client_get_collection(client, "mydb", "mycoll");
    query = bson_new();
    cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

    while (mongoc_cursor_next(cursor, &doc)) {
        str = bson_as_json(doc, NULL);
        printf("%s\n", str);
        bson_free(str);
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
}

void mongoUpsert() {
    mongoc_collection_t *collection;
    mongoc_client_t *client;
    bson_error_t error;
    bson_oid_t oid;
    bson_t *doc = NULL;
    bson_t *update = NULL;
    bson_t *query = NULL;

    client = mongoc_client_new("mongodb://localhost:27017/");
    collection = mongoc_client_get_collection(client, "mydb", "mycoll");

    bson_oid_init(&oid, NULL);
    doc = BCON_NEW("_id", BCON_OID(&oid),
                    "key", BCON_UTF8("old_value"));

    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
        goto fail;
    }

    query = BCON_NEW("_id", BCON_OID(&oid));
    update = BCON_NEW("$set", "{",
                       "key", BCON_UTF8("new_value"),
                       "updated", BCON_BOOL(true),
                       "}");

    if (!mongoc_collection_update(collection, MONGOC_UPDATE_NONE, query, update, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
        goto fail;
    }

    fail:
    if (doc)
        bson_destroy(doc);
    if (query)
        bson_destroy(query);
    if (update)
        bson_destroy(update);

    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
}

void mongoInsert() {
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    bson_error_t error;
    bson_oid_t oid;
    bson_t *doc;

    mongoc_init();

    client = mongoc_client_new("mongodb://localhost:27017/");
    collection = mongoc_client_get_collection(client, "mydb", "mycoll");

    doc = bson_new();
    bson_oid_init(&oid, NULL);
    BSON_APPEND_OID(doc, "_id", &oid);
    BSON_APPEND_UTF8(doc, "hello", "world");

    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
    }

    bson_destroy(doc);
    mongoc_collection_destroy(collection);
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
    uri = mongoc_uri_new("mongodb://localhost:27017/");
    pool = mongoc_client_pool_new(uri);

    struct HandlerData *data = malloc(sizeof(HandlerData));
    data->pool = pool;

    /*
     * Start http daemon
     */
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &accessHandler, data,
                                                 MHD_OPTION_END);
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