#pragma once
#include "counters.h"

// =============================================    класс  TCounters (счетчики задержки событий)   =======================================
 TCounters::TCounters(){
  for( byte i = 0; i < CountersN; i++) {   CountersA.counter[ i ] = -1; };  // -1 значит что счетчик остановлен
 }

  void TCounters::cycleAll() {      // уменьшение всех работающих счетчиков
  for( byte i = 0; i < CountersN; i++) {   
    if (CountersA.counter[ i ] >= 0) {   CountersA.counter[ i ]--;  };   // если счетчик запущен , то вычитаем
    }; //for  
  }

  void TCounters::resetAll() {      // сброс всех работавших счетчиков
  for( byte i = 0; i < CountersN; i++) {   CountersA.counter[ i ] = -1; };  // -1 значит что счетчик остановлен  
  }

  void TCounters::reset( TNameCounters i ){      // сброс  счетчика  i
  CountersA.counter[ i ] = -1;  // -1 значит что счетчик остановлен  
  }

  void TCounters::start( TNameCounters i ){      // сброс  счетчика  i
  CountersA.counter[ i ] = CountersA.maxCounter[ i ];  // загружаем стартовое значение счетчика
  }

  bool TCounters::isOn( TNameCounters i ){   // проверка запущен ли уже счетчик, true - если счетчик работает
  if (i==0){   // проверка всех счетчиков Any
    for (byte j=0; j<CountersN; j++) {
      if ( CountersA.counter[ i ] != -1 ) {  return true; }   // если есть запущеный счетчик для Any, то выходим  возвращая истину
      };
    return false; // если не вышли раньше, то запущеных счетчиков нет
  }
  else {
          return CountersA.counter[ i ] != -1;      
  }
 }

  void TCounters::load( TNameCounters i, long int t){        // установка нового предельного значения для счетчика, t - в мсек
      if( t >= 0  ){ // если параметры допустимы
        CountersA.maxCounter[ i ] = t / T_DELAY - 1;
      }        
  }

bool TCounters::isOver( TNameCounters i ){   // проверка отсчитал ли счетчик заданное количество, true - если счетчик сработал
    return CountersA.counter[ i ] == -1;      
 }

 // -------------------------------------------------------------------------------------------------------------------------------------
static TCounters counters = TCounters() ;                                      
