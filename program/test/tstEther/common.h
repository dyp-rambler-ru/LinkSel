#pragma once
#include <Arduino.h>
#include "debug.h"
#include "SD.h"

// перенос в счетчики #define T_DELAY  200  // время основного цикла микроконтроллера в мс, должно быть менее секунды, но более 50 мс 
//( согласно замерам реальное время исполнения  цикла  - не более ?? мс)

#define LED_ON LOW
#define LED_OFF HIGH
// константы отключения/ включения реле (переключения на сеть 1 и сеть2
#define LAN_A_ON LOW
#define LAN_B_ON HIGH

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
#define CS_SD_CARD 42


// дата и время возникновения  АПС или дата и время из часов  в формате D M Y h m s ( день месяц год часы минуты секунды каджое по байту)
struct  TTimeStamp { byte DD;   byte MM;   byte YY;    byte hh;    byte mm;     byte ss; };
 
// ====================================================  ГЛАВНАЯ ГЛОБАЛЬНАЯ СТРУКТУРА  ==================================================================
struct TGlobalD {   
  byte port = 0;  // номер рабочего порта 0- А, 1 - В 
  bool autoSW = false;  // признак работы в автоматическом режиме
  byte lostPing;  // количество непрерывно потеряных пакетов на текущем канала  
  // статистика
  uint32_t workTime[2] = {0,0};   // время работы на порту в минутах   
  uint32_t totalLost[2] = {0,0};   // общее количество потерянных пакетов
  float lostPerMin[2] = {0,0};    // нужно для страницы State   ?? рассчитывать по буферу пингов при получении запроса на информацию. не хранить
  uint16_t upTime = 0;  // время непрерывной работы устройства в часах       
  TTimeStamp Date_Time;     // дата и время из часов, во время работы меняем значения на текущие каждый такт контроллера. При записи параметров - сохраняется время последней записи.  
};

String wordToStr( word n);                    // преобразование беззнакового целого в строку
int memoryFree();                             // функция возвращающая количество свободной оперативной памяти между кучей и стеком     

String copyPMEMtoStr( void* addr, byte len );    //копирование строки из программной памяти    

void changePort();          // сменить рабочий порт Ethernet
String intToStr(int num);   // преобразование целого в строку
String readLn( File f );  // чтение строки из файла

static TGlobalD D;      // создаем глобальную структуру данных
