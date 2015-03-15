GPSHistory = function(maxNumberOfPoints) {
  var records = [];
  var recordsCounter = 0;

  function getRecordCount() {
    return Math.min(recordsCounter, maxNumberOfPoints);
  }
  this.getRecordCount = getRecordCount;

  function getRecord(index) {
    var recordsIndex = 
        (recordsCounter - getRecordCount() + index) % maxNumberOfPoints;
    return records[recordsIndex];
  }
  this.getRecord = getRecord;

  this.saveCoordinate = function(coord, time_in_ms) {
    var points_index = recordsCounter % maxNumberOfPoints;
    records[points_index] = {'coord': coord, 'time_in_ms': time_in_ms};
    recordsCounter++;
  };

  this.sumDistancesInKM = function() {
    var distance = 0.0;
    for (var i = 0; i < getRecordCount() - 1; i++) {
      distance += calculateDistanceInKM(
          getRecord(i).coord, getRecord(i + 1).coord);
    }
    return distance;
  };

  this.sumTimeInMsec = function() {
    var total_time = 0.0;
    for (var i = 0; i < getRecordCount() - 1; i++) {
      total_time += getRecord(i + 1).time_in_ms - getRecord(i).time_in_ms;
    }
    return total_time;
  };

  this.meanSpeedKnots = function() {
    var totalMsecs = this.sumTimeInMsec();
    if (totalMsecs === 0.0) {
      return 0.0;
    }
    var kms = this.sumDistancesInKM();
    return kmsToNms(kms) / msecToHours(totalMsecs);
  };

  this.meanBearingDegrees = function() {
    var bearingSum = 0.0;
    for (var i = 0; i < getRecordCount() - 1; i++) {
      var msecElapsed = getRecord(i + 1).time_in_ms - getRecord(i).time_in_ms;
      var bearing = calculateBearing(
          getRecord(i).coord, getRecord(i + 1).coord);
      bearingSum += bearing;
    }
    if (getRecordCount() > 1) {
      return bearingSum / (getRecordCount() - 1);
    }
    return 0.0;
  };
};


function calculateDistanceInKM(coord0, coord1) {
  // http://en.wikipedia.org/wiki/Haversine_formula
  // http://www.movable-type.co.uk/scripts/latlong.html
  var earthRadiusInKM = 6371.0;
  var deltaLatitude = degreesToRadians(coord1.latitude - coord0.latitude);
  var deltaLongitude = degreesToRadians(coord1.longitude - coord0.longitude);
  var angleSquared =
    (Math.sin(deltaLatitude/2) * Math.sin(deltaLatitude/2)) +
    (Math.cos(degreesToRadians(coord0.latitude)) *
     Math.cos(degreesToRadians(coord1.latitude)) *
     Math.sin(deltaLongitude / 2) * Math.sin(deltaLongitude / 2));
  return 2 * earthRadiusInKM *
    Math.atan2(Math.sqrt(angleSquared), Math.sqrt(1 - angleSquared));
}


function calculateBearing(coord0, coord1) {
  // http://www.movable-type.co.uk/scripts/latlong.html
  var y = Math.sin(degreesToRadians(coord1.longitude - coord0.longitude)) *
    Math.cos(degreesToRadians(coord1.latitude));
  var x = (Math.cos(degreesToRadians(coord0.latitude)) *
           Math.sin(degreesToRadians(coord1.latitude))) -
    (Math.sin(degreesToRadians(coord0.latitude)) *
     Math.cos(degreesToRadians(coord1.latitude)) *
     Math.cos(degreesToRadians(coord1.longitude - coord0.longitude)));
  return (radiansToDegrees(Math.atan2(y, x)) + 360) % 360;
}


function degreesToRadians(degrees) {
  return degrees * (Math.PI/180);
}


function radiansToDegrees(radians) {
  return radians * (180/Math.PI);
}


function kmsToNms(kms) {
  return kms * 0.539957;
}


function msecToHours(msec) {
  return (msec / 1000.0) / (60.0 * 60.0);
}


if (typeof require !== 'undefined') {
  exports.calculateDistanceInKM = calculateDistanceInKM;
  exports.calculateBearing = calculateBearing;
  exports.GPSHistory = GPSHistory;
}
