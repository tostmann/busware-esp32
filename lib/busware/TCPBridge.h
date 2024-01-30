#ifndef tcpbridge_h
#define tcpbridge_h

#include <busware.h>
#include <AsyncTCP.h>

#ifndef MAX_SRV_CLIENTS
#define MAX_SRV_CLIENTS 2
#endif

class TCPBridge : public AsyncServer {
    
public:
    TCPBridge(uint16_t port, GenericTransceiver *transceiver);
    void begin();
    void loop();
    
    uint32_t BytesIn, BytesOut;
    GenericTransceiver *transceiver;
    AsyncClient *clients[MAX_SRV_CLIENTS];
    size_t minspace(void);
    uint8_t maxclients = MAX_SRV_CLIENTS;
    
};

#endif
