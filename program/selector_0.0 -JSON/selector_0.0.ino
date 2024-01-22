#pragma once
#include <Arduino.h>
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

#include "debug.h"

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
  bool SD_open = SD.begin(CS_SD_CARD);
  _LOOK(SD_open);

  Log.writeMsg( F("start") );  //сообщение о старте контроллера

_MFREE
  // устанавливаем настройки по умолчанию
  setDefaultConfig( config ); 

_PRN("after set default");
_LOOK(config[PGM IP].as<String>() ); 
_LOOK(config[PGM maskIP].as<String>() ); 
_LOOK(config[PGM pingIP].as<String>() ); 
_LOOK(config[PGM gatewayIP].as<String>() ); 
_LOOK(config[PGM timePing].as<uint16_t>() );  
_LOOK(config[PGM timeoutPing].as<uint16_t>() );  
_LOOK(config[PGM maxLosesFromSent].as<uint16_t>());    
_LOOK(config[PGM numPingSent].as<uint16_t>());  
_LOOK(config[PGM maxLostPing].as<uint8_t>() );  
_LOOK(config[PGM delayBackSwitch].as<uint16_t>());  
_LOOK(config[PGM delayReturnA].as<uint32_t>());  
_LOOK(config[PGM stepDelay].as<uint32_t>());  
_LOOK(config[PGM maxDelayReturnA].as<uint32_t>());  
_LOOK(config[PGM timeServerIP].as<String>() );  
_LOOK(config[PGM portTimeServer].as<uint16_t>());      
_LOOK(config[PGM date].as<String>() );   
_LOOK(config[PGM time].as<String>() );   
_LOOK(config[PGM login].as<String>() );   
_LOOK(config[PGM password].as<String>() );   


  //если старт с нажатой кнопкой, то перезаписываем настройки по умолчанию
  if( !digitalRead( SW_RESET_CONF ) ){
    Log.writeMsg( F("rst_conf") );  //сообщение в лог о перезаписи setup  на параметры по умолчанию по нажатию на кнопук reset
    File cfgFile = SD.open( CONFIG_FILE, FILE_WRITE);    
    serializeJsonPretty(config, cfgFile);    // Write a prettified JSON document to the file
    cfgFile.close();
  };

 // SD.remove( CONFIG_FILE );  // временно для стирания конфигурации на карте и записи конфига по умолчанию
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
_PRN( " open cfg file")
     errJSON = deserializeJson(config, cfgFile);  // считываем конфиг из файла
     //Serial.print(F("deserializeJson() cfgFile failed: "));
     //Serial.println(errJSON.f_str());  
    if (errJSON || !config.containsKey( PGM IP ) || !config.containsKey( PGM maskIP ) ) {  // если ошибка в конфиге, (не удалось прочесть или нет IP адреса ) то ...
      cfgFile.close();
      SD.remove( CONFIG_FILE );
      Log.writeMsg( F("err_conf") );//  сообщение в лог о перезаписи ошибочного setup  
      File cfgFile = SD.open( CONFIG_FILE, FILE_WRITE);    
      serializeJsonPretty(config, cfgFile);    // перезаписываем в файл настройки по умолчанию ( установлены выше )
      cfgFile.close();
    };
};

_MFREE
_PRN("after load FILE");
_LOOK(config[PGM IP].as<String>() ); 
_LOOK(config[PGM maskIP].as<String>() ); 
_LOOK(config[PGM pingIP].as<String>() ); 
_LOOK(config[PGM gatewayIP].as<String>() ); 
_LOOK(config[PGM timePing].as<uint16_t>() );  
_LOOK(config[PGM timeoutPing].as<uint16_t>() );  
_LOOK(config[PGM maxLosesFromSent].as<uint16_t>());    
_LOOK(config[PGM numPingSent].as<uint16_t>());  
_LOOK(config[PGM maxLostPing].as<uint8_t>() );  
_LOOK(config[PGM delayBackSwitch].as<uint16_t>());  
_LOOK(config[PGM delayReturnA].as<uint32_t>());  
_LOOK(config[PGM stepDelay].as<uint32_t>());  
_LOOK(config[PGM maxDelayReturnA].as<uint32_t>());  
_LOOK(config[PGM timeServerIP].as<String>() );  
_LOOK(config[PGM portTimeServer].as<uint16_t>());      
_LOOK(config[PGM date].as<String>() );   
_LOOK(config[PGM time].as<String>() );   
_LOOK(config[PGM login].as<String>() );   
_LOOK(config[PGM password].as<String>() );   


