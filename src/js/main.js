function copyCoordinate(coord) {
  return {'latitude': coord.latitude,
          'longitude': coord.longitude,
          'speed': coord.speed};
}

function updateWatch(gpsHistory, conditions)
{
  var record = gpsHistory.getRecordCount() ?
    gpsHistory.getRecord(gpsHistory.getRecordCount() - 1) : null;
  var latitude = record ? record.coord.latitude.toFixed(5) : '(Unknown)';
  var longitude = record ? record.coord.longitude.toFixed(5) : '(Unknown)';
  var tidesStationName = conditions.getTidesStationName() ?
    conditions.getTidesStationName() : '(Unknown)';
  var currentsStationName = conditions.getCurrentsStationName() ?
    conditions.getCurrentsStationName() : '(Unknown)';
  var message = {'knots': gpsHistory.meanSpeedKnots().toFixed(1),
                 'bearing': gpsHistory.meanBearingDegrees().toFixed(0),
                 'current_knots': conditions.current.speed.toFixed(1),
                 'current_bearing': conditions.current.direction.toFixed(0),
                 'tide_change': conditions.getTideString(),
                 'latitude': latitude,
                 'longitude': longitude,
                 'currents_station_name': currentsStationName,
                 'tide_station_name': tidesStationName};

  var transactionId = Pebble.sendAppMessage(
    message,
    function(e) {/* Success */},
    function(e) {
      if (e && e.error) {
        console.warn('Error delivering message:', e.error.message);
      } else {
        console.warn('Error delivering message');
      }
    }
  );
}

Pebble.addEventListener('ready',
    function(e) {
      console.log('Ready event listener');

      var gpsHistory = new GPSHistory(10);
      var conditions = new Conditions();
      var maxKilometersFromStation = 32.1869; // 20 miles

      function updateConditions() {
        var recordCount = gpsHistory.getRecordCount();
        if (recordCount >= 1) {
          var coordinate = gpsHistory.getRecord(recordCount - 1).coord;
          getStations(coordinate, 'tides', function(result, error) {
            if (result && result[0].distance < maxKilometersFromStation) {
              console.log('Tide station: ' + result[0].station.name);
              conditions.tidesStation = result[0].station;
            } else {
              console.log('No tide stations');
              conditions.tidesStation = null;
            }
          });
          getStations(coordinate, 'currents', function(result, error) {
            if (result && result[0].distance < maxKilometersFromStation) {
              console.log('Current station: ' + result[0].station.name);
              conditions.currentsStation = result[0].station;
            } else {
              console.log('No current stations');
              conditions.currentsStation = null;
            }
          });
        }

        if (conditions.getCurrentsStationId()) {
          CoOpsGet({product: 'currents',
                    units: 'english',
                    station: conditions.getCurrentsStationId(),
                    date: 'latest'},
                   function(result) {
                     conditions.setCurrents(result.data);
                   });
        }
        if (conditions.getTidesStationId()) {
          CoOpsGet({product: 'predictions',
                    units: 'english',
                    station: conditions.getTidesStationId(),
                    begin_date: CoOpsGmtime(),
                    range: 24},
                   function(result) {
                     conditions.setTide(result.predictions);
                   });
        }
      }

      // http://www.w3.org/TR/geolocation-API/
      // Request repeated updates.
      var watchId = navigator.geolocation.watchPosition(
        function (position) {
          var timestamp = position.timestamp;
          gpsHistory.saveCoordinate(copyCoordinate(position.coords), timestamp);
          if (gpsHistory.getRecordCount() === 1) {
            updateConditions();
          }
          updateWatch(gpsHistory, conditions);
        },
        function (error) {
          console.log('Error getting position: ' + error.message);
        },
        {'enableHighAccuracy': true});

      Pebble.addEventListener('appmessage', function(e) {
        updateConditions();
      });
    }
);

if (typeof module !== 'undefined' && module.exports) {
  var Pebble = {'addEventListener': function() {}};
}
