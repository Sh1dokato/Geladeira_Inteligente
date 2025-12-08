#include <WiFi.h>              // Biblioteca para conexão Wi-Fi
#include <WebServer.h>         // Servidor Web embutido no ESP32
#include <ESP32Servo.h>        // Controle do servo motor no ESP32
#include "index.h"             // Página HTML armazenada em variável
#include <ArduinoJson.h>       // Manipulação de JSON
#include "time.h"              // Biblioteca para pegar data/hora via NTP

// ==============================
// === CONFIGURAÇÕES DE PINOS ===
// ==============================
const uint8_t PIN_TRIG    = 5;   // Trigger do sensor ultrassônico
const uint8_t PIN_ECHO    = 6;   // Echo do ultrassônico
const uint8_t PIN_BUZZER  = 22;  // Buzzer para alerta sonoro
const uint8_t PIN_SERVO   = 23;  // Servo para despejar a ração
Servo servo;                     // Objeto Servo

// ==============================
// === NTP PARA HORARIO ATUAL ===
// ==============================
const char* ntpServer = "pool.ntp.org";   // Servidor NTP para pegar horário
const long gmtOffset_sec = -10800;        // Fuso horário GMT-3 (Brasil)
short int ultimoMinutoExecutado[4] = { -1, -1, -1, -1 };  // Evita repetição de rotinas no mesmo minuto

// Função que imprime data e hora local no Serial Monitor
void printLocalDateTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Não foi possível obter o tempo.");
    return;
  }

  char timeStringBuff[64];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeInfo);
  Serial.println(timeStringBuff);
}

// =====================================
// === STRUCTS DE CONFIGS DO SISTEMA ===
// =====================================

// Representa uma rotina diária programada
struct Rotina {
    unsigned short int hora;
    unsigned short int minuto;
    bool ativo;
};

// Configurações gerais do sistema
struct Config {
    unsigned short int nivel_despejo;  // Tempo que o servo deve despejar ração
    Rotina rotinas[4];                 // Até 4 horários programados
};

struct Rotina rotinas[4];
struct Config sConfig;

// ===============
// ==== WI-FI ====
// ===============

const char* SSID     = "ESP";          // Nome da rede Wi-Fi
const char* PASSWORD = "123456esp";    // Senha da rede

WebServer server(80);  // Cria servidor web na porta 80

// Função que conecta ao Wi-Fi e inicializa horário NTP
void configurarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

  WiFi.disconnect(true);
  delay(1000);

  Serial.print("\nConectando ao WiFi...");
  WiFi.begin(SSID, PASSWORD);

  // Espera conexão
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Configura horário
  configTime(gmtOffset_sec, 0, ntpServer);
  printLocalDateTime();
}

// ======================================================
// ===   CONFIGURAÇÕES DAS AÇÕES DE CADA COMPONENTE   ===
// ======================================================

// Função que mede distância usando o sensor ultrassônico
long medirDistancia() {
  digitalWrite(PIN_TRIG, LOW); 
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH);  // Tempo do eco

  long distancia = duracao / 58;           // Converte para cm

  Serial.print("Distância:");
  Serial.println(distancia);

  return distancia;
}

// Aciona o servo para despejar ração
void despejarRacao () {
  if(!sConfig.nivel_despejo || sConfig.nivel_despejo == 0){
    Serial.println("Despejo negado, insira um nivel de despejo");
  }
  else {
    Serial.print("Despejando ração. Nível de despejo: ");
    Serial.println(sConfig.nivel_despejo);

    somChamarAtencao();           // Toca alerta antes do servo
    servo.write(170);             // Abre o compartimento
    delay(sConfig.nivel_despejo * 1000); // Tempo baseado no nível configurado
    servo.write(0);               // Fecha a porta de ração
  }
}

// Sons de aviso no buzzer
void somChamarAtencao() {
    for (uint8_t i = 0; i < 2; i++){
      tone(PIN_BUZZER, 300); delay(100); noTone(PIN_BUZZER); delay(100);
      tone(PIN_BUZZER, 500); delay(100); noTone(PIN_BUZZER); delay(100);
      tone(PIN_BUZZER, 600); delay(100); noTone(PIN_BUZZER); delay(100);
      tone(PIN_BUZZER, 400); delay(100); noTone(PIN_BUZZER); delay(100);
      delay(800);
    }
    delay(1200);
}

// ===============================
// ===   MÓDULO: PÁGINAS WEB   ===
// ===============================

// Página principal
void handleRoot() {
  Serial.println("Acessando página principal");
  server.send(200, "text/html", index_html);
}

