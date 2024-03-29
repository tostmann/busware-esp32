#ifdef OTA_URL
#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

void update_started() {
  Serial.println("HTTP update: process started");
}

void update_finished() {
  Serial.println("HTTP update: process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("HTTP update: process at %d of %d bytes...\r\n", cur, total);
}

void update_error(int err) {
  Serial.printf("HTTP update: fatal error code %d\r\n", err);
}

void firmwareUpdate() {
    WiFiClient client;
    HTTPUpdate httpUpdate;
    
    httpUpdate.onStart(update_started);
    httpUpdate.onEnd(update_finished);
    httpUpdate.onProgress(update_progress);
    httpUpdate.onError(update_error);
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);	    
    
    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    httpUpdate.setLedPin(LED_BUILTIN, HIGH);
    
    t_httpUpdate_return ret = httpUpdate.update(client, OTA_URL);
    // Or:
    //t_httpUpdate_return ret = httpUpdate.update(client, "server", 80, "/file.bin");
    
    switch (ret) {
    case HTTP_UPDATE_FAILED:
	Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\r\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
	break;
	
    case HTTP_UPDATE_NO_UPDATES:
	Serial.println("HTTP_UPDATE_NO_UPDATES");
	break;
	
    case HTTP_UPDATE_OK:
	Serial.println("HTTP_UPDATE_OK");
	break;
    }
}
#endif
