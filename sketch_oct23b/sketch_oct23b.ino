#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
// #include <Blynk.h>
#include <vector>
#include <time.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define HIGROMETRO 32
#define RELE_BOMBA_CHUVA 23
#define RELE_BOMBA_CASA 22
#define RELE_SOLENOIDE 4
#define NIVEL_AGUA_CHUVA 21
#define NIVEL_AGUA_CASA 19
#define MEDIDOR_VAZAO 5

#define BLYNK_TEMPLATE_ID "TMPL2hqh699u4"
#define BLYNK_TEMPLATE_NAME "TCC"
#define BLYNK_AUTH_TOKEN "9TmbVVxjlXIbbzY3Q9wkBPpSTKh5Az-D"

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Hallan2.4GZ";
const char* password = "36120174";
int connectionControl = 0;

const char* serverURL = "http://192.168.15.82:3000";

volatile int pulseCount = 0;
float flowRate = 0.0;
unsigned long oldTime = 0;
int curentPercent = 0;

// Lista para armazenar objetos JSON
std::vector<String> pendingRequests;

int percent = 0;
float totalFlow = 0.0;
float humidity = 0.0;
float temperature = 0.0;

time_t lastIrrigation;

int minWaterPercent = 30;
int idealWaterPercent = 50;
float maxTemperatureClimate = 20.0;
float minTemperatureClimate = 10;
float irrigationFrequency = 0;

void pulseCounter() {
  pulseCount++;
}

void valueToPercent() {
  int analogValue = analogRead(HIGROMETRO);
  curentPercent = map(analogValue, 1700, 4095, 100, 0);
}

// TO:DO Criar função de evento que vai informar se a planta foi ou não irrigada
bool irrigationIntervalVerify() {

}
// void sendIrrigationEvent() // TO:DO

void sendSensorsDataToAPI() {
  if (WiFi.status() == WL_CONNECTED) { // Verifica se está conectado ao Wi-Fi
    HTTPClient http;
    http.begin(String(serverURL) + "/sensors/register-measurement");
    http.addHeader("Content-Type", "application/json");

    // Se houver pedidos pendentes, envie-os primeiro
    for (const auto& requestBody : pendingRequests) {
      int httpResponseCode = http.POST(requestBody);
      if (httpResponseCode > 0) {
        Serial.println("Pedido pendente enviado com sucesso!");
        Serial.println(httpResponseCode);   // Código de resposta do servidor
        Serial.println(http.getString());   // Resposta do servidor
      } else {
        Serial.print("Erro na requisição HTTP pendente: ");
        Serial.println(httpResponseCode);
        break;  // Se falhar, pare de tentar enviar os pedidos pendentes
      }
    }
    // Limpa a lista de pedidos pendentes
    pendingRequests.clear();

    // Cria o objeto JSON atual
    StaticJsonDocument<200> doc;
    doc["hygrometer"] = percent;
    doc["waterFlow"] = totalFlow;
    doc["dht11Humidity"] = humidity;
    doc["dht11Temperature"] = temperature;

    String requestBody;
    serializeJson(doc, requestBody);

    // Envia o pedido atual
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      Serial.println(httpResponseCode);   // Código de resposta do servidor
      Serial.println(http.getString());   // Resposta do servidor
    } else {
      Serial.print("Erro na requisição HTTP: ");
      Serial.println(httpResponseCode);
      // Salva o pedido atual na lista de pendentes
      pendingRequests.push_back(requestBody);
    }

    http.end();
  } else {
    Serial.println("Erro na conexão Wi-Fi");
    // Cria o objeto JSON atual
    StaticJsonDocument<200> doc;
    doc["hygrometer"] = percent;
    doc["waterFlow"] = totalFlow;
    doc["dht11Humidity"] = humidity;
    doc["dht11Temperature"] = temperature;

    String requestBody;
    serializeJson(doc, requestBody);
    // Salva o pedido atual na lista de pendentes
    pendingRequests.push_back(requestBody);
  }
}