// Caso rota não exista
void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// Retorna a próxima rotina programada
String pegarProximaRotina() {
    struct tm timeInfo;
    getLocalTime(&timeInfo);

    int horaAtual = timeInfo.tm_hour;
    int minutoAtual = timeInfo.tm_min;

    int menorDiferenca = 1440;
    String proxima = "Nenhuma programada";

    for (int i = 0; i < 4; i++) {
        Rotina r = sConfig.rotinas[i];
        if (!r.ativo) continue;

        int atual = horaAtual * 60 + minutoAtual;
        int rotinaMin = r.hora * 60 + r.minuto;

        int diff = rotinaMin - atual;
        if (diff < 0) diff += 1440;

        if (diff < menorDiferenca) {
            menorDiferenca = diff;

            char buffer[6];
            sprintf(buffer, "%02d:%02d", r.hora, r.minuto);
            proxima = buffer;
        }
    }

    return proxima;
}

// Endpoint /status → retorna JSON com dados do sistema
void handleStatus() {
    StaticJsonDocument<300> doc;

    doc["distancia"] = medirDistancia();
    doc["nivelDespejo"] = sConfig.nivel_despejo;
    doc["proximaRefeicao"] = pegarProximaRotina();

    JsonArray rot = doc.createNestedArray("rotinas");
    for (int i = 0; i < 4; i++) {
        JsonObject obj = rot.createNestedObject();
        obj["hora"]   = sConfig.rotinas[i].hora;
        obj["minuto"] = sConfig.rotinas[i].minuto;
        obj["ativo"]  = sConfig.rotinas[i].ativo;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);

    server.send(200, "application/json", jsonStr);
}

void handleDespejoManual(){
    Serial.println("Despejo manual solicitado...");
    despejarRacao();
}

// Endpoint /salvar → recebe JSON com configurações
void handleJSON() {
    Serial.println("Recebendo JSON via POST");

    String jsonString = server.arg("plain");
    Serial.println(jsonString);

    StaticJsonDocument<512> doc;

    if (deserializeJson(doc, jsonString)) {
        server.send(500, "text/plain", "Erro ao processar JSON");
        return;
    }

    // Atualiza nível do servo
    if (doc.containsKey("nivelDespejo")) {
        sConfig.nivel_despejo = doc["nivelDespejo"];
    }

    // Atualiza rotinas
    if (doc.containsKey("rotina")) {
        JsonArray rotinaArray = doc["rotina"];
        int i = 0;

        for (JsonObject obj : rotinaArray) {
            if (i >= 4) break;

            rotinas[i].hora = obj["hora"];
            rotinas[i].minuto = obj["minuto"];
            rotinas[i].ativo = obj["ativo"];
            sConfig.rotinas[i] = rotinas[i];

            i++;
        }
    }

    server.send(200, "text/plain", "Ok!");
}

// Configura rotas do servidor
void configurarRotas() {
  server.on("/", handleRoot);
  server.on("/salvar", handleJSON);
  server.on("/status", handleStatus);
  server.on("/despejoManual", handleDespejoManual);
  server.onNotFound(handleNotFound);
}

// =====================================
// === EXECUÇÃO DAS ROTINAS AUTOMÁTICAS ===
// =====================================

void verificarRotinas() {
    struct tm timeInfo;
    getLocalTime(&timeInfo);

    int horaAtual = timeInfo.tm_hour;
    int minutoAtual = timeInfo.tm_min;

    for (int i = 0; i < 4; i++) {
        Rotina r = sConfig.rotinas[i];

        if (!r.ativo) continue;

        // Se agora é o horário da rotina
        if (r.hora == horaAtual && r.minuto == minutoAtual) {

            // Evita repetir dentro do mesmo minuto
            if (ultimoMinutoExecutado[i] == minutoAtual) continue;

            Serial.printf("Executando rotina %d\n", i);
            despejarRacao();

            ultimoMinutoExecutado[i] = minutoAtual;
        }
    }
}

// =============
// === SETUP ===
// =============

void setup() {
  Serial.begin(115200);

  servo.attach(PIN_SERVO, 544, 2400);
  servo.write(0);  // Fecha porta do dispenser

  configurarWiFi();
  delay(2000);

  configurarRotas();
  server.begin();

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  Serial.println("Servidor Web iniciado!");
}

// ============
// === LOOP ===
// ============

void loop() {
  server.handleClient();   // Processa requisições HTTP
  verificarRotinas();      // Verifica se é hora de despejar ração
  delay(10);
}
