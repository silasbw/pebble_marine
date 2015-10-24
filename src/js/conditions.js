Conditions = function()
{
  this.currentsStation = null;
  this.tidesStation = null;

  this.current = {
    speed: 0,
    direction: 0
  };

  this.tide = {
    type: '',
    height: 0,
    date: null
  };

  this.getCurrentsStationId = function() {
    if (this.currentsStation) {
      return this.currentsStation.id;
    }
    return null;
  };

  this.getCurrentsStationName = function() {
    if (this.currentsStation) {
      return this.currentsStation.name;
    }
    return null;
  };

  this.getTidesStationId = function() {
    if (this.tidesStation) {
      return this.tidesStation.id;
    }
    return null;
  };

  this.getTidesStationName = function() {
    if (this.tidesStation) {
      return this.tidesStation.name;
    }
    return null;
  };

  this.getTideString = function()
  {
    if (!this.tide.date)
    {
      return '';
    }
    var msecondsToChange = this.tide.date - Date.now();
    var minutesToChange = msecondsToChange / 1000 / 60;
    tideChange =  this.tide.type + minutesToChange.toFixed(0);
    return tideChange;
  };

  this.setTide = function(predictions)
  {
    if (predictions.length < 2)
    {
      return;
    }

    var initialDelta =
      parseFloat(predictions[0].v) - parseFloat(predictions[1].v);
    for (var index = 2; index < predictions.length; index++) {
      var delta = parseFloat(predictions[index - 1].v) -
        parseFloat(predictions[index].v);
      // Does the slope change signs?
      if (initialDelta * delta < 0) {
        var type = initialDelta < 0 ? 'H' : 'L';
        this.tide.type = type;
        this.tide.height = parseFloat(predictions[index - 1].v);
        // CoOps gives us: YYY-MM-DD HH:mm. We need to convert it to the
        // simplified ISO, which is supported by most JS implementations.
        // http://www.ecma-international.org/ecma-262/5.1/#sec-15.9.1.15
        var simplified_iso = predictions[index - 1].t.replace(' ', 'T') + 'Z';
        this.tide.date = new Date(simplified_iso);
        return;
      }
    }
  };

  this.setCurrents = function(currents)
  {
    this.current.speed = parseFloat(currents[0].s);
    this.current.direction = parseFloat(currents[0].d);
  };
};
