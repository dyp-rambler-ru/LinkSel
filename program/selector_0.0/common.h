#pragma once
#include <Arduino.h>
#include "debug.h"
#include "SD.h"

#define NOT_STATE_VARIABLE "unknow PARAM"  // признак того, что не нашли переменную в setup

#define LED_ON LOW
#define LED_OFF HIGH
// константы отключения/ включения реле (переключения на сеть 1 и сеть2
#define LAN_A_ON HIGH
#define LAN_B_ON LOW

// определяем выводы 
#define LED_LAN_A 22                            
#define LED_LAN_B 23                          
#define LED_PING 25                        
#define LED_AUTO 24                           

#define SW_RESET_CONF 37
#define SW_SWITCH 35      // переключение LAN1/ LAN2 при нажатии менее 1 сек, переключение AUTO/MANUAL  при нажатии более 3 сек
#define OUT_K 26  // выход включения реле

// RTC
#define CLK 47
#define DAT 48
#define PIN_RST 49

//SPI
#define MOSI 51
#define MISO 50
#define SCK 52
#define CS_W5500 10
#define CS_SD_CARD 30    //42

#define W5500_RESET 40  // сброс сетевой карты

const char portN[] PROGMEM ="port";
const char auto_SW[] PROGMEM ="autoSW";     
const char lostPing[] PROGMEM ="lostPing";   
const char workTimeA[] PROGMEM ="workTimeA";  
const char workTimeB[] PROGMEM ="workTimeB";  
const char totalLostA[] PROGMEM ="totalLostA"; 
const char totalLostB[] PROGMEM ="totalLostB"; 
const char lostPerMinA[] PROGMEM ="lostPerMinA";   
const char lostPerMinB[] PROGMEM ="lostPerMinB";   
const char upTime[] PROGMEM ="upTime";
const char DateTime[] PROGMEM ="Date_Time";
const char scheduledRst[] PROGMEM ="scheduledReset";


// ====================================================  ГЛАВНАЯ ГЛОБАЛЬНАЯ СТРУКТУРА  ==================================================================
class TState {   
  public:
  TState();
  byte port = 0;                  // номер рабочего порта 0- А, 1 - В 
  bool autoSW = false;            // признак работы в автоматическом режиме
  byte lostPing = 0;                  // количество непрерывно потеряных пакетов на текущем канала  
  // статистика
  uint32_t workTime[2] = {0,0};   // время работы на порту в минутах   
  uint32_t totalLost[2] = {0,0};  // общее количество потерянных пакетов
  float lostPerMin[2] = {0,0};    // нужно для страницы State   ?? рассчитывать по буферу пингов при получении запроса на информацию. не хранить
  uint16_t upTime = 0;            // время непрерывной работы устройства в часах       
  String Date_Time;               // дата и время из часов, во время работы меняем значения на текущие каждый такт контроллера. 
  bool scheduledReset = false;    // запланирован перезапуск контроллера по команде в вэб страницы   

  String getParam( String name ); // выдает параметр из TState по его символьному имени ( например для вставки в вэб страицу). Параметр выдается в виде текстовой строки если параметр не найден то выдает NOT_STATE_VARIABLE
};

String wordToStr( word n);        // преобразование беззнакового целого в строку
int memoryFree();                 // функция возвращающая количество свободной оперативной памяти между кучей и стеком     
String copyPMEMtoStr( void* addr, byte len );    //копирование строки из программной памяти    
void changePort( TState _D );  // сменить рабочий порт Ethernet
String intToStr(int num);         // преобразование целого в строку
bool readLn( File f, String& s ); // чтение строки из файла, возвращает false , если достигли конца файла
void resetMCU();                  // сброс контроллера

extern TState D;          // создаем глобальную структуру данных о состоянии устройства
