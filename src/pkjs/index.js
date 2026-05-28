// jpeg-js uses Buffer to allocate its output array; shim it with Uint8Array
if (typeof Buffer === 'undefined') {
  Buffer = function(size) { return new Uint8Array(size); };
}

var Clay = require('@rebble/clay');
var clayConfig = require('./config');
new Clay(clayConfig);

var jpegjs = require('jpeg-js');

const POLL_INTERVAL_MS = 10000;
const CAMERA_W = 120;
const CAMERA_H = 120;
const CHUNK_SIZE = 800;

var pollTimer = null;

function getMoonrakerUrl() {
  var settings = localStorage.getItem('clay-settings');
  if (settings) {
    try {
      var parsed = JSON.parse(settings);
      if (parsed.MoonrakerUrl) {
        return parsed.MoonrakerUrl;
      }
    } catch (e) {}
  }
  return '';
}

function fetchPrinterStatus() {
  var url =
    getMoonrakerUrl() +
    '/printer/objects/query' +
    '?extruder=temperature,target' +
    '&heater_bed=temperature,target' +
    '&print_stats=state,filename,print_duration' +
    '&virtual_sdcard=progress';

  var req = new XMLHttpRequest();
  req.onload = function () {
    try {
      var json = JSON.parse(this.responseText);
      var status = json.result.status;

      var nozzleTemp = Math.round(status.extruder.temperature);
      var nozzleTarget = Math.round(status.extruder.target);
      var bedTemp = Math.round(status.heater_bed.temperature);
      var bedTarget = Math.round(status.heater_bed.target);

      var printState = status.print_stats.state || 'unknown';
      var progress = Math.round(status.virtual_sdcard.progress * 100);

      var printDuration = status.print_stats.print_duration || 0;
      var timeLeft = 0;
      if (progress > 0 && printState === 'printing') {
        var totalEstimate = printDuration / (progress / 100);
        timeLeft = Math.round(totalEstimate - printDuration);
      }

      Pebble.sendAppMessage(
        {
          NozzleTemp: nozzleTemp,
          NozzleTarget: nozzleTarget,
          BedTemp: bedTemp,
          BedTarget: bedTarget,
          PrintState: printState,
          PrintProgress: progress,
          PrintTimeLeft: timeLeft,
        },
        function () {
          console.log('Status sent to watch');
        },
        function (e) {
          console.log('Send failed: ' + JSON.stringify(e));
        }
      );
    } catch (err) {
      console.log('Error parsing Moonraker response: ' + err.message);
    }
  };

  req.onerror = function () {
    console.log('XHR error - is Moonraker reachable?');
    Pebble.sendAppMessage({ PrintState: 'error' });
  };

  req.open('GET', url);
  req.send();
}

function rgbToGColor8(r, g, b) {
  var r2 = Math.min(3, Math.round(r / 85));
  var g2 = Math.min(3, Math.round(g / 85));
  var b2 = Math.min(3, Math.round(b / 85));
  return 0xc0 | (r2 << 4) | (g2 << 2) | b2;
}

function sendCameraChunks(pixels, total, index) {
  if (index >= total) {
    console.log('Camera transfer complete');
    return;
  }
  var start = index * CHUNK_SIZE;
  var chunk = Array.prototype.slice.call(pixels, start, start + CHUNK_SIZE);
  Pebble.sendAppMessage(
    {
      CameraChunkData: chunk,
      CameraChunkIndex: index,
      CameraChunkTotal: total,
    },
    function () {
      sendCameraChunks(pixels, total, index + 1);
    },
    function (e) {
      console.log('Chunk ' + index + ' failed: ' + JSON.stringify(e));
    }
  );
}

function fetchCameraSnapshot() {
  var url = getMoonrakerUrl() + '/webcam/?action=snapshot';
  console.log('Fetching camera snapshot from ' + url);

  var req = new XMLHttpRequest();
  req.responseType = 'arraybuffer';

  req.onload = function () {
    try {
      var raw = new Uint8Array(this.response);
      var decoded = jpegjs.decode(raw, { useTArray: true });

      var pixels = new Uint8Array(CAMERA_W * CAMERA_H);
      for (var y = 0; y < CAMERA_H; y++) {
        for (var x = 0; x < CAMERA_W; x++) {
          var srcX = Math.floor((x * decoded.width) / CAMERA_W);
          var srcY = Math.floor((y * decoded.height) / CAMERA_H);
          var idx = (srcY * decoded.width + srcX) * 4;
          pixels[y * CAMERA_W + x] = rgbToGColor8(
            decoded.data[idx],
            decoded.data[idx + 1],
            decoded.data[idx + 2]
          );
        }
      }

      var total = Math.ceil(pixels.length / CHUNK_SIZE);
      console.log('Sending camera in ' + total + ' chunks');
      sendCameraChunks(pixels, total, 0);
    } catch (e) {
      console.log('Camera decode error: ' + e.message);
    }
  };

  req.onerror = function () {
    console.log('Camera fetch failed');
  };

  req.open('GET', url);
  req.send();
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
  console.log('PebbleKit JS ready - starting Klipper Pebble');
  startPolling();
});

Pebble.addEventListener('appmessage', function (e) {
  var dict = e.payload;
  if (dict['RequestUpdate']) {
    console.log('Manual refresh requested');
    fetchPrinterStatus();
  }
  if (dict['CameraRequest']) {
    fetchCameraSnapshot();
  }
});
