#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
// #include <BlynkSimpleEsp32.h>
#include <vector>
#include <time.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define HIGROMETRO 32
#define RELE_BOMBA_CHUVA 4
#define RELE_BOMBA_CASA 18
#define RELE_SOLENOIDE 22
#define RELE_POWER_SOURCE 23
#define NIVEL_AGUA_CHUVA 2
#define NIVEL_AGUA_CASA 19
#define MEDIDOR_VAZAO 5
#define BATTERY 34

// #define BLYNK_TEMPLATE_ID "TMPL2hqh699u4"
// #define BLYNK_TEMPLATE_NAME "TCC"
// #define BLYNK_AUTH_TOKEN "MVBcS8Gv-cNL_XUaEwenm69fwWYPS5jJ"

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Hallan2.4GZ";
const char* password = "36120174";
int connectionControl = 0;

const char* serverURL = "http://192.168.15.82:3000";

volatile int pulseCount = 0;
float flowRate = 0.0;
unsigned long oldTime = 0;
int curentPercent = 0;
String stringMessage = "";

// Lista para armazenar objetos JSON
std::vector<String> pendingRequestsIrrigation;
std::vector<String> pendingRequestsSensors;

int percent = 0;
float totalFlow = 0.0;
float humidity = 0.0;
float temperature = 0.0;
float batteryPercent = 0.0;

int rainwaterLevel = 0;
int housewaterLevel = 0;


int minWaterPercent = 30;
int idealWaterPercent = 50;
float maxTemperatureClimate = 35.0;
float minTemperatureClimate = 10;
float irrigationFrequency = 0;
time_t lastIrrigation = 0;
time_t now = time(nullptr);
bool irrigationControl;

void pulseCounter() {
  pulseCount++;
}

void valueToPercent() {
  int analogValue = analogRead(HIGROMETRO);
  curentPercent = map(analogValue, 1700, 4095, 100, 0);
}

bool irrigationIntervalVerify() {
  now = time(nullptr);

  if (lastIrrigation == 0 || difftime(now, lastIrrigation) >= irrigationFrequency * 3600){
    irrigationControl = true;
  } else {
    false;
  }
}

/* BLYNK_WRITE(V3) {
  int pumpChuvaState = param.asInt();
  digitalWrite(RELE_BOMBA_CHUVA, pumpChuvaState);
  Serial.print("Estado da bomba de chuva: ");
  Serial.println(pumpChuvaState ? "Ligada" : "Desligada");
}

BLYNK_WRITE(V4) {
  int pumpCasaState = param.asInt();
  digitalWrite(RELE_BOMBA_CASA, pumpCasaState);
  Serial.print("Estado da bomba de casa: ");
  Serial.println(pumpCasaState ? "Ligada" : "Desligada");
}

BLYNK_WRITE(V5) {
  int valveState = param.asInt();
  digitalWrite(RELE_SOLENOIDE, valveState);
  Serial.print("Estado da válvula: ");
  Serial.println(valveState ? "Aberta" : "Fechada");
} */

void sendIrrigationEvent(String action) {
    if (WiFi.status() == WL_CONNECTED) { // Verifica se está conectado ao Wi-Fi
    HTTPClient http;
    http.begin(String(serverURL) + "/events/register");
    http.addHeader("Content-Type", "application/json");

    // Se houver pedidos pendentes, envie-os primeiro
    for (const auto& requestBody : pendingRequestsIrrigation) {
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
    pendingRequestsIrrigation.clear();

    // Cria o objeto JSON atual
    StaticJsonDocument<200> doc;
    doc["action"] = action;
    doc["soilHumidity"] = percent;
    doc["climateTemperature"] = temperature;

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
      pendingRequestsIrrigation.push_back(requestBody);
    }

    http.end();
  } else {
    Serial.println("Erro na conexão Wi-Fi");
    // Cria o objeto JSON atual
    StaticJsonDocument<200> doc;
    doc["action"] = action;
    doc["soilHumidity"] = percent;
    doc["climateTemperature"] = temperature;

    String requestBody;
    serializeJson(doc, requestBody);
    // Salva o pedido atual na lista de pendentes
    pendingRequestsIrrigation.push_back(requestBody);
  }
}

