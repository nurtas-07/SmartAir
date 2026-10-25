#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "DHT.h"

// --- НАСТРОЙКИ СЕТИ ---
const char* ssid = "NUrt"; 
const char* password = "281221241"; 

// --- НАСТРОЙКИ TELEGRAM ---
#define BOTtoken "8705356376:AAGpY-nmgwcAffDersNZqmqLicxqGj7Wk8s" 
#define MY_CHAT_ID "6888927793" 

// --- ПИНЫ ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ135_PIN 34
#define MQ7_PIN 35

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
int threshold = 1200;               // Порог тревоги для зажигалки/дыхания
unsigned long lastTriggerTime = 0; 
unsigned long lastBotCheck = 0;
const unsigned long botDelay = 500; 
String myChatId = MY_CHAT_ID;             

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Красивое меню с кнопками
String keyboardJson = "[[\"/status\"], [\"/start\"]]";

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    myChatId = chat_id; 
    String text = bot.messages[i].text;

    if (text == "/status") {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      int m135 = analogRead(MQ135_PIN);
      int m7 = analogRead(MQ7_PIN);

      String msg = "📊 *ОТЧЕТ SMART AIR*:\n\n";
      msg += "🌡 ТЕМПЕРАТУРА: *" + String(t, 1) + "°C*\n";
      msg += "💧 ВЛАЖНОСТЬ: *" + String(h, 1) + "%*\n";
      msg += "───\n";
      msg += "💨 MQ135 (Газы): *" + String(m135) + "*\n";
      msg += "🌫 MQ7 (CO): *" + String(m7) + "*\n";
      msg += "───\n";
      msg += "✅ *Система работает стабильно*";
      
      bot.sendMessageWithReplyKeyboard(chat_id, msg, "Markdown", keyboardJson, true);
    }

    if (text == "/start") {
      String welcome = "🚀 *Система SMART AIR активирована!*\n\n";
      welcome += "Используй кнопки внизу.\n";
      welcome += "Порог срабатывания: *" + String(threshold) + "*";
      bot.sendMessageWithReplyKeyboard(chat_id, welcome, "Markdown", keyboardJson, true);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ7_PIN, INPUT);

  WiFi.begin(ssid, password);
  client.setInsecure(); 

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Ready!");

  // Исправленная строка: используем myChatId
  String startMsg = "⚡️ *СИСТЕМА ОНЛАЙН* ⚡️\n\n";
  startMsg += "✅ *SMART AIR* успешно запущена!\n";
  startMsg += "📡 Подключено к: *" + String(ssid) + "*\n";
  startMsg += "📟 Нажми */status* для проверки.";
  
  bot.sendMessageWithReplyKeyboard(myChatId, startMsg, "Markdown", keyboardJson, true);
}

void loop() {
  // Постоянный мониторинг датчиков
  int v135 = analogRead(MQ135_PIN);
  int v7 = analogRead(MQ7_PIN);

  // Мгновенная реакция на газ
  if ((v135 > threshold || v7 > threshold) && (millis() - lastTriggerTime > 15000)) {
    if (myChatId != "") {
      String alert = "🚨 *ВНИМАНИЕ: ЗАФИКСИРОВАН ГАЗ!* 🚨\n\n";
      alert += "МАКС. УРОВЕНЬ: *" + String(max(v135, v7)) + "*\n";
      alert += "⚠️ *Требуется проверка Smart Air!*";
      
      bot.sendMessageWithReplyKeyboard(myChatId, alert, "Markdown", keyboardJson, true);
      lastTriggerTime = millis();
    }
  }

  // Проверка сообщений от тебя
  if (millis() - lastBotCheck > botDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheck = millis();
  }
}