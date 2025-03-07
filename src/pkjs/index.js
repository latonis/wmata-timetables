localStorage.setItem(
  "favorite_stations",
  JSON.stringify(["Dupont Circle", "Foggy Bottom", "Metro Center"])
);

var keys = require("message_keys");

function sendNextItem(items, index) {
  // Build message
  var key = keys.FavoriteStations + index;
  var dict = {};
  dict[key] = items[index];

  // Send the message
  Pebble.sendAppMessage(
    dict,
    function () {
      // Use success callback to increment index
      index++;

      if (index < items.length) {
        // Send next item
        sendNextItem(items, index);
      } else {
        console.log("Last item sent!");
      }
    },
    function () {
      console.log("Item transmission failed at index: " + index);
    }
  );
}

function sendList(items) {
  var index = 0;
  sendNextItem(items, index, keys.FavoriteStations);
}

Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready!");
  Pebble.sendAppMessage({ JSReady: 1 });
  const favoriteStations = JSON.parse(
    localStorage.getItem("favorite_stations")
  );
  Pebble.sendAppMessage({ FavoriteStationsLen: favoriteStations.length });
  sendList(favoriteStations);
});

Pebble.addEventListener("appmessage", function (e) {
  var dict = e.payload;

  console.log("Got message: " + JSON.stringify(dict));

  if (dict["TrainRequest"]) {
    console.log("TrainRequest: " + dict["TrainRequest"]);
    nextTrain(dict["TrainRequest"]);
  }
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
