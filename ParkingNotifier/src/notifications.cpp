#include "notifications.h"

void connectwifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to Wi-Fi:");
    Serial.print(WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.print(" Connected with IP: ");
    Serial.println(WiFi.localIP());
}

void connectTelegramBot() {
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected, attempting to send Telegram message...");
        bool success = bot.sendMessage(Mychat_id, "Bot connecté! \n/help pour afficher les commandes disponibles.", "");
        if (!success) {
            Serial.println("Failed to send message.");
        }
    } else {
        Serial.println("WiFi not connected.");
    }
}

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = bot.messages[i].chat_id;
        String text = bot.messages[i].text;
        bot.sendMessage(chat_id, text, "");
        
        if (text == "/status") {
            bot.sendMessage(chat_id, "Système connecté et en marche.", "");
        } else if (text == "/log") {
            bot.sendMessage(chat_id, 
                "Dernière auto détectée : " + String(timestr) + ".\n"
                "Dernier enregistrement de données : " + String(timeDatastr) + ".\n"
                "Dernière intensité lumineuse enregistrée : " + String(lightValue) + "."
                , ""
            );
        } else if (text == "/data") {
            bot.sendMessage(chat_id, 
                "Données reçues du transmetteur:\n"
                "Dernière intensité lumineuse : " + String(lightValue) + "\n"
                "Dernière distance : " + String(cm) + " cm"
                , ""
            );
        } else if (text == "/off") {
            notificationsEnabled = false;
            bot.sendMessage(chat_id, "Notifications désactivées.", "Markdown");
        } else if (text == "/on") {
            notificationsEnabled = true;
            bot.sendMessage(chat_id, "Notifications activées.", "Markdown");
        } else if (text == "/help") {
            bot.sendMessage(chat_id,
                "*Commandes disponibles:*\n"
                "/status - Affiche l'état du système\n"
                "/log - Affiche l'heure de la dernière auto détectée\n"
                "/data - Affiche les valeurs des différentes sondes présentement\n"
                "/on - Active les notifications lors d'une détection d'auto\n"
                "/off - Désactive les notifications jusqu'à réactivation avec /on. Enregistre tout de même les détections\n"
                "/help - Affiche cette aide\n"
                , "Markdown"
            );
        } else {
            bot.sendMessage(chat_id, "Commande inconnue. Tapez /help pour la liste des commandes.", "");
        }
    }
}

void processTelegramCommands() {
    unsigned long currentMillis = millis();
    if (currentMillis - bot_lasttime > BOT_MTBS) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while (numNewMessages) {
            Serial.println("got response");
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        bot_lasttime = currentMillis;
    }
}

void sendCarDetectionNotification() {
    if (notificationsEnabled) {
        bot.sendMessage(Mychat_id, "Une auto a traversé!", "Markdown");
    }
}

void sendStatusUpdate() {
    String status = "Status Update:\n";
    status += "Battery: " + String(heltec_vbat(), 2) + "V\n";
    status += "Light: " + String(lightValue) + "\n";
    status += "Distance: " + String(cm) + "cm\n";
    status += "Packets sent: " + String(counter);
    
    bot.sendMessage(Mychat_id, status, "");
}