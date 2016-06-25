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
>use upkids

>db.usercollection.find().pretty()
>db.usercollection.insert({ "username" : "testuser1", "email" : "testuser1@testdomain.com" })

>newstuff = [{ "username" : "testuser2", "email" : "testuser2@testdomain.com" }, { "username" : "testuser3", "email" : "testuser3@testdomain.com" }]
>db.usercollection.insert(newstuff);
>

Backup
======

mongodump -d prod -o prod_dump_thurs_oct8_2015/


Restore
======

mongorestore -d new_db_name mongo_db_dump/db_name


Shell
======
Start shell with command ```mongo```
Exit shell with command ```quit()```

show dbs
use geofence 
show collections
db.fences.find({})
db.gps_logs.find({},{_id:1, time_window:1})
db.gps_logs.remove({_id:ObjectId("")})

db.users.update({"user_name" : "wkamp"},{$set: {"hash": "SzudL+cYLrREGAvNVwZJdRX7OhP0BdhfVFpyc9ic+Nk="}})
db.users.update({"user_name" : "user"},{$set: {"organization": "Example"}})

db.getCollection('lessons').find({"html_content": {$regex : ".*Search Term*"}})
db.getCollection('lessons').find({"html_content": {$regex : "Search Term"}})

db.fs.files.find({},{filename:1})
db.fs.files.remove({_id:ObjectId("56131b73d4c6fe08421b9f3b")})