void sendSensorsDataToAPI() {
  if (WiFi.status() == WL_CONNECTED) { // Verifica se está conectado ao Wi-Fi
    HTTPClient http;
    http.begin(String(serverURL) + "/sensors/register-measurement");
    http.addHeader("Content-Type", "application/json");

    // Se houver pedidos pendentes, envie-os primeiro
    for (const auto& requestBody : pendingRequestsSensors) {
      int httpResponseCode = http.POST(requestBody);
      if (httpResponseCode > 0) {
        Serial.println("Pedido pendente enviado com sucesso!");
        Serial.println(httpResponseCode);   // Código de resposta do servidor
        Serial.println(http.getString());   // Resposta do servidor
      } else {
        Serial.print("Erro na requisição HTTP pendente: ");
        Serial.println(httpResponseCode);\
        break;  // Se falhar, pare de tentar enviar os pedidos pendentes
      }
    }
    // Limpa a lista de pedidos pendentes
    pendingRequestsSensors.clear();

    // Cria o objeto JSON atual
    StaticJsonDocument<200> doc;
    doc["hygrometer"] = percent;
    doc["waterFlow"] = totalFlow;
    doc["dht11Humidity"] = humidity;
    doc["dht11Temperature"] = temperature;
    doc["batteryPercent"] = batteryPercent;

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
      pendingRequestsSensors.push_back(requestBody);
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
    doc["batteryPercent"] = batteryPercent;

    String requestBody;
    serializeJson(doc, requestBody);
    // Salva o pedido atual na lista de pendentes
    pendingRequestsSensors.push_back(requestBody);
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
      irrigationFrequency = doc["data"]["irrigationFrequency"];

/*       // Converte a string ISO 8601 para time_t
      const char* lastIrrigationStr = doc["data"]["lastIrrigation"];
      if (strcmp(lastIrrigationStr, "none") == 0) {
        lastIrrigation = 0; // ou qualquer valor padrão que você escolher
      } else {
        struct tm tm = {0}; // Inicializa a struct tm com zeros
        if (strptime(lastIrrigationStr, "%Y-%m-%dT%H:%M:%S", &tm)) {
          lastIrrigation = mktime(&tm);
        }
      } */

      // Exibe os novos valores
      Serial.print("minWaterPercent: ");
      Serial.println(minWaterPercent);
      Serial.print("idealWaterPercent: ");
      Serial.println(idealWaterPercent);
      Serial.print("maxTemperatureClimate: ");
      Serial.println(maxTemperatureClimate);
      Serial.print("minTemperatureClimate: ");
      Serial.println(minTemperatureClimate);
      Serial.print("irrigationFrequency: ");
      Serial.println(irrigationFrequency);
      Serial.print("lastIrrigation: ");
      Serial.println(lastIrrigation);
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

void changePowerSource() {
  int rawValue = analogRead(BATTERY);
  batteryPercent = (rawValue * 100) / 4095.0;

  if (batteryPercent <= 90.0) {
    digitalWrite(RELE_POWER_SOURCE, LOW);
  } else {
    digitalWrite(RELE_POWER_SOURCE, HIGH);
  }

  delay(500);
}

void setup() {
  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  pinMode(HIGROMETRO, INPUT);
  pinMode(RELE_BOMBA_CHUVA, OUTPUT);
  pinMode(RELE_BOMBA_CASA, OUTPUT);
  pinMode(RELE_SOLENOIDE, OUTPUT);
  pinMode(RELE_POWER_SOURCE, OUTPUT);
  pinMode(NIVEL_AGUA_CHUVA, INPUT);
  pinMode(NIVEL_AGUA_CASA, INPUT); 
  pinMode(MEDIDOR_VAZAO, INPUT);
  pinMode(BATTERY, INPUT);

  attachInterrupt(digitalPinToInterrupt(MEDIDOR_VAZAO), pulseCounter, FALLING);

  digitalWrite(RELE_BOMBA_CHUVA, HIGH);
  digitalWrite(RELE_BOMBA_CASA, HIGH);
  digitalWrite(RELE_SOLENOIDE, HIGH);
  digitalWrite(RELE_POWER_SOURCE, HIGH);
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

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Esperando sincronização de tempo...");
  while (!time(nullptr)) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nTempo sincronizado!");

}

void loop() {
  // Blynk.run();

  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
  } else {
    getData();
  }

  changePowerSource();
/*   Blynk.virtualWrite(V6, batteryPercent); // Envia o nível da bateria para o widget V6

  valueToPercent();
  int analogValue = analogRead(HIGROMETRO);       // Este código é o mesmo da função "valueToPercent()" mas essa função é chamada
  percent = map(analogValue, 1700, 4095, 100, 0); // Várias vezes ao longo do cóodigo e precisamos salvar a primeira medição
  // Blynk.virtualWrite(V0, percent); // Envia a umidade do solo para o widget V0
  
  delay(1500);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  delay(500);

/*   Blynk.virtualWrite(V1, temperature); // Envia a temperatura para o widget V1
  Blynk.virtualWrite(V2, humidity); // Envia a umidade para o widget V2 */

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

  // irrigationIntervalVerify();
  if (curentPercent <= minWaterPercent ) {
    if (temperature > minTemperatureClimate) {
      if (temperature < maxTemperatureClimate) {
        /* if (irrigationControl == true) {

        } else {
          sendIrrigationEvent("Não foi possível irrigar a planta pois ainda não passou o intervalo entre irrigações"); // TO:DO
        }*/
        rainwaterLevel = digitalRead(NIVEL_AGUA_CHUVA); 
        // Blynk.virtualWrite(V7, rainwaterLevel); // Envia o nível da água da chuva para o widget V7
        if (rainwaterLevel < 1) {
          Serial.println("Irrigando com água da chuva.");

          digitalWrite(RELE_BOMBA_CHUVA, LOW);
          while (curentPercent <= idealWaterPercent && digitalRead(NIVEL_AGUA_CHUVA) < 1) {
            valueToPercent();
/*             Blynk.virtualWrite(V0, curentPercent); // Envia a umidade do solo para o widget V0
            rainwaterLevel = digitalRead(NIVEL_AGUA_CHUVA); 
            Blynk.virtualWrite(V7, rainwaterLevel); // Envia o nível da água da chuva para o widget V7 */

            delay(1000);
          }
          digitalWrite(RELE_BOMBA_CHUVA, HIGH);

          valueToPercent();
/*           Blynk.virtualWrite(V0, curentPercent); // Envia a umidade do solo para o widget V0
          rainwaterLevel = digitalRead(NIVEL_AGUA_CHUVA); 
          Blynk.virtualWrite(V7, rainwaterLevel); // Envia o nível da água da chuva para o widget V7 */
          
          now = time(nullptr);
          lastIrrigation = now;

          sendIrrigationEvent("Planta irrigada com água da chuva!");
        } else {
          Serial.println("Sem água");
          housewaterLevel = digitalRead(NIVEL_AGUA_CASA);
          // Blynk.virtualWrite(V9, housewaterLevel); // Envia o nível da água da casa para o widget V9
          if (housewaterLevel >= 1) {
            digitalWrite(RELE_SOLENOIDE, LOW);

            while (housewaterLevel >= 1) {
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

                housewaterLevel = digitalRead(NIVEL_AGUA_CASA);
                // Blynk.virtualWrite(V9, housewaterLevel); // Envia o nível da água da casa para o widget V9
              }

              delay(500);
            }

            Serial.print("Total final de água: ");
            Serial.print(totalFlow);
            Serial.println(" Litros");

            digitalWrite(RELE_SOLENOIDE, HIGH);

            housewaterLevel = digitalRead(NIVEL_AGUA_CASA);
            // Blynk.virtualWrite(V9, housewaterLevel); // Envia o nível da água da casa para o widget V9
          }

          Serial.println("Irrigando com água encanada.");
          digitalWrite(RELE_BOMBA_CASA, LOW);

          valueToPercent();
          // Blynk.virtualWrite(V0, curentPercent); // Envia a umidade do solo para o widget V0

          while (curentPercent <= idealWaterPercent && digitalRead(NIVEL_AGUA_CASA) < 1) {
            valueToPercent();
/*             Blynk.virtualWrite(V0, curentPercent); // Envia a umidade do solo para o widget V0
            housewaterLevel = digitalRead(NIVEL_AGUA_CASA);
            Blynk.virtualWrite(V9, housewaterLevel); // Envia o nível da água da casa para o widget V9 */
            delay(1000);
          }
          digitalWrite(RELE_BOMBA_CASA, HIGH);

          valueToPercent();
/*           Blynk.virtualWrite(V0, curentPercent); // Envia a umidade do solo para o widget V0
          housewaterLevel = digitalRead(NIVEL_AGUA_CASA);
          Blynk.virtualWrite(V9, housewaterLevel); // Envia o nível da água da casa para o widget V9 */

          now = time(nullptr);
          lastIrrigation = now;

          sendIrrigationEvent("Planta irrigada com água encanada!");
        }
      } else {
        sendIrrigationEvent("Não foi possível irrigar a planta pois o clima estava muito quente");
      }
    } else {
      sendIrrigationEvent("Não foi possível irrigar a planta pois o clima estava muito frio");
    }
  } else {
    sendIrrigationEvent("Não foi necessário irrigar a planta!");
  }

  sendSensorsDataToAPI();
  delay(5000);
}
