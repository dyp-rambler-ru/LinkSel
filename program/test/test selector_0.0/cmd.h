#pragma once
#include <Arduino.h>

#define MAX_LEN_CMD 8 // длина массива с названиями команд
// символьные имена команд
const char reboot[] PROGMEM = "reboot";  
const char save[] PROGMEM = "defConf";
const char defConf[] PROGMEM = "defConf";  
const char clearLog[] PROGMEM = "clearLog";
const char changeP[] PROGMEM = "changePort";  
const char logOn[] PROGMEM = "logOn";
const char reserve2[] PROGMEM = "noCMD";  
const char reserve3[] PROGMEM = "noCMD";

/*
// массив символьных имен параметров i=  0          1        2         3          4           5         6        7 
const char* const names[] PROGMEM = {  reboot,    save,   defConf, clearLog,  changeP,   logOn,  reserve2,  reserve3 };
*/