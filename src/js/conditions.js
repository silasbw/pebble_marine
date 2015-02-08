Conditions = function()
{
  this.current = {
    speed: 0,
    direction: 0
  };

  this.tide = {
    type: '',
    height: 0,
    date: null
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
        var type = initialDelta < 0 ? 'L' : 'H';
        this.tide.type = type;
        this.tide.height = parseFloat(predictions[index - 1].v);
        this.tide.date = new Date(predictions[index - 1].t + ' GMT');
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
