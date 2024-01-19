
/*
 *
 */

#include "version.h"
#include <WiFi.h>

#ifdef USE_IMPROV
#include <ImprovWiFiLibrary.h>
ImprovWiFi improvSerial(&Serial);
#endif

#define MAXBUF 1024
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

uint16_t inByte; // for reading from serial
byte smlMessage[MAXBUF]; // for storing the the isolated message. 

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

#ifdef RF_RESET
    digitalWrite(RF_RESET, HIGH);
#endif

    av = Serial.available();
    if (av > 0) {

	if (av > MAXBUF) av = MAXBUF;
	Serial.readBytes( sbuf, av );

#ifdef USE_IMPROV
	if (!improvSerial.handleBuffer(sbuf, av) )
#endif	
	    TCM.write( sbuf, av );
	digitalWrite(LED_BUILTIN, HIGH);
	previousMillis = currentMillis;
    }

    av = TCM.available();
    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;
	TCM.readBytes( sbuf, av );
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

