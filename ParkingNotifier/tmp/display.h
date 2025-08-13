#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

// Display initialization and management
void init_display();
void displayTransmitterStatus();
void displayReceiverStatus();

// Transmitter display functions
void show_transmission(int counter, String data);
void show_transmission_fail(int state);
void show_transmission_success();
void displayTransmitterInfo();

// Receiver display functions
void displayReceivedData();
void showDetailedStats();
void displayReceiverInfo();

#endif