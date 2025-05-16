#include <version.h>
#include <knx.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>

#define MYNAME "Thelsing KNX Coupler for TUL"

#include <ESPAsyncWebServer.h>
AsyncWebServer webserver(80);

#include <Preferences.h>
Preferences prefs;

#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);

const char* NTP_SERVER = "pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

void init_webserver() {

    webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
	    File htmlFile = SPIFFS.open("/index.html");
	    if (!htmlFile) {
		request->send(404, "text/plain", "index.html nicht gefunden");
		return;
	    }

/*
  Platzhalter ersetzen mit:
  - %name%: Systemname
  - %cpu_id%: CPU Identifikation
  - %version%: Softwareversion
  - %ip_address%: IPv4 Adresse
  - %freitext%: Freie Beschreibungstexte
*/	    
	    String htmlContent = htmlFile.readString();
	    htmlContent.replace("%name%", MYNAME);
	    htmlContent.replace("%cpu_id%", WiFi.getHostname());
	    htmlContent.replace("%version%", VERSION);
            htmlContent.replace("%freitext%", "setup multicast addr: 224.0.23.12 port: 3671 ip: %ip_address% tunnels: %num_tunnels%");
	    htmlContent.replace("%ip_address%", WiFi.localIP().toString());
            htmlContent.replace("%num_tunnels%", String(KNX_TUNNELING,DEC));
	    
	    if (knx.bau().getSecondaryDataLinkLayer()) {
		htmlContent.replace("%TxFrameCounter%", String( knx.bau().getSecondaryDataLinkLayer()->getTxFrameCounter(), DEC));
		htmlContent.replace("%RxProcessdFrameCounter%", String( knx.bau().getSecondaryDataLinkLayer()->getRxProcessdFrameCounter(), DEC));
		htmlContent.replace("%TxProcessedFrameCounter%", String( knx.bau().getSecondaryDataLinkLayer()->getTxProcessedFrameCounter(), DEC));

		/*
		  uint32_t getRxInvalidFrameCounter();
		  uint32_t getRxIgnoredFrameCounter();
		  uint32_t getRxUnknownControlCounter();
		  uint32_t getTxProcessedFrameCounter();
		*/
	    }

	    
	    htmlContent.replace("%uptime%", String(millis()/60000, DEC));
	    
	    htmlFile.close();
    
	    request->send(200, "text/html", htmlContent);
	});

    webserver.begin();
}

void onImprovWiFiConnectedCb(const char *ssid, const char *password) {
    // Save ssid and password here

    if (ssid && password) {
	prefs.begin("credentials", false);
	prefs.putString("ssid", ssid);
	prefs.putString("password", password);
	prefs.end();
    }

    WiFi.setAutoReconnect(true);

    Serial.println(WiFi.localIP());
    
    Serial.println("Adjusting system time...");
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TZ_INFO, 1);
    delay(1000);
    
    Serial.println("Connected.");

    // read adress table, association table, groupobject table and parameters from eeprom
    knx.readMemory();

    // print values of parameters if device is already configured
    if (knx.configured())
    {
        Serial.println("Coupler configured.");
    }

    // pin or GPIO the programming led is connected to. Default is LED_BUILTIN
    knx.ledPin(KNX_LED);
    // is the led active on HIGH or low? Default is LOW
    // knx.ledPinActiveOn(HIGH);
    // pin or GPIO programming button is connected to. Default is 0
    knx.buttonPin(9);

    // start the framework.
    knx.start();

    init_webserver();
}

void setup() {
    Serial.begin(115200);
    ArduinoPlatform::SerialDebug = &Serial;

    pinMode(KNX_LED, OUTPUT);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

//    delay(2000);
    Serial.printf("%s on %s - %s\r\n", MYNAME, WiFi.getHostname(), VERSION);

    if (!SPIFFS.begin(true)) {
	Serial.println("SPIFFS Mount fehlgeschlagen");
    }
    
    improvSerial.setDeviceInfo(
#if defined(CONFIG_IDF_TARGET_ESP32C3)
	ImprovTypes::ChipFamily::CF_ESP32_C3,
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	ImprovTypes::ChipFamily::CF_ESP32_S2,
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
	ImprovTypes::ChipFamily::CF_ESP32_S3,
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
	ImprovTypes::ChipFamily::CF_ESP32_C6,
#endif	
	WiFi.getHostname(), VERSION_SHORT, MYNAME );

    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);

    if (prefs.begin("credentials", true)) {
	Serial.println("Found credentials!");
	
	String ssid     = prefs.getString("ssid", ""); 
	String password = prefs.getString("password", "");
	prefs.end();
	
	if (ssid != "" && password != "") {
	    Serial.println("Connecting to Wi-Fi...");
	    if (improvSerial.tryConnectToWifi(ssid.c_str(), password.c_str())) {
		onImprovWiFiConnectedCb( nullptr, nullptr );
	    }
	}
    }

    randomSeed(millis());

}

void loop() {

    improvSerial.handleSerial();

    if (!improvSerial.isConnected())
	return;
    
    // don't delay here to much. Otherwise you might lose packages or mess up the timing with ETS
    knx.loop();

    // only run the application code if the device was configured with ETS
    if (!knx.configured())
        return;
}
