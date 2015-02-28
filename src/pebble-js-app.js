Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKitJS is ready and running!');
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    //Load the remote config page
    Pebble.openURL("https://dl.dropboxusercontent.com/u/24511867/pebble-enlightenedqld/config.html");
  }
);

Pebble.addEventListener('webviewclosed',
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(configuration));
    Pebble.sendAppMessage(configuration,
      function(e) {
        console.log("Pebble received configuration data");
      },
      function(e) {
        console.log("Failed to receive configuration");
      }
    );
  }
);