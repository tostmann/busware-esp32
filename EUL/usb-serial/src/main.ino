
/*
 *
 */

#include "version.h"
#include <WiFi.h>

#ifdef USE_IMPROV
#include <Preferences.h>
Preferences prefs;

#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);
#endif

#define MAXBUF 1024
#define MAX_SRV_CLIENTS 1

// main definitions:
#if defined(BUSWARE_EUL)

#define MDNS_SRV "tcm515"
#define MYNAME "EUL"

#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define RF_RESET 3
#define RF_TURBO 5
#define TCM Serial0
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#undef  LED_BUILTIN
#define LED_BUILTIN 2
#define RF_RESET 21
#define TCM Serial1
#endif

#elif defined(BUSWARE_TUL)

#define TCM Serial0
#define MYNAME "TUL"
#define MDNS_SRV "ncn5130"

#elif defined(BUSWARE_CUN)
#define TCM Serial0
#define MYNAME "CUN"
#define MDNS_SRV "culfw"

#else

#error "No matching gadget"

#endif

uint32_t BytesIn  = 0; 
uint32_t BytesOut = 0; 

#ifdef TCP_SVR_PORT
WiFiServer server(TCP_SVR_PORT); // TCP Port to listen on 
WiFiClient serverClients[MAX_SRV_CLIENTS];
bool Server_running = false;
unsigned long previousConnect = 0;
const long Reconnect_interval = 5000;

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

AsyncWebServer webserver(80);

void homepage(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String( WiFi.getHostname() ) + " - Version: " + VERSION + "\n\nSSID: " + WiFi.SSID() + " - RSSI: " + WiFi.RSSI() + "dBm - uptime: " + String(millis()/1000)+ "sec - Bytes in: " + String(BytesIn) + " out: " + String(BytesOut) + "\n\nTCP bridge active @ " + WiFi.localIP().toString() + ":" + String(TCP_SVR_PORT) + "\n\n"  );
}

#endif

uint16_t inByte; // for reading from serial
byte smlMessage[MAXBUF]; // for storing the the isolated message. 

#ifdef USE_IMPROV
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
#endif

void setup(void) {

    String UniqueName = String(MYNAME) + "-" + WiFi.macAddress();
    UniqueName.replace( ":", "" );				   
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname( UniqueName.c_str() );

#if defined(BUSWARE_CUN)
    TCM.begin(38400, SERIAL_8N1);;

#elif defined(BUSWARE_TUL)
    TCM.begin(38400, SERIAL_8E1);;

#elif defined(BUSWARE_EUL)

#ifdef RF_RESET    
    pinMode(RF_RESET, OUTPUT);
    digitalWrite(RF_RESET, LOW);
#endif

#ifdef RF_TURBO
    pinMode(RF_TURBO, OUTPUT);
    digitalWrite(RF_TURBO, LOW);
    TCM.begin(460800);
#else    
    TCM.begin(57600);
#endif

#endif
    
    Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);

    
#ifdef USE_IMPROV
    improvSerial.setDeviceInfo(
#if defined(CONFIG_IDF_TARGET_ESP32C3)
	ImprovTypes::ChipFamily::CF_ESP32_C3,
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	ImprovTypes::ChipFamily::CF_ESP32_S2,
#endif	
	WiFi.getHostname(), VERSION_SHORT, MYNAME);

    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);
    improvSerial.setCustomConnectWiFi(ImprovWiFiTryConnect);
#endif

#ifdef TCP_SVR_PORT
    webserver.on("/", HTTP_GET, homepage);    
    webserver.onNotFound(homepage);
#endif

   
    Serial.print( WiFi.getHostname() );
    Serial.print(" - init succeed - running: ");
    Serial.print( VERSION );

    Serial.println();
    
    
}

unsigned long previousMillis  = 0;
unsigned long ReqMillis = 0;

int ledState = LOW;
const long interval = 500;

// the loop function runs over and over again forever
void loop() {
    uint8_t sbuf[MAXBUF], i;
    uint16_t av;
    
    unsigned long currentMillis = millis();
    
#ifdef RF_RESET
    digitalWrite(RF_RESET, HIGH);
#endif
    
#ifdef TCP_SVR_PORT
    
    if (improvSerial.isConnected()) {

	previousConnect = currentMillis;

	if (!Server_running) {
	    Serial.print( "Server listening @ " + WiFi.localIP().toString() );
	    Serial.println( ":" + String(TCP_SVR_PORT) );
	    
	    server.begin();
	    server.setNoDelay(true);

	    webserver.begin();

	    MDNS.begin(WiFi.getHostname());
	    MDNS.addService( MDNS_SRV, "tcp", TCP_SVR_PORT);
	    MDNS.addService( "http", "tcp", 80);

	    Server_running = true;
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
		    TCM.write( sbuf, av ) ;
		    BytesIn += av;
		    digitalWrite(LED_BUILTIN, HIGH);
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
	    server.end();
	    webserver.end();
	    MDNS.end();
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
    
#endif
    
#ifdef TCP_SVR_PORT
#ifdef USE_IMPROV
    improvSerial.handleSerial();
#endif	
#else
    av = Serial.available();
    if (av > 0) {
	
	if (av > MAXBUF) av = MAXBUF;
	Serial.readBytes( sbuf, av );
	BytesIn += av;
	
#ifdef USE_IMPROV
	if (!improvSerial.handleBuffer(sbuf, av) )
#endif	
	    TCM.write( sbuf, av );
	digitalWrite(LED_BUILTIN, HIGH);
	previousMillis = currentMillis;
    }
#endif

    av = TCM.available();
    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;
	TCM.readBytes( sbuf, av );
	BytesOut += av;

#ifdef TCP_SVR_PORT
	for(i = 0; i < MAX_SRV_CLIENTS; i++){
	    if (serverClients[i] && serverClients[i].connected()){
		serverClients[i].write(sbuf, av);
		delay(1);
	    }
	}
#else
	Serial.write( sbuf, av ) ;
#endif    
	digitalWrite(LED_BUILTIN, HIGH);
	previousMillis = currentMillis;
    }

    // LED blinking
    if (currentMillis - previousMillis >= interval) {
	previousMillis = currentMillis;
	
	if (ledState == HIGH)
	    ledState = LOW;
	else
	    ledState = HIGH;
	
	digitalWrite(LED_BUILTIN, ledState);
    }
}

