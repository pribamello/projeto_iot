#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ST_HW_HC_SR04.h> //sensor de distância  

//Configurações do MQTT
#define MQTT_HOST "wpk18t.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:wpk18t:ESP8266:dev01"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "Pribamello_42"
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"  //dados devolvidos
#define MQTT_TOPIC_ITERATION "iot-2/cmd/iteration/fmt/json" //dados recebidos

#define RGB_PIN D4 // Pino do LED
#define TRG_PIN D5 //Pino do sinal enviado (trigger)
#define ECHO_PIN D6 //Pino do sinal de retorno (echo)

#define NEOPIXEL_TYPE NEO_GRB + NEO_KHZ800 //Definições do LED

//Distancias que definem a cor do LED (em centímetros)
#define ALARM_CLOSE 10.0
#define WARN_CLOSE 30.0

//Configurações do WiFi
char ssid[] = "bia"; //Nome da rede
char pass[] = "tsugihagi"; //Senha da rede

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, RGB_PIN, NEOPIXEL_TYPE);

//Objetos MQTT
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

//Variáveis para transmitir os dados
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
StaticJsonDocument<100> jsonReceiveDoc;
static char msg[60];

//Define variáveis
float tempo, dist;
unsigned char r = 0; //valores da cor do LED 
unsigned char g = 0;

//////////////////////////////////////////////
//Variáveis usadas na parte que calcula a velocidade do som
float tempotemp;
float vsom, vsomtemp;
float distpreset = 0.0; //Distância a ser fornecida pelo usuario para calcular a velocidade do som
int i; //Medidas da velocidade do som
int imax = 30;
//////////////////////////////////////////////

// Função que recebe os comandos da nuvem
////////////Por enquanto alterei para receber o numero de iterações apenas
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");
  
  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string
  Serial.println((char *)payload);
  DeserializationError err = deserializeJson(jsonReceiveDoc, (char *)payload);
  if (err) {
    Serial.print(F("deserializeJson() failed with code ")); 
    Serial.println(err.c_str());
  } else {
    JsonObject cmdData = jsonReceiveDoc.as<JsonObject>();
    if (0 == strcmp(topic, MQTT_TOPIC_ITERATION)) {
      //valid message received
      //imax = cmdData["Iteracoes"].as<int32_t>(); // this form allows you specify the type of the data you want from the JSON object
      //Serial.print("Número de iterações foi modificado:");
      distpreset = float(cmdData["Distance"].as<int32_t>()); // this form allows you specify the type of the data you want from the JSON object
      //distpreset = cmdData["Distance"].as<float_t>() //OLHA AQUI OÓÓ, NÃO SEI SE ASSIM FUNCIONA, TESTE NO SENSOR
      //Serial.print("A distância escolhida foi modificada: ");
      Serial.println(distpreset);
      jsonReceiveDoc.clear();
    } else {
      Serial.println("Unknown command received");
    }
  }
}



void setup() {
 
  Serial.begin(115200);
  while(!Serial) {};
  Serial.println();
  Serial.println("ESP8266 Sensor Application");

  //Inicia conexão WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Conectado");
  pixel.begin();
  
 // Connect to MQTT - IBM Watson IoT Platform
  while(! mqtt.connected()){
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) { // Token Authentication
      Serial.println("MQTT Conectado");
      mqtt.subscribe(MQTT_TOPIC_ITERATION);
    } else {
      Serial.println("MQTT Failed to connect! ... retrying");
      delay(500);
    }
  }
}

void loop() {
  //Inicia conexão do MQTT
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_ITERATION);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }

  ST_HW_HC_SR04 ultrasonicSensor(TRG_PIN,ECHO_PIN);
  ultrasonicSensor.setTimeout(20000);
  tempo = ultrasonicSensor.getHitTime();
  dist = tempo/29.1; //dist = tempo x velocidade_som
 /////////Colocar condiçao de erro
  status["Distância"] = dist;
  //jsonDoc["Tempo"] = tempo;
 
//Ajuste de cores do LED
  r = (dist <= ALARM_CLOSE) ? 255 : ((dist < WARN_CLOSE) ? dist/ALARM_CLOSE*255 : 0);
  g = (dist < WARN_CLOSE) ? ((dist > ALARM_CLOSE) ? dist/WARN_CLOSE*255 : 0) : 255;
  pixel.setPixelColor(0,r,g,0);
  pixel.show();

//ATENÇÃO: EXCLUIR ESSA PARTE!!!!!!!!
//distpreset = 30.0; //Exluir essa linha pois a distância será fornecida pelo app
//ATENÇÃO: EXCLUIR ESSA PARTE!!!!!!!!

//////////////////////////////////////////////
  if(distpreset>0.01 && imax != 0){
    i = 0; //zera o contador
    vsomtemp = 0;
    vsom = 0; //zera a velocidade
   
    while(i<imax){
      tempotemp = ultrasonicSensor.getHitTime();
      if(tempotemp!=0){ //se o valor obtido do hitTime for zero, ele pula essa parte e refaz a medida
        vsomtemp = distpreset/tempotemp;
        vsom += vsomtemp;
        i+=1;
        }
      }
     
    //tempotemp = ultrasonicSensor.getHitTime();      
    //vsom = distpreset/tempotemp;
    vsom = 10000*vsom/float(imax);//retorna o resultado em m/s
    status["Vsom"] = vsom;
    }
//////////////////////////////////////////////

  serializeJson(jsonDoc, msg, 60);
  Serial.println(msg);
  if (!mqtt.publish(MQTT_TOPIC, msg)) {
    Serial.println("MQTT Publish failed");
  }
  Serial.print("Número de iteraçoes:");
  Serial.print(imax);
  Serial.println();
  delay(1000);
  //delay(10000);
}
