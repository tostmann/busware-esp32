
#include "version.h"
#include <SPI.h>
#include <ESP_AVRISP.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "updateOTA.h"

#define MYNAME "CUN-AVRISP"

#include <Preferences.h>
Preferences prefs;

#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);

const uint16_t port = 328;
const uint8_t  reset_pin = 2;
const uint32_t connectTimeoutMs = 10000;
bool Server_running = false;
bool request_update = false;
unsigned long previousMillis  = 0;
int ledState = LOW;
const long interval = 500;

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer webserver(80);

void homepage(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String( WiFi.getHostname() ) + " - Version: " + VERSION + "\n\nSSID: " + WiFi.SSID() + " - RSSI: " + WiFi.RSSI() + "dBm - uptime: " + String(millis()/1000)+ "sec\n\n\nUse your avrdude:\n\navrdude -c arduino -p m328pb -P net:" + WiFi.localIP().toString() + ":" + String(port) + " -e -U flash:w:nanoCUL.hex -U lfuse:w:0xe2:m -U hfuse:w:0xd1:m\n\n"  );    
}

#ifdef OTA_URL
void webupdate(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OTA update from " + String(OTA_URL) + " started ...\n\nsee serial console for details" );
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    request->send(response);
    request_update = true;
}
#endif

void onImprovWiFiConnectedCb(const char *ssid, const char *password) {
    prefs.begin("credentials", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
}

bool ImprovWiFiTryConnect(const char *ssid, const char *password) {
    uint8_t count = 0;
    int bestNetworkDb = INT_MIN;
    uint8_t bestBSSID[6];
    int32_t bestChannel = 0;
    bool known = false;
        
    if (WiFi.isConnected()) {
	WiFi.disconnect();
	delay(100);
    }

    int networkNum = WiFi.scanNetworks(false, false); // Wait for scan result, hide hidden

    if (networkNum==0)
	networkNum = WiFi.scanNetworks(false, false); 
    
    if (networkNum<=0)
	return false;

    for(int32_t i = 0; i < networkNum; ++i) {
	
	String ssid_scan;
	int32_t rssi_scan;
	uint8_t sec_scan;
	uint8_t* BSSID_scan;
	int32_t chan_scan;
	
	WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan);

	if (String(ssid) == ssid_scan) {
	    if(rssi_scan > bestNetworkDb) {
		known = true;
		bestNetworkDb = rssi_scan;
		bestChannel   = chan_scan;
		memcpy((void*) &bestBSSID, (void*) BSSID_scan, sizeof(bestBSSID));
	    }
	}
	
    }

    if (!known) 
	return false;
    
    WiFi.begin( ssid, password, bestChannel, bestBSSID);
    
    while (!WiFi.isConnected()) {
	delay(DELAY_MS_WAIT_WIFI_CONNECTION);
	if (count > MAX_ATTEMPTS_WIFI_CONNECTION) {
	    WiFi.disconnect();
	    return false;
	}
	count++;
    }
    
    return true;
}

ESP_AVRISP avrprog(port, reset_pin);

void setup() {
    String UniqueName = String(MYNAME) + "-" + WiFi.macAddress();
    UniqueName.replace( ":", "" );				   
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname( UniqueName.c_str() );

    Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, ledState);

    improvSerial.setDeviceInfo(
#if defined(CONFIG_IDF_TARGET_ESP32C3)
	ImprovTypes::ChipFamily::CF_ESP32_C3,
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	ImprovTypes::ChipFamily::CF_ESP32_S2,
#endif	
	WiFi.getHostname(), VERSION_SHORT, MYNAME);

    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);
    improvSerial.setCustomConnectWiFi(ImprovWiFiTryConnect);
    
    Serial.println("");
    Serial.println("Arduino AVR-ISP over TCP");
    Serial.print( WiFi.getHostname() );
    Serial.print(" - init succeed - running: ");
    Serial.println( VERSION );

    webserver.on("/", HTTP_GET, homepage);

#ifdef OTA_URL    
    webserver.on("/update", HTTP_GET, webupdate);    
#endif
    webserver.onNotFound(homepage);

}

unsigned long previousConnect = 0;
const long Reconnect_interval = 5000;

void loop() {
    unsigned long currentMillis = millis();

    improvSerial.handleSerial();

    if (improvSerial.isConnected()) {

	previousConnect = currentMillis;

#ifdef OTA_URL
	if (request_update) {
	    firmwareUpdate();
	    request_update = false;
	}
#endif
	
	if (!Server_running) {
	    // listen for avrdudes
	    avrprog.begin();
	    avrprog.setReset(false); // let the AVR run
	    
	    MDNS.begin(WiFi.getHostname());
	    MDNS.addService("avrisp", "tcp", port);
	    webserver.begin();

	    IPAddress local_ip = WiFi.localIP();

	    Serial.print("WiFi connected: ");
	    Serial.print(WiFi.SSID());
	    Serial.print(" - ");
	    Serial.print(WiFi.RSSI());
	    Serial.print("dBm - IP address: ");
	    Serial.println(local_ip);

#ifdef OTA_URL
	    Serial.print("This firmware supports OTA updates, by calling: http://");
	    Serial.print(local_ip);
	    Serial.println("/update?t=99");
#else	    
	    Serial.println("No OTA support");
#endif
	    
	    Serial.println("\nUse your avrdude:");
	    Serial.print("avrdude -c arduino -p m328pb -P net:");
	    Serial.print(local_ip);
	    Serial.print(":");
	    Serial.print(port);
	    Serial.println(" -e -U flash:w:nanoCUL.hex -U lfuse:w:0xe2:m -U hfuse:w:0xd1:m\n");

	    Server_running = true;
	}
	
	static AVRISPState_t last_state = AVRISP_STATE_IDLE;
	AVRISPState_t new_state = avrprog.update();
	
	if (last_state != new_state) {
	    switch (new_state) {
            case AVRISP_STATE_IDLE: {
                Serial.printf("[AVRISP] now idle\r\n");
                // Use the SPI bus for other purposes
                break;
            }
            case AVRISP_STATE_PENDING: {
                Serial.printf("[AVRISP] connection pending\r\n");
                // Clean up your other purposes and prepare for programming mode
                break;
            }
            case AVRISP_STATE_ACTIVE: {
                Serial.printf("[AVRISP] programming mode\r\n");
                // Stand by for completion
                break;
            }
	    }
	    last_state = new_state;
	}
	// Serve the client
	if (last_state != AVRISP_STATE_IDLE) {
	    avrprog.serve();
	}

	// LED blinking
	if (currentMillis - previousMillis >= interval) {
	    previousMillis = currentMillis;
	    
	    if (ledState == HIGH)
		ledState = LOW;
	    else
		ledState = HIGH;
	    
	}
	
    } else {
	
	// not connected ...
	ledState = HIGH;
	
	if (Server_running) {
	    MDNS.end();
	    webserver.end();
	    avrprog.setReset(true); // stop the AVR 
	    Server_running = false;
	}

	if (currentMillis - previousConnect >= Reconnect_interval) {
	    
	    prefs.begin("credentials", true);
	    String ssid     = prefs.getString("ssid", ""); 
	    String password = prefs.getString("password", "");
	    prefs.end();
	    
	    if (ssid != "" && password != "")
		ImprovWiFiTryConnect(ssid.c_str(), password.c_str());

	    previousConnect = currentMillis;

	}
	
	
    }

    digitalWrite(LED_BUILTIN, ledState);
	
}
