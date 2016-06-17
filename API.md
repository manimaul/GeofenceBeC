###Api Endpoints

----

#### GET /fence_entry?i={identifier}

Response when found - 200

Response with corresponding log
```
{
  "message": "ok",
  "record": {
    "longitude": -122.123456,
    "identifier": "abc123",
    "latitude": 47.123455999999997,
    "entry_time": 1466017784,
    "radius": 625.10000000000002
  },
  "actual_entry_time": 0,
  "corresponding_log": []
}
```

OR Response without corresponding log

```
{
  "message": "ok",
  "record": {
    "longitude": -122.123456,
    "identifier": "abc123",
    "latitude": 47.123455999999997,
    "entry_time": 1466017784,
    "radius": 625.10000000000002
  },
  "actual_entry_time": 1466027923,
  "corresponding_log": [
    {
      "latitude": 47.000,
      "longitude": -122.000,
      "time": 1465967784
    },
    {
      "latitude": 47.123456,
      "longitude": -122.123456,
      "time": 1466027923
    }
  ]
}
```

Response when not found - 404
```
{
  "message": "Record not found"
}
```

----

#### POST /fence_entry

Body is required
```
{
  "identifier": "abc123",
  "latitude": 47.123456,
  "longitude": -122.123456,
  "entry_time": 1466017784,
  "radius": 625.1
}
```

Response when inserted - 200
```
{
  "message": "ok",
  "record": {
    "longitude": -122.123456,
    "identifier": "abc123",
    "latitude": 47.123455999999997,
    "entry_time": 1466017784,
    "radius": 625.10000000000002
  }
}
```