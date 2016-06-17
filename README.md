#GeoFenceBeC

#### An http micro-web service and REST API written in C which includes a NoSQL data store and an http server.

####Library Dependencies

* CMake<br/>
https://cmake.org/<br/>
`brew install cmake`

* MongoDb C Driver<br/>
https://github.com/mongodb/mongo-c-driver
http://api.mongodb.com/c/current/<br/>
`brew install mongo-c`

* Jansson<br/>
http://www.digip.org/jansson/
https://jansson.readthedocs.io/en/2.7/apiref.html<br/>
`brew install jansson`

* Libmicrohttpd<br/>
https://www.gnu.org/software/libmicrohttpd/<br/>
`brew install microhttpd`

##Conventions

* Private functions are prefixed with `_`
* Private static functions are prefixed with `__`
* Public (including static) functions are prefixed with abbreviation representing file's single responsibility e.g.
database.c functions are prefixed with `DB_`

* Code is organized with the following regions which are collapsible in [CLion](https://www.jetbrains.com/clion/).

```
//region STRUCTURES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PRIVATE INTERFACE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region STATIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PRIVATE FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion

//region PUBLIC FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//endregion
```