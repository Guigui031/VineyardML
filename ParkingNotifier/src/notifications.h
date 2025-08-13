#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "common.h"

// WiFi and Telegram functions
void connectwifi();
void connectTelegramBot();
void handleNewMessages(int numNewMessages);
void processTelegramCommands();

// Notification functions
void sendCarDetectionNotification();
void sendStatusUpdate();

#endif