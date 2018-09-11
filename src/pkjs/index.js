var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, {autoHandleEvents: false});
var messageKeys = require('message_keys');

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  var dict = clay.getSettings(e.response);

  Pebble.sendAppMessage(dict, function() {
    console.log('Message sent successfully: ' + JSON.stringify(dict));
  }, function(e) {
    console.log('Message failed: ' + JSON.stringify(e));
  });
});

Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');
  }
);

Pebble.addEventListener('appmessage', function(e) {
  var dict = e.payload;
  console.log('Got message: ' + JSON.stringify(dict));
});