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

// инициализируем контроллер Ethernet, назначаем MAC адрес
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

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

bool WaitAnswer;  // признак что пинг отправлен и ждем ответа
bool BusyWebServer;  // вэб сервер занят обработкой  запроса от клиента к интерфейсу конфигурации
bool buttonPressed;  // признак, что кнопка была нажата на прошлом цикле
int vRAM;  //  объем ОЗУ для кучи и стека, используется для контроля утечки памяти
int  instantRAM;
uint32_t num_cycle;
bool largeT; // флаг превышения времени выполнения основного цикла
uint16_t errorTimeSrv; // счетчик ошибок обращения к тайм серверу

File root;  // файловая переменная для корневой директории

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

  randomSeed(analogRead(0));

  watch.begin();    // Инициируем работу с модулем RTC.
    // читаем время от системных часов в Setup
  D.Date_Time = watch.gettime("Y-m-d H:i:s");

  // инициализируем SD card
  //SD.begin(CS_SD_CARD);
  //пробовать ини на разные пины
  bool openSD= SD.begin(CS_SD_CARD);
  _LOOK(openSD);

  Log.writeMsg( F("start") );  //сообщение о старте контроллера

  config.setDefaultField();   // вначале всегда настройки по умолчанию, вдруг файла нет, поврежден или не все параметры

/*
  // стартовый вариант для перезаписи файла конфигурации
   config.save( CONFIG_FILE ); // запись установок в файл
   _HALT
*/

 //если старт с нажатой кнопкой, то перезаписываем настройки по умолчанию
  if( !digitalRead( SW_RESET_CONF ) ){
    Log.writeMsg( F("rst_conf") );  //сообщение в лог о перезаписи setup  на параметры по умолчанию по нажатию на кнопук reset      
    config.save( CONFIG_FILE );              // запись установок в EEPROM    
  };
  
  // считываем настройки
  if( !config.load( CONFIG_FILE ) ){ // прочитали , если с ошибкой, то перезаписываем
    _PRN(" write conf def ")
    Log.writeMsg( F("err_conf") );//  сообщение в лог о перезаписи ошибочного setup  
    config.setDefaultField();   // еще раз сбросить установки в значение по умолчанию
    config.save( CONFIG_FILE ); // запись установок в файл
    _PRN("set Def ")
  };

// ???  временно для отладки
  config.resetLoginPassword();
  config.field.login[0] = 'q';  config.field.login[1] = 0;
  config.field.password[0] = '1';    config.field.password[1] = 0;
   
_PRN("after load");
_LOOK_IP(config.field.IP); 
_LOOK_IP(config.field.maskIP); 
_LOOK_IP(config.field.pingIP); 
_LOOK_IP(config.field.gatewayIP); 
_LOOK(config.field.timePing);  
_LOOK(config.field.timeoutPing);  
_LOOK(config.field.maxLosesFromSent);    
_LOOK(config.field.numPingSent);  
_LOOK(config.field.maxLostPing);  
_LOOK(config.field.delayBackSwitch);  
_LOOK(config.field.delayReturnA);  
_LOOK(config.field.stepDelay);  
_LOOK(config.field.maxDelayReturnA);  
_LOOK_IP(config.field.timeServerIP);  
_LOOK(config.field.portTimeServer);      
_LOOK(config.field.date);   
_LOOK(config.field.time);   
_LOOK(config.field.login);   
_LOOK(config.field.password);   

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
  Ethernet.init(CS_W5500); 
  //Ethernet.begin(mac, ip, ip_dns, gateway, subnet);  //  такой вариант работал  
  Ethernet.begin(mac, config.field.IP, config.field.gatewayIP, config.field.gatewayIP, config.field.maskIP);  // считаем что localDNS совпадает с gatewayIP
  server.begin();

  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());
  
  String TS_IP = config.stringIP(config.field.timeServerIP); 
  TS_IP = "89.109.251.21";  // ntp1.vniiftri.ru временно ??
  timeClient.setPoolServerName( TS_IP.c_str() ); // устанавливаем адрес сервера времени
  timeClient.begin(); // config.field.portTimeServer ); ?? // активация клиента времени
