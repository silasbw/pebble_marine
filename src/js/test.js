var GPS = require('./gps')


// The distance between northern tip of Emeryville Marina Park and
// southern tip of Berkeley Shorebird Park.
var emeryvilleCoord = {'latitude': 37.8425204, 'longitude': -122.314369};
var berkeleyCoord = {'latitude': 37.858968, 'longitude': -122.316289};
var googleMapsDistanceKM = 1.835;
// http://www.movable-type.co.uk/scripts/latlong.html
var mtlBearing = 354.7333;


exports.testCalculateDistanceBetweenPoints = function(test) {
  var distance = GPS.calculateDistanceInKM(emeryvilleCoord, berkeleyCoord);
  var difference = Math.abs(distance - googleMapsDistanceKM);
  test.ok(difference < 0.01, 'Difference too high');
  test.done();
}

exports.testGPSDistanceAndTimeCalculation = function(test) {
  var maxNumberOfRecords = 5;
  var numberOfMilliseconds = 1000;
  var gpsHistory = new GPS.GPSHistory(maxNumberOfRecords);
  var coordinates = [emeryvilleCoord, berkeleyCoord]
  for (var i = 0; i < maxNumberOfRecords; i++) {
    gpsHistory.saveCoordinate(coordinates[i % 2], i*numberOfMilliseconds);
  }

  var distance = gpsHistory.sumDistancesInKM()
  var difference = distance - ((maxNumberOfRecords - 1) * googleMapsDistanceKM);
  test.ok(difference < 0.01, 'Difference too high');

  var milliseconds = gpsHistory.sumTimeInMsec();
  test.strictEqual(milliseconds, numberOfMilliseconds * (maxNumberOfRecords - 1));
  test.done();

  var knots = gpsHistory.meanSpeedKnots();
  var hours = numberOfMilliseconds * (maxNumberOfRecords - 1) / 1000.0 / (60 * 60);
  var googleMapsSpeed = 0.539957 * googleMapsDistanceKM * (maxNumberOfRecords - 1) / hours;
  difference = Math.abs(knots - googleMapsSpeed);
  test.ok(difference / googleMapsSpeed < 0.01, 'Difference too high');
}

exports.testGPSDistanceAndTimeCalculationFewRecords = function(test) {
  var maxNumberOfRecords = 5;
  var numberOfMilliseconds = 1000;
  var gpsHistory = new GPS.GPSHistory(maxNumberOfRecords);
  var coordinates = [emeryvilleCoord, berkeleyCoord]

  // One record
  gpsHistory.saveCoordinate(coordinates[0], 0);
  test.strictEqual(gpsHistory.sumDistancesInKM(), 0.0);
  test.strictEqual(gpsHistory.sumTimeInMsec(), 0.0);

  // Two records
  gpsHistory.saveCoordinate(coordinates[1], numberOfMilliseconds);
  var difference = Math.abs(
      googleMapsDistanceKM - gpsHistory.sumDistancesInKM());
  test.ok(difference < 0.01, 'Difference too high');
  test.strictEqual(gpsHistory.sumTimeInMsec(), numberOfMilliseconds);
  test.done();
}

exports.testGPSDistanceAndTimeCalculationManyRecords = function(test) {
  var maxNumberOfRecords = 5;
  var numberOfRecords = 100;
  var numberOfMilliseconds = 1000;
  var gpsHistory = new GPS.GPSHistory(maxNumberOfRecords);
  var coordinates = [emeryvilleCoord, berkeleyCoord]

  for (var i = 0; i < numberOfRecords; i++) {
    gpsHistory.saveCoordinate(coordinates[i % 2], i*numberOfMilliseconds);
  }

  var distance = gpsHistory.sumDistancesInKM()
  var difference = distance - ((maxNumberOfRecords - 1) * googleMapsDistanceKM);
  test.ok(difference < 0.01, 'Difference too high');

  var milliseconds = gpsHistory.sumTimeInMsec()
  test.strictEqual(milliseconds, numberOfMilliseconds * (maxNumberOfRecords - 1));
  test.done();
}

exports.testCalculateBearingBetweenTwoPoints = function(test) {
  var bearing = GPS.calculateBearing(emeryvilleCoord, berkeleyCoord);
  var difference = Math.abs(bearing - mtlBearing);
  test.ok(difference < 0.01, 'Difference too high');
  test.done();
}
