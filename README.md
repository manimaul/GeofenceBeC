#GeoFenceBeC

#### An http micro-web service and REST API written in C which includes a NoSQL data store and an http server.

####Library Dependencies

* CMake<br/>
https://cmake.org/<br/>
OSX: `brew install cmake`<br>
Ubuntu / Linux: `sudo apt-get install cmake build-essential`<br/>

* Clang (optional)<br/>
http://clang.llvm.org/<br/>
Ubuntu / Linux: `sudo apt-get install clang`<br/>
`sudo update-alternatives --config c++` <choose clang>

* MongoDb C Driver<br/>
https://github.com/mongodb/mongo-c-driver
http://api.mongodb.com/c/current/<br/>
OSX: `brew install mongo-c`<br>
Ubuntu / Linux: `sudo apt-get install libmongoc-dev libbson-dev`

* Jansson<br/>
http://www.digip.org/jansson/
https://jansson.readthedocs.io/en/2.7/apiref.html<br/>
OSX: `brew install jansson`<br>
Ubuntu / Linux: `sudo apt-get install libjansson-dev`

* Libmicrohttpd<br/>
https://www.gnu.org/software/libmicrohttpd/<br/>
OSX: `brew install microhttpd`<br>
Ubuntu / Linux: `sudo apt-get install libmicrohttpd-dev`

Ubuntu / Linux:
`sudo apt-get install cmake build-essential clang-3.8 clang libmongoc-dev libbson-dev libmicrohttpd-dev `<br/>

####Build

`cmake .`<br/>
`make`


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