function getStations(coordinate, stationType, callback) {
  var url = 'http://friscosailing.com/api/station_search?coordinate=';
  url += coordinate.latitude.toString() + ',' + coordinate.longitude.toString();
  url += '&type=' + stationType;
  var req = new XMLHttpRequest();
  req.open('GET', url);
  req.onload = function(e) {
    if (req.readyState === 4 && req.status === 200) {
      var result = JSON.parse(req.responseText).result;
      callback(result, null);
    } else if (error_cb) {
      callback(null, e);
    }
  };
  req.send(null);
}

if (typeof module !== 'undefined' && module.exports) {
  var XMLHttpRequest = require('xhr2');
  exports.getStations = getStations;
}
