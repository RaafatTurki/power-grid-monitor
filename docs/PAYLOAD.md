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
g:id,lon,lat

types:
g:uint,float,float

example:
g:42,33.123,44.123
```
providing `0` for both `lon` and `lat` indicates that the given power stations location is unimportant, private or unknown

#### Data
`Arduino -> Server`

This method is used to register the voltage, current and temprature values of any given power station
```
schema:
d:id,state,voltage,current,temp

types:
d:uint,uint,float,float,float

example:
d:42,1,1,2.1,3.3
```
The `state` parameter can either be a `0` (off) or a `1` (on)

#### Power
`Server -> Arduino`

This method is used to control the power state of any given station
```
schema:
p:id,state

types:
p:uint,uint

example:
p:42,1
```
The `state` parameter can either be a `0` (off) or a `1` (on)

## Server <-> Frontend
TBA
