
/*
 *
 */

#include "version.h"

#define MYNAME "EUL"

//String UniqueName = String(MYNAME) + "-" + WiFi.macAddress();
String UniqueName = String(MYNAME);
const char *uniquename = UniqueName.c_str();

#define LED_BUILTIN D2
#define MAXBUF 1024
#define RF_RESET D1
#define RF_TURBO D3

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
	Serial0.write( sbuf, av ) ;
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
