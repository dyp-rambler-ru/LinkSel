#pragma once
#include <Arduino.h>
#include "debug.h"
#include <SPI.h>
#include "iarduino_RTC.h"  // Подключаем библиотеку iarduino_RTC для работы с модулями реального времени.
#include <Ethernet2.h>
#include <NTPClient.h>
#include "ICMPPing.h"
#include "SD.h"

#include "log.h"
#include "conf.h"
#include "common.h"
#include "counters.h"
#include "bufPing.h"
#include "web.h"

/*  убрать потом
// останов программы бесконечным циклом
#define _HALT while( true ){ delay(10);};
// печать парамера: название значение
#define _LOOK(x,y) Serial.print(x); Serial.println(y);

#define _LOOK_IP(x,y) Serial.print(x); Serial.print(y[0]); Serial.print("."); Serial.print(y[1]); Serial.print("."); Serial.print(y[2]); Serial.print("."); Serial.println(y[3]); 
*/

// PING_REQUEST_TIMEOUT_MS -- timeout in ms.  between 1 and 65000 or so
// save values: 1000 to 5000, say.
#define PING_REQUEST_TIMEOUT_MS 2500

#ifndef ICMPPING_ASYNCH_ENABLE
#error "Asynchronous functions only available if ICMPPING_ASYNCH_ENABLE is defined -- see ICMPPing.h"
#endif

iarduino_RTC watch(RTC_DS1302, PIN_RST, CLK, DAT);  // Объявляем объект watch для работы с RTC модулем на базе чипа DS1302

// инициализируем контроллер Ethernet

const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 140, 33, 148);// собственный алрес Ethernet интерфейса

IPAddress pingAddr(10, 140, 33, 111); // ip address to ping
SOCKET pingSocket = 0;

ICMPPing ping(pingSocket, 1);
ICMPEchoReply echoResult;  // результат ответа по пингу

EthernetServer server(80);
EthernetClient client;  // переменная для подключенного к вэб интерфейсу клиента

//?? что здесь указать?  было в примере WiFiUDP ntpUDP;

EthernetUDP ntpUDP; // создаем экземпляр Udp 

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
//  ???  NTPClient timeClient(ntpUDP);
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).

bool WaitAnswer = false;  // признак что пинг отправлен и ждем ответа
bool BusyWebServer = false;  // вэб сервер занят обработкой  запроса от клиента к интерфейсу конфигурации
bool buttonPressed = false;  // признак, что кнопка была нажата на прошлом цикле
int vRAM;  //  объем ОЗУ для кучи и стека, используется для контроля утечки памяти
int  instantRAM;
uint32_t num_cycle = 0;
bool largeT = false; // флаг превышения времени выполнения основного цикла

File root;  // файловая переменная для корневой директории

//==================================   ОПРЕДЕЛЕНИЯ ФУНКЦИЙ  ==============================================