void getData() {
  HTTPClient http;
  http.begin(String(serverURL) + "/plants/water-and-tempreature-info");

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) { 
    String payload = http.getString();
    Serial.println(httpResponseCode);   // Código de resposta do servidor
    Serial.println(payload);            // Resposta do servidor

    // Processa a resposta JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      minWaterPercent = doc["data"]["minWaterPercent"];
      idealWaterPercent = doc["data"]["idealWaterPercent"];
      maxTemperatureClimate = doc["data"]["maxTemperatureClimate"];
      minTemperatureClimate = doc["data"]["minTemperatureClimate"];
      // irrigationFrequency = doc["data"]["irrigationFrequency"]; // TO:DO

      // Exibe os novos valores
      Serial.print("minWaterPercent: ");
      Serial.println(minWaterPercent);
      Serial.print("idealWaterPercent: ");
      Serial.println(idealWaterPercent);
      Serial.print("maxTemperatureClimate: ");
      Serial.println(maxTemperatureClimate);
      Serial.print("minTemperatureClimate: ");
      Serial.println(minTemperatureClimate);
    } else {
      Serial.println("Erro ao analisar JSON");
    }
  } else {
    Serial.print("Erro na requisição HTTP: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void wifiConnect() {
  while (connectionControl < 5 && WiFi.status() != WL_CONNECTED) {
    Serial.println("Conectando ao Wi-fi...");
    connectionControl++;
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    connectionControl = 0;
    Serial.println("Conectado ao Wi-fi");
  } else {
    connectionControl = 0;
    Serial.println("Não foi possível se conectar ao Wi-fi");
  }
}

void setup() {
  pinMode(HIGROMETRO, INPUT);
  pinMode(RELE_BOMBA_CHUVA, OUTPUT);
  pinMode(RELE_BOMBA_CASA, OUTPUT);
  pinMode(RELE_SOLENOIDE, OUTPUT);
  pinMode(NIVEL_AGUA_CHUVA, INPUT);
  pinMode(NIVEL_AGUA_CASA, INPUT); 
  pinMode(MEDIDOR_VAZAO, INPUT);

  attachInterrupt(digitalPinToInterrupt(MEDIDOR_VAZAO), pulseCounter, FALLING);

  digitalWrite(RELE_BOMBA_CHUVA, HIGH);
  digitalWrite(RELE_BOMBA_CASA, HIGH);
  digitalWrite(RELE_SOLENOIDE, HIGH);
  delay(1000);

  Serial.begin(115200);
  delay(1000);
  dht.begin();
  delay(1000);
  WiFi.begin(ssid, password);
  delay(1000);

  wifiConnect();
  if (WiFi.status() == WL_CONNECTED) {
    getData();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  }

  if (WiFi.status() == WL_CONNECTED) {
    getData();
  }

  valueToPercent();
  int analogValue = analogRead(HIGROMETRO);       // Este código é o mesmo da função"valueToPercent() mas essa função é chamada
  percent = map(analogValue, 1700, 4095, 100, 0); // Várias vezes ao longo do cóodigo e precisamos salvar a primeira medição
  
  delay(1500);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  delay(500);

  if (isnan(temperature)) {
    Serial.println("Erro DHT11");
  }
  Serial.print("Medição Higrômetro: ");
  Serial.print(analogRead(HIGROMETRO));
  Serial.print("   ");
  Serial.print("Umidade do solo: ");
  Serial.print(curentPercent);
  Serial.print("%   ");
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.print(" °C   ");
  Serial.print("Umidade do ar: ");
  Serial.print(humidity);
  Serial.println("%");
  delay(1000);

  if (curentPercent <= minWaterPercent && temperature > minTemperatureClimate && temperature < maxTemperatureClimate) {  // TO:DO adicionar verificação de período desde a última rega
    if (digitalRead(NIVEL_AGUA_CHUVA) < 1) {
      Serial.println("Irrigando com água da chuva.");

      digitalWrite(RELE_BOMBA_CHUVA, LOW);
      while (curentPercent <= idealWaterPercent && digitalRead(NIVEL_AGUA_CHUVA) < 1) {
        valueToPercent();
        delay(1000);
      }
      digitalWrite(RELE_BOMBA_CHUVA, HIGH);
    } else {
      Serial.println("Sem água");
      if (digitalRead(NIVEL_AGUA_CASA) >= 1) {
        digitalWrite(RELE_SOLENOIDE, LOW);

        while (digitalRead(NIVEL_AGUA_CASA) >= 1) {
          if ((millis() - oldTime) > 1000) { // calcula a cada segundo
            detachInterrupt(digitalPinToInterrupt(MEDIDOR_VAZAO));
            
            // TO:DO ajustar valores. Testar na prática o medidor de vazão.
            flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / 7.5;
            oldTime = millis();
            pulseCount = 0;

            // Adiciona a vazão calculada ao acumulador totalFlow
            totalFlow += flowRate / 60.0; // converte L/min para Litros por segundo
            
            Serial.print("Vazão: ");
            Serial.print(flowRate);
            Serial.print(" L/min   ");

            Serial.print("Total de água: ");
            Serial.print(totalFlow);
            Serial.println(" Litros");
            
            attachInterrupt(digitalPinToInterrupt(MEDIDOR_VAZAO), pulseCounter, FALLING);
          }

          delay(500);
        }

        Serial.print("Total final de água: ");
        Serial.print(totalFlow);
        Serial.println(" Litros");

        digitalWrite(RELE_SOLENOIDE, HIGH);
      }

      Serial.println("Irrigando com água encanada.");
      digitalWrite(RELE_BOMBA_CASA, LOW);
      while (curentPercent <= idealWaterPercent && digitalRead(NIVEL_AGUA_CASA) < 1) {
        valueToPercent();
        delay(1000);
      }
      digitalWrite(RELE_BOMBA_CASA, HIGH);
     
    }
  }

  sendSensorsDataToAPI();

  delay(15000);
}