// ???  временное для проверки , все убрать 
// вручную меняем чтоб увидеть копи рование
//config[PGM IP] = "111.222.111.222.111.222";
// ??????????????     проблемы с 32 разрядными значениями, некоторые принимают значения, некоторые - нет, принцип не понятен!!!!   ?????????????????



    //??? меняем на отладочный конфиг
//  в ручную переставляем
//ip(10, 140, 33, 147);// собственный алрес Ethernet интерфейса
config[PGM IP]="192.168.1.27";
config[PGM pingIP]="192.168.1.1";
config[PGM maskIP] = "255.255.255.0";  
config[PGM gatewayIP] = "192.168.1.1";


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
  Ethernet.begin(mac, charsToIP(config[IP].as<const char*>() ), charsToIP(config[gatewayIP].as<const char*>() ), 
                      charsToIP(config[gatewayIP].as<const char*>() ), charsToIP(config[maskIP].as<const char*>() ) );  // считаем что localDNS совпадает с gatewayIP
  // Ethernet.begin(mac, charsToIP(config[IP) );  так работало до добавления gateway
  server.begin();

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
 
  // ??  убрать после отладки ICMPPing::setTimeout(PING_REQUEST_TIMEOUT_MS);  // надо ставить таймаут из конфига.
  ICMPPing::setTimeout( config[timeoutPing] ); 
  ping.asyncStart(charsToIP(config[pingIP]), 1, echoResult); //послать первый запрос 
  //  замер количества свободной памяти
  vRAM = memoryFree(); // запоминаем количество свободного места в ОЗУ  

  _LOOK(vRAM)
/*
// =====================   временная проверка пинга  - работает на шлюз .... 1.1
// время с сервера так же получает
_LOOK_IP("new pingIP-",config[pingIP]);
while(true){
  _LOOK("start ping", "")
if (! ping.asyncStart(config[pingIP], 3, echoResult)){  // если не удалось послать нормально запрос
       _LOOK("bad start","")
    }
  else { _LOOK("OK start","")  };  // ожидаем ответа

delay(500);

while( !ping.asyncComplete(echoResult)){  // если ждем ответа  и ответ получен или истек тайм аут  
  delay(100);
};

  // проверяем какой результат пинга
  if (echoResult.status != SUCCESS){  // нет ответа- тайм-аут
    _LOOK(echoResult.status ,"-bad ping res")  }
    else {_LOOK(echoResult.status," - OK request")};
  
delay(1000);
bool timeRes=timeClient.forceUpdate();
 _LOOK("upd time-", timeRes)
 delay(3000);
  Serial.println(timeClient.getFormattedTime());
  delay(3000);

};  // while
*/
//-------------  проверка кнопки  ----------------------------------------------
/*
while(true){
_LOOK("button ON ?- ", !digitalRead( SW_SWITCH ))
delay(200);
};
*/

//  ??  добавить стирание лог файла если он очень большой в класс LOG

 _PRN("end setup")
_HALT

}

