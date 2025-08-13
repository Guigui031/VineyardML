#ifndef RADIO_H
#define RADIO_H

#include "common.h"

// LoRa Functions
void initRadio();
void setupTransmitter();
void setupReceiver();
bool transmitData(String data);
void startReceiving();

// Interrupt handler for received packets
void rx();

// Data parsing functions
void parseReceivedData(String data);

// Create data packet to transmit
String createDataPacket();

#endif
