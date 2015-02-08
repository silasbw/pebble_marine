if (typeof require !== 'undefined') {
  // If in nodejs, pull in some dependencies.
  var XMLHttpRequest = require('xhr2');
}


function DateToCoOpsGmtime(date) {
  var addPadding = function(str, requiredLength) {
    while (str.length < requiredLength) {
      str = "0" + str;
    }
    return str;
  };
  
  return date.getUTCFullYear().toString() +
    addPadding((date.getUTCMonth() + 1).toString(), 2) +
    addPadding(date.getUTCDate().toString(), 2) + ' ' +
    addPadding(date.getUTCHours().toString(), 2) + ':' +
    addPadding(date.getUTCMinutes().toString(), 2);
}


function CoOpsGmtime() {
  return DateToCoOpsGmtime(new Date());
}


function CoOpsGet(params, success_cb, error_cb) {
  var waterLevelProducts = [
    'water_level',
    'hourly_height',
    'high_low',
    'daily_mean',
    'monthly_mean',
    'one_minute_water_level',
    'predictions'
  ];

  var request_params = {
    format: 'json',
    time_zone: 'gmt',
    units: 'metric',
    application: 'co-ops-js',
  };

  for (var attribute in params) {
    request_params[attribute] = params[attribute];
  }

  if ((waterLevelProducts.indexOf(request_params.product) !== -1) &&
      request_params.datum === undefined)
  {
    request_params.datum = 'MLLW';
  }

  if (request_params.begin_date === undefined &&
      request_params.end_date === undefined &&
      request_params.date === undefined &&
      request_params.range === undefined)
  {
    request_params.date = 'latest';
  }

  var paramsStrings = [];
  for (var paramKey in request_params) {
    if (request_params.hasOwnProperty(paramKey)) {
      paramsStrings.push(paramKey + '=' + request_params[paramKey]);
    }
  }
  var url = 'http://tidesandcurrents.noaa.gov/api/datagetter?' +
    paramsStrings.join('&');

  var req = new XMLHttpRequest();
  req.open('GET', url);
  req.onload = function(e) {
    if (req.readyState === 4 && req.status === 200) {
      var response = JSON.parse(req.responseText);
      success_cb(response);
    } else if (error_cb) {
      error_cb(req.readyState, req.status);
    }
  };
  req.send(null);
}


if (typeof require !== 'undefined') {
  exports.get = CoOpsGet;
  exports.gmtime = CoOpsGmtime;
  exports.gmtDateFormat = DateToCoOpsGmtime;
}
