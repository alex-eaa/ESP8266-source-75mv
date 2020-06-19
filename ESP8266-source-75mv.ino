/*
   Добавил файловый менеджер
   Добавил обновление через вебстраницу "Системные настройки"
   Автозапуск АР Default, если не найден файл настроек
   Подключение внешних сигналов:
    - GPIO14 (D5) - вход датчика движения №1;
    - GPIO5 (D1) - вход датчика движения №2;
    - GPIO16 (D0) - выход управления реле.
   Подключение внутренних элементов (для платы типа ESP8266 Witty):
    - GPIO2 (D4) - голубой wifi светодиод;
    - GPIO4 (D2) - кнопка, подключена к пину (для платы типа ESP8266 Witty);
    - GPIO12 (D6) - зеленый цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO13 (D7) - синий цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO15 (D8) - красный цвет RGB-светодиода (для платы типа ESP8266 Witty).
*/
#define DEBUG 1

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>    //https://github.com/Links2004/arduinoWebSockets
#include <ArduinoJson.h>         /*https://github.com/bblanchon/ArduinoJson 
                                   https://arduinojson.org/?utm_source=meta&utm_medium=library.properties */

#define GPIO_LED_WIFI 2     // номер пина светодиода GPIO2 (D4)
#define GPIO_LED_RED 15     // пин, красного светодиода 
#define GPIO_LED_GREEN 12   // пин, зеленого светодиода 
#define GPIO_LED_BLUE 13    // пин, синего светодиода
#define GPIO_BUTTON 4       // номер пина кнопки GPIO4 (D2)
#define GPIO_SENSOR1 14     // пин, вход датчика движения №1
#define GPIO_SENSOR2 5      // пин, вход датчика движения №2
#define GPIO_RELAY 16       // пин, выход управления реле

#define FILE_MCP4725 "/mcp4725.txt"     //Имя файла для сохранения настроек и статистики РЕЛЕ
#define FILE_NETWORK "/net.txt"         //Имя файла для сохранения настроек сети

#define DEVICE_TYPE "esplink_ms_"
#define TIMEOUT_T_broadcastTXT 100000   //таймаут отправки скоростных сообщений T_broadcastTXT, мкс

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
unsigned int speedT = 200;  //период отправки данных, миллисек

unsigned int unity = 0;
bool dataUpdateBit = false;


WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
WiFiClient espClient;
WiFiUDP ntpUDP;


void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  pinMode(GPIO_LED_WIFI, OUTPUT);
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
  digitalWrite(GPIO_LED_WIFI, HIGH);

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
}



void loop() {
  wifi_init();
  webSocket.loop();
  server.handleClient();
  MDNS.update();


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



