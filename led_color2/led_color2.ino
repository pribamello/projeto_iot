//O sinal enviado é o trigger (pino D5) o sinal de retorno é o ECHO (pino D6)

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ST_HW_HC_SR04.h> //sensor de distância  

// Add GPIO pins used to connect devices
#define RGB_PIN D4 // GPIO pin the data line of RGB LED is connected to
#define TRG_PIN D5 //Pino do sinal enviado
#define ECHO_PIN D6 //Pino do sinal de retorno

#define NEOPIXEL_TYPE NEO_GRB + NEO_KHZ800

//Distancias que definem a cor do LED (em centímetros)
#define ALARM_CLOSE 10.0
#define WARN_CLOSE 30.0

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, RGB_PIN, NEOPIXEL_TYPE);

StaticJsonDocument<100> jsonDoc;
static char msg[50];

//Define variáveis
float tempo, dist;
unsigned char r = 0; //LED Red value
unsigned char g = 0; //LED Green value

//////////////////////////////////////////////
//Variáveis usadas na parte que calcula a velocidade do som
float tempotemp;
float vsom, vsomtemp;
float distpreset = 99999999.0; //Distância a ser fornecida pelo usuario para calcular a velocidade do som
int i; //Medidas da velocidade do som
int imax = 30; //pegar da nuvem
//////////////////////////////////////////////


void setup() {
  Serial.begin(115200);
  while(!Serial);
}

void loop() {
  ST_HW_HC_SR04 ultrasonicSensor(TRG_PIN,ECHO_PIN);
  ultrasonicSensor.setTimeout(2000);
  tempo = ultrasonicSensor.getHitTime();
  dist = tempo/29.1; //dist = tempo x velocidade_som
  jsonDoc["Distância"] = dist;
  jsonDoc["Tempo"] = tempo;
// dist < limite vermelho = amarelo
// dist <limite amarelo (warn) = verde
  r = (dist >= ALARM_CLOSE) ? 255 : ((dist > WARN_CLOSE) ? dist/ALARM_CLOSE*255 : 0);
  g = (dist > WARN_CLOSE) ? ((t < ALARM_CLOSE) ? dist/WARN_CLOSE*255 : 0) : 255);
  pixel.setPixelColor(0,r,g,0);
  pixel.show();

//ATENÇÃO: EXCLUIR ESSA PARTE!!!!!!!!
distpreset = 30.0; //Exluir essa linha pois a distância será fornecida pelo app
//ATENÇÃO: EXCLUIR ESSA PARTE!!!!!!!!

//////////////////////////////////////////////
  if(abs(distpreset - 99999999.0)>0.001 && imax != 0){
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
    vsom = 1000*vsom/float(imax);//retorna o resultado em m/s
    jsonDoc["Vsom"] = vsom;
    }
//////////////////////////////////////////////


  serializeJson(jsonDoc, msg, 50);
  Serial.println(msg);
  //delay(10000);
  delay(1000);
}