void setup() {

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
  // digitalWrite( CS_W5500, LOW);
  pinMode(CS_SD_CARD, OUTPUT);
  // digitalWrite( CS_SD_CARD, LOW);

  pinMode(SW_RESET_CONF, INPUT);
  pinMode(SW_RESET_CONF, INPUT_PULLUP);
  pinMode(SW_SWITCH, INPUT);
  pinMode(SW_SWITCH, INPUT_PULLUP);

 Serial.begin(9600);
 Serial.println("start");
  
 watch.begin();    // Инициируем работу с модулем RTC.
 watch.gettime();  // Считываем текущее время из модуля в буфер библиотеки.
  // читаем время от системных часов в Setup
  D.Date_Time.DD = watch.day;
  D.Date_Time.MM = watch.month;
  D.Date_Time.YY = watch.year;
  D.Date_Time.hh = watch.Hours;
  D.Date_Time.mm = watch.minutes;
  D.Date_Time.ss = watch.seconds;
  _LOOK("start ", watch.gettime("Y-m-d H:i:s"))
  // инициализируем SD card
  //SD.begin(CS_SD_CARD);
  bool b1= SD.begin(CS_SD_CARD);
  _LOOK("SD-",b1);
 
 Log.ini( &watch );  // инициализация ЛОГа
 Log.writeMsg( F("start") );  //сообщение о старте контроллера
 //если старт с нажатой кнопкой, то перезаписываем настройки по умолчанию
if( !digitalRead( SW_RESET_CONF ) ){
  Log.writeMsg( F("rst_conf") );  //сообщение в лог о перезаписи setup  на параметры по умолчанию по нажатию на кнопук reset
  config.setDefaultField();   // сбросить установки в значение по умолчанию
  config.save();              // запись установок в EEPROM    
};

// считываем настройки (сброшенные или старые)
if( !config.load() ){ // прочитали , если с ошибкой, то перезаписываем
  Log.writeMsg( F("err_conf") );//  сообщение в лог о перезаписи ошибочного setup  
  config.setDefaultField();   // сбросить установки в значение по умолчанию
  config.save();              // запись установок в EEPROM
  _LOOK("set Def ","")
};

_LOOK("after load","-----------");
_LOOK_IP("IP-",config.field.IP); 
_LOOK_IP("maskIP-",config.field.maskIP); 
_LOOK_IP("pingIP-",config.field.pingIP); 
_LOOK("timePing-",config.field.timePing);  
_LOOK("timeoutPing-", config.field.timeoutPing);  
_LOOK("maxLosesFromSent", config.field.maxLosesFromSent);    
_LOOK("numPingSent-",config.field.numPingSent);  
_LOOK("maxLostPing-",config.field.maxLostPing);  
_LOOK("delayBackSwitch-",config.field.delayBackSwitch);  
_LOOK("delayReturnA-",config.field.delayReturnA);  
_LOOK("stepDelay-",config.field.stepDelay);  
_LOOK("maxDelayReturnA-",config.field.maxDelayReturnA);  
_LOOK_IP("timeServerIP-",config.field.timeServerIP);  
_LOOK("portTimeServer-",config.field.portTimeServer);      
    //     char login[8];      char password[8];

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
 Ethernet.begin(mac, ip); // CS - 10
  //  Ethernet.init(ETH_CS_PIN);  - ?? вроде не нужно
 server.begin();

  // increase the default time-out, if needed, assuming a bad
  // connection or whatever.
 // ICMPPing::setTimeout(PING_REQUEST_TIMEOUT_MS);  //??? надо ставить таймаут из конфига. а не контролируем где то еще таймаут пинга??
 ICMPPing::setTimeout( config.field.timeoutPing );
  
 //String TS_IP = intToStr(config.field.timeServerIP[0]) + "." + intToStr(config.field.timeServerIP[1]) + "." + intToStr(config.field.timeServerIP[2]) + "." + intToStr(config.field.timeServerIP[3]);  
 String TS_IP = String(config.field.timeServerIP[0]) + ".";
 TS_IP += String(config.field.timeServerIP[1]) + ".";
 TS_IP += String(config.field.timeServerIP[2]) + ".";
 TS_IP += String(config.field.timeServerIP[3]);  
 // отключено для тестирования timeClient.setPoolServerName( TS_IP.c_str() ); // устанавливаем адрес сервера времени
 TS_IP = "89.109.251.21";  // ntp1.vniiftri.ru
 //??? timeClient.setPoolServerName( TS_IP.c_str() ); // устанавливаем адрес сервера времени
 // ???timeClient.begin(); // config.field.portTimeServer );  // активация клиента времени

  counters.resetAll();  // первичный сброс всех счетчиков

  //  замер количества свободной памяти
  vRAM = memoryFree(); // запоминаем количество свободного места в ОЗУ  

  _LOOK("ram-", vRAM)

// =====================   временная проверка пинга
//ни хрена не работает

while(true){
if (! ping.asyncStart(pingAddr, 3, echoResult)){  // если не удалось послать нормально запрос
       _LOOK("bad ping start","")
    }
  else { _LOOK("OK ping start","")  };  // ожидаем ответа

delay(500);

while( !ping.asyncComplete(echoResult)){  // если ждем ответа  и ответ получен или истек тайм аут  
  // проверяем какой результат пинга
  if (echoResult.status != SUCCESS){  // нет ответа- тайм-аут
    _LOOK(""," bad ping res")  }
    else {_LOOK(""," OK request")};
  };  // while  complete
delay(1000);

};  // while


/*  не работает
delay(3000);
 timeClient.update();
 _LOOK("upd time","")
 delay(3000);
  Serial.println(timeClient.getFormattedTime());
   */

  //=================    временно для отладки ??????
/*  нихрена не работает и виснет похоже на timeClient.Update()
   // или  bool timeClient.Update(); ??
if( !timeClient.update() ){
  _LOOK("error t serv","_")     
    }
    else{ // время получили
    _LOOK("","get time ")
    uint32_t time = timeClient.getEpochTime();
    _LOOK("time:", time);
      watch.settimeUnix( time );
    };
    */
 _LOOK("","end setup")

}

