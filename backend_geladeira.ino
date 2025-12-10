#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DHT11.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include "index.h"

// =====================================
// ========== PINAGEM ==================
// =====================================
#define PIN_DHT_GELADEIRA 15
#define PIN_DHT_FREEZER   7

#define PIN_BTN_PORTA   20
#define PIN_BTN_FREEZER 21

#define PIN_BUZZER 5
#define PIN_LED    11
#define PIN_TRAVA  13

// RFID
#define RFID_SDA 10
#define RFID_RST 3

// =====================================
// ========== OBJETOS ==================
// =====================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

DHT11 dhtGeladeira(PIN_DHT_GELADEIRA);
DHT11 dhtFreezer(PIN_DHT_FREEZER);

MFRC522 rfid(RFID_SDA, RFID_RST);

WebServer server(80);

// =====================================
// ===== VARIÁVEIS DE CONTROLE =========
// =====================================
bool acessoLiberado = false;
bool geladeiraTrancada = false;
bool buzzerDesligado = false;

unsigned long portaOpenTime = 0;
unsigned long freezerOpenTime = 0;

bool portaOpen = false;
bool freezerOpen = false;

const unsigned long TEMPO_LIMITE = 10000;

byte masterUID[4];
bool masterSet = false;

// =====================================
// ========== WIFI =====================
// =====================================
const char* SSID     = "Fébrikis";
const char* PASSWORD = "minecraftt";

void configurarWiFi() {
  Serial.println("\nConectando ao WiFi...");
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(600);
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =====================================
// ========== ROTAS WEB ================
// =====================================

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleDesligaBuzzer() {
  buzzerDesligado = true;
  digitalWrite(PIN_BUZZER, LOW);

  // ---- Serial Monitor ----
  Serial.println("Buzzer desligado via Web!");

  server.send(200, "text/plain", "OK");
}

void handleTrancar() {
  geladeiraTrancada = true;
  digitalWrite(PIN_TRAVA, HIGH);

  // ---- Serial Monitor ----
  Serial.println("GELADEIRA TRANCADA via Web!");

  server.send(200, "text/plain", "OK");
}

void handleDestrancar() {
  geladeiraTrancada = false;
  digitalWrite(PIN_TRAVA, LOW);

  // ---- Serial Monitor ----
  Serial.println("GELADEIRA DESTRANCADA via Web!");

  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  StaticJsonDocument<200> doc;

  int tGel = dhtGeladeira.readTemperature();
  int hGel = dhtGeladeira.readHumidity();

  if (tGel < 0 || tGel > 100) tGel = 0;
  if (hGel < 0 || hGel > 100) hGel = 0;

  doc["temp"] = tGel;
  doc["umid"] = hGel;
  doc["trancada"] = geladeiraTrancada;
  doc["buzzerDesligado"] = buzzerDesligado;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/desligaBuzzer", handleDesligaBuzzer);
  server.on("/trancar", handleTrancar);
  server.on("/destrancar", handleDestrancar);
}

// =====================================
// ============ SETUP ==================
// =====================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRAVA, OUTPUT);

  pinMode(PIN_BTN_PORTA, INPUT_PULLUP);
  pinMode(PIN_BTN_FREEZER, INPUT_PULLUP);

  // LCD
  Wire.begin(8, 9);  
  lcd.init();
  lcd.backlight();

  lcd.print("Iniciando...");
  delay(1000);
  lcd.clear();

  // RFID
  SPI.begin(6, 22, 4, RFID_SDA);
  rfid.PCD_Init();

  configurarWiFi();
  configurarRotas();
  server.begin();

  Serial.println("Sistema iniciado!");
}

// =====================================
// ============== LOOP =================
// =====================================
void loop() {
  server.handleClient();
  unsigned long agora = millis();

  // RFID
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {

    if (!masterSet) {
      for (byte i = 0; i < 4; i++)
        masterUID[i] = rfid.uid.uidByte[i];

      masterSet = true;
      lcd.clear();
      lcd.print("TAG cadastrada");
      delay(1200);
      lcd.clear();
    }
    else {
      bool match = true;

      for (byte i = 0; i < 4; i++)
        if (masterUID[i] != rfid.uid.uidByte[i])
          match = false;

      if (match) {
        acessoLiberado = !acessoLiberado;

        lcd.clear();
        lcd.print(acessoLiberado ? "Acesso liberado" : "Acesso fechado");
        delay(1200);
        lcd.clear();
      }
    }

    rfid.PICC_HaltA();
  }

  // BOTÕES PORTA / FREEZER
  bool portaBtn = digitalRead(PIN_BTN_PORTA);
  bool freezerBtn = digitalRead(PIN_BTN_FREEZER);

  if (portaBtn == LOW && !portaOpen) {
    portaOpen = true;
    portaOpenTime = agora;
  }
  if (portaBtn == HIGH && portaOpen) {
    portaOpen = false;
  }

  if (freezerBtn == LOW && !freezerOpen) {
    freezerOpen = true;
    freezerOpenTime = agora;
  }
  if (freezerBtn == HIGH && freezerOpen) {
    freezerOpen = false;
  }

  // ALERTA DE PORTA
  bool alerta = false;

  if (portaOpen && agora - portaOpenTime > TEMPO_LIMITE) alerta = true;
  if (freezerOpen && agora - freezerOpenTime > TEMPO_LIMITE) alerta = true;

  if (!buzzerDesligado && alerta)
    digitalWrite(PIN_BUZZER, HIGH);
  else
    digitalWrite(PIN_BUZZER, LOW);

  // LCD - mostra temperaturas
  int tg = dhtGeladeira.readTemperature();
  int tf = dhtFreezer.readTemperature();

  if (tg < 0 || tg > 100) tg = 0;
  if (tf < 0 || tf > 100) tf = 0;

  lcd.setCursor(0, 0);
  lcd.print("F:");
  lcd.print(tf);
  lcd.print(" G:");
  lcd.print(tg);

  lcd.setCursor(0, 1);

  if (portaOpen) {
    lcd.print("Porta ");
    lcd.print((agora - portaOpenTime) / 1000);
    lcd.print("s   ");
  }
  else if (freezerOpen) {
    lcd.print("Freezr ");
    lcd.print((agora - freezerOpenTime) / 1000);
    lcd.print("s   ");
  }
  else {
    lcd.print("Fechado      ");
  }

  delay(200);
}
