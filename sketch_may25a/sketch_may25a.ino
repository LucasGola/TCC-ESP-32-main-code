// Defines Globais


// Bibliotecas
#include <ESP8266WiFi.h>
#include <String.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <HTTPClient.h>


//Variáveis
char ssid[] = "haven"; // Nome Wi-Fi
char pass[] = "Gloom23@01"; // Senha Wi-Fi

int percent;
StaticJsonDocument<200> doc; // Cria um objeto JsonDocument

// Pinos
#define UMIDADE_SOLO A0
#define DHT11 D0
#define RELE_BOMBA D1
#define NIVEL_AGUA D2
#define RELE_VALVULA D3
#define MEDIDOR_VAZAO D4

void valueToPercent() {
  int analogValue = analogRead(UMIDADE_SOLO);
  percent = map(analogValue, 490, 1000, 100, 0); // TO:DO Ajustar esses valores para melhor interpretação da umidade
}

void sendSensor() {
  // TO:DO Criar código para enviar o status dos sensores 
}

void sendEvent() {
  String water = "O reservatório está ";
  String bomb = "A bomba está ";
  String humidity = "O nível de umidade do solo é: ";

  water.concat((digitalRead(NIVEL_AGUA) == 1) ? "cheio" : "vazio");
  bomb.concat((!digitalRead(RELE) == 1) ? "ligada" : "desligada");
  humidity.concat(percent);
  humidity.concat("%");

// TO:DO Criar código de envio de eventos
}

getData() {
  if ((WiFi.status() == WL_CONNECTED)) { //Verifica a conexão WiFi

    HTTPClient http;

    http.begin("http://your-server.com/api"); //Inicia a conexão com a URL
    int httpCode = http.GET();     
    delay(2000)                                  

    if (httpCode > 0) { //Verifica o código de status da resposta HTTP
      String payload = http.getString();
      DeserializationError error = deserializeJson(doc, payload); // Analisa a resposta

      if (error) {
        Serial.print(F("deserializeJson() falhou: "));
        Serial.println(error.f_str());
        return;
      }

      Serial.println(doc);
      http.end(); //Encerra a conexão
      return "ok";
    }

    else {
      Serial.println("Erro na requisição HTTP");
      http.end(); //Encerra a conexão
      return
    }
  }
}

// ###################################################################
void setup() {
  pinMode(RELE_BOMBA, OUTPUT);
  pinMode(RELE_VALVULA, OUTPUT);
  pinMode(NIVEL_AGUA, INPUT);

  // Conexão Wifi
  Serial.begin(9600);
  delay(5000)
  Wifi.begin(ssid, pass)

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }

  Serial.println("Conectado ao WiFi");
  // WiFi conectado


  timer.setInterval(5000, sendSensor);
  timer.setInterval(1000, sendEvent);
}

void loop() {
  valueToPercent();

  timer.run();

  Serial.print("Analógico:");
  Serial.print(percent);
  Serial.print("%");
  Serial.println(" ");

  const string requestVerify = getData();

  if (requestVerify == "ok") {
    const string idealSoilHumidity = doc["umidadeSolo"];
    const string idealAirHumidity = doc["umidadeAr"];
    const string idealWeather = doc["temperatura"]
    const string irrigationFrequency = doc["frequenciaDeIrrigacao"];
    const time_t lastIrrigation = parseTime(doc["ultimaIrrigacao"]);
    const time_t irrigationInterval = doc["irrigationInterval"]
    const time_t currentDate = now()
    
    const long diference = abs(lastIrrigation - currentDate)


    if (percent < idealSoilHumidity && /* TO:DO receber temperatura */ <= /* TO:DO Pesquisar clima ideal */) {
      if (digitalRead(NIVEL_AGUA) == 1) {
        Serial.println("Irrigando Planta");
        digitalWrite(RELE_BOMBA, LOW);
        sendEvent()
        sendSensor()
        while (percent < idealSoilHumidity) {
          valueToPercent();
          delay(30000); // Delay de 30s
        }
        
        Serial.println("Planta irrigada!");
        digitalWrite(RELE_BOMBA, HIGH);
        sendEvent()
        sendSensor()
        delay(900000); // Delay de 15m
      } else {
        Serial.println("Irrigando Planta");
        digitalWrite(RELE_VALVULA, LOW);
        sendEvent()
        sendSensor()
        while (percent < idealSoilHumidity) {
          valueToPercent();
          delay(30000); // Delay de 30s
        }
        
        Serial.println("Planta irrigada!");
        digitalWrite(RELE_VALVULA, HIGH);
        sendEvent()
        sendSensor()
        delay(900000); // Delay de 15m
      }
    } else {
      Serial.println("Não é necessário irrigar");
      delay(900000); // Delay de 15m
    }
  }  
}
