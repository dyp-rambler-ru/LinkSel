//  библиотека отладочных макросов
#pragma once
#include <Arduino.h>
#include "common.h"

// останов программы бесконечным циклом
#define _HALT while( true ){ delay(100);};
// печать парамера
#define _SEE(x) Serial.print( __func__ ); Serial.print("(");  Serial.print( __LINE__ );  Serial.print(") ");  Serial.print(#x); Serial.print("="); Serial.println( (x) );
        
#define _PRN(x) Serial.print( __func__ ); Serial.print("(");  Serial.print( __LINE__ );  Serial.print(") ");  Serial.println( (x) );

#define _SEE_IP(x) Serial.print( __func__ ); Serial.print("(");  Serial.print( __LINE__ );  Serial.print(") ");  Serial.print(#x);  Serial.print("="); Serial.print( (x[0]) ); Serial.print("."); Serial.print( (x[1]) ); Serial.print("."); Serial.print( (x[2]) ); Serial.print("."); Serial.println( (x[3]) ); 

#define _MFREE Serial.print( __func__ ); Serial.print("(");  Serial.print( __LINE__ );  Serial.print(") ");  Serial.print(" Memory FREE= "); Serial.println(memoryFree() );

// _BP(0) - точка останова, пока не введен любой символ
// если выражение х ложно, то есть точка останова, иначе пропуск
#define _BP(x) if( (x) == 0 ){ while( Serial.available() > 0 ){ Serial.read(); }; Serial.print( __LINE__ ); Serial.println("--BREAKPOINT-- Press ENTER for continue."); while( Serial.available() == 0 ){ delay(10); }; };

/*
#define _SET(x) {  Serial.setTimeout(3600 * 1000); while( Serial.available() > 0 ){ Serial.read(); };  Serial.print( __LINE__ ); Serial.print(F("--SET >> "));      \
                   Serial.print( #x );  x = __inputSetDebug( x );   Serial.print(" Set value ");  Serial.print( #x ); Serial.print("=");  Serial.println( x ); };


uint16_t __inputSetDebug( uint16_t x ){
  Serial.print(F("? "));   
  x = (uint16_t) Serial.parseInt();
  Serial.println();
  return x;
}

uint32_t __inputSetDebug( uint32_t x ){
  Serial.print(F("? "));   
  x = (uint32_t) Serial.parseInt();
  Serial.println();
  return x;
}

char __inputSetDebug( char x ){ // ввод одного символа
  Serial.print("? ");   
  while( (x = Serial.read()) == -1 ){ delay(10);};
  return x;
}

int16_t __inputSetDebug( int16_t x ){
  Serial.print("? ");   
  x = (int16_t) Serial.parseInt();
  Serial.println();
  return x;
}

int32_t __inputSetDebug( int32_t x ){
  Serial.print("? ");   
  x = (uint32_t) Serial.parseInt();
  Serial.println();
  return x;
}
*/


