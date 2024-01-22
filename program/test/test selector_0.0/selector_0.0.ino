#pragma once
#include <Arduino.h>
#include "debug.h"
#include <SPI.h>
#include <Wire.h>
#include "iarduino_RTC.h"  // Подключаем библиотеку iarduino_RTC для работы с модулями реального времени.
#include <Ethernet2.h>
#include <NTPClient.h>
#include "ICMPPing.h"
#include "SD.h"
#include <ArduinoJson.h>

#include "log.h"
#include "conf.h"
#include "common.h"
#include "counters.h"
#include "bufPing.h"
#include "web.h"

// PING_REQUEST_TIMEOUT_MS -- timeout in ms.  between 1 and 65000 or so
// save values: 1000 to 5000, say.
#define PING_REQUEST_TIMEOUT_MS 2500
#ifndef ICMPPING_ASYNCH_ENABLE
#error "Asynchronous functions only available if ICMPPING_ASYNCH_ENABLE is defined -- see ICMPPing.h"
#endif

#define MAX_ERROR_TIME_SRV 23  // максимальное количество ошибок при обращении к тайм серверу после которого считается что синхронизация времени потеряна и идет запись в лог, если попытка раз в час, то запись об ошибке после 24 часов потери синхронизации

iarduino_RTC watch(RTC_DS1302, PIN_RST, CLK, DAT);  // Объявляем объект watch для работы с RTC модулем на базе чипа DS1302

// инициализируем контроллер Ethernet
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

//  ?? временно прямое назначение , в рабочей создавать пустые и присвоение в setuo
/*
  IPAddress ip(10, 140, 33, 147);// собственный алрес Ethernet интерфейса
  IPAddress ip_dns(10, 140, 33, 1);
  IPAddress gateway(10, 140, 33, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress pingAddr(10, 140, 33, 1); // 111); // ip address to ping  мой комп не хочет пинговаться - адрес 111 ??? почему, а шлюз пингует
*/
IPAddress TS(89,109,251,21);

SOCKET pingSocket = 0;

ICMPPing ping(pingSocket, 1);
ICMPEchoReply echoResult;  // результат ответа по пингу

EthernetServer server(80);
EthernetClient client;  // переменная для подключенного к вэб интерфейсу клиента

EthernetUDP ntpUDP; // создаем экземпляр Udp 

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP, TS);
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).

bool flagEmptyLine;   // ??? временно для пробы простого вэб сервера

bool WaitAnswer;  // признак что пинг отправлен и ждем ответа
bool BusyWebServer;  // вэб сервер занят обработкой  запроса от клиента к интерфейсу конфигурации
bool buttonPressed;  // признак, что кнопка была нажата на прошлом цикле
int vRAM;  //  объем ОЗУ для кучи и стека, используется для контроля утечки памяти
int  instantRAM;
uint32_t num_cycle;
bool largeT; // флаг превышения времени выполнения основного цикла
uint16_t errorTimeSrv; // счетчик ошибок обращения к тайм серверу

File root;  // файловая переменная для корневой директории

DeserializationError errJSON;  // тип ошибки 

//==================================   ОПРЕДЕЛЕНИЯ ФУНКЦИЙ  ==============================================

void applyChangeConfig();  // функция проверки изменений конфига и их обработки, использует как глобальные переменные config и webConfig

