/*
   Подключение внешних сигналов:

   Подключение ЦАП MCP4725 12-бит (1 LSB = VCC Voltage / 4096):
      OUT – A0
      GND – Gnd
      VCC – nodeMCU 3.3v
      SCL – nodeMCU GPIO5 (D1)
      SDA – nodeMCU GPIO4 (D2)

      Подключение других элементов (для платы типа nodeMCU ESP8266):
    - GPIO2  (D4) - голубой wifi светодиод;
    - GPIO14 (D5) - красный светодиод;
    - GPIO12 (D6) - кнопка запуска с настройками сети по умолчанию;

      Напряжение батареи: верхний предел 100% - 4,2В - 780 единиц на АЦП
                          нижний предел  0%   - 3,7В - 668 единиц на АЦП

*/

//#define DEBUG 1

#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>    //https://github.com/Links2004/arduinoWebSockets
#include <ArduinoJson.h>         /*https://github.com/bblanchon/ArduinoJson 
                                   https://arduinojson.org/?utm_source=meta&utm_medium=library.properties */

#define GPIO_LED_WIFI 2      // номер пина светодиода GPIO2 (D4)
#define GPIO_LED_RED 14      // номер пина красного светодиода GPIO14 (D5)
#define GPIO_BUTTON 12       // номер пина кнопки GPIO12 (D6) 

#define FILE_NETWORK "/net.txt"         //Имя файла для сохранения настроек сети
#define DEVICE_TYPE "esplink_75v_"
#define TIMEOUT_T_broadcastTXT 100000   //таймаут отправки скоростных сообщений T_broadcastTXT, мкс
#define TIME_BAT_VOLT 5000              //периодичность измерения напряжения батареи, мс
#define BAT_HIGH_VOLT 93400             //100% - 4,16В - 934 единиц на АЦП, умноженное на 100
#define BAT_LOW_VOLT 77800              //0%   - 3,46В - 778 единиц на АЦП, умноженное на 100
#define BAT_DELTA_1_VOLT 156            //разница напряжения в 1% - 0,007В - 1,56 единиц на АЦП, умноженное на 100

#define DEFAULT_AP_NAME "ESP"           //имя точки доступа запускаемой по кнопке
#define DEFAULT_AP_PASS "11111111"      //пароль для точки доступа запускаемой по кнопке

bool wifiAP_mode = 0;
bool static_IP = 0;
byte ip[4] = {192, 168, 1, 43};
byte sbnt[4] = {255, 255, 255, 0};
byte gtw[4] = {192, 168, 1, 1};
char *p_ssid = new char[0];
char *p_password = new char[0];
char *p_ssidAP = new char[0];
char *p_passwordAP = new char[0];

bool sendSpeedDataEnable[] = {0, 0, 0, 0, 0};
String ping = "ping";
unsigned int speedT = 500;  //период отправки данных, миллисек

unsigned int unity = 0;
unsigned int unity_old = 0;
bool outState = 0;
bool outState_old = 0;

bool dataUpdateBit = false;

unsigned int startTimeBatVolt = 0;         //вспом. для TIME_BAT_VOLT
unsigned int startTimeRedLedBlink = 0;     //вспом. для времени мигания красного LED

int vBat = 0;                      //переменная для значения на аналоговом входе
int vBatPercent = 0;                      //переменная для значения напряжения на аналоговом входе в процентах


WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
WiFiClient espClient;
WiFiUDP ntpUDP;
Adafruit_MCP4725 MCP4725;


