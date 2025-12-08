#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT11.h>
#include <Wire.h>
#include "index.h"

// =====================================
// ==== CONFIGURAÇÕES DE PINOS =========
// =====================================
#define PIN_DHT    15     // Sensor DHT11
#define PIN_BUZZER 5
#define PIN_TRAVA  13
#define PIN_LED    2

DHT11 dht;   // <-- A SUA BIBLIOTECA É ASSIM!

// =====================================
// === VARIÁVEIS DE ESTADO =============
// =====================================
bool geladeiraTrancada = false;
bool buzzerDesligado  = false;

// =====================================
// === WI-FI ============================
// =====================================
const char* SSID     = "ESP";
const char* PASSWORD = "123456esp";

WebServer server(80);

// ===============================
// === PAGINA PRINCIPAL =========
// ===============================
void handleRoot() {
  server.send(200, "text/html", index_html);
}

// ===============================
// === DESLIGAR BUZZER ==========
// ===============================
void handleDesligaBuzzer() {
  buzzerDesligado = true;
  digitalWrite(PIN_BUZZER, LOW);

  Serial.println("Buzzer desligado manualmente.");
  server.send(200, "text/plain", "Buzzer desligado");
}

// ===============================
// === TRANCAR GELADEIRA ========
// ===============================
void handleTrancar() {
  geladeiraTrancada = true;
  digitalWrite(PIN_TRAVA, HIGH);

  Serial.println("Geladeira TRANCADA.");
  server.send(200, "text/plain", "Trancada");
}

// ===============================
// === DESTRANCAR GELADEIRA =====
// ===============================
void handleDestrancar() {
  geladeiraTrancada = false;
  digitalWrite(PIN_TRAVA, LOW);

  Serial.println("Geladeira DESTRANCADA.");
  server.send(200, "text/plain", "Destrancada");
}

// ===============================
// === STATUS PARA O SITE =======
// ===============================
void handleStatus() {
    StaticJsonDocument<200> doc;

    // === LEITURA CORRETA DO DHT11 (SUA BIBLIOTECA) ===
    dht.read(PIN_DHT);

    int temperature = dht.temperature;
    int humidity    = dht.humidity;

    doc["temp"]  = temperature;
    doc["umid"]  = humidity;
    doc["trancada"] = geladeiraTrancada;
    doc["buzzerDesligado"] = buzzerDesligado;

    String jsonStr;
    serializeJson(doc, jsonStr);

    server.send(200, "application/json", jsonStr);
}

// ===============================
// === ROTA 404 ==================
// ===============================
void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// ===============================
// === CONFIGURAR ROTAS =========
// ===============================
void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/desligaBuzzer", handleDesligaBuzzer);
  server.on("/trancar", handleTrancar);
  server.on("/destrancar", handleDestrancar);
  server.onNotFound(handleNotFound);
}

// ===============================
// === CONFIGURAR WI-FI =========
// ===============================
void configurarWiFi() {
  Serial.println("\nConectando ao WiFi...");
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(700);
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ===============================
// ========== SETUP ==============
// ===============================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_TRAVA, OUTPUT);
  digitalWrite(PIN_TRAVA, LOW);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  configurarWiFi();
  configurarRotas();

  server.begin();
  Serial.println("Servidor iniciado!");
}

// ===============================
// =========== LOOP ==============
// ===============================
void loop() {
  server.handleClient();

  if (!buzzerDesligado && geladeiraTrancada) {
    // digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }
}
