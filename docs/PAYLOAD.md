# Payload Schema

## Server <-> Arduino

WS Payloads are byte arrays, our payloads will be of the shape
```
METHOD:VALUE,VALUE,VALUE,...
```
Where `METHOD` is a single char indicating the requested method and `VALUE` being any basic non-nested data type.


#### Geo-Location
`Arduino -> Server`

This method is used to register/update the longitude and latitude of any given power station
```
schema:
geo:id,lon,lat

types:
geo:uint,float,float

example:
geo:42,33.123,44.123
```
providing `0` for both `lon` and `lat` indicates that the given power stations location is unimportant, private or unknown

#### Data
`Arduino -> Server`

This method is used to register the voltage, current and temprature values of any given power station
```
schema:
dat:id,state,voltage,current,temp

types:
dat:uint,uint,float,float,float

example:
dat:42,1,1,2.1,3.3
```
The `state` parameter can either be a `0` (off) or a `1` (on)

#### Power
`Server -> Arduino`

This method is used to control the power state of any given station
```
schema:
pow:id,state

types:
pow:uint,uint

example:
pow:42,1
```
The `state` parameter can either be a `0` (off) or a `1` (on)

## Server <-> Frontend

#### Register
`Frontend -> Server`

This method is used to register frontend for new feed data
```
schema:
reg:
```

`Frontend -> Server`

This method is used to unregister frontend for new feed data
```
schema:
unreg:
```

#### Feed
`Server -> Frontend`

This method is used to feed new station data to the frontend
```
schema:
feed:id,state,voltage,current,temp,lon,lat

types:
feed:uint,uint,float,float,float,float,float

example:
feed:42,1,1.0,2.0,3.0,33.0,44.0
```
