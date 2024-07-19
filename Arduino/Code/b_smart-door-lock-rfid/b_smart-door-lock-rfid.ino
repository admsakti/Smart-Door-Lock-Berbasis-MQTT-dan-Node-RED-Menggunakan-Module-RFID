// ----- Library -----
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <WiFiManager.h>
#include "WiFi.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>


// ----- Variabel dan pendefinisian objek -----
#define RST_PIN 27
#define SS_PIN  5
#define RELAY 32
#define REDLED 12
#define GREENLED 15
#define BUTTON_PIN 14
#define MAGNETIC 2

// Mendefiniskan keadaan pada Program
#define STATE_STARTUP       0
#define STATE_STARTING      1
#define STATE_WAITING       2
#define STATE_SCAN_INVALID  3
#define STATE_SCAN_VALID    4
#define STATE_SCAN_MASTER   5
#define STATE_ADDED_CARD    6
#define STATE_REMOVED_CARD  7

const int cardArrSize = 10;
const int cardSize = 4;
byte cardArr[cardArrSize][cardSize];
byte masterCard[cardSize] = {194, 214, 255, 44}; // {163, 107, 56, 10} // {0xC2, 0xD6, 0xFF, 0x2C}
byte readCard[cardSize];
byte cardsStored = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* mqtt_server = "10.10.0.191";   // Server MQTT JTI Polije

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char relayMsg[MSG_BUFFER_SIZE];
char userMsg[MSG_BUFFER_SIZE];
int value = 0;

byte currentState = STATE_STARTUP;
unsigned long LastStateChangeTime;
unsigned long StateWaitTime;

unsigned long currentMillis = 0;
unsigned long update_interval = 2000;

bool btTemp = true;
bool magSwTemp = true;

String userTemp;


// ----- Fungsi -----
int readCardState() {
  Serial.print("Data Kartu - ");
  for (int index = 0; index < 4; index++) {
    readCard[index] = mfrc522.uid.uidByte[index];
    Serial.print(readCard[index]);
    if (index < 3) {
      Serial.print(", ");
    }
  }
  Serial.println();

  if (memcmp(readCard, masterCard, 4) == 0) {
    return STATE_SCAN_MASTER;
  }

  if (cardsStored == 0) {
    return STATE_SCAN_INVALID;
  }

  for (int index = 0; index < cardsStored; index++) {
    if (memcmp(readCard, cardArr[index], 4) == 0) {
      return STATE_SCAN_VALID;
    }
  }
  return STATE_SCAN_INVALID;
}

void addReadCard() {
  if (cardsStored < cardArrSize) {
    for (int index = 0; index < 4; index++) {
      cardArr[cardsStored][index] = readCard[index];
    }
    cardsStored++;

    EEPROM.write(0, cardsStored);
    for (int i = 0; i < cardsStored; i++) {
      for (int j = 0; j < 4; j++) {
        EEPROM.write((i * 4) + j + 1, cardArr[i][j]);
      }
    }
    EEPROM.commit();
  }
}

void removeReadCard() {
  if (cardsStored == 0) {
    return;
  }

  boolean found = false;
  int cardIndex = -1;

  for (int index = 0; index < cardsStored; index++) {
    if (memcmp(readCard, cardArr[index], 4) == 0) {
      found = true;
      cardIndex = index;
      break;
    }
  }

  if (found) {
    for (int index = cardIndex; index < cardsStored - 1; index++) {
      for (int j = 0; j < 4; j++) {
        cardArr[index][j] = cardArr[index + 1][j];
      }
    }
    cardsStored--;

    EEPROM.write(0, cardsStored);
    for (int i = 0; i < cardsStored; i++) {
      for (int j = 0; j < 4; j++) {
        EEPROM.write((i * 4) + j + 1, cardArr[i][j]);
      }
    }
    EEPROM.commit();
  }
}

