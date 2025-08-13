#ifndef SENSORS_H
#define SENSORS_H

#include "common.h"

// Sensor initialization
void initSensors();

// Distance sensor functions
int get_distance();
bool checkCarDetection();

// Light sensor functions
void measureData(int log_counter, bool from_command);
int getLightLevel();

#endif