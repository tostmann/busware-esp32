
/*
 *
 */

#include "version.h"
#include <WiFi.h>

#ifdef USE_IMPROV
#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);
#endif

#define MYNAME "EUL"

String UniqueName = String(MYNAME) + "-" + WiFi.macAddress();
//String UniqueName = String(MYNAME);
const char *uniquename = UniqueName.c_str();

#define MAXBUF 1024
#undef  LED_BUILTIN

#ifdef BUSWARE_C3
  #define LED_BUILTIN 4
  #define RF_RESET 3
  #define RF_TURBO 5
#endif
#ifdef BUSWARE_S2
  #define LED_BUILTIN 2
  #define RF_RESET 21
  #define RF_TURBO 33
#endif


uint16_t inByte; // for reading from serial
byte smlMessage[MAXBUF]; // for storing the the isolated message. 

void setup(void) {
    
    pinMode(RF_RESET, OUTPUT);
    digitalWrite(RF_RESET, LOW);
    
    pinMode(RF_TURBO, OUTPUT);
    digitalWrite(RF_TURBO, LOW);
    
    Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial0.begin(460800);

#ifdef USE_IMPROV
    improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32_C3, UniqueName.c_str(), VERSION_SHORT, MYNAME);
#endif
    
    delay(1000);

    Serial.print( UniqueName );
    Serial.print(" - init succeed - running: ");
    Serial.println( VERSION );
    
    delay(1000);

}

unsigned long previousMillis = 0;
unsigned long ReqMillis = 0;

int ledState = LOW;
const long interval = 500;

// the loop function runs over and over again forever
void loop() {
    uint8_t sbuf[MAXBUF];
    uint16_t av;

    unsigned long currentMillis = millis();

    digitalWrite(RF_RESET, HIGH);

    av = Serial.available();
    if (av > 0) {

	if (av > MAXBUF) av = MAXBUF;
	Serial.readBytes( sbuf, av );

#ifdef USE_IMPROV
	if (!improvSerial.handleBuffer(sbuf, av) )
#endif	
	    Serial0.write( sbuf, av );
	digitalWrite(LED_BUILTIN, HIGH);
	previousMillis = currentMillis;
    }

    av = Serial0.available();
    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;
	Serial0.readBytes( sbuf, av );
	Serial.write( sbuf, av ) ;
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