void updateState(byte aState) {
  if (aState == currentState) {
    return;
  }

  switch (aState) {
    case STATE_STARTING:
      StateWaitTime = 1000;
      digitalWrite(REDLED, HIGH);
      digitalWrite(GREENLED, LOW);
      break;
    case STATE_WAITING:
      StateWaitTime = 0;
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, LOW);
      break;
    case STATE_SCAN_INVALID:
      if (currentState == STATE_SCAN_MASTER) {
        addReadCard();
        aState = STATE_ADDED_CARD;
        StateWaitTime = 2000;
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, HIGH);
        Serial.println("Akses Diterima. Kartu Ditambahkan");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Akses Diterima ");
        lcd.setCursor(0, 1);
        lcd.print(" ID Ditambahkan ");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Scan RFID Anda ");
        lcd.setCursor(0, 1);
        lcd.print(" Kartu / Tag :v ");

        for (int index = 0; index < 4; index++) {
          userTemp += readCard[index];
          if (index < 3) {
            userTemp += ", ";
          }
        }
        snprintf (userMsg, MSG_BUFFER_SIZE, userTemp.c_str());
        Serial.print("User add: ");
        Serial.println(userMsg);
        client.publish("b/smart_door_lock/pub/addUser", userMsg);
        userTemp = "";
      } else {
        StateWaitTime = 2000;
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, LOW);
        Serial.println("Akses Ditolak, Kartu Tidak Valid");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Akses  Ditolak ");
        lcd.setCursor(0, 1);
        lcd.print(" ID Tidak Valid ");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Scan RFID Anda ");
        lcd.setCursor(0, 1);
        lcd.print(" Kartu / Tag :v ");
      }
      break;
    case STATE_SCAN_VALID:
      if (currentState == STATE_SCAN_MASTER) {
        removeReadCard();
        aState = STATE_REMOVED_CARD;
        StateWaitTime = 2000;
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, HIGH);
        Serial.println("Akses Diterima, Kartu Dihapus");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Akses Diterima ");
        lcd.setCursor(0, 1);
        lcd.print(" Kartu Dihapus! ");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Scan RFID Anda ");
        lcd.setCursor(0, 1);
        lcd.print(" Kartu / Tag :v ");

        for (int index = 0; index < 4; index++) {
          userTemp += readCard[index];
          if (index < 3) {
            userTemp += ", ";
          }
        }
        snprintf (userMsg, MSG_BUFFER_SIZE, userTemp.c_str());
        Serial.print("User del: ");
        Serial.println(userMsg);
        client.publish("b/smart_door_lock/pub/delUser", userMsg);
        userTemp = "";
      } else {
        StateWaitTime = 2000;
        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, HIGH);
        Serial.println("Akses Diterima, Kartu Valid");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Akses Diterima ");
        lcd.setCursor(0, 1);
        lcd.print(" Pintu Terbuka! ");
        digitalWrite(RELAY, LOW);
        magSwTemp = true;
        btTemp = false;
        delay(200);

        if (magSwTemp && digitalRead(MAGNETIC) == HIGH) {
          delay(4000);
          digitalWrite(RELAY, HIGH);
          btTemp = true;
          magSwTemp = false;
        }

        snprintf (relayMsg, MSG_BUFFER_SIZE, "false");
        Serial.print("Relay status: ");
        Serial.println(relayMsg);
        client.publish("b/smart_door_lock/pub/relay", relayMsg);

        for (int index = 0; index < 4; index++) {
          userTemp += readCard[index];
          if (index < 3) {
            userTemp += ", ";
          }
        }
        snprintf (userMsg, MSG_BUFFER_SIZE, userTemp.c_str());
        Serial.print("Akses oleh: ");
        Serial.println(userMsg);
        client.publish("b/smart_door_lock/pub/akses", userMsg);

        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Scan RFID Anda ");
        lcd.setCursor(0, 1);
        lcd.print(" Kartu / Tag :v ");
        userTemp = "";
      }
      break;
    case STATE_SCAN_MASTER:
      StateWaitTime = 4000;
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, HIGH);
      Serial.println("Akses Diterima, Kartu Admin Terdeteksi");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("  Kartu Admin  ");
      lcd.setCursor(0, 1);
      lcd.print("  Terdeteksi!  ");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Scan RFID Anda ");
      lcd.setCursor(0, 1);
      lcd.print(" Kartu / Tag :v ");
      break;
  }
  currentState = aState;
  LastStateChangeTime = millis();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msgTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msgTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "b/smart_door_lock/sub/relay") {
    if (msgTemp == "false") {
      Serial.println("Pintu Terbuka Dari Node-Red");
      digitalWrite(RELAY, LOW);
      delay(100);
      magSwTemp = true;
      btTemp = false;
    } else {
      Serial.println("Pintu Tertutup Dari Node-Red");
      digitalWrite(RELAY, HIGH);
      delay(100);
      magSwTemp = false;
      btTemp = true;
    }
  }

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to");
    lcd.setCursor(0, 1);
    lcd.print("MQTT Server");
    if (client.connect("ESP32DoorLock")) {
      Serial.println("connected");
      client.publish("b/smart_door_lock/pub/relay", "true");
      client.subscribe("b/smart_door_lock/sub/relay");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Scan RFID Anda ");
      lcd.setCursor(0, 1);
      lcd.print(" Kartu / Tag :v ");
      Serial.println("Tempelkan Kartu Anda Pada RFID...");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  EEPROM.begin(512); // Ukuran EEPROM sesuai kebutuhan

  // inisialisasi wifimanager
  WiFiManager wm;
  // wm.resetSettings();  // for reset

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFiManager =>");
  lcd.setCursor(0, 1);
  lcd.print("Kel_1 - smrtlock");

  bool res;
  res = wm.autoConnect("Kel_1", "smrtlock"); // password protected ap
  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();   // for reset
    // delay(2000);
  }
  else {
    // ESP32 terkoneksi dengan wifi
    Serial.print("Wifi Connected : ");
    Serial.println(WiFi.localIP());
  }

  cardsStored = EEPROM.read(0);
  if (cardsStored > cardArrSize) {
    cardsStored = 0;
  }
  for (int i = 0; i < cardsStored; i++) {
    for (int j = 0; j < 4; j++) {
      cardArr[i][j] = EEPROM.read((i * 4) + j + 1);
    }
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  delay(2000);
  LastStateChangeTime = millis();
  updateState(STATE_STARTING);

  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(MAGNETIC, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);

  delay(5000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int buttonState = digitalRead(BUTTON_PIN);
  int magneticSwitchState = digitalRead(MAGNETIC);

  if (btTemp && buttonState == LOW) {
    Serial.println("Pintu Terbuka Dengan Tombol");
    digitalWrite(RELAY, LOW);
    delay(200);
    magSwTemp = true;
    btTemp = false;
    snprintf (relayMsg, MSG_BUFFER_SIZE, "false");
    Serial.print("Relay status: ");
    Serial.println(relayMsg);
    client.publish("b/smart_door_lock/pub/relay", relayMsg);
  }

  if (magSwTemp && magneticSwitchState == HIGH) {
    Serial.println("Pintu Terkunci Karena Magnetic Switch");
    delay(4000);
    digitalWrite(RELAY, HIGH);
    magSwTemp = false;
    btTemp = true;
    snprintf (relayMsg, MSG_BUFFER_SIZE, "true");
    Serial.print("Relay status: ");
    Serial.println(relayMsg);
    client.publish("b/smart_door_lock/pub/relay", relayMsg);
  }

  byte cardState;

  if ((currentState != STATE_WAITING) &&
      (StateWaitTime > 0) &&
      (LastStateChangeTime + StateWaitTime < millis())) {
    updateState(STATE_WAITING);
  }
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  cardState = readCardState();
  updateState(cardState);
}