void loop() {
  
  // проверка утечки памяти
  instantRAM = memoryFree();
  if( ( instantRAM - vRAM) < 0 ) {  //  есть утечка
    Log.writeMsg( F("mem_leak") ); //  сообщение в лог об утечке памяти 
    // timeClient при смене с ошибочного обновления на нормальное обновление занимает доп память, что приводит к ошибочному сообщению об утечке. на самом деле просто после ошибочного обновления больше памяти
    //  может timeClient надо инициализировать каждый раз ? подумать
  };
  vRAM = instantRAM;

  unsigned long Tstart = millis();
  //  **********************************************  ЧТЕНИЕ СОСТОЯНИЯ ОБОРУДОВАНИЯ И КОНТРОЛЛЕРА   ************************************************************
  num_cycle++;

  D.Date_Time = watch.gettime("Y-m-d H:i:s");  // обновляем время
  // переносим время в конфигурацию
  config["date"] = D.Date_Time.substring(0, D.Date_Time.indexOf(" "));
  config["time"] = D.Date_Time.substring(D.Date_Time.indexOf(" ")+1);
  
// проверенно при периоде опроса тайм сервера 30 сек, на 89.109.251.21- ntp1.vniiftri.ru. нормальный опрос- таймаут 23 раза по 30 сек- сообщение в файл- восстановление нормального опроса (восстановление подключения RJ-45 )
  // если интервал истек, то синхронизируемся с time server  
  if( !( config[timeServerIP][0] == 127 && config[timeServerIP][1] == 0 && config[timeServerIP][2] == 0 && config[timeServerIP][3] == 1 ) ){  // вообще по настройкам требуется обращение к тайм серверу ?
    if( counters.isOver( SyncTimeSrv )){     
      if( !timeClient.update() ){
        errorTimeSrv++;       
      }
      else{ // время получили
        watch.settimeUnix( timeClient.getEpochTime() );      
        errorTimeSrv = 0;
        //   убрал чтоб не забивать лог сообщениями Log.writeMsg( F("ok_tsrv") ); //  сообщение в лог об обновлении времени
      };
      counters.start( SyncTimeSrv );  // начинаем отсчет периода заново в любом случае
      if( errorTimeSrv > MAX_ERROR_TIME_SRV ){  // если превышено количество попыток синхронизации времени
        Log.writeMsg( F("err_tsrv") ); //  сообщение в лог о потере синхронизации с сервером времени
        errorTimeSrv = 0;
      };
    };// если интервал истек, то синхронизируемся с time server
};  // сервер не 127.0.0.1
      
// если истек период проверки результата и посылки следующего пинга то
if( counters.isOver( Tping ) && ping.asyncComplete(echoResult) ){   
  _PRN("ping")
  counters.start( Tping ); // перезапуск таймера
// обработка пинга - разобрать какое состояние сейчас после предыдущей отправки
  switch( echoResult.status ){  // если ASYNC_SENT то запрос послан, ожидаем ответ - ничего не делать      
    case SUCCESS: { //если успех
      //проверяем адрес ответившего хоста
      bool eqAddrIP = true;
      for( byte i = 0; i < 4; i++){   eqAddrIP = eqAddrIP & (echoResult.addr[i] == config[pingIP][i]); };
      if( eqAddrIP ){  // адрес ответа совпадаетс адресом посылки
          pingResult.set( true );
          D.lostPing = 0; // сбрасываем счетчик непрпрывно потеряных пакетов
          _PRN(" SUCCESS ");
          if( !pingResult.get( 1 ) ){ Log.writeMsg( F("ping_ok") );  }// если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был потерян то сообщение в лог восствновлении связи     
      }
      else{  // ответил другой хост
          pingResult.set( false );
          _PRN(" other host answer ");
          D.lostPing++;
          D.totalLost[D.port]++;    
          if( pingResult.get( 1 ) ){ Log.writeMsg( F( "othr_hst" ) );  }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог об ответе с другого хоста
      };
      break;
    };
    case SEND_TIMEOUT:{  // Время ожидания отправки запроса истекло не могу отправить запрос . вероятно нет линка
      pingResult.set( false );
      _PRN(" send timeout ");
      if( pingResult.get( 1 ) ){ Log.writeMsg( F("fld_ping") ); }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о проблемах      
      break;
    };
    case NO_RESPONSE:{ // превышено время ожидания ответа 
    _PRN(" ping lost ");
      pingResult.set( false );
      D.lostPing++;
      D.totalLost[D.port]++;    
      if( pingResult.get( 1 ) ){ Log.writeMsg( F("lost_png") );  }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о потере связи              
      break;
    };
    case BAD_RESPONSE:{  // получен неверный ответ сервера 
    _PRN(" bad response ");
      pingResult.set( false );
      D.lostPing++;
      D.totalLost[D.port]++;    
      if( pingResult.get( 1 ) ){ Log.writeMsg( F("bad_resp") );  }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о потере связи              
      break;
    };
    // без default:{ };
  }; // switch
_LOOK(echoResult.status)
  if( echoResult.status != ASYNC_SENT ){  // отправка нового пинга, результаты прошлого запроса разобрали выше
    bool f = ping.asyncStart(charsToIP(config[pingIP]), 1, echoResult); 
    _LOOK(f);
   /*  анализ уже реализован по результату  SEND_TIMEOUT
   // ping.asyncStart(addr, n, echoResult) - запускает новый запрос ping асинхронно. addr: IP-адрес для проверки в виде массива из четырех октетов. n: Количество повторений, прежде чем сдаваться. echoResult: ICMPEchoReply, который будет содержать статус == ASYNC_SENT при успешном выполнении.
  // возвращает  true при удачной отправке асинхронного запроса, false в противном случае.
   if ( !ping.asyncStart(config[pingIP, 1, echoResult)){  // если не удалось послать нормально запрос
       Log.writeMsg( F(не удалось отправить пинг) ); // запись в лог о невозможности отправки сообщения      
      };
   */   
  };
};//  если сработал счетчик Tping

/*
// счетчик LockSwitch для задержки переключения если оба канала не работают
// если потерь подряд или потерь из заданного количества больше порога и таймер запрета перехода на другой канал истек, то    
if( (( D.lostPing >= config[maxLostPing] ) || !pingResult.isOK( config[maxLosesFromSent], config[numPingSent] ) ) && counters.isOver( LockSwitch ) ){  // переключаемся на другой канал
  changePort(); 
  if (D.port == 0) { Log.writeMsg( F("auto_toA") ); }  // канал А теперь
  else{ Log.writeMsg( F("auto_toB") ); }  // канал B теперь
  counters.start( LockSwitch );  // а если это пробное переключение? надо состояние переключения "из-за потерь" из-за возврата на приоритетный канал"  ??
  if( D.port == 1 ){ counters.start( returnA ); };  // если переключились на B, то запускаем счетчик возврата на канал А
};


// ??если не было возврата на основной канал, то обрабатываем потерю ping, сбрасываем счетчик интервала ping и счетчик ожидания ответа
// ?? если был возврат на основной канал и потерь больше допустимого, то возвращаемся на резервный канал 

   // если канал В и истекло время автовозврата, то возвращаемся на А
  if( ( D.port == 1 ) && counters.isOver( returnA ) ){ 
    changePort(); 
    Log.writeMsg( F("ret_toA") ); 
  };
 */


 // ========================    обработка запросов к вэб интерфейсу   ==========================
// вроде как для W5500 скорость передачи по spi 8МГц т.е. 1 МБ/сек. Т.к. данные надо считывать из microSD по  spi то получаем 500 кБ сек. 
// за 1 такт в 200 мсек можно передать  порядка 100 кБ информации. Размеры страниц для вэб интерфейса не более 10 кБ. экономить и разделять передачу на такты нет смысла.
 //_LOOK("state in loop= ", _state)
if( webServer.isWait() ){  //если вэб сервер в ожидании 
    client = server.available(); // есть клиент?
    if( client ){ //если есть ожидающий клиент, то запустить вэб сервер и счетчик тайм аута
    Serial.println("New request from client:");
    copyConfig( config, webConfig );  // копируем текущую конфигурацию для вэб страницы
    webServer.startConnection( client );
    counters.start( timeOutWeb );    
  };  
}        
else if( webServer.isFinished() ){  // вэб сервер завершил обработку запроса
    _PRN(" finished in loop")
    delay(1);
    client.stop(); // разрываем соединение    
    webServer.reset();
    counters.reset( timeOutWeb );   
    // какой конфиг проверяем ????????????????????
    verifyConfig( webConfig );  // проверяем допустимость установок и корректируем их при необходимости
    applyChangeConfig(); //сравниваем были ли изменения в конфигурацию переносим изменения в основную конфигурацию и отрабатываем изменения

_PRN("after web ----------------------------------------------------------------------------------");
_LOOK(config[IP].as<const char*>() ); 
_LOOK(config[maskIP].as<const char*>() ); 
_LOOK(config[pingIP].as<const char*>() ); 
_LOOK(config[gatewayIP].as<const char*>() ); 
_LOOK(config[timePing].as<uint16_t>() );  
_LOOK(config[timeoutPing].as<uint16_t>() );  
_LOOK(config[maxLosesFromSent].as<uint16_t>());    
_LOOK(config[numPingSent].as<uint16_t>());  
_LOOK(config[maxLostPing].as<uint8_t>() );  
_LOOK(config[delayBackSwitch].as<uint16_t>());  
_LOOK(config[delayReturnA].as<uint32_t>());  
_LOOK(config[stepDelay].as<uint32_t>());  
_LOOK(config[maxDelayReturnA].as<uint32_t>());  
_LOOK(config[timeServerIP].as<const char*>() );  
_LOOK(config[portTimeServer].as<uint16_t>());      
_LOOK(config[date].as<const char*>() );   
_LOOK(config[time].as<const char*>() );   
_LOOK(config[login].as<const char*>() );   
_LOOK(config[password].as<const char*>() );   

}
else{   //иначе такт работы вэб сервера
      webServer.run(); // такт работы сделан и рестартуем таймер тайм аута
      counters.start( timeOutWeb );
};

/*  временно убрал
if( counters.isOver( timeOutWeb ) ){  //если истек тайм аут то сбросить вэб сервер
  webServer.sendHeaderAnswer( 408 );  
  webServer.reset();
  counters.reset( timeOutWeb );  
  client.stop(); // разрываем соединение
  Log.writeMsg( F("err408_1") );
};
 */
// ?? если меняем конфиг, то надо перезагружать устройство и дело с концом, ено потеряется статистика текущая- ну и фиг с нет


 /* 
// ----------------------  тестовый вариант работы выдающий простую страницу
if( !WaitAnswer ){ //если вэбсервер не занят обработкой данных от клиента и нет ожидания пинга  
  client = server.available(); // ожидаем объект клиент
  if (client) {
    flagEmptyLine = true;
    Serial.println("New request from client:");

    while (client.connected()) {
      if (client.available()) {
        char tempChar = client.read();
        Serial.write(tempChar);

        if (tempChar == '\n' && flagEmptyLine) {
          // пустая строка, ответ клиенту
          client.println("HTTP/1.1 200 OK"); // стартовая строка
          client.println("Content-Type: text/html; charset=utf-8"); // тело передается в коде HTML, кодировка UTF-8
          client.println("Connection: close"); // закрыть сессию после ответа
          client.println("Refresh: 5"); // время обновления страницы !!!! работает и обновляет
          client.println(); // пустая строка отделяет тело сообщения
          client.println("<!DOCTYPE HTML>"); // тело сообщения
          client.println("<html>");
          client.println("<H1> Первый WEB-сервер</H1>"); // выбираем крупный шрифт
          client.print("Cycle:"); 
          client.println( num_cycle );
          client.println("</html>");
          break;
        }
        if (tempChar == '\n') {   // новая строка
          flagEmptyLine = true;
        }
        else if (tempChar != '\r') {  // в строке хотя бы один символ
          flagEmptyLine = false;
        }
      }
    }
    delay(1);
    // разрываем соединение
    client.stop();
    Serial.println("Break");
  }
};//если сервер не занят обработкой данных от клиента
//-------   конец тестового варианта работы выдающего простую страницу   
*/

/*
if( D.autoSW ){//  если авто режим 
  if( (( config[maxLosesFromSent] == 0 ) || ( config[numPingSent] == 0 )) && ( config[maxLostPing] == 0 ) ){  // и нет параметров для автоматической работы
    // помигать светодиодом и отключить
    for( byte i = 0; i<3; i++){
      digitalWrite( LED_AUTO, LED_OFF);
      delay(300);
      digitalWrite( LED_AUTO, LED_ON);
      delay(300);
    };
    digitalWrite( LED_AUTO, LED_OFF);
    D.autoSW = false;
    Log.writeMsg( F("rst_auto") ); // запись о сбросе автоматического режима
  };
};//  если авто режим 
*/

// реакция на кнопку коротко и длинно - ОК
bool buttonOn = !digitalRead( SW_SWITCH );  // 1 - если кнопка сейчас нажата
// обработка нажатий на кнопку ручного управления
if( !buttonOn ){ 
                          // кнопка НЕ нажата и не была нажата - ничего
    if( buttonPressed ){ //кнопка НЕ нажата и была нажата
        buttonPressed = false;
        if( counters.isOver( press3s ) ){   //если таймер истек,         
          //то переключить режим авто и запомнить, что кнопке не была нажата - чтоб не пересечся с переключением канала при коротком нажатии          
          D.autoSW = !(D.autoSW);    
          if( D.autoSW ){ Log.writeMsg( F("set_auto") );  }
          else{ Log.writeMsg( F("set_man") );  };
        }
        else{  // таймер 3 сек не истек, значит короткое нажатие и меняем порт
            changePort( D ); 
            if (D.port == 0) {  Log.writeMsg( F("man_to_A") ); }  // канал А теперь
            else{ Log.writeMsg( F("man_to_B") ); }  // канал B теперь
        };
    };
};
if( buttonOn ){ //кнопка нажата 
  if( !buttonPressed ){ //и не была нажата ранее - запустить таймер на 3с
        counters.start( press3s );
        buttonPressed = true;
  };
};
// кнопка нажата и была нажата - ничего

// вывод индикации в соотвтествии с сотоянием устройства
if( D.autoSW ){ digitalWrite( LED_AUTO, LED_ON); }
else{ digitalWrite( LED_AUTO, LED_OFF); };


// автоматическое восстановление настроек каждый час исключено из программы

counters.cycleAll();  // работа счетчиков  задержки

  //  ВРЕМЯ ИСПОЛНЕНИЯ ЦИКЛА  - измеренное время цикла  не более ?? мс
  // задержка для одинакового времени цикла
  // Из-за перехода времени через через 0 запись в лог будет даже без опоздания контроллера ( происходит один раз в 50 дней)
if (millis() >= Tstart) {                               // если не было перехода через 0 в milis то
    largeT = false;
    while ((millis() - Tstart) < T_DELAY) { delay(1); };  // ожидание если слишком быстро выполнили основной цикл
}
else{
    if( !largeT ){ // если первое превышение времени цикла после серии нормальных циклов
      largeT = true;
      Log.writeMsg( F("large_t") );  // сообщение о превышении времени цикла
    };
};

}

