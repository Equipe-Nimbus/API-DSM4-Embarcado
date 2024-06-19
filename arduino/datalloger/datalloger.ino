#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#include "Grove_Temperature_And_Humidity_Sensor.h"

#define DHTTYPE DHT11
#define DHTPIN 14
#define LUMINOSIDADEPIN 4

DHT dht(DHTPIN, DHTTYPE);

String uidPlaca; 

char *ssid = "Nome Rede";
char *pwd = "Senha Rede";

WiFiClient wclient;
HTTPClient http_post;

char *nptServer = "br.pool.ntp.org";
long gmtOffset = 0;
int daylight = 0;
time_t now;
struct tm timeinfo;

String servidorRecepcao = "<IP-Rede+Url-Aplicacao>";

TaskHandle_t tTask1;
TaskHandle_t tTask2;
TaskHandle_t tTask3;

SemaphoreHandle_t mutex;

float temperatura = 0.0;
float umidade = 0.0;
float bateria = 100.0;
float lux = 0.00;
int contador = 1;

void minhaTask1 (void *pvPrametes) {
  Serial.println("Iniciando a Task1");
  while(true) {
    xSemaphoreTake(mutex, portMAX_DELAY);

    analogReadResolution(10);

    float volts = analogRead(LUMINOSIDADEPIN) * 5 / 1024.0;
    float amps = volts / 1000.0;
    float microamps = amps * 1000000;
    lux = microamps * 2.0;

    xSemaphoreGive(mutex);
    delay(5000);
  } 
}

void minhaTask2 (void *pvPrametes) {
  Serial.println("Iniciando a Task2");
  while(true) {
    xSemaphoreTake(mutex, portMAX_DELAY);

    float temp_hum_val[2] = {0};

    if (!dht.readTempAndHumidity(temp_hum_val)) {
      umidade = temp_hum_val[0];
      temperatura = temp_hum_val[1];
    } else {
        Serial.println("Failed to get temprature and humidity value.");
    }

    xSemaphoreGive(mutex);
    delay(5000);
  } 
}

void minhaTask3 (void *pvPrametes) {
  Serial.println("Iniciando a Task3");
  while(true) {
    xSemaphoreTake(mutex, portMAX_DELAY);

    bateria = bateria - 1.0;

    xSemaphoreGive(mutex);
    delay(1000);
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
  uidPlaca.replace(":", "");
  WiFi.begin(ssid, pwd);
  connectWiFi();
  configTime(gmtOffset, daylight, nptServer);
  Wire.begin();
  dht.begin();

  pinMode(LUMINOSIDADEPIN, INPUT);
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

    xTaskCreatePinnedToCore(
      minhaTask2,
      "MinhaTask2",
      10000,
      NULL,
      1,
      &tTask2,
      1
    );

    xTaskCreatePinnedToCore(
      minhaTask3,
      "MinhaTask3",
      10000,
      NULL,
      1,
      &tTask3,
      0
    );
  }
}

void loop() {
  
  if (WiFi.status() == WL_CONNECTED) {
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Erro ao coletar data/hora");
    }
    xSemaphoreTake(mutex, portMAX_DELAY);
    xSemaphoreGive(mutex);

    http_post.begin(wclient, servidorRecepcao);
    http_post.addHeader("Content-Type", "application/json");

    String data = "{\"idPlacaEstacao\":\"" + uidPlaca + "\",\"unix\":" + time(&now) + ",\"bateria\":" + bateria + ",\"temperatura\":" + temperatura + ",\"umidade\":" + umidade + ",\"luminosidade\":" + lux + "}";

    int http_get_status = http_post.POST(data.c_str());

    Serial.println("");
    Serial.println("contador: ");
    Serial.println(contador);
    Serial.println(data);
    Serial.println(http_get_status);
    if (http_get_status >= 0) {
      Serial.println("");
      Serial.println(data);
      Serial.println(http_post.getString());
      contador = contador + 1;
    } else {
      Serial.println("Erro ao executar o GET");
    }
  } else {
    Serial.println("Problema na internet");
    connectWiFi();
  }
  delay(10000);

}
