#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>

String uidPlaca; 

char *ssid = "<NOME-REDE>";
char *pwd = "<SENHA-REDE>";

char *nptServer = "br.pool.ntp.org";
long gmtOffset = 0;
int daylight = 0;
time_t now;
struct tm timeinfo;

String servidorRecepcao = "<IP-REDE+ROTA-SERVIÇO-APLICAÇÃO>";

TaskHandle_t tTask1;

SemaphoreHandle_t mutex;

float temperatura = 0.0;
float umidade = 0.0;
float bateria = 100.0;

void minhaTask1 (void *pvPrametes) {
  Serial.println("Iniciando a Task1");
  while(true) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    temperatura = temperatura + 0.5;
    umidade = umidade + 0.5;
    bateria = bateria - 0.1;
    xSemaphoreGive(mutex);
    delay(10000);
  } 
}

void connectWiFi() {
  Serial.print("Conectando ");
  while(WiFi.status() != WL_CONNECTED)  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado com sucesso, com o IP ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  uidPlaca = WiFi.macAddress();
  uidPlaca.replace(":", " ");
  WiFi.begin(ssid, pwd);
  connectWiFi();
  configTime(gmtOffset, daylight, nptServer);
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Erro ao aceesar o servidor do NTP");
  } else {
    Serial.print("A hora agora eh ");
    Serial.println(time(&now));
    
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL)
      Serial.println("Mutex está nulo");
    
    xTaskCreatePinnedToCore(
      minhaTask1,
      "MinhaTask1",
      10000,
      NULL,
      1,
      &tTask1,
      0
    );
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Erro ao coletar data/hora");
    }

    WiFiClient wclient;
    HTTPClient http_post;

    http_post.begin(wclient, servidorRecepcao);
    http_post.addHeader("Content-Type", "application/json");

    String data = "{\"uuid\":\"" + uidPlaca + "\",\"unix\":" + time(&now) + ",\"bateria\":" + bateria + ",\"temperatura\":" + temperatura + ",\"umidade\":" + umidade + "}";

    int http_get_status = http_post.POST(data.c_str());

    Serial.println("");
    Serial.println(http_get_status);
    if (http_get_status == 200) {
      Serial.println("");
      Serial.println(data);
      Serial.println(http_post.getString());
    } else {
      Serial.println("Erro ao executar o GET");
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    xSemaphoreGive(mutex);
  } else {
    Serial.println("Problema na internet");
    connectWiFi();
  }
  delay(10000);

}
