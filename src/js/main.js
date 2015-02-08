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
      console.log('[ready event listener]');

      var gpsHistory = new GPSHistory(10);
      var conditions = new Conditions();

      // http://www.w3.org/TR/geolocation-API/
      // Request repeated updates.
      var watchId = navigator.geolocation.watchPosition(
        function (position) {
          var timestamp = position.timestamp;
          gpsHistory.saveCoordinate(copyCoordinate(position.coords), timestamp);
          updateWatch(gpsHistory, conditions);
        },
        function (error) {
          console.log('Error getting position: ' + error.message);
        },
        {'enableHighAccuracy': true});

      Pebble.addEventListener('appmessage', function(e) {
        // CO-OPS conditions.
        // SF Bay stations:
        //   http://tidesandcurrents.noaa.gov/stations.html?type=Water+Levels
        //   http://tidesandcurrents.noaa.gov/cdata/StationList?type=Current+Data&filter=active
        CoOpsGet({product: 'currents',
            units: 'english',
            station: 's08010',
            date: 'latest'},
          function(result) {
            conditions.setCurrents(result.data);
          });
        CoOpsGet({product: 'predictions',
            units: 'english',
            station: 9414863,
            begin_date: CoOpsGmtime(),
            range: 24},
          function(result) {
            conditions.setTide(result.predictions);
          });
      });
    }
);
