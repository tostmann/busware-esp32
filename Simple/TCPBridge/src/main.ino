
/*
 *
 */

#include "version.h"
#include "busware.h"
#include <WiFi.h>

#include "updateOTA.h"

#include <Preferences.h>
Preferences prefs;

#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);

#define MAXBUF 1024
#define MAX_SRV_CLIENTS 1

#include <ESPmDNS.h>
#include "mdns.h"

// main definitions:
#if defined(BUSWARE_EUL)

#define MDNS_SRV "_tcm515"
#define MYNAME "EUL"

#if defined(CONFIG_IDF_TARGET_ESP32C3)
TCMTransceiver Transceiver(&Serial0, 3, 5);
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
TCMTransceiver Transceiver(&Serial1, 21);
#endif

#elif defined(BUSWARE_TUL)

TPUARTTransceiver Transceiver(&Serial0);

#define MYNAME "TUL"
#define MDNS_SRV "_ncn5130"

#elif defined(BUSWARE_ZUL)

#define MYNAME "ZUL"
#define MDNS_SRV "_zstack"
#define MDNS_TXT_COUNT 2
mdns_txt_item_t mDNSTxtData[MDNS_TXT_COUNT] = {{(char*)"radio_type", (char*)"zstack"}, {(char*)"baud_rate", (char*)"115200"}};
ZigbeeTransceiver Transceiver(&Serial0, 3, 2);

#elif defined(BUSWARE_CUN)

CSMTransceiver Transceiver(&Serial0, 2);

#define MYNAME "CUN"
#define MDNS_SRV "_culfw"

#else

#error "No matching gadget"

#endif

uint32_t BytesIn  = 0; 
uint32_t BytesOut = 0;

#ifndef TCP_SVR_PORT
#error need TCP_SVR_PORT definition
#endif

WiFiServer server(TCP_SVR_PORT); // TCP Port to listen on 
WiFiClient serverClients[MAX_SRV_CLIENTS];
bool Server_running = false;
unsigned long previousConnect = 0;
const long Reconnect_interval = 5000;
bool request_update = false;
bool request_restart = false;
int8_t ledmode = 2;
long led_interval = 100;

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer webserver(80);

void homepage(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/plain");

//    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", String( WiFi.getHostname() ) + " - Version: " + VERSION + "\n\nSSID: " + WiFi.SSID() + " - RSSI: " + WiFi.RSSI() + "dBm - uptime: " + String(millis()/1000)+ "sec - Bytes in: " + String(BytesIn) + " out: " + String(BytesOut) + "\n\nTCP bridge active @ " + WiFi.localIP().toString() + ":" + String(TCP_SVR_PORT) + "\n\n"  );
    response->print( String( WiFi.getHostname() ) + " - Version: " + VERSION + "\n\nSSID: " + WiFi.SSID() + " - RSSI: " + WiFi.RSSI() + "dBm - uptime: " + String(millis()/1000)+ "sec - Bytes in: " + String(BytesIn) + " out: " + String(BytesOut) + "\n\nTCP bridge active @ " + WiFi.localIP().toString() + ":" + String(TCP_SVR_PORT) + "\n\n"  );
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");

    if (request->hasParam("ledmode")) {
	String message = request->getParam("ledmode")->value();
	int8_t mode = message.toInt();
	if (mode != ledmode) {
	    response->printf("new ledmode is: %d was %d\n\n", mode, ledmode);
	    ledmode = mode;
	    prefs.begin("credentials", false);
	    prefs.putInt("ledmode", ledmode);
	    prefs.end();
	    if (ledmode)
		led_interval = ledmode * 250;
	}
    }
    
    for(uint8_t i = 0; i < MAX_SRV_CLIENTS; i++){
	if (serverClients[i] && serverClients[i].connected()){
	    response->printf("Client %d from: ", i+1);
	    response->print( serverClients[i].remoteIP().toString() );
	    response->printf(":%d\n", serverClients[i].remotePort() );
	}
    }
    request->send(response);
}

void restart(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String( "restarting...\n\n" ));
    request_restart = true;
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

uint16_t inByte; // for reading from serial
byte smlMessage[MAXBUF]; // for storing the the isolated message. 

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

void setup(void) {
    
    String UniqueName = String(MYNAME) + "-" + getBase32ID();

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname( UniqueName.c_str() );

    Transceiver.begin();
    
    Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);
    
    improvSerial.setDeviceInfo(
#if defined(CONFIG_IDF_TARGET_ESP32C3)
	ImprovTypes::ChipFamily::CF_ESP32_C3,
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	ImprovTypes::ChipFamily::CF_ESP32_S2,
#endif	
	WiFi.getHostname(), VERSION_SHORT, MYNAME);

    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);
    improvSerial.setCustomConnectWiFi(ImprovWiFiTryConnect);

    webserver.on("/", HTTP_GET, homepage);    
    webserver.on("/reboot", HTTP_GET, restart);    
    webserver.on("/restart", HTTP_GET, restart);    
    webserver.onNotFound(homepage);