void setup() {
  // инициализируем глобальные переменные
  WaitAnswer = false;  // признак что пинг отправлен и ждем ответа
  BusyWebServer = false;  // вэб сервер занят обработкой  запроса от клиента к интерфейсу конфигурации
  buttonPressed = false;  // признак, что кнопка была нажата на прошлом цикле
  num_cycle = 0;
  largeT = false; // флаг превышения времени выполнения основного цикла
  errorTimeSrv = 0; // счетчик ошибок обращения к тайм серверу

  // инициализируем входы/выходы
  pinMode(LED_LAN_A, OUTPUT);
  digitalWrite(LED_LAN_A, LED_OFF);
  pinMode(LED_LAN_B, OUTPUT);
  digitalWrite(LED_LAN_B, LED_OFF);
  pinMode(LED_PING, OUTPUT);
  digitalWrite(LED_PING, LED_OFF);
  pinMode(LED_AUTO, OUTPUT);
  digitalWrite(LED_AUTO, LED_OFF);
  pinMode(OUT_K, OUTPUT);
  digitalWrite(OUT_K, LAN_A_ON);

  pinMode(CS_W5500, OUTPUT);
  digitalWrite( CS_W5500, HIGH);
  pinMode(CS_SD_CARD, OUTPUT);
  digitalWrite(CS_SD_CARD, HIGH);
  pinMode(W5500_RESET, OUTPUT);
  digitalWrite(W5500_RESET, HIGH);
  
  pinMode(SW_RESET_CONF, INPUT);
  pinMode(SW_RESET_CONF, INPUT_PULLUP);
  pinMode(SW_SWITCH, INPUT);
  pinMode(SW_SWITCH, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("start Serial");
  
  watch.begin();    // Инициируем работу с модулем RTC.
    // читаем время от системных часов в Setup
  D.Date_Time = watch.gettime("Y-m-d H:i:s");

  // инициализируем SD card
  //  ?????   в основной проге делать так - SD.begin(CS_SD_CARD);
  bool b1= SD.begin(CS_SD_CARD);
  _LOOK("SD-",b1);

  Log.writeMsg( F("start") );  //сообщение о старте контроллера

  // устанавливаем настройки по умолчанию
  setDefaultConfig(); 
  //если старт с нажатой кнопкой, то перезаписываем настройки по умолчанию
  if( !digitalRead( SW_RESET_CONF ) ){
    Log.writeMsg( F("rst_conf") );  //сообщение в лог о перезаписи setup  на параметры по умолчанию по нажатию на кнопук reset
    File cfgFile = SD.open( CONFIG_FILE, FILE_WRITE);    
    serializeJsonPretty(config, cfgFile);    // Write a prettified JSON document to the file
    cfgFile.close();
  };

// считываем настройки из файла
File cfgFile = SD.open( CONFIG_FILE );
// если файла нет, то пишем дефолтный
if( !cfgFile ){
    cfgFile.close();  // на всякий случай 
    Log.writeMsg( F("not_conf") );  //сообщение в лог об отсутствии конфиг файла и его перезаписи
    File cfgFile = SD.open( CONFIG_FILE, FILE_WRITE);    
    serializeJsonPretty(config, cfgFile);    // Write a prettified JSON document to the file
    cfgFile.close();
    // файл уже не перечитываем, т.к. по умолчанию конфиг установлен
}
else{  // файл есть
    errJSON = deserializeJson(config, cfgFile);  // считываем конфиг из файла
    if (errJSON) {  // если ошибка в конфиге, то ...
      Log.writeMsg( F("err_conf") );//  сообщение в лог о перезаписи ошибочного setup  
      File cfgFile = SD.open( CONFIG_FILE, FILE_WRITE);    
      serializeJsonPretty(config, cfgFile);    // перезаписываем в файл настройки по умолчанию ( установлены выше )
      cfgFile.close();
    };
};

_LOOK("after load","--FILE--");
_LOOK("IP-",config[IP].as<const char*>() ); 
_LOOK("maskIP-",config[maskIP].as<const char*>() ); 
_LOOK("pingIP-",config[pingIP].as<const char*>() ); 
_LOOK("gatewayIP-",config[gatewayIP].as<const char*>() ); 
_LOOK("timePing-",config[timePing].as<uint16_t>() );  
_LOOK("timeoutPing-", config[timeoutPing].as<uint16_t>() );  
_LOOK("maxLosesFromSent-", config[maxLosesFromSent].as<uint16_t>());    
_LOOK("numPingSent-",config[numPingSent].as<uint16_t>());  
_LOOK("maxLostPing-",config[maxLostPing].as<uint8_t>() );  
_LOOK("delayBackSwitch-",config[delayBackSwitch].as<uint16_t>());  
_LOOK("delayReturnA-",config[delayReturnA].as<uint32_t>());  
_LOOK("stepDelay-",config[stepDelay].as<uint32_t>());  
_LOOK("maxDelayReturnA-",config[maxDelayReturnA].as<uint32_t>());  
_LOOK("timeServerIP-",config[timeServerIP].as<const char*>() );  
_LOOK("portTimeServer-",config[portTimeServer].as<uint16_t>());      
_LOOK("date-",config[date].as<const char*>() );   
_LOOK("time-",config[time].as<const char*>() );   
_LOOK("date-",config[login].as<const char*>() );   
_LOOK("time-",config[password].as<const char*>() );   
    
_HALT


  // тестируем индикаторы включением на 1 секунду всего
  digitalWrite(LED_LAN_A, LED_ON);
  digitalWrite(LED_LAN_B, LED_ON);
  digitalWrite(LED_PING, LED_ON);
  digitalWrite(LED_AUTO, LED_ON);
  delay(1000);
  digitalWrite(LED_LAN_A, LED_OFF);
  digitalWrite(LED_LAN_B, LED_OFF);
  digitalWrite(LED_PING, LED_OFF);
  digitalWrite(LED_AUTO, LED_OFF);

  // инициализируем контроллер Ethernet  
  Ethernet.init(CS_W5500); // - ?? вроде не нужно
  
  //Ethernet.begin(mac, ip, ip_dns, gateway, subnet);  //  такой вариант работал  
  Ethernet.begin(mac, charsToIP(config[IP].as<const char*>()), charsToIP(config[gatewayIP].as<const char*>()), charsToIP(config[gatewayIP].as<const char*>()), charsToIP(config[maskIP].as<const char*>()) );  // считаем что localDNS совпадает с gatewayIP
  // Ethernet.begin(mac, charsToIP(config[IP) );  так работало до добавления gateway
  server.begin();
/*
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());
  
  String TS_IP = config[timeServerIP]; 
  TS_IP = "89.109.251.21";  // ntp1.vniiftri.ru временно ??
  // timeClient.setPoolServerName( TS_IP.c_str() ); // устанавливаем адрес сервера времени
  timeClient.begin(); // config[portTimeServer ); ?? // активация клиента времени

  //  ??? что делать ? этот таймер перестраиваемый  returnA = 3 
  counters.load( Tping, config[timePing].as<uint16_t>() * 1000 );// настраиваем период повторения пинга (переводим в мс)
  counters.load( SyncTimeSrv, 3600 * 1000 );  // период обновления времени с тайм сервера - 1 час
  counters.load( LockSwitch, config[delayBackSwitch].as<uint16_t>() * 1000);
  counters.load( returnA, config[delayReturnA].as<uint32_t>() * 1000);
  counters.load( timeOutWeb, 1 * 1000);  // тайм аут обслуживания клиента 1 сек
  // не используется counters.load( oneHour, 3600 * 1000);  // таймер на 1 час
  counters.load( press3s, 3 * 1000);  // таймер на 3 сек - длительное нажатие на кнопку

  counters.resetAll();  // первичный сброс всех счетчиков, рестарт произойдет в основном цикле после срабатывания условий
/* 
  // ??  убрать после отладки ICMPPing::setTimeout(PING_REQUEST_TIMEOUT_MS);  // надо ставить таймаут из конфига.
  ICMPPing::setTimeout( config[timeoutPing] ); 
  ping.asyncStart(charsToIP(config[pingIP]), 1, echoResult); //послать первый запрос 
  //  замер количества свободной памяти
  vRAM = memoryFree(); // запоминаем количество свободного места в ОЗУ  
*/
  _LOOK("ram-", vRAM)

 _LOOK("","end setup")

}

void loop() {
 
}

void applyChangeConfig(){  // функция проверки изменений конфига и их обработки, использует как глобальные переменные config и webConfig
 
}
