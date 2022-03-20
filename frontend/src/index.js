// import $ from 'npm:jquery';
(function (global) { 
var gauges = [];
  function update_ch(ch_id, title, val) {
    if (gauges[ch_id] !== undefined) {
      gauges[ch_id].set(val);
      return;
    }
    var amps = (ch_id == 1 ? 20 : 15);
    var obj_id = (ch_id == 1 ? "kitchen_power" : "house_power");
    var title = (ch_id == 1 ? "Kitchen/Bath" : "House");

    var max_power = (120*amps);
    var safe_power = (max_power*0.8);
    var critical_power = (max_power*1.10);

    var opts = {
      angle: -0.2, // The span of the gauge arc
      lineWidth: 0.2, // The line thickness
      radiusScale: 1, // Relative radius
      pointer: {
        length: 0.6, // // Relative to gauge radius
        strokeWidth: 0.035, // The thickness
        color: 'darkgray' // Fill color
      },
      limitMax: false,     // If false, max value increases automatically if value > maxValue
      limitMin: false,     // If true, the min value of the gauge will be fixed
      colorStart: '#6FADCF',   // Colors
      colorStop: '#8FC0DA',    // just experiment with them
      strokeColor: '#E0E0E0',  // to see which ones work best for you
      generateGradient: true,
      highDpiSupport: true,     // High resolution support
      staticLabels: {
        font: "10px sans-serif",  // Specifies font
        labels: [safe_power, max_power, critical_power],  // Print labels at these values
        color: "white",  // Optional: Label text color
        fractionDigits: 0  // Optional: Numerical precision. 0=round off.
      },
      staticZones: [
        {strokeStyle: "#555",     min: 0,           max: safe_power},
        {strokeStyle: "#990",     min: safe_power,  max: max_power},
        {strokeStyle: "crimson",  min: max_power,   max: critical_power}
      ],
    };
    var target = $('#'+obj_id); // your canvas element
    var o_title = $('<div />');
    var o_canvas = $('<canvas />');
    var o_value = $('<div />');
    o_title.addClass('title');
    o_canvas.addClass('canvas');
    o_value.addClass('value');
    target.append(o_title);
    target.append(o_canvas);
    target.append(o_value);

    o_title.text(title);

    var gauge = new Gauge(o_canvas[0]).setOptions(opts); // create sexy gauge!
    gauge.maxValue = critical_power; // set max gauge value
    gauge.setMinValue(0);  // Prefer setter over gauge.minValue = 0
    gauge.animationSpeed = 32; // set animation speed (32 is default value)
    gauge.set(val); // set actual value
    gauge.setTextField(o_value[0]);
    gauges[ch_id] = gauge;
  }

  function update_latest() {
    $.ajax('./api/latest.php').done((data) => {
      for (var name in data) {
        var ch = data[name];
        if ('name' in ch) {
          update_ch(ch.ch_id, ch.name, ch.power);
        }
      }
      $('#stale').css('opacity', 1.0);
      setTimeout(update_latest, 1000);
    });
  }

  function update_stale() {
    var stale = $('#stale');
    var o = stale.css('opacity');
    if (o > 0.25) {
      o = o * 0.75;
      stale.css('opacity', o);
    } else if (o < 0.25 && o != 0.0) {
      stale.css('opacity', 0.0);
    }
    setTimeout(update_stale, 100);
  }

  Promise.all([
    import("npm:gaugeJS").then((m) => {
      global.Gauge = m.Gauge;
      console.log("gaugeJS loaded...");
    }),
    import("npm:jquery").then((m) => {
      global.$ = m;
      console.log("jquery loaded...");
    }),
  ]).then(() => {
    update_latest();
    update_stale();
  });

})(typeof window !== "undefined" ? window : global);