void setup() {
  Serial.begin(115200);
  MCP4725.begin(0x60);
  MCP4725.setVoltage(0, false);
  Serial.println("\n");

  pinMode(GPIO_LED_WIFI, OUTPUT);
  digitalWrite(GPIO_LED_WIFI, HIGH);

  pinMode(GPIO_LED_RED, OUTPUT);
  digitalWrite(GPIO_LED_RED, HIGH);

  pinMode(GPIO_BUTTON, INPUT_PULLUP);

  SPIFFS.begin();

#ifdef DEBUG
  printChipInfo();
  scanAllFile();
  printFile(FILE_NETWORK);
#endif


  //Запуск точки доступа с параметрами поумолчанию
  if ( !loadFile(FILE_NETWORK) ||  digitalRead(GPIO_BUTTON) == 0)  startAp(DEFAULT_AP_NAME, DEFAULT_AP_PASS);
  //Запуск точки доступа
  else if (digitalRead(GPIO_BUTTON) == 1 && wifiAP_mode == 1)   startAp(p_ssidAP, p_passwordAP);
  //Запуск подключения клиента к точке доступа
  else if (digitalRead(GPIO_BUTTON) == 1 && wifiAP_mode == 0) {
    if (WiFi.getPersistent() == true)    WiFi.persistent(false);
    WiFi.softAPdisconnect(true);
    WiFi.persistent(true);
    if (WiFi.SSID() != p_ssid || WiFi.psk() != p_password) {
      Serial.println(F("\nCHANGE password or ssid"));
      WiFi.disconnect();
      WiFi.begin(p_ssid, p_password);
    }
    if (static_IP == 1) {
      set_staticIP();
    }
  }

  webServer_init();      //инициализация HTTP сервера
  webSocket_init();      //инициализация webSocket сервера

  delay(100);
  readVoltageBat();
}



void loop() {
  wifi_init();
  webSocket.loop();
  server.handleClient();
  MDNS.update();

  //Периодическое считывание напряжения на аналоговом входе
  if (millis() - startTimeBatVolt > TIME_BAT_VOLT) {
    readVoltageBat();
    startTimeBatVolt = millis();
  }

  //Мигание красным LED, если напряжение батареи ниже 30% и ниже 10%
  if (vBatPercent < 30 && vBatPercent >= 10) {
    if (millis() - startTimeRedLedBlink > 500) {
      digitalWrite(GPIO_LED_RED, !digitalRead(GPIO_LED_RED));
      startTimeRedLedBlink = millis();
    }
  } else if (vBatPercent < 10) {
    if (millis() - startTimeRedLedBlink > 150) {
      digitalWrite(GPIO_LED_RED, !digitalRead(GPIO_LED_RED));
      startTimeRedLedBlink = millis();
    }
  } else {
    digitalWrite(GPIO_LED_RED, HIGH);
  }



  if (outState == 0 && outState != outState_old) {
    MCP4725.setVoltage(0, false);
    outState_old = outState;
    unity_old = 0;
#ifdef DEBUG
    Serial.println("Set output Voltage OFF");
#endif
  }


  if (outState == 1 && unity != unity_old ) {
    MCP4725.setVoltage(unity, false);
    outState_old = outState;
    unity_old = unity;
#ifdef DEBUG
    Serial.print("Set output Voltage unity: ");
    Serial.println(unity);
#endif
  }



  //Отправка Speed данных клиентам при условии что данныее обновились и клиенты подключены
  if (dataUpdateBit == 1) {
    if (sendSpeedDataEnable[0] || sendSpeedDataEnable[1] || sendSpeedDataEnable[2] || sendSpeedDataEnable[3] || sendSpeedDataEnable[4] ) {
      String data = serializationToJson_index();
      int startT_broadcastTXT = micros();
#ifdef DEBUG
      Serial.print("\nwebSocket.broadcastTXT: ");
      Serial.println(data);
#endif
      webSocket.broadcastTXT(data);
      int T_broadcastTXT = micros() - startT_broadcastTXT;
      if (T_broadcastTXT > TIMEOUT_T_broadcastTXT)  checkPing();
    }
    dataUpdateBit = 0;
  }

}


//Функция считывания напряжения на аналоговом входе и пересчет в проценты
void readVoltageBat() {
  vBat = analogRead(A0) * 100;
  if (vBat < BAT_LOW_VOLT)   vBat = BAT_LOW_VOLT;
  vBatPercent = (vBat - BAT_LOW_VOLT) / BAT_DELTA_1_VOLT;
  dataUpdateBit = 1;
#ifdef DEBUG
  Serial.print("vBat=");   Serial.println(vBat);
  Serial.print("vBatPercent=");   Serial.println(vBatPercent);
#endif
}



