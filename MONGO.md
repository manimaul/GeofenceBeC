Mongodb installation
=====

brew install mongodb

Startup mongo
=====

mkdir ~/code/nordstrom/GeoFenceBeC/mongodb_data/

mongod --dbpath ~/code/nordstrom/GeoFenceBeC/mongodb_data/

Mongodb console
=====

$mongo
>`use geofence`
>`db.getCollectionNames()`
>`db.fences.find().pretty()`
>`db.fences.insert({ "key" : "value" })`

>`new_stuff = [{ "key" : "value", "qt1" : 1 }, { "key" : "value", "qty" : 2 }]`
>`db.usercollection.insert(new_stuff);`

Backup
======

mongodump -d geofence -o geofence_dump/


Restore
======

mongorestore -d geofence geofence_dump/geofence

Shell
======
Start shell with command ```mongo```
Exit shell with command ```quit()```

`show dbs`</br>
`use geofence` </br>
`show collections`</br>
`db.fences.find({})`</br>
`db.gps_logs.find({},{_id:1, time_window:1})`</br>
`db.gps_logs.remove({_id:ObjectId("577483ad421aa94fa02cc316")})`</br>
`db.users.update({"key" : "value"},{$set: {"qty": 1}})`</br>
`db.getCollection('lessons').find({"html_content": {$regex : ".*Search Term*"}})`</br>
`db.getCollection('lessons').find({"html_content": {$regex : "Search Term"}})`</br>
`db.fs.files.find({},{filename:1})`</br>
`db.fs.files.remove({_id:ObjectId("56131b73d4c6fe08421b9f3b")})`</br>
