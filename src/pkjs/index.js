var Clay = require('@rebble/clay');
var clayConfig = require('./config');
new Clay(clayConfig);

const POLL_INTERVAL_MS = 10000;
const REQUEST_TIMEOUT_MS = 5000;

const CONN_OK = 1;
const CONN_ERROR = 2;
const CONN_UNREACHABLE = 3;
const CONN_NOT_CONFIGURED = 4;

var PRINT_UNKNOWN = 0;
var PRINT_STATE_MAP = {
  standby: 1,
  printing: 2,
  paused: 3,
  complete: 4,
  cancelled: 5,
  error: 6,
};

var pollTimer = null;

function getMoonrakerUrl() {
  var settings = localStorage.getItem('clay-settings');
  if (settings) {
    try {
      var settingsJson = JSON.parse(settings);
      if (settingsJson.MoonrakerUrl) {
        return settingsJson.MoonrakerUrl;
      }
    } catch (e) {}
  }
  return '';
}

function fetchPrinterStatus() {
  var base = getMoonrakerUrl();
  if (!base) {
    sendToWatch({ ConnectionState: CONN_NOT_CONFIGURED });
    stopPolling();
    return;
  }

  var url =
    base +
    '/printer/objects/query' +
    '?extruder=temperature,target' +
    '&heater_bed=temperature,target' +
    '&print_stats=state,filename,print_duration' +
    '&virtual_sdcard=progress';

  var req = new XMLHttpRequest();

  req.onload = function () {
    if (this.status !== 200) {
      sendToWatch({ ConnectionState: CONN_ERROR });
      return;
    }
    try {
      var status = JSON.parse(this.responseText).result.status;

      var progress = Math.round(status.virtual_sdcard.progress * 100);
      var printState = toPrintState(status.print_stats.state);
      var printDuration = status.print_stats.print_duration || 0;

      var timeLeft = 0;
      if (progress > 0 && printState === PRINT_STATE_MAP.printing) {
        timeLeft = Math.round(printDuration / (progress / 100) - printDuration);
      }

      var dict = {
        NozzleTemp: Math.round(status.extruder.temperature),
        NozzleTarget: Math.round(status.extruder.target),
        BedTemp: Math.round(status.heater_bed.temperature),
        BedTarget: Math.round(status.heater_bed.target),
        PrintState: printState,
        PrintProgress: progress,
        PrintTimeLeft: timeLeft,
        ConnectionState: CONN_OK,
      };

      sendToWatch(dict);
    } catch (err) {
      console.log('Error parsing Moonraker response: ' + err.message);
      console.log(err.stack);
      sendToWatch({ ConnectionState: CONN_ERROR });
    }
  };

  function unreachable() {
    console.log('Moonraker is unreachable');
    sendToWatch({ ConnectionState: CONN_UNREACHABLE });
  }

  req.onerror = unreachable;
  req.ontimeout = unreachable;

  req.open('GET', url);
  req.timeout = REQUEST_TIMEOUT_MS;
  req.send();
}

function sendToWatch(dict) {
  Pebble.sendAppMessage(
    dict,
    function () {
      console.log('Sent: ' + JSON.stringify(dict));
    },
    function (e) {
      console.log('Send failed: ' + JSON.stringify(e));
    }
  );
}

function toPrintState(state) {
  var s = PRINT_STATE_MAP[state];
  return s === undefined ? PRINT_UNKNOWN : s;
}

function startPolling() {
  fetchPrinterStatus();
  pollTimer = setInterval(fetchPrinterStatus, POLL_INTERVAL_MS);
}

function stopPolling() {
  if (pollTimer) {
    clearInterval(pollTimer);
    pollTimer = null;
  }
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready - starting Klipper Monitor');
  startPolling();
});

Pebble.addEventListener('appmessage', function (e) {
  var dict = e.payload;
  if (dict['RequestUpdate']) {
    console.log('Manual refresh requested');
    fetchPrinterStatus();
  }
});

Pebble.addEventListener('webviewclosed', function () {
  console.log('Config window closed - restarting polling');
  stopPolling();
  startPolling();
});
