Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready!");

  Pebble.sendAppMessage({ JSReady: 1 });
});

const api_key = "";

function getRailLines() {
  var method = "GET";
  var url = "https://api.wmata.com/Rail.svc/json/jLines";
  var request = new XMLHttpRequest();

  request.onload = function () {
    console.log("Response: ");
    console.log(this.responseText);
  };

  request.open(method, url);
  request.setRequestHeader("api_key", api_key);
  request.send();
}

function getRailStations() {
  var method = "GET";
  var url = "https://api.wmata.com/Rail.svc/json/jStations";
  var request = new XMLHttpRequest();

  request.onload = function () {
    console.log("Response: ");
    console.log(this.responseText);
  };

  request.open(method, url);
  request.setRequestHeader("api_key", api_key);
  request.send();
}

function getRailTime(station) {
  var method = "GET";
  var url = "https://api.wmata.com/Rail.svc/json/jStationTimes";
  var request = new XMLHttpRequest();
  var params = "StationCode=" + station;
  request.onload = function () {
    console.log("Response: ");
    console.log(this.responseText);
  };

  request.open(method, url + "?" + params);
  request.setRequestHeader("api_key", api_key);
  request.send();
}

function nextTrain(station) {
  var method = "GET";
  var url =
    "http://api.wmata.com/StationPrediction.svc/json/GetPrediction/" + station;
  var request = new XMLHttpRequest();
  request.onload = function () {
    console.log("Response: ");
    console.log(this.responseText);
  };

  request.open(method, url);
  request.setRequestHeader("api_key", api_key);
  request.send();
}
