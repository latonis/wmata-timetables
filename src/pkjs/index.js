var keys = require("message_keys");

const stationMap = {
  "Metro Center": "A01",
  "Farragut North": "A02",
  "Dupont Circle": "A03",
  "Woodley Park-Zoo/Adams Morgan": "A04",
  "Cleveland Park": "A05",
  "Van Ness-UDC": "A06",
  "Tenleytown-AU": "A07",
  "Friendship Heights": "A08",
  "Bethesda": "A09",
  "Medical Center": "A10",
  "Grosvenor-Strathmore": "A11",
  "North Bethesda": "A12",
  "Twinbrook": "A13",
  "Rockville": "A14",
  "Shady Grove": "A15",
  "Gallery Pl-Chinatown": "B01",
  "Judiciary Square": "B02",
  "Union Station": "B03",
  "Rhode Island Ave-Brentwood": "B04",
  "Brookland-CUA": "B05",
  "Fort Totten": "B06",
  "Takoma": "B07",
  "Silver Spring": "B08",
  "Forest Glen": "B09",
  "Wheaton": "B10",
  "Glenmont": "B11",
  "NoMa-Gallaudet U": "B35",
  "McPherson Square": "C02",
  "Farragut West": "C03",
  "Foggy Bottom-GWU": "C04",
  "Rosslyn": "C05",
  "Arlington Cemetery": "C06",
  "Pentagon": "C07",
  "Pentagon City": "C08",
  "Crystal City": "C09",
  "Ronald Reagan Washington National Airport": "C10",
  "Potomac Yard": "C11",
  "Braddock Road": "C12",
  "King St-Old Town": "C13",
  "Eisenhower Avenue": "C14",
  "Huntington": "C15",
  "Federal Triangle": "D01",
  "Smithsonian": "D02",
  "L'Enfant Plaza": "D03",
  "Federal Center SW": "D04",
  "Capitol South": "D05",
  "Eastern Market": "D06",
  "Potomac Ave": "D07",
  "Stadium-Armory": "D08",
  "Minnesota Ave": "D09",
  "Deanwood": "D10",
  "Cheverly": "D11",
  "Landover": "D12",
  "New Carrollton": "D13",
  "Mt Vernon Sq 7th St-Convention Center": "E01",
  "Shaw-Howard U": "E02",
  "U Street/African-Amer Civil War Memorial/Cardozo": "E03",
  "Columbia Heights": "E04",
  "Georgia Ave-Petworth": "E05",
  "West Hyattsville": "E07",
  "Hyattsville Crossing": "E08",
  "College Park-U of Md": "E09",
  "Greenbelt": "E10",
  "Waterfront": "F04",
  "Navy Yard-Ballpark": "F05",
  "Anacostia": "F06",
  "Congress Heights": "F07",
  "Southern Avenue": "F08",
  "Naylor Road": "F09",
  "Suitland": "F10",
  "Branch Ave": "F11",
  "Benning Road": "G01",
  "Capitol Heights": "G02",
  "Addison Road-Seat Pleasant": "G03",
  "Morgan Boulevard": "G04",
  "Downtown Largo": "G05",
  "Van Dorn Street": "J02",
  "Franconia-Springfield": "J03",
  "Court House": "K01",
  "Clarendon": "K02",
  "Virginia Square-GMU": "K03",
  "Ballston-MU": "K04",
  "East Falls Church": "K05",
  "West Falls Church": "K06",
  "Dunn Loring-Merrifield": "K07",
  "Vienna/Fairfax-GMU": "K08",
  "McLean": "N01",
  "Tysons": "N02",
  "Greensboro": "N03",
  "Spring Hill": "N04",
  "Wiehle-Reston East": "N06",
  "Reston Town Center": "N07",
  "Herndon": "N08",
  "Innovation Center": "N09",
  "Washington Dulles International Airport": "N10",
  "Loudoun Gateway": "N11",
  "Ashburn": "N12"
};

const MAX_CHARS_STATION_RESULT = 128;

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

  let favorite_stations = JSON.parse(localStorage.getItem("favorite_stations"));

  if (favorite_stations == null) {
    favorite_stations = [];
  }

  let tmp_bytes = stringToBytes(favorite_stations.join("|"));
  tmp_bytes.push(0)

  Pebble.sendAppMessage({ Favorites: tmp_bytes });

  let stations = Object.keys(stationMap)
  Pebble.sendAppMessage({ StationsLen: stations.length });
  sendList(stations, "Stations");
});

Pebble.addEventListener("appmessage", function (e) {
  var dict = e.payload;

  console.log("Got message: " + JSON.stringify(dict));

  if (dict["TrainRequest"]) {
    console.log("TrainRequest: " + dict["TrainRequest"]);
    nextTrain(getCodeFromName(dict["TrainRequest"]));
  } else if (dict["AddFavorite"]) {
    console.log("AddFavorite" + dict["AddFavorite"])
    let favorite_stations = JSON.parse(localStorage.getItem("favorite_stations"));
    if (favorite_stations == null) {
      favorite_stations = [];
    }
    if (!favorite_stations.includes(dict["AddFavorite"])) {
      favorite_stations.push(dict["AddFavorite"]);
      localStorage.setItem("favorite_stations", JSON.stringify(favorite_stations));
      console.log(favorite_stations.join("|"))
    }
  } else if (dict["RemoveFavorite"]) {
    console.log("RemoveFavorite" + dict["RemoveFavorite"])
    let favorite_stations = JSON.parse(localStorage.getItem("favorite_stations"));
    if (favorite_stations == null) {
      favorite_stations = [];
    }
    favorite_stations = favorite_stations.filter((station) => station !== dict["RemoveFavorite"]);
    localStorage.setItem("favorite_stations", JSON.stringify(favorite_stations));
    console.log(favorite_stations.join("|"))
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
    for (let i = 0; i < r.Trains.length; i++) {
      let train = r.Trains[i];

      if (train.Min === "ARR") {
      } else if (train.Min === "BRD") {
      } else if (train.Min === "1") {
        train.Min = "1min";
      } else {
        train.Min = train.Min + "mins";
      }

      let trainString =
        train.Line + " " + train.DestinationName + " " + train.Min;
      if (length + trainString.length > MAX_CHARS_STATION_RESULT) {
        i = r.Trains.length;
      } else {
        trainResponse.push(trainString);
        length += trainString.length;
      }
    }

    if (trainResponse.length === 0) {
      trainResponse.push("No trains scheduled! :(");
    }

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

function getCodeFromName(name) {
  return stationMap[name] || null;
}
