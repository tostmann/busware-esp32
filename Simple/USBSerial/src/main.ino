
/*
 *
 */

#include "version.h"
#include "busware.h"
#include <WiFi.h>

#ifdef USE_IMPROV
#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);
#endif

#define MAXBUF 1024

// main definitions:
#if defined(BUSWARE_EUL)

#define MYNAME "EUL"

#if defined(CONFIG_IDF_TARGET_ESP32C3)
TCMTransceiver Transceiver(&Serial0, 3, 5);
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
TCMTransceiver Transceiver(&Serial1, 21);
#endif

#elif defined(BUSWARE_TUL)

#define MYNAME "TUL"
TPUARTTransceiver Transceiver(&Serial0);

#elif defined(BUSWARE_ZUL)

#define MYNAME "ZUL"
ZigbeeTransceiver Transceiver(&Serial0, 3, 2);

#elif defined(BUSWARE_CUN)

#define MYNAME "CUN"
CSMTransceiver Transceiver(&Serial0, 2);

#else

#error "No matching gadget"

#endif

uint16_t inByte; // for reading from serial
byte smlMessage[MAXBUF]; // for storing the the isolated message. 

void setup(void) {

    String UniqueName = String(MYNAME) + "-" + getBase32ID();

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname( UniqueName.c_str() );

    Transceiver.begin();
    
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
const long interval = 500;

// the loop function runs over and over again forever
void loop() {
    uint8_t sbuf[MAXBUF], i;
    uint16_t av;
    
    unsigned long currentMillis = millis();

    Transceiver.set_reset(false);

    av = Serial.available();
    if (av > 0) {
	
	if (av > MAXBUF) av = MAXBUF;
	Serial.readBytes( sbuf, av );
	
#ifdef USE_IMPROV
	if (!improvSerial.handleBuffer(sbuf, av) )
#endif	
	    Transceiver.write( sbuf, av );
	digitalWrite(LED_BUILTIN, HIGH);
	previousMillis = currentMillis;
    }

    av = Transceiver.available();
    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;
	Transceiver.readBytes( sbuf, av );

	Serial.write( sbuf, av ) ;
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

