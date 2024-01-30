#pragma once
#include <Arduino.h>
#include "bufPing.h"
#include "debug.h"

  TpingResult::TpingResult(){// конструктор
      for(uint16_t i = 0; i < LEN_BUF_RES; i++){ pings[i] = true;  };
  }
  
  void TpingResult::clear(){  // очистка буфера
      for(uint16_t i = 0; i < LEN_BUF_RES; i++){ pings[i] = true;  };
  }
  
  void TpingResult::set( bool res ){   // сохранение текущего результата пинга в буфер
      // сдвигаем все вперед
      for( uint16_t i = LEN_BUF_RES-1; i > 0; i--){  pings[i] = pings[i-1];   };
      // сохраняем новое значение
      pings[0] = res;
      for( uint16_t i = 0; i < LEN_BUF_RES; i++){  Serial.print(pings[i]);  };
      Serial.println();
  }
  
  bool TpingResult::get( uint16_t n ){    // запрос n ого результата из буфера
      if(n < LEN_BUF_RES ){ return pings[n];  }
      else{ return false; }
  }
  
  bool TpingResult::isOK( uint16_t _maxLosesFromSent, uint16_t _numPingSent ){  // проверка превышено ли количество потерянных пакетов из общего количества отправленных, если НЕ превышено, то - true
      // проверка входных данных
      if( (_maxLosesFromSent >= LEN_BUF_RES) || ( _numPingSent >= LEN_BUF_RES) || (_maxLosesFromSent > _numPingSent ) ){ return false;   };   
      int lost = 0;
      for(uint16_t i = 0; i < _numPingSent; i++){ 
        if( !pings[i] ){ lost++; }
      };
      if( lost >= _maxLosesFromSent ){ return false; }
      else{ return true; };
  }

 uint16_t TpingResult::numLost(){  // подстчет количества потерянных пакетов во всем буфере
      uint16_t lost = 0;
      for(uint16_t i = 0; i < LEN_BUF_RES; i++){ 
        if( !pings[i] ){ lost++; }
      };
      return lost;
  }
