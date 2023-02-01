# Payload Schema

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
`Frontend -> Server`

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

#### Register
`Frontend -> Server`
`Arduino -> Server`

This method is used to register clients/stations
```
schema:
reg:type

types:
reg:char

example:
reg:c
```

`Frontend -> Server`
`Arduino -> Server`

This method is used to unregister clients/stations
```
schema:
unreg:type

types:
unreg:char

example:
unreg:c
```

#### Regen

`Frontend -> Server`
`Server -> Frontend`

This method is used to tell the server to regen the data file
```
schema:
regen:

example:
regen:
```
When the server uses this method, a single argument of `done` is used