_PRN("time request")
  
  counters.load( Tping, config.field.timePing * 1000 );// настраиваем период повторения пинга (переводим в мс)
  counters.load( SyncTimeSrv, 3600 * 1000 );  // период обновления времени с тайм сервера - 1 час
  counters.load( LockSwitch, config.field.delayBackSwitch * 1000);
  D.calcDelayReturnA = config.field.delayReturnA;  // устанавливаем начальную задержку 
  counters.load( returnA, D.calcDelayReturnA * 1000);
  counters.load( timeOutWeb, 1 * 1000);  // тайм аут обслуживания клиента 1 сек
  // не используется counters.load( oneHour, 3600 * 1000);  // таймер на 1 час
  counters.load( press3s, 3 * 1000);  // таймер на 3 сек - длительное нажатие на кнопку
  counters.load( oneMin, 60 * 1000);  // для учета времени работы и др. служебных функций 

  counters.resetAll();  // первичный сброс всех счетчиков, рестарт произойдет в основном цикле после срабатывания условий

    // явно переключаемся на канал А
  digitalWrite(OUT_K, LAN_A_ON);
  digitalWrite(LED_LAN_A, LED_ON);
  digitalWrite(LED_LAN_B, LED_OFF);
 
  ICMPPing::setTimeout( config.field.timeoutPing ); 
  ping.asyncStart(config.field.pingIP, 1, echoResult); //послать первый запрос 
  //  замер количества свободной памяти
  vRAM = memoryFree(); // запоминаем количество с   вободного места в ОЗУ  

  _LOOK(vRAM)
