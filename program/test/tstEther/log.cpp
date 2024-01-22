#pragma once
#include <Arduino.h>
#include "log.h"
#include "iarduino_RTC.h" 
#include "SD.h"
#include "debug.h"
#include "common.h"

Tlog::Tlog(){    // конструктор
}

void  Tlog::ini( iarduino_RTC *watch ){//  инициализация    
    _watch = watch;  
 }

void Tlog::clear(){   // очистка файла лога

}

bool Tlog::write(String msg){  // записать в лог файл сообщение msg. К сообщению дописывается текущая дата и время  
  File logFile = SD.open(LOG_FILE, FILE_WRITE);
  if(!logFile){ return false;};
  String timeStamp = (*_watch).gettime("Y-m-d H:i:s");
  String s = timeStamp + " " + msg;
  logFile.println( s );
  logFile.close();
  return true; 
}

bool Tlog::writeMsg( String msgName){       // записать сообщение по шаблону из файла с именем msgName (папка msg)
   String strMsg = "";
  // открыть файл с сообщением
  File msgFile; 
  String fName = MSG_DIR;
  fName = "/" + fName +"/" + msgName + ".txt";   
  _LOOK("FileName: ", fName)
  msgFile = SD.open(fName, FILE_READ);
  _LOOK("msgFile-", msgFile)
  if( !msgFile ){ 
    _LOOK("out", 0)
    return false;
    };  
  _LOOK(" file OK ",0)  
  String s = readLn( msgFile );
  msgFile.close();// закрыть файл
  //  записать в лог с датой и временм  
  _LOOK("msg: ", s)
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

