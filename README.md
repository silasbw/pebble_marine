# Pebble Marine

Pebble Marine provides useful instrumentation while sailing,
windsurfing, kayaking, paddle boarding, or boating. Your Pebble watch
displays speed (knots), bearing (degrees), current speed (knots),
current bearing (degrees), and tides (minutes to change).

Pebble Marine requires a bluetooth connection to a phone. It uses the
[CO OPS API](http://co-ops.nos.noaa.gov/api/) to get
current speed, current bearing, and tide predictions from the closest stations
less than 20 miles (32 kilometers) away.

## Main window

![alt tag](https://raw.githubusercontent.com/silasbw/pebble_marine/master/screenshot-main.png)

## Status window

Hold *select* to display a status window:

![alt tag](https://raw.githubusercontent.com/silasbw/pebble_marine/master/screenshot-status.png)

## Developement

Pebble Marine requires [Pebble SDK 3.X](http://developer.getpebble.com/sdk/download/).

`pebble build` requires [jshint](http://jshint.com/):

```sh
npm install jshint
```

Build, install, and check tail the logs:

```sh
export PEBBLE_PHONE=your phone's IP address
pebble build
pebble install
pebble logs
```