/*
// =====================   временная проверка пинга  - работает на шлюз .... 1.1
// время с сервера так же получает
_LOOK_IP(config.field.pingIP);
while(true){
  _PRN("start ping")
if (! ping.asyncStart(config.field.pingIP, 3, echoResult)){  // если не удалось послать нормально запрос
       _PRN("bad start")
    }
  else { _PRN("OK start")  };  // ожидаем ответа

delay(500);

while( !ping.asyncComplete(echoResult)){  // если ждем ответа  и ответ получен или истек тайм аут  
  delay(100);
};

  // проверяем какой результат пинга
  if (echoResult.status != SUCCESS){  // нет ответа- тайм-аут
    _LOOK(echoResult.status)  }
    else {_LOOK(echoResult.status")};
  
delay(1000);
bool timeRes=timeClient.forceUpdate();
 _LOOK( timeRes)
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

 _PRN("end setup")

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

  uint32_t Tstart = millis();
  //  **********************************************  ЧТЕНИЕ СОСТОЯНИЯ ОБОРУДОВАНИЯ И КОНТРОЛЛЕРА   ************************************************************
  num_cycle++;
  D.Date_Time = watch.gettime("Y-m-d H:i:s");  // обновляем время
  // переносим время в конфигурацию
  config.setField( "date", D.Date_Time.substring(0, D.Date_Time.indexOf(" ")) );
  config.setField( "time", D.Date_Time.substring(D.Date_Time.indexOf(" ")+1) );

// проверенно при периоде опроса тайм сервера 30 сек, на 89.109.251.21- ntp1.vniiftri.ru. нормальный опрос- таймаут 23 раза по 30 сек- сообщение в файл- восстановление нормального опроса (восстановление подключения RJ-45 )
  // если интервал истек, то синхронизируемся с time server  
  if( !( config.field.timeServerIP[0] == 127 && config.field.timeServerIP[1] == 0 && config.field.timeServerIP[2] == 0 && config.field.timeServerIP[3] == 1 ) ){  // вообще по настройкам требуется обращение к тайм серверу ?
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

//  ------------------------ PING -----------------------------
// если истек период проверки результата и посылки следующего пинга то
if( counters.isOver( Tping ) && ping.asyncComplete(echoResult) ){   
  counters.start( Tping ); // перезапуск таймера
// обработка пинга - разобрать какое состояние сейчас после предыдущей отправки
  switch( echoResult.status ){  // если ASYNC_SENT то запрос послан, ожидаем ответ - ничего не делать      
    case SUCCESS: { //если успех
      //проверяем адрес ответившего хоста
      bool eqAddrIP = true;
      for( byte i = 0; i < 4; i++){   eqAddrIP = eqAddrIP & (echoResult.addr[i] == config.field.pingIP[i]); };
      if( eqAddrIP ){  // адрес ответа совпадаетс адресом посылки
          pingResult.set( true );
          digitalWrite(LED_PING, LED_ON);  //  включаем индикатор успешного пинга
          D.lostPing = 0; // сбрасываем счетчик непрпрывно потеряных пакетов          
          if( D.port == 0 ){ //если канал А , то действия связанные с автовозвратом  т.к. был хороший пинг и канал А был исправен
            D.autoReturnA = false;   // был "хороший" пинг, значит автовозврат успешный и увеличивать задержку не надо
            D.calcDelayReturnA = config.field.delayReturnA;
            counters.load( returnA, D.calcDelayReturnA * 1000);   // загружаем в счетчик
          };
          _PRN(" SUCCESS ");
          if( !pingResult.get( 1 ) ){ Log.writeMsg( F("ping_ok") );  }// если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был потерян то сообщение в лог восствновлении связи     
      }
      else{  // ответил другой хост
          pingResult.set( false );
          digitalWrite(LED_PING, LED_OFF);  //  выключаем индикатор успешного пинга
          _PRN(" other host answer ");
          D.lostPing++;
          D.totalLost[D.port]++;    
          if( pingResult.get( 1 ) ){ Log.writeMsg( F( "othr_hst" ) );  }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог об ответе с другого хоста
      };
      break;
    };
    case SEND_TIMEOUT:{  // Время ожидания отправки запроса истекло не могу отправить запрос . вероятно нет линка
      pingResult.set( false );
      digitalWrite(LED_PING, LED_OFF);  //  выключаем индикатор успешного пинга
      _PRN(" send timeout ");
      if( pingResult.get( 1 ) ){ Log.writeMsg( F("fld_ping") ); }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о проблемах      
      break;
    };
    case NO_RESPONSE:{ // превышено время ожидания ответа 
    _PRN(" ping lost ");
      pingResult.set( false );
      digitalWrite(LED_PING, LED_OFF);  //  выключаем индикатор успешного пинга
      D.lostPing++;
      D.totalLost[D.port]++;    
      if( pingResult.get( 1 ) ){ Log.writeMsg( F("lost_png") );  }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о потере связи              
      break;
    };
    case BAD_RESPONSE:{  // получен неверный ответ сервера 
    _PRN(" bad response ");
      pingResult.set( false );
      digitalWrite(LED_PING, LED_OFF);  //  выключаем индикатор успешного пинга
      D.lostPing++;
      D.totalLost[D.port]++;    
      if( pingResult.get( 1 ) ){ Log.writeMsg( F("bad_resp") );  }; // если прошлый пинг (1 - т.к. уже сохранили текущий результат в 0) был нормальный то сообщение в лог о потере связи              
      break;
    };
    // без default:{ };
  }; // switch
_LOOK(echoResult.status)
  if( echoResult.status != ASYNC_SENT ){  // отправка нового пинга, результаты прошлого запроса разобрали выше
    bool f = ping.asyncStart(config.field.pingIP, 1, echoResult); 
    _LOOK(f);
  };
};//  если сработал счетчик Tping


// ----------------  если автоматический режим -----------------
if( D.autoSW){

  // ----------------- ПЕРЕКЛЮЧЕНИЕ НА ДРУГОЙ КАНАЛ, ЕСЛИ ПОТЕРИ PING БОЛЬШЕ ПОГРОГОВ   ---------------------  
  if( ( D.lostPing >= config.field.maxLostPing ) || !pingResult.isOK( config.field.maxLosesFromSent, config.field.numPingSent) ){

    // -----------  возврат на B после неудачного автовозврата на A ( не используем задержку LockSwitch )  -----------------
    if( D.autoReturnA && (D.port == 0) && ( config.field.delayReturnA > 0 ) ){  
        //  если был автовозврат на А  и на А были только потери ( не было сброса autoReturnA) и переключаемся с В на А и автовозврат не заблокирован delayReturnA==0
        changePort( D ); // переход обратно на B  
        //рассчитываем новую задержку с увеличенным шагом, загружаем счетчик новым значением и запускаем счетчик
        D.calcDelayReturnA += stepDelay;
        if( D.calcDelayReturnA > config.field.maxDelayReturnA){  D.calcDelayReturnA = config.field.maxDelayReturnA;  };  // ограничиваем задержку максимальным  maxDelayReturnA            
        counters.load( returnA, D.calcDelayReturnA * 1000);   // загружаем в счетчик          
        counters.start( returnA );  // запустить таймер автовозврата на канал А 
        D.autoReturnA = false; // автовозврат так же сбрасывается при каждом успешном пинге, чтоб исключить приращение времени автовозврата при обычной работе и отсутствии постоянного отказа канала А
    }
    else{ //  ------------------------    ОБЫЧНОЕ ПЕРЕКЛЮЧЕНИЕ ПО ПОТЕРЯМ PING   -------------------
        if( counters.isOver( LockSwitch ) ){  // если потерь подряд или потерь из заданного количества больше порога и таймер запрета перехода на другой канал истек, то    
            changePort( D ); 
            counters.start( LockSwitch );  
            if (D.port == 0) {  // канал А теперь
                Log.writeMsg( F("auto_toA") ); 
            } 
            else{   // канал B теперь
                counters.start( returnA );  // запустить таймер автовозврата на канал А
                Log.writeMsg( F("auto_toB") ); 
            };              
        };
    };
  }; // ПЕРЕКЛЮЧЕНИЕ НА ДРУГОЙ КАНАЛ, ЕСЛИ ПОТЕРИ PING БОЛЬШЕ ПОГРОГОВ 
    // прочая обработка в режиме Auto
    //  ------------------  AUTO RETURN TO A  -------------------------------
    if( (delayReturnA > 0) && ( D.port == 1 ) && counters.isOver( returnA ) ){ // автовозврат настроен и канал В и истекло время автовозврата, то возвращаемся на А
      changePort( D );     
      counters.start(returnA);    
      Log.writeMsg( F("ret_toA") ); 
      D.autoReturnA = true;  // флаг автовозврата на А
    };
    //  ---------------------  СБРОС РЕЖИМА AUTO , если нет параметров для работы в режиме Auto    -------------------
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

 // ========================    обработка запросов к вэб интерфейсу   ==========================
// вроде как для W5500 скорость передачи по spi 8МГц т.е. 1 МБ/сек. Т.к. данные надо считывать из microSD по  spi то получаем 500 кБ сек. 
// за 1 такт в 200 мсек можно передать  порядка 100 кБ информации. Размеры страниц для вэб интерфейса не более 10 кБ. экономить и разделять передачу на такты нет смысла.
 //_LOOK("state in loop= ", _state)
if( webServer.isWait() ){  //если вэб сервер в ожидании 
    client = server.available(); // есть клиент?
    if( client ){ //если есть ожидающий клиент, то запустить вэб сервер и счетчик тайм аута
    _PRN("New request from client:");
    webConfig.load( config );  // копируем текущую конфигурацию для вэб страницы
    webConfig.resetLoginPassword();
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
    webConfig.verifySetup();  // проверяем допустимость установок и корректируем их при необходимости
    applyChangeConfig(); //сравниваем были ли изменения в конфигурацию переносим изменения в основную конфигурацию и отрабатываем изменения


_PRN("after web ----------------------------------------------------------------------------------");
_LOOK_IP(config.field.IP); 
_LOOK_IP(config.field.maskIP); 
_LOOK_IP(config.field.pingIP); 
_LOOK_IP(config.field.gatewayIP); 
_LOOK(config.field.timePing);  
_LOOK(config.field.timeoutPing);  
_LOOK(config.field.maxLosesFromSent);    
_LOOK(config.field.numPingSent);  
_LOOK(config.field.maxLostPing);  
_LOOK(config.field.delayBackSwitch);  
_LOOK(config.field.delayReturnA);  
_LOOK(config.field.stepDelay);  
_LOOK(config.field.maxDelayReturnA);  
_LOOK_IP(config.field.timeServerIP);  
_LOOK(config.field.portTimeServer);      
_LOOK(config.field.date);   
_LOOK(config.field.time);   
_LOOK(config.field.login);   
_LOOK(config.field.password);     

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
          if( D.autoSW ){               
            if( (D.port == 1) && ( delayReturnA > 0 ) ){  //если канал B то запустить счетчик автовозврата
            D.autoReturnA = false; // автовозврат так же сбрасывается при каждом успешном пинге, чтоб исключить приращение времени автовозврата при обычной работе и отсутствии постоянного отказа канала А
            D.calcDelayReturnA = config.field.delayReturnA;  // задержка возврата на А устанавливается по конфигурации, без учета возможных приращений возникших из-за неисправности канала А
            counters.load( returnA, D.calcDelayReturnA * 1000);
            counters.start( returnA );  // запустить таймер автовозврата на канал А ( если мы уже здесь, то автовозврат разрешен)
            };
            Log.writeMsg( F("set_auto") );  
          }
          else{ Log.writeMsg( F("set_man") );  };
        }
        else{  // таймер 3 сек не истек, значит короткое нажатие 
            // если режим авто, то отключаем авторежим
            if( D.autoSW ){ D.autoSW = false; }
            else{  // если раежим ручного управления, то переключаем порт
                changePort( D ); 
                if (D.port == 0) {  Log.writeMsg( F("man_to_A") ); }  // канал А теперь
                else{  Log.writeMsg( F("man_to_B") );   }  // канал B теперь
            };            
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
// вывод индикации в соотвтествии с состоянием устройства
if( D.autoSW ){ digitalWrite( LED_AUTO, LED_ON); }
else{ digitalWrite( LED_AUTO, LED_OFF); };

//  учет времени работы и расчет статистики
D.lostPerMin[D.port] = (60.0 * pingResult.numLost() ) / ( LEN_BUF_RES * config.field.timePing);  //  с переводом в потерянные за минуту
if( counters.isOver( oneMin ) ){
    counters.start( oneMin );
    // учет статистики
    D.workTime[D.port]++;
    D.upTime = (D.workTime[0] + D.workTime[1]) / 60;    
};

counters.cycleAll();  // работа счетчиков  задержки

  //  ВРЕМЯ ИСПОЛНЕНИЯ ЦИКЛА  - измеренное время цикла  не более ?? мс
  //  ?? временно
  uint32_t Tcycle = millis() - Tstart;
  _LOOK( Tcycle)
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
    bool change = false;
    for( byte i = 0; i < 4; i++ ){ change = change || ( config.field.IP[i] != webConfig.field.IP[i] ); };
    for( byte i = 0; i < 4; i++ ){ change = change || ( config.field.maskIP[i] != webConfig.field.maskIP[i] ); };
    for( byte i = 0; i < 4; i++ ){ change = change || ( config.field.gatewayIP[i] != webConfig.field.gatewayIP[i] ); };

    if( change ){  // если есть изменения
          // запоминаем все из конфига
          for( byte i = 0; i < 4; i++ ){ 
            config.field.IP[i] = webConfig.field.IP[i]; 
            config.field.maskIP[i] = webConfig.field.maskIP[i];
            config.field.gatewayIP[i] = webConfig.field.gatewayIP[i];            
          };
          digitalWrite(W5500_RESET, LOW);
          delay(1000);
          digitalWrite(W5500_RESET, HIGH);
          delay(1000);
          Ethernet.begin(mac, config.field.IP, config.field.gatewayIP, config.field.gatewayIP, config.field.maskIP);  // считаем что localDNS совпадает с gatewayIP
          server.begin();
          Log.writeMsg( F("set_net.txt") );
               
    };  // если есть изменения в сетевых настройках

    change = false;
    for( byte i = 0; i < 4; i++ ){ change = change || ( config.field.pingIP[i] != webConfig.field.pingIP[i] ); };
    if( change ){ 
        for( byte i = 0; i < 4; i++ ){ config.field.pingIP[i] = webConfig.field.pingIP[i]; };
        Log.writeMsg( F("set_ping.txt") );        
     };

    if( config.field.timePing != webConfig.field.timePing ){ 
      config.field.timePing = webConfig.field.timePing;
      counters.load( Tping, config.field.timePing * 1000 );// настраиваем период повторения пинга (переводим в мс)
      counters.start( Tping );
      Log.writeMsg( F("t_ping.txt") );
    };  // период повтора ping
    if( config.field.timeoutPing != webConfig.field.timeoutPing ){ config.field.timeoutPing = webConfig.field.timeoutPing;     ICMPPing::setTimeout( config.field.timeoutPing );  Log.writeMsg( F("to_ping.txt") );  };  // тайма-аут пинга в мсек, изменится со следующего пинга    
    if( config.field.maxLosesFromSent != webConfig.field.maxLosesFromSent ){ config.field.maxLosesFromSent = webConfig.field.maxLosesFromSent; Log.writeMsg( F("l_ping.txt") );  }; // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    if( config.field.numPingSent != webConfig.field.numPingSent ){  config.field.numPingSent = webConfig.field.numPingSent;  Log.writeMsg( F("ping_s.txt") );   };      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    if( config.field.maxLostPing != webConfig.field.maxLostPing ){ config.field.maxLostPing = webConfig.field.maxLostPing;  Log.writeMsg( F("max_lost.txt") );   };  // предельное число подряд (непрерывно) потеряных пакетов
    if( config.field.delayBackSwitch != webConfig.field.delayBackSwitch ){
      config.field.delayBackSwitch = webConfig.field.delayBackSwitch;
      counters.load( LockSwitch, config.field.delayBackSwitch * 1000);
      counters.start( LockSwitch );
      Log.writeMsg( F("t_lock.txt") );
     };  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    if( config.field.delayReturnA != webConfig.field.delayReturnA ){ 
      config.field.delayReturnA = webConfig.field.delayReturnA;  // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
      D.calcDelayReturnA = config.field.delayReturnA;  // устанавливаем начальную задержку  
      counters.load( returnA, D.calcDelayReturnA * 1000);      // здесь фактически происходит рестарт счетчика если он работал или просто установка нового значения      
      Log.writeMsg( F("d_ret_a.txt") ); 
      } ;   
    if( config.field.stepDelay != webConfig.field.stepDelay ){ config.field.stepDelay = webConfig.field.stepDelay;   Log.writeMsg( F("step_d.txt") );    };      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    if( config.field.maxDelayReturnA != webConfig.field.maxDelayReturnA ){   config.field.maxDelayReturnA = webConfig.field.maxDelayReturnA;   Log.writeMsg( F("max_d.txt") );  };  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки

    change = false;
    for( byte i = 0; i < 4; i++ ){ change = change || ( config.field.timeServerIP[i] != webConfig.field.timeServerIP[i] ); };
    if( change ){
        for( byte i = 0; i < 4; i++ ){ config.field.timeServerIP[i] = webConfig.field.timeServerIP[i]; };
        // больше ничего не делаем, т.к. все изменения произойдут только после сохранения config и перезагрузки
         Log.writeMsg( F("set_tsip.txt") );
      };
    
    if( config.field.portTimeServer != webConfig.field.portTimeServer ){   
      config.field.portTimeServer = webConfig.field.portTimeServer; 
      // больше ничего не делаем, т.к. все изменения произойдут только после сохранения config и перезагрузки      
      Log.writeMsg( F("set_ts_p.txt") );      
      };    

    // если таймсервер 127.0.0.1
    if( (webConfig.field.timeServerIP[0] == 127) && (webConfig.field.timeServerIP[1] == 0) && (webConfig.field.timeServerIP[2] == 0) && (webConfig.field.timeServerIP[3] == 1) ){ 
      // если время или дата не совпадают  
      if( ( strcmp( config.field.date, webConfig.field.date ) != 0 ) || ( strcmp( config.field.time, webConfig.field.time ) != 0 ) ){ 
          String d = webConfig.getField("date");
          String t = webConfig.getField("time");   
          //	(сек, мин, час, день, мес, год, день_недели)
          watch.settime( t.substring( t.lastIndexOf(":") + 1 ).toInt(), // сек
                         t.substring( t.indexOf(":") +1, t.lastIndexOf(":") ).toInt(),  // мин
                         t.substring( 0, t.indexOf(":")).toInt(),       // час      
                         d.substring( d.lastIndexOf("-") + 1 ).toInt(), // день
                         d.substring( d.indexOf("-") +1, d.lastIndexOf("-") ).toInt(),  // мес
                         d.substring( 0, d.indexOf("-")).toInt()       // год
          );		// день недели не указываем
          Log.writeMsg( F("change_t.txt") );
      };
    };    
}
