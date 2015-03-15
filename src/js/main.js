if (typeof require !== 'undefined') {
  // If in nodejs, fake some dependencies.
  var Pebble = {'addEventListener': function() {}};
}

function copyCoordinate(coord) {
  return {'latitude': coord.latitude,
          'longitude': coord.longitude,
          'speed': coord.speed};
}

function updateWatch(gpsHistory, conditions)
{
  var message = {'knots': gpsHistory.meanSpeedKnots().toFixed(1),
                 'bearing': gpsHistory.meanBearingDegrees().toFixed(0),
                 'current_knots': conditions.current.speed.toFixed(1),
                 'current_bearing': conditions.current.direction.toFixed(0),
                 'tide_change': conditions.getTideString()};

  var transactionId = Pebble.sendAppMessage(
    message,
    function(e) {/* Success */},
    function(e) {
      console.log('Error delivering message: ' + e.error.message);
    }
  );
}

Pebble.addEventListener('ready',
    function(e) {
      console.log('Ready event listener');

      var gpsHistory = new GPSHistory(10);
      var conditions = new Conditions();
      var maxKilometersFromStation = 32.1869; // 20 miles
      var tidesStationId = null;
      var currentsStationId = null;

      function updateConditions() {
        var recordCount = gpsHistory.getRecordCount();
        if (recordCount >= 1) {
          var coordinate = gpsHistory.getRecord(recordCount - 1).coord;
          getStations(coordinate, 'tides', function(result, error) {
            if (result && result[0].distance < maxKilometersFromStation) {
              console.log('Tide station: ' + result[0].station.name);
              tidesStationId = result[0].station.id;
            } else {
              console.log('No tide stations');
              tidesStationId = null;
            }
          });
          getStations(coordinate, 'currents', function(result, error) {
            if (result && result[0].distance < maxKilometersFromStation) {
              console.log('Current station: ' + result[0].station.name);
              currentsStationId = result[0].station.id;
            } else {
              console.log('No current stations');
              currentsStationId = null;
            }
          });
        }

        if (currentsStationId) {
          CoOpsGet({product: 'currents',
                    units: 'english',
                    station: currentsStationId,
                    date: 'latest'},
                   function(result) {
                     conditions.setCurrents(result.data);
                   });
        }
        if (tidesStationId) {
          CoOpsGet({product: 'predictions',
                    units: 'english',
                    station: tidesStationId,
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
