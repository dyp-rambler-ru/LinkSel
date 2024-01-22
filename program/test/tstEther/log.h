#pragma once
#include <Arduino.h>
#include "SD.h"
#include "iarduino_RTC.h" 

//библиотека для записи сообщений в лог файл
#define LOG_FILE "log.txt"  //имя лог файла
#define MSG_DIR "msg"  //имя каталога с файлами сообщений, каждое сообщение храниться в отдельном файле типа <имя сообщения>.msg


 class Tlog {
    
  public:
  Tlog();                               // конструктор
  void ini( iarduino_RTC *watch );      //  инициализация
  void clear();                         // очистка файла лога
  bool write( String msg );            // записать в лог файл сообщение - строку msg. К сообщению дописывается текущая дата и время
  bool writeMsg( String msgName);      // записать сообщение по шаблону из файла с именем msgName (папка msg, имя без расширения, расширение у всех файлов txt)
  bool beginRead();                     // начинаем чтение сообщений из лога
  String read();                        // читаем очередное сообщение  
  
 private:
  iarduino_RTC *_watch;  // переменная часов
  uint16_t _pos;  // позиция в файле
 
};

// -------------------------------------------------------------------------------------------------------------------------------------
static Tlog Log = Tlog();
//Tlog Log = Tlog();
