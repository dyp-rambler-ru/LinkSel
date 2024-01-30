#pragma once
#include <Arduino.h>

#define LEN_BUF_RES 200

 class TpingResult {
    
  public:
  TpingResult();                 // конструктор
  void clear();                  // очистка буфера
  void set( bool res );          // сохранение текущего результата пинга в буфер
  bool get( uint16_t n );        // запрос n ого результата из буфера, n=0 - последний результат (самый свежий)
  bool isOK( uint16_t _maxLosesFromSent, uint16_t _numPingSent );  // проверка превышено ли количество потерянных пакетов из общего количества отправленных, если НЕ превышено, то - true
  uint16_t numLost();            // подстчет количества потерянных пакетов во всем буфере
  
 private:
	bool pings[LEN_BUF_RES];      // массив результатов посылки пингов для определения необходирмости переключения на другой канал 
 
};

// -------------------------------------------------------------------------------------------------------------------------------------
//extern TpingResult pingResult;
// единственный экземпляр класса
static TpingResult pingResult = TpingResult();