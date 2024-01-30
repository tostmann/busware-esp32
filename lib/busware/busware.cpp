
#include "busware.h"
#include <HardwareSerial.h>
#include <WiFi.h>
#include "base32.h"

String getBase32ID() {
    byte    mac_base[6] = {0};
    uint8_t outByte;
    byte    oBytes[16];
    esp_efuse_mac_get_default(mac_base);

    String UniqueName;

    outByte = toBase32((byte*)(mac_base+3), 3, oBytes, false);

    if (outByte>0) {
	oBytes[outByte] = 0;
	UniqueName = String( (char*)oBytes );
    } else {
	UniqueName = WiFi.macAddress();
	UniqueName.replace( ":", "" );
    }

    return UniqueName;
}

void GenericTransceiver::begin() {
   if (_bootloadPIN) {
	pinMode(_bootloadPIN, OUTPUT);
	digitalWrite(_bootloadPIN, HIGH);
   }
    if (_resetPIN)
	pinMode(_resetPIN, OUTPUT);
    set_reset( true );
}

void GenericTransceiver::set_reset(bool res) {
    if (_resetPIN) 
	digitalWrite(_resetPIN, res ? LOW : HIGH );
}

void GenericTransceiver::call_bootloader(bool res) {
    if (_bootloadPIN) {
	digitalWrite(_bootloadPIN, res ? LOW : HIGH);
	set_reset( true );
	delay(100); 
	set_reset( false );
	delay(100); 
    }
}


void TPUARTTransceiver::begin() {
    GenericTransceiver::begin();
    _serial->begin(38400, SERIAL_8E1);;
}

void CSMTransceiver::begin() {
    GenericTransceiver::begin();
    _serial->begin(38400, SERIAL_8N1);;
}

void TCMTransceiver::begin() {
    GenericTransceiver::begin();
    if (_turboPIN) {
	pinMode(_turboPIN, OUTPUT);
	digitalWrite(_turboPIN, LOW);
	_serial->begin(460800);
    } else
	_serial->begin(57600);
}

void ZigbeeTransceiver::begin() {
    GenericTransceiver::begin();
    //    _serial->begin(500000, SERIAL_8N1);;
    _serial->begin(115200, SERIAL_8N1);;
}

