#pragma once
#include <Arduino.h>

#define POSTFIX_CMD ".cmd"   // расширение в имени файла в запросе показывающее, что это запрос на выполнение команды

#define MAX_LEN_CMD 8 // длина массива с названиями команд
// символьные имена команд
const char reboot[] PROGMEM = "reboot";  
const char save[] PROGMEM = "save_reboot";
const char defConf[] PROGMEM = "def_config";  
const char clearLog[] PROGMEM = "clear_log";
const char changeP[] PROGMEM = "change_port";  
const char logOut[] PROGMEM = "exit";
const char reserve2[] PROGMEM = "noCMD";  
const char reserve3[] PROGMEM = "noCMD";

class Tcmd{

public:
  Tcmd();                 // конструктор
  bool execute( String line ); // выполнение команды

};

extern Tcmd cmd;