#pragma once
#include <Arduino.h>
#include "common.h"
#include "log.h"
#include "iarduino_RTC.h" 
#include "SD.h"
#include "debug.h"

Tlog::Tlog(){    // конструктор
}

void Tlog::clear(){   // очистка файла лога

}

bool Tlog::write(String msg){  // записать в лог файл сообщение msg. К сообщению дописывается текущая дата и время  
  File logFile = SD.open(LOG_FILE, FILE_WRITE);
  if(!logFile){ return false;};
  String s = D.Date_Time;  
  s = s + " ";
  s = s + msg;  
  _LOOK( s )  
  logFile.println( s );
  _MFREE
  logFile.close();
  return true; 
}

bool Tlog::writeMsg( String msgName){       // записать сообщение по шаблону из файла с именем msgName (папка msg)  
  // открыть файл с сообщением
  File msgFile; 
  String fName = MSG_DIR;
  fName = "/" + fName +"/" + msgName + ".txt";     
  msgFile = SD.open(fName, FILE_READ);  
  if( !msgFile ){   
    //  ?????? добавить запись об исключении - не найден файл ИМЯ ФАЙЛА с информацией для лога  
    return false;
  };    
  uint16_t sizeF= msgFile.size();  
  // ?? добавить проверку, чтоб не пыталось выделить памяти больше чем есть свободной??
  String s;
  s.reserve(sizeF);
  char c;  
  while( (c = msgFile.read()) != -1) {  // читаем в с пока что то есть в файле      
          s = s + c;      
  };  
  msgFile.close();// закрыть файл
  //  записать в лог с датой и временм  
  _LOOK( s)
  write(s); // пишем в лог
  return true; 
}

bool Tlog::beginRead(){ // начинаем чтение сообщений из лога
  File logFile = SD.open(LOG_FILE, FILE_READ);
 // ????как читать произвольно
}

String Tlog::read(){  // читаем очередное сообщение  
  //?? читать строку
}

static Tlog Log = Tlog();
