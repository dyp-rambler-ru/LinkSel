//  библиотека отладочных макросов
#pragma once
#include <Arduino.h>

// останов программы бесконечным циклом
#define _HALT while( true ){ delay(10);};
// печать парамера: название значение
#define _LOOK(x,y) Serial.print(x); Serial.println(y);

#define _LOOK_IP(x,y) Serial.print(x); Serial.print(y[0]); Serial.print("."); Serial.print(y[1]); Serial.print("."); Serial.print(y[2]); Serial.print("."); Serial.println(y[3]); 