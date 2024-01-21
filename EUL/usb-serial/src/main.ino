
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
#undef  LED_BUILTIN

#ifdef BUSWARE_EUL_C3
  #define LED_BUILTIN 4
  #define RF_RESET 3
  #define RF_TURBO 5
  #define TCM Serial0
  #define MYNAME "EUL"
#endif
#ifdef BUSWARE_EUL_S2
  #define LED_BUILTIN 2
  #define RF_RESET 21
  #define TCM Serial1
  #define MYNAME "EUL"
#endif
#ifdef BUSWARE_TUL_C3
  #define LED_BUILTIN 4
  #define TCM Serial0
  #define MYNAME "TUL"
#endif

String UniqueName = String(MYNAME) + "-" + WiFi.macAddress();
//String UniqueName = String(MYNAME);
const char *uniquename = UniqueName.c_str();


#ifdef TCP_SVR_PORT
WiFiServer server(TCP_SVR_PORT); // TCP Port to listen on 
WiFiClient serverClients[MAX_SRV_CLIENTS];
bool Server_running = false;
unsigned long previousConnect = 0;
const long Reconnect_interval = 5000;

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer webserver(80);

void homepage(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", UniqueName + " - Version: " + VERSION + "\n\nTCP bridge active @ " + WiFi.localIP().toString() + ":" + String(TCP_SVR_PORT) + "\n\n"  );
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
#endif

void setup(void) {
    
#ifdef BUSWARE_TUL_C3
    TCM.begin(38400, SERIAL_8E1);;
#else
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
#ifdef CONFIG_IDF_TARGET_ESP32C3
	ImprovTypes::ChipFamily::CF_ESP32_C3,
#elif CONFIG_IDF_TARGET_ESP32S2
	ImprovTypes::ChipFamily::CF_ESP32_S2,
#endif	
	UniqueName.c_str(), VERSION_SHORT, MYNAME);

    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);
#endif

#ifdef TCP_SVR_PORT
    webserver.on("/", HTTP_GET, homepage);    
    webserver.onNotFound(homepage);
#endif

   
    Serial.print( UniqueName );
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
	
	if (!Server_running) {
	    Serial.print( "Server listening @ " + WiFi.localIP().toString() );
	    Serial.println( ":" + String(TCP_SVR_PORT) );
	    
	    server.begin();
	    server.setNoDelay(true);

	    webserver.begin();
	    
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
	    Server_running = false;
	}

	if (currentMillis - previousConnect >= Reconnect_interval) {
	    previousConnect = currentMillis;
	    
	    prefs.begin("credentials", true);
	    String ssid     = prefs.getString("ssid", ""); 
	    String password = prefs.getString("password", "");
	    prefs.end();
	    
	    if (ssid != "" && password != "")
		improvSerial.tryConnectToWifi(ssid.c_str(), password.c_str());
	}
	
    }
    
#endif
    
    av = Serial.available();
    if (av > 0) {

	if (av > MAXBUF) av = MAXBUF;
	Serial.readBytes( sbuf, av );

#ifdef USE_IMPROV
	if (!improvSerial.handleBuffer(sbuf, av) )
#endif	
#ifdef TCP_SVR_PORT
	    delay(1);
#else
	TCM.write( sbuf, av );
#endif	    
	digitalWrite(LED_BUILTIN, HIGH);
	previousMillis = currentMillis;
    }

    av = TCM.available();
    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;
	TCM.readBytes( sbuf, av );

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

	if (0) {
	    ledState = LOW;
	} else {
	    if (ledState == HIGH)
		ledState = LOW;
	    else
		ledState = HIGH;
	}
	digitalWrite(LED_BUILTIN, ledState);
    }
}

