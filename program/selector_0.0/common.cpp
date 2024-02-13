#pragma once
#include "common.h"
#include "SD.h"

extern int *__brkval;  // указатель на адрес последней выделенной ячейки кучи ( или 0)

String wordToStr(word n) {  // преобразование беззнакового целого в строку
  String s = "";
  while (n > 0) {
    s = (char)(n % 10 + 0x30) + s;
    n = n / 10;
  };
  if (s == "") { s = "0"; };
  return s;
}

String copyPMEMtoStr(void *addr, byte len) {  //копирование строки из программной памяти
  String res = "";
  char arrayBuf[len];  // создаём буфер
  // копируем в arrayBuf при помощи strcpy_P
  strcpy_P(arrayBuf, pgm_read_word(addr));
  // копируем из буфера в строку
  for (byte j = 0; j < len; j++) {
    res = res + (char)arrayBuf[j];
  };
  return res;
}

int memoryFree() {                                                                 // функция возвращающая количество свободной оперативной памяти между кучей и стеком
  int freeValue;                                                                           // последний объект выделенный в куче
  if ((int)__brkval == 0) { freeValue = ((int)&freeValue) - ((int)__malloc_heap_start); }  // куча пустая
  else                                                                                     // куча не пустая, используем последний адрес в куче
  {
    freeValue = ((int)&freeValue) - ((int)__brkval);
  }
  return freeValue;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void changePort( TState& _D ) {  // сменить рабочий порт Ethernet
  _D.lostPing = 0;            // начинаем отсчет подряд потерянных пингов снова на новом канале
  _D.port = (_D.port + 1) % 2; 
  if (_D.port == 0) {  // канал А теперь
    digitalWrite(OUT_K, LAN_A_ON);
    digitalWrite(LED_LAN_A, LED_ON);
    digitalWrite(LED_LAN_B, LED_OFF);    
  } else {  // канал В теперь  
    digitalWrite(OUT_K, LAN_B_ON);
    digitalWrite(LED_LAN_A, LED_OFF);
    digitalWrite(LED_LAN_B, LED_ON);     
  };  
}

String intToStr(int num){   // преобразование целого в строку
  String s="";
  s = wordToStr( abs(num));
  if( num < 0 ){ s = "-" + s;};
  return s;
}

bool readLn( File f, String& s ){  // чтение строки из файла, возвращает false , если достигли конца файла
  s = "";
  char c;
  if( !f.available() ){ return false; };
  while ((c = f.read()) != -1) {  // читаем в с пока что то есть в файле      
    // если есть возврат каретки или перевод строки, то выход из цикла
    if ((c != '\r') && (c != '\n')) {   s = s + c; }
    else{  break; };  // если возврат каретки или перевод строки то выходим из цикла
  };
  // пропускаем все возвраты и переводы строки  CR, LF и CR+LF, для следующего чтения из файла
  while( (f.peek() == '\n') || (f.peek() == '\r') ){ f.read(); };
  if( f.available() == -1){ s = s + F(" \n\r -- end of file --"); return false; }  // вдруг уже конец файла
  else{ return true; };
}

void resetMCU(){
  /*  предварительно сброс сетевой карты */ 
  digitalWrite( W5500_RESET, LOW);
  delay(1000);
  digitalWrite( W5500_RESET, HIGH);  
  asm volatile ("jmp 0");

}

// ================================   класс параметров устройства =========================
TState::TState(){

}

String TState::getParam( String name ){ // выдает параметр из TState по его символьному имени ( например для вставки в вэб страицу). Параметр выдается в виде текстовой строки если параметр не найден то выдает NOT_STATE_VARIABLE
_PRN("-------------------------------    get PARAM  -----------------------------------")
_SEE(name)
  char* c_name = name.c_str();
  // индусский код детектед  
  if( strcmp_P( c_name, pgm_port) == 0 ){  
    if( port == 0 ){  return String("A"); } 
    else if( port == 1 ){  return String("B"); }   
    else{  return String("-"); };
    };
  if( strcmp_P( c_name, pgm_autoSW) == 0 ){ 
    if( autoSW ){  return String("Auto");   }
    else{ return String("Manual");  };
  };
  if( strcmp_P( c_name, pgm_lostPing) == 0 ){  return String(lostPing);   };
  if( strcmp_P( c_name, pgm_workTimeA) == 0 ){  return String(workTime[0]);   };
  if( strcmp_P( c_name, pgm_workTimeB) == 0 ){  return String(workTime[1]);   };
  if( strcmp_P( c_name, pgm_totalLostA) == 0 ){  return String(totalLost[0]);   };
  if( strcmp_P( c_name, pgm_totalLostB) == 0 ){  return String(totalLost[1]);   };
  if( strcmp_P( c_name, pgm_lostPerMinA) == 0 ){  return String(lostPerMin[0]);   };
  if( strcmp_P( c_name, pgm_lostPerMinB) == 0 ){  return String(lostPerMin[1]);   };
  if( strcmp_P( c_name, pgm_upTime) == 0 ){  return String(upTime);   };
  if( strcmp_P( c_name, pgm_Date_Time) == 0 ){  return String(Date_Time);   };
  if( strcmp_P( c_name, pgm_scheduledReset) == 0 ){  return String(scheduledReset);   };
  //а если не нашли имени
  return NOT_STATE_VARIABLE;
}


static TState D;          // создаем глобальную структуру данных, важно именно с предварительным описанием в .h и объявлением здесь иначе проблемы с видимостью в статических классах