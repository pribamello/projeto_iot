//O sinal enviado é o trigger (pino D5) o sinal de retorno é o ECHO (pino D6)

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ST_HW_HC_SR04.h> //sensor de distância

// --------------------------------------------------------------------------------------------
//        UPDATE CONFIGURATION TO MATCH YOUR ENVIRONMENT
// --------------------------------------------------------------------------------------------

// Watson IoT connection details
#define MQTT_HOST "wpk18t.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:wpk18t:ESP8266:dev01"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "Pribamello_42"
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"
#define MQTT_TOPIC_INTERVAL "iot-2/cmd/interval/fmt/json"

// Add GPIO pins used to connect devices
#define RGB_PIN D4 // GPIO pin the data line of RGB LED is connected to
#define TRG_PIN D5 //Pino do sinal enviado
#define ECHO_PIN D6 //Pino do sinal de retorno

#define NEOPIXEL_TYPE NEO_GRB + NEO_KHZ800

// Temperatures to set LED by (assume temp in C)
#define ALARM_COLD 0.0 //modificar para ler distâncias
#define ALARM_HOT 30.0
#define WARN_COLD 10.0
#define WARN_HOT 25.0

// Add WiFi connection information
char ssid[] = "cloud-iot-ufrj";  // your network SSID (name)
char pass[] = "ufrj-ibm-cloud";  // your network password

// --------------------------------------------------------------------------------------------
//        SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// --------------------------------------------------------------------------------------------
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, RGB_PIN, NEOPIXEL_TYPE);

///////////
ST_HW_HC_SR04 ultrasonicSensor(TRG_PIN,ECHO_PIN); //define o sensor
ultrasonicSensor.setTimeout(20000); //tempo de espera do sinal ECHO
//////////

// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

// variables to hold data
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
StaticJsonDocument<100> jsonReceiveDoc;
static char msg[50];


//////////////////////////////////
float time //Define o valor tempo para o sensor
float dist //
//////////////////////////////////


unsigned char r = 0; // LED RED value
unsigned char g = 0; // LED Green value
unsigned char b = 0; // LED Blue value
int32_t ReportingInterval = 10;  // Reporting Interval seconds

void callback(char* topic, byte* payload, unsigned int length) {
   // handle message arrived
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
    if (0 == strcmp(topic, MQTT_TOPIC_INTERVAL)) {
      //valid message received
      ReportingInterval = cmdData["Interval"].as<int32_t>(); // this form allows you specify the type of the data you want from the JSON object
      Serial.print("Reporting Interval has been changed:");
      Serial.println(ReportingInterval);
      jsonReceiveDoc.clear();
    } else {
      Serial.println("Unknown command received");
    }
  }
}


void setup() {
  // Start serial console
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
  Serial.println();
  Serial.println("ESP8266 Sensor Application");

  // Start WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");

  // Start connected devices
  dht.begin();
  pixel.begin();
  
  // Connect to MQTT - IBM Watson IoT Platform
   while(! mqtt.connected()){
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) { // Token Authentication
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_INTERVAL);
    } else {
      Serial.println("MQTT Failed to connect! ... retrying");
      delay(500);
    }
  }
}


void loop() {
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_INTERVAL);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
    


///////////////////////////
  tempo = ultrasonicSensor.getHitTime();
  jsonDoc["tempo"] = tempo;
  dist = tempo/29.1 // tempo x velocidade
  serializeJson(jsonDoc, msg, 50);
  Serial.println(msg);
  delay(1000);
///////////////////////////



  // Check if any reads failed and exit early (to try again).
  if (False) {///////////////////////////////////////////////////////////////////////////////////////////////CONFIGURAR ESSA CONDIÇÃO PARA O TIMEOUT DO SENSOR
    Serial.println("Failed to read from ultrasonic sensor!");
  } 
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////MUDAR AQUI A TEMPERATURA PARA DISTANCIA t --> dist
  else {
    // Set RGB LED Colour based on temp
    b = (t < ALARM_COLD) ? 255 : ((t < WARN_COLD) ? 150 : 0);
    r = (t >= ALARM_HOT) ? 255 : ((t > WARN_HOT) ? 150 : 0);
    g = (t > ALARM_COLD) ? ((t <= WARN_HOT) ? 255 : ((t < ALARM_HOT) ? 150 : 0)) : 0;
    pixel.setPixelColor(0, r, g, b);
    pixel.show();

    // Print Message to console in JSON format
    status["Distância"] = dist;
    serializeJson(jsonDoc, msg, 50);
    Serial.println(msg);
    if (!mqtt.publish(MQTT_TOPIC, msg)) {
      Serial.println("MQTT Publish failed");
    }
  }

  Serial.print("ReportingInterval :");
  Serial.print(ReportingInterval);
  Serial.println();
  // Pause - but keep polling MQTT for incoming messages
  for (int32_t i = 0; i < ReportingInterval; i++) {
    mqtt.loop();
    delay(1000);
  }
}
