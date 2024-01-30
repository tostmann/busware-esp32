#ifndef busware_h
#define busware_h

#include <HardwareSerial.h>

String getBase32ID(void);

class GenericTransceiver {

protected:
    HardwareSerial* _serial;
    uint8_t _resetPIN    = 0;
    uint8_t _turboPIN    = 0;
    uint8_t _bootloadPIN = 0;
    
public:
    GenericTransceiver(HardwareSerial* serial ) { _serial = serial; };
    void   begin();
    int    available(void) { return (_serial) ? _serial->available() : -1; };
    size_t write( uint8_t* buffer, size_t size) { return (_serial) ? _serial->write(buffer, size) : -1; }
    size_t readBytes(uint8_t *buffer, size_t length) { return (_serial) ? _serial->readBytes( buffer, length) : -1; };
    void   set_reset(bool res);
    void   call_bootloader(bool res);
    };

/*

 TPUART

 */

class TPUARTTransceiver : public GenericTransceiver {
    
public:
    TPUARTTransceiver(HardwareSerial* serial) : GenericTransceiver(serial){};
    void begin();
    
};

/*

 CSM

 */

class CSMTransceiver : public GenericTransceiver {
    
public:
    CSMTransceiver(HardwareSerial* serial, uint8_t resetPIN = 0)  : GenericTransceiver(serial){ _resetPIN = resetPIN; };
    void begin();
    
};

/*

 TCM

 */

class TCMTransceiver : public GenericTransceiver {
    
public:
    TCMTransceiver(HardwareSerial* serial, uint8_t resetPIN = 0, uint8_t turboPIN = 0)  : GenericTransceiver(serial){ _resetPIN = resetPIN; _turboPIN = turboPIN; };
    void begin();
    
};

/*

 RF2652R

 */

class ZigbeeTransceiver : public GenericTransceiver {
    
public:
    ZigbeeTransceiver(HardwareSerial* serial, uint8_t resetPIN = 0, uint8_t bootloadPIN = 0)  : GenericTransceiver(serial){ _resetPIN = resetPIN; _bootloadPIN = bootloadPIN; };
    void begin();
    
};

#endif

