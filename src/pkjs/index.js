localStorage.setItem(
  "favorite_stations",
  JSON.stringify(["Dupont Circle", "Foggy Bottom", "Metro Center"])
);

var keys = require("message_keys");

function sendNextItem(items, index, key) {
  // Build message
  var temp_key = key + index;
  var dict = {};
  dict[temp_key] = items[index];

  // Send the message
  Pebble.sendAppMessage(
    dict,
    function () {
      // Use success callback to increment index
      index++;

      if (index < items.length) {
        // Send next item
        sendNextItem(items, index, key);
      } else {
        console.log("Last item sent!");
      }
    },
    function () {
      console.log("Item transmission failed at index: " + index);
    }
  );
}

function sendList(items, key) {
  var index = 0;
  console.log(key);
  sendNextItem(items, index, keys[key]);
}

Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready!");
  Pebble.sendAppMessage({ JSReady: 1 });

  // let stations = getRailStations();

  // Pebble.sendAppMessage({StationsLen: stations.length})
  // sendList(stations, "Stations");

  const favoriteStations = JSON.parse(
    localStorage.getItem("favorite_stations")
  );

  Pebble.sendAppMessage({ FavoriteStationsLen: favoriteStations.length });
  sendList(favoriteStations, "FavoriteStations");
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
    let stations = JSON.parse(this.responseText);
    stations = stations.Stations.map((station) => {
      return [station.Code, station.Name];
    });

    localStorage.setItem("stations", JSON.stringify(stations));
    return stations;
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
    let r = JSON.parse(this.responseText);
    console.log(this.responseText);
    let trainResponse = [];
    let length = 0;
    for (let i = 0; i < r.Trains.length && length < 50; i++) {
      let train = r.Trains[i];

      if (train.Min === "ARR") {
        train.Min = "Arriving";
      } else if (train.Min === "BRD") {
        train.Min = "Boarding";
      } else if (train.Min === "1") {
        train.Min = "1min";
      } else {
        train.Min = train.Min + "mins";
      }

      let trainString =
        train.Line + " " + train.DestinationName + " " + train.Min;
      trainResponse.push(trainString);
      length += trainString.length;
    }
    console.log(trainResponse);
    let bytes = stringToBytes(trainResponse.join("\n"));
    Pebble.sendAppMessage({ TrainResponse: [...bytes, 0] });
  };

  request.open(method, url);
  request.setRequestHeader("api_key", api_key);
  request.send();
}

function stringToBytes(val) {
  const result = [];
  for (let i = 0; i < val.length; i++) {
    result.push(val.charCodeAt(i));
  }
  return result;
}
