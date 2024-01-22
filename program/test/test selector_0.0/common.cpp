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

void changePort(  TGlobalD& _D ) {  // сменить рабочий порт Ethernet
  _D.lostPing = 0;            // начинаем отсчет подряд потерянных пингов снова на новом канале
  _D.port = (_D.port + 1) % 2; 
  if (_D.port == 0) {  // канал А теперь
    digitalWrite(OUT_K, LAN_A_ON);
    digitalWrite(LED_LAN_A, LED_ON);
    digitalWrite(LED_LAN_B, LED_OFF);    
  } else {  // канал В теперь
    // запустить таймер автовозврата на канал А
    //??counters.start(returnA);
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

String readLn( File f ){  // чтение строки из файла
  String s = "";
  char c;  
  while ((c = f.read()) != -1) {  // читаем в с пока что то есть в файле      
    // если есть возврат каретки или перевод строки, то выход из цикла
    if ((c != '\r') && (c != '\n')) {   s = s + c;   }
    else{  break; };  // если возврат каретки или перевод строки то выходим из цикла
  };
  // пропускаем все возвраты и переводы строки  CR, LF и CR+LF, для следующего чтения из файла
  while ( c = f.peek() != -1 ) {    
    if ((c == '\n') || (c == '\r') ) { f.read(); };  //читать символ
  };
  return s;
}

void resetMCU(){
  /*  ???сброс сетевой карты */ 
  digitalWrite( W5500_RESET, LOW);
  delay(500);
  digitalWrite( W5500_RESET, HIGH);  
  asm volatile ("jmp 0");

}


static TGlobalD D;          // создаем глобальную структуру данных, важно именно с предварительным описанием в .h и объявлением здесь иначе проблемы с видимостью в статических классах