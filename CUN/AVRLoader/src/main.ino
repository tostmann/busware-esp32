
#include <Arduino.h>
#include <SPI.h>

#include "AVRStreamISP.h"
#include "SDHexSession.h"
#include <LittleFS.h>

SDHexSession mSDHexSession;
AVRStreamISP mAVRStreamISP;

uint8_t	mInSession;

#define FORMAT_LITTLEFS_IF_FAILED false

void setup() {
    Serial.begin(115200);
    
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        return;
    }

    Serial.println("starting ...");
    
    SPI.begin();
    SPI.setFrequency(300000ul);
    SPI.setHwCs(false);
    
    mAVRStreamISP.begin();

    mInSession = mSDHexSession.begin( "/busware-cun-m328p-culfw.hex", nullptr, &mAVRStreamISP, false, millis());

    if (mInSession)
	Serial.println("success on mSDHexSession.begin");

    delay(5000);
}

void loop() {
    if (mInSession) {
	 mAVRStreamISP.Update();
	 if (!mSDHexSession.Update()) {
	     mInSession = 0;
	     mSDHexSession.Halt();
	     mAVRStreamISP.Halt();
	 }
    }
}