void loop() {
  // проверка утечки памяти
  instantRAM = memoryFree();
  if( ( instantRAM - vRAM) > 0 ) {  //  есть утечка
    Log.writeMsg( F("mem_leak") ); //  сообщение в лог об утечке памяти    
    vRAM = instantRAM;
  };

  unsigned long Tstart = millis();
  //  **********************************************  ЧТЕНИЕ СОСТОЯНИЯ ОБОРУДОВАНИЯ И КОНТРОЛЛЕРА   ************************************************************
  num_cycle++;

  watch.gettime();  // Считываем текущее время из модуля в буфер библиотеки.
  // читаем время от системных часов в Setup
  D.Date_Time.DD = watch.day;
  D.Date_Time.MM = watch.month;
  D.Date_Time.YY = watch.year;
  D.Date_Time.hh = watch.Hours;
  D.Date_Time.mm = watch.minutes;
  D.Date_Time.ss = watch.seconds;

  // если интервал истек, то синхронизируемся с time server
  if( counters.isOver( SyncTimeSrv )){
    // или  bool timeClient.Update(); ??
    if( !timeClient.forceUpdate() ){
      Log.writeMsg( F("err_tsrv") ); //  сообщение в лог о неудачном обновлении времени
    }
    else{ // время получили
      watch.settimeUnix( timeClient.getEpochTime() );
      Log.writeMsg( F("ok_tsrv") ); //  сообщение в лог об обновлении времени
      counters.start( SyncTimeSrv );  // начинаем отсчет периода заново
  // ?? как вариант Устанавливаем время в модуль: 12:30:00 25 октября 2022 года
  // 00 сек, 30 мин, 12 час, 25 число, октябрь, 2022 год, вторник
  /*  watch.settime(timeClient.getSeconds(), timeClient.getMinutes(), timeClient.getHours(), 25, 10, 22, 2);
    int timeClient.getDay()  а дату где взять?  
    как вариант получаем текстовый ответ от сервера времени и парсим его 

  если меняем порт или адрес надо остановить
  timeClient.end();
  и заново инициализировать!
  String TS_IP = D.setup.timeServerIP[3] + "." + D.setup.timeServerIP[2] + "." + D.setup.timeServerIP[1] + "." + D.setup.timeServerIP[0];
  timeClient.setPoolServerName( TS_IP ); // устанавливаем адрес сервера времени
  timeClient.begin( D.setup.portTimeServer );  // активация клиента времени
*/
   // Serial.println(timeClient.getFormattedTime());// отладочная печать
    };
  };// если интервал истек, то синхронизируемся с time server


  
// если пинг в этом периоде еще не посылали и интервал истек
if( !WaitAnswer &&  counters.isOver( Tping ) ){   // послать ping и  включить счетчик ожидания ответа на ping
  // ping.asyncStart(addr, n, echoResult) - запускает новый запрос ping асинхронно. Параметры те же, что и для обычного ping, но метод возвращает значение false при ошибке.
  // addr: IP-адрес для проверки в виде массива из четырех октетов.
  // n: Количество повторений, прежде чем сдаваться.
  // echoResult: ICMPEchoReply, который будет содержать статус == ASYNC_SENT при успешном выполнении.
  // возвращает  true при удачной отправке асинхронного запроса, false в противном случае.

  // ??????? нужно брать адрес из  сетапа, а не назначеный жестко

  if (! ping.asyncStart(pingAddr, 3, echoResult)){  // если не удалось послать нормально запрос
       Log.writeMsg( F("fld_ping") ); // запись в лог о невозможности отправки сообщения      
    }
  else { WaitAnswer = true;  };  // ожидаем ответа
};

if( WaitAnswer && ping.asyncComplete(echoResult)){  // если ждем ответа  и ответ получен или истек тайм аут
  WaitAnswer = false;  
  pingResult.set( (echoResult.status == SUCCESS) );
  // проверяем какой результат пинга
  if (echoResult.status != SUCCESS){  // нет ответа- тайм-аут
    D.lostPing++;
    D.totalLost[D.port]++;    
    if( pingResult.get( 1 ) ){ Log.writeMsg( F("lost_png") );  }// если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о потере связи        
  } else {  // получили успешный ответ
    D.lostPing = 0; // сбрасываем счетчик непрпрывно потеряных пакетов
    if( !pingResult.get( 1 ) ){ Log.writeMsg( F("ping_ok") );  }// если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был потерян то сообщение в лог восствновлении связи      
  }
}; // если ждем ответа  и ответ получен или истек тайм аут

// счетчик LockSwitch для задержки переключения если оба канала не работают
// если потерь подряд или потерь из заданного количества больше порога и таймер запрета перехода на другой канал истек, то    
if( (( D.lostPing >= config.field.maxLostPing ) || !pingResult.isOK( config.field.maxLosesFromSent, config.field.numPingSent) ) && counters.isOver( LockSwitch ) ){  // переключаемся на другой канал
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
 

// ========================    обработка запросов к вэб интерфейсу   ==========================
// вроде как для W5500 скорость передачи по spi 8МГц т.е. 1 МБ/сек. Т.к. данные надо считывать из microSD по  spi то получаем 500 кБ сек. 
// за 1 такт в 200 мсек можно передать  порядка 100 кБ информации. Размеры страниц для вэб интерфейса не более 10 кБ. экономить и разделять передачу на такты нет смысла.
if( webServer.isWait() ){  //если вэб сервер в ожидании 
  client = server.available(); // есть клиент?
  if( client ){ //если есть ожидающий клиент, то запустить вэб сервер и счетчик тайм аута
    webServer.startConnection( client );
    counters.start( timeOutWeb );    
  };  
}        
else {   //иначе такт работы вэб сервера
  if( webServer.run() ){  // если сервер вернул true, то такт работы сделан и рестартуем таймер тайм аута
      counters.start( timeOutWeb );
  }
  else{  // вэб сервер отработал не штатно, сбрасываем соединение
    webServer.reset();
    counters.reset( timeOutWeb );  
    client.stop(); // разрываем соединение
  }  
};

if( counters.isOver( timeOutWeb ) ){  //если истек тайм аут то сбросить вэб сервер
  webServer.sendHeaderAnswer( 408 );  
  webServer.reset();
  counters.reset( timeOutWeb );  
  client.stop(); // разрываем соединение
  Log.writeMsg( F("err408_1") );
};
 
// ?? если меняем конфиг, то надо перезагружать устройство и дело с концом, ено потеряется статистика текущая- ну и фиг с нет


/*   ----------------------  тестовый вариант работы выдающий простую страницу
if( !WaitAnswer ){ //если вэбсервер не занят обработкой данных от клиента и нет ожидания пинга  
  client = server.available(); // ожидаем объект клиент
  if (client) {
    flagEmptyLine = true;
    Serial.println("New request from client:");

    while (client.connected()) {
      if (client.available()) {
        tempChar = client.read();
        Serial.write(tempChar);

        if (tempChar == '\n' && flagEmptyLine) {
          // пустая строка, ответ клиенту
          client.println("HTTP/1.1 200 OK"); // стартовая строка
          client.println("Content-Type: text/html; charset=utf-8"); // тело передается в коде HTML, кодировка UTF-8
          client.println("Connection: close"); // закрыть сессию после ответа
          client.println(); // пустая строка отделяет тело сообщения
          client.println("<!DOCTYPE HTML>"); // тело сообщения
          client.println("<html>");
          client.println("<H1> Первый WEB-сервер</H1>"); // выбираем крупный шрифт
          client.println("</html>");
          break;
          // другой вариант страницы
          //client.println("HTTP/1.1 200 OK");
          //  client.println("Content-Type: text/html");
          //  client.println("Connection: close");
          //  client.println("Refresh: 5"); // время обновления страницы 
          //  client.println();
          //  client.println("<!DOCTYPE HTML>");
          //  client.println("<html><meta charset='UTF-8'>");  
          //  client.println("<h1> Тестовая страницы </h1>");
          //  client.println("</html>"); 
          //  break;            
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
-------   конец тестового варианта работы выдающего простую страницу   */

if( D.autoSW ){//  если авто режим 
  if( (( config.field.maxLosesFromSent == 0 ) || ( config.field.numPingSent == 0 )) && ( config.field.maxLostPing == 0 ) ){  // и нет параметров для автоматической работы
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
            changePort(); 
            if (D.port == 0) { Log.writeMsg( F("man_to_A") ); }  // канал А теперь
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

if( counters.isOver( oneHour )){  //раз в час проверка целостности настроек 
 // контроль CRC для текущего setup, если не совпадает, то читаем из EEPROM (раз в час)
  if( !config.check() ){
     Log.writeMsg( F("err2conf") ); // запись в лог об ошибке конфигурации в ОЗУ
    if( !config.load() ){ // прочитали , если с ошибкой, то перезаписываем
        Log.writeMsg( F("err_conf") );//  сообщение в лог о перезаписи ошибочного setup    
        config.setDefaultField(); 
        config.load(); // читаем установки по умолчанию       
    }  
    else{
      counters.start( oneHour ); // запускаем счетчик заново
      //увеличиваем счетчик часов
      D.workTime[D.port]++;        
    };
  };
}; //раз в час проверка целостности настроек  

counters.cycleAll();  // работа счетчиков  задержки

  //  ВРЕМЯ ИСПОЛНЕНИЯ ЦИКЛА
  //  измеренное время цикла  не более ?? мс
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