void applyChangeConfig(){  // функция проверки изменений конфига и их обработки, использует как глобальные переменные config и webConfig
    //  проверяем смену сетевых настроек
  if( (! EQ( config[IP], webConfig[IP] ) ) || 
        (! EQ( config[maskIP], webConfig[maskIP] ) ) || 
        (! EQ( config[gatewayIP], webConfig[gatewayIP] ) ) ){  // если есть изменения

        strcpy( config[IP].as<const char*>(), webConfig[IP].as<const char*>() );
        strcpy( config[maskIP].as<const char*>(), webConfig[maskIP].as<const char*>() );
        strcpy( config[gatewayIP].as<const char*>(), webConfig[gatewayIP].as<const char*>() );
          
        digitalWrite(W5500_RESET, LOW);
        delay(1000);
        digitalWrite(W5500_RESET, HIGH);
        delay(1000);
        Ethernet.begin(mac, charsToIP(config[IP].as<const char*>() ), charsToIP(config[gatewayIP].as<const char*>() ), 
                      charsToIP(config[gatewayIP].as<const char*>() ), charsToIP(config[maskIP].as<const char*>() ) );  // считаем что localDNS совпадает с gatewayIP
        server.begin();
        Log.writeMsg( F("set_net.txt") );
               
  };  // если есть изменения в сетевых настройках

  if(! EQ( config[pingIP], webConfig[pingIP] ) ) {  // если есть изменения
        strcpy( config[pingIP].as<const char*>(), webConfig[pingIP].as<const char*>() );
        Log.writeMsg( F("set_ping.txt") );
  };
  
    //if( ! EQ(config[timePing].as<const char*>(), webConfig[timePing].as<const char*>() )  ){ 
    if( ! EQ(config[timePing], webConfig[timePing] ) ){   
      strcpy( config[timePing].as<const char*>(), webConfig[timePing].as<const char*>());
      counters.load( Tping, config[timePing].as<uint16_t>() * 1000 );// настраиваем период повторения пинга (переводим в мс)
      counters.start( Tping );
      Log.writeMsg( F("t_ping.txt") );
    };  // период повтора ping
    if( ! EQ(config[timeoutPing], webConfig[timeoutPing] )  ){ 
      strcpy( config[timeoutPing].as<const char*>(), webConfig[timeoutPing].as<const char*>());     
      ICMPPing::setTimeout( config[timeoutPing] );  
      Log.writeMsg( F("to_ping.txt") );  
    };  // тайма-аут пинга в мсек, изменится со следующего пинга    
    if( ! EQ(config[maxLosesFromSent], webConfig[maxLosesFromSent] )  ){ 
      strcpy( config[maxLosesFromSent].as<const char*>(), webConfig[maxLosesFromSent].as<const char*>());
      Log.writeMsg( F("l_ping.txt") );  
    }; // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    if( ! EQ(config[numPingSent], webConfig[numPingSent])  ){  
      strcpy( config[numPingSent].as<const char*>(), webConfig[numPingSent].as<const char*>());
      Log.writeMsg( F("ping_s.txt") );   
    };      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    if( ! EQ(config[maxLostPing], webConfig[maxLostPing])  ){ 
      strcpy( config[maxLostPing].as<const char*>(), webConfig[maxLostPing].as<const char*>());  
      Log.writeMsg( F("max_lost.txt") );   
      };  // предельное число подряд (непрерывно) потеряных пакетов
    if( ! EQ(config[delayBackSwitch], webConfig[delayBackSwitch] )  ){
      strcpy( config[delayBackSwitch].as<const char*>(), webConfig[delayBackSwitch].as<const char*>());
      counters.load( LockSwitch, config[delayBackSwitch].as<uint16_t>() * 1000);
      counters.start( LockSwitch );
      Log.writeMsg( F("t_lock.txt") );
     };  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    if( ! EQ(config[delayReturnA], webConfig[delayReturnA] )  ){ 
      strcpy( config[delayReturnA].as<const char*>(), webConfig[delayReturnA].as<const char*>());  // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
      counters.load( returnA, config[delayReturnA].as<uint32_t>() * 1000);
      counters.start( returnA );
      Log.writeMsg( F("d_ret_a.txt") ); 
      } ;   
    if( ! EQ(config[stepDelay], webConfig[stepDelay] )  ){ 
      strcpy( config[stepDelay].as<const char*>(), webConfig[stepDelay].as<const char*>());   
      Log.writeMsg( F("step_d.txt") );    
      };      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    if( ! EQ(config[maxDelayReturnA], webConfig[maxDelayReturnA] )  ){   
      strcpy( config[maxDelayReturnA].as<const char*>(), webConfig[maxDelayReturnA].as<const char*>());   
      Log.writeMsg( F("max_d.txt") );  
      };  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки

  if( ! EQ( config[timeServerIP], webConfig[timeServerIP] )  ) {  // если есть изменения
        strcpy( config[timeServerIP].as<const char*>(), webConfig[timeServerIP].as<const char*>() );
        Log.writeMsg( F("set_tsip.txt") );
  };
    
    if( ! EQ(config[portTimeServer], webConfig[portTimeServer] )  ){   
      strcpy( config[portTimeServer].as<const char*>(), webConfig[portTimeServer].as<const char*>()); 
      // больше ничего не делаем, т.к. все изменения произойдут только после сохранения config и перезагрузки      
      Log.writeMsg( F("set_ts_p.txt") );      
      };    

    // если таймсервер 127.0.0.1    
    if( ! EQ( webConfig[timeServerIP], "127.0.0.1" ) == 0 ){ 
      // если время или дата не совпадают  
      if( ! EQ( config[date], webConfig[date] ) || ! EQ( config[time], webConfig[time] ) ){ 
          String d = webConfig["date"];
          String t = webConfig["time"];      
          //	(сек, мин, час, день, мес, год, день_недели)
          watch.settime( t.substring( t.lastIndexOf(":") + 1 ).toInt(), // сек
                         t.substring( t.indexOf(":") +1, t.lastIndexOf(":") ).toInt(),  // мин
                         t.substring( 0, t.indexOf(":")).toInt(),       // час      
                         d.substring( d.lastIndexOf("-") + 1 ).toInt(), // день
                         d.substring( d.indexOf("-") +1, d.lastIndexOf("-") ).toInt(),  // мес
                         d.substring( 0, d.indexOf(":")).toInt()       // год
          );		// день недели не указываем
          Log.writeMsg( F("change_t.txt") );
      };
    };  

    // ???      char login[8];
    //  ??   char password[8];
}
