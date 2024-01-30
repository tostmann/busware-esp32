

#include "busware.h"
#include "TCPBridge.h"

static void handleData(void *arg, AsyncClient *client, void *data, size_t len) {
    Serial.printf("data received %d bytes from client %s \r\n", len, client->remoteIP().toString().c_str());

    TCPBridge *bridge = (TCPBridge*)(arg);
    
    bridge->transceiver->write( (uint8_t *)data, len ) ;
    bridge->BytesIn += len;
    /*
    if (ledmode)
	digitalWrite(LED_BUILTIN, LOW);
    previousMillis = millis();
    */
}

static void handleError(void *arg, AsyncClient *client, int8_t error) {
    Serial.printf("connection error %s from client %s \r\n", client->errorToString(error), client->remoteIP().toString().c_str());
}

static void handleDisconnect(void *arg, AsyncClient *client) {
    Serial.printf("client %s disconnected \r\n", client->remoteIP().toString().c_str());

    TCPBridge *bridge = (TCPBridge*)(arg);

    for(uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) 
	if (bridge->clients[i] == client)
	    bridge->clients[i] = NULL;
}

static void handleTimeOut(void *arg, AsyncClient *client, uint32_t time) {
    Serial.printf("client ACK timeout ip: %s \r\n", client->remoteIP().toString().c_str());
}

static void handleNewClient(void *arg, AsyncClient *client) {
    Serial.printf("new client has been connected to server, ip: %s\r\n", client->remoteIP().toString().c_str());

    TCPBridge *bridge = (TCPBridge*)(arg);

    for(uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) {
	
	if (bridge->clients[i] == NULL) {
	    bridge->clients[i] = client;
	    // register events
	    client->onData(&handleData, bridge);
	    client->onError(&handleError, bridge);
	    client->onDisconnect(&handleDisconnect, bridge);
	    client->onTimeout(&handleTimeOut, bridge);
	    client->setNoDelay(true);
	    return;
	    
	}
    }

    Serial.println("no free slots - rejecting ...");
    client->close();
}

TCPBridge::TCPBridge(uint16_t port, GenericTransceiver *trans) : AsyncServer(port) {
    transceiver = trans;
    BytesIn  = 0;
    BytesOut = 0;
    this->onClient(&handleNewClient, this);

}

#define MAXBUF 1024

void TCPBridge::loop() {
    uint8_t  sbuf[MAXBUF], i;
    uint16_t av = transceiver->available();

    if (av > 0) {
	if (av > MAXBUF) av = MAXBUF;

	// check if all sockets can take this data ...
	int16_t min = minspace();
	if (min<0 || min>av) {
	    
	    transceiver->readBytes( sbuf, av );
	    
	    if (min>av) {
		BytesOut += av;
		
		for(i = 0; i < MAX_SRV_CLIENTS; i++){
		    if (clients[i] && clients[i]->connected()){
			clients[i]->add((char *)sbuf, av);
			clients[i]->send();
			delay(1);
		    }
		}
	    }
	    /*	    
	    if (ledmode)
		digitalWrite(LED_BUILTIN, LOW);
	    previousMillis = currentMillis;
	    */
	}
    }
}

size_t TCPBridge::minspace() {
    int16_t min = -1;
    for(uint8_t i = 0; i < MAX_SRV_CLIENTS; i++) {
	if (clients[i] == NULL)
	    continue;
	if (min<0 || clients[i]->space()<min)
	    min = clients[i]->space();
    }

    //    Serial.println( min );
    return min;
}

void TCPBridge::begin() {
    AsyncServer::begin();
    memset( clients, 0, sizeof(clients) );
}