#ifdef OTA_URL    
    webserver.on("/update", HTTP_GET, webupdate);    
#endif
   
    Serial.print( WiFi.getHostname() );
    Serial.print(" - init succeed - running: ");
    Serial.print( VERSION );
    Serial.print(" @ ");
    Serial.print( getCpuFrequencyMhz() );
    Serial.println(" MHz");

}

unsigned long previousMillis  = 0;
unsigned long ReqMillis = 0;

int ledState = LOW;

// the loop function runs over and over again forever
void loop() {
    uint8_t sbuf[MAXBUF], i;
    uint16_t av;
    
    unsigned long currentMillis = millis();

    if (request_restart) {
	delay(1000);
	ESP.restart();
    }
    
    if (improvSerial.isConnected()) {

	previousConnect = currentMillis;

#ifdef OTA_URL
	if (request_update) {
	    firmwareUpdate();
	    request_update = false;
	}
#endif

	if (!Server_running) {
	    Serial.println( "Server listening @ " + WiFi.localIP().toString() + ":" + String(TCP_SVR_PORT));
#ifdef OTA_URL
	    Serial.println("This firmware supports OTA updates, by calling: http://" + WiFi.localIP().toString() + "/update?t=99");
#else	    
	    Serial.println("No OTA support");
#endif
	    
	    server.begin();
	    server.setNoDelay(true);

	    webserver.begin();

	    MDNS.begin(WiFi.getHostname());
	    MDNS.addService( "http", "tcp", 80);

#ifdef MDNS_TXT_COUNT
	    mdns_service_add(NULL, MDNS_SRV, "_tcp", TCP_SVR_PORT, mDNSTxtData, MDNS_TXT_COUNT);
#else
	    MDNS.addService( MDNS_SRV, "tcp", TCP_SVR_PORT);
#endif
		
	    Server_running = true;
	    Transceiver.set_reset(false);
	
	}

	//check if there are any new clients
	if (server.hasClient()){
	    for(i = 0; i < MAX_SRV_CLIENTS; i++){
		//find free/disconnected spot
		if (!serverClients[i] || !serverClients[i].connected()){
		    if(serverClients[i]) serverClients[i].stop();
		    serverClients[i] = server.available();
		    break;
		}
	    }
	    
	    if (i >= MAX_SRV_CLIENTS) {
		//no free/disconnected spot so reject
		server.available().stop();
	    }
	}
	//check clients for data
	for(i = 0; i < MAX_SRV_CLIENTS; i++){
	    if (serverClients[i] && serverClients[i].connected()){
		av = serverClients[i].available();
		if(av > 0 ){
		    //get data from the telnet client and push it to the UART
		    if (av > MAXBUF) av = MAXBUF;
		    serverClients[i].readBytes( sbuf, av );
		    Transceiver.write( sbuf, av ) ;
		    BytesIn += av;
		    if (ledmode)
			digitalWrite(LED_BUILTIN, LOW);
		    previousMillis = currentMillis;
		}
	    }
	    else {
		if (serverClients[i]) {
		    serverClients[i].stop();
		}
	    }
	}

    } else {
	
	
	for(i = 0; i < MAX_SRV_CLIENTS; i++) {
	    if (serverClients[i]) serverClients[i].stop();
	}
	
	if (Server_running) {
	    Serial.println("WiFi disconnected!");
	    Transceiver.set_reset(true);
	    server.end();
	    webserver.end();
	    MDNS.end();
	    Server_running = false;
	}

	if (currentMillis - previousConnect >= Reconnect_interval) {
	    
	    prefs.begin("credentials", true);
	    String ssid     = prefs.getString("ssid", ""); 
	    String password = prefs.getString("password", "");
	    ledmode         = prefs.getInt("ledmode", 2);
	    prefs.end();
	    
	    if (ssid != "" && password != "")
		ImprovWiFiTryConnect(ssid.c_str(), password.c_str());

	    if (ledmode)
		led_interval = ledmode * 250;
	    previousConnect = currentMillis;

	}
	
    }
    
    improvSerial.handleSerial();

    av = Transceiver.available();
    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;
	Transceiver.readBytes( sbuf, av );
	BytesOut += av;

	for(i = 0; i < MAX_SRV_CLIENTS; i++){
	    if (serverClients[i] && serverClients[i].connected()){
		serverClients[i].write(sbuf, av);
		delay(1);
	    }
	}
	if (ledmode)
	    digitalWrite(LED_BUILTIN, LOW);
	previousMillis = currentMillis;
    }

    // LED blinking
    if (currentMillis - previousMillis >= led_interval) {
	previousMillis = currentMillis;
	
	if (ledState == HIGH && ledmode>1)
	    ledState = LOW;
	else
	    ledState = HIGH;
	
	digitalWrite(LED_BUILTIN, ledState);
    }
}

