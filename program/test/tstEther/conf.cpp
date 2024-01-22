#include "conf.h"
#include "common.h"
//#include <avr/eeprom.h>
#include "crcDallas.h"
#include "SD.h"
#include "debug.h"

TSetup::TSetup(){// конструктор

}

String TSetup::getField( String name ){     // выдает параметр из Setup по его символьному имени ( например для вставки в вэб страицу). Параметр выдается в виде текстовой строки если параметр не найден то выдает "??????"
  // индусский код детектед
  if( name == String(login) ){   String s="";    s.toCharArray( field.login, 8);    return s;   };
  if( name == String(password) ){   String s="";   s.toCharArray( field.password, 8);    return s;   };
  if( name == String(IP) ){  return String(field.IP[0]) + "." + String(field.IP[1]) + "." + String(field.IP[2]) + "." + String(field.IP[3]); };
  if( name == String(maskIP) ){ return String(field.maskIP[0]) + "." + String(field.maskIP[1]) + "." + String(field.maskIP[2]) + "." + String(field.maskIP[3]);};
  if( name == String(pingIP) ){ return String(field.pingIP[0]) + "." + String(field.pingIP[1]) + "." + String(field.pingIP[2]) + "." + String(field.pingIP[3]); };
  if( name == String(timePing) ){ return String( field.timePing ); };
  if( name == String(timeoutPing) ){ return String( field.timeoutPing ); };
  if( name == String(maxLosesFromSent) ){ return String( field.maxLosesFromSent ); };
  if( name == String(numPingSent) ){ return String( field.numPingSent ); };  
  if( name == String(maxLostPing) ){ return String( field.maxLostPing ); };   
  if( name == String(delayBackSwitch) ){ return String( field.delayBackSwitch ); };
  if( name == String(delayReturnA) ){ return String( field.delayReturnA ); };  
  if( name == String(stepDelay) ){ return String( field.stepDelay ); };    
  if( name == String(maxDelayReturnA) ){ return String( field.maxDelayReturnA ); };
  if( name == String(timeServerIP) ){ return String(field.timeServerIP[0]) + "." + String(field.timeServerIP[1]) + "." + String(field.timeServerIP[2]) + "." + String(field.timeServerIP[3]);  };  
  if( name == String(portTimeServer) ){ return String( field.portTimeServer ); };  
  //а если не нашли имени
  return NOT_CONFIG_VARIABLE;
}

uint32_t TSetup::limitField( uint32_t val, byte n){
  if( val > maxVal[n] ){ return maxVal[n]; };
  if( val < minVal[n] ){ return minVal[n]; };
  return val;
}

bool TSetup::setField( String name, String val ){ // устанавливает значение поля Setup по символьнму имени, возвращает true , если параметр имеет допустимое значение и был установлен
  
  if( name == login ){ val.toCharArray(field.login, 8); return true; };
  if( name == password ){ val.toCharArray(field.password, 8); return true; };
  if( name == IP ){
    for( byte i = 0; i < 3; i++){ field.IP[i] = limitField( (val.substring( i*4, (i+1)*4 )).toInt(), 2);  };  
    field.IP[3] = limitField( (val.substring(12)).toInt(), 2);  // дописываем последний 
    return true;
  };  
  if( name == maskIP ){ 
    for( byte i = 0; i < 3; i++){ field.maskIP[i] = limitField( (val.substring( i*4, (i+1)*4 )).toInt(), 3);  };  
    field.maskIP[3] = limitField( (val.substring(12)).toInt(), 3);  // дописываем последний 
    return true;
  };
  if( name == pingIP ){ 
    for( byte i = 0; i < 3; i++){ field.pingIP[i] = limitField( (val.substring( i*4, (i+1)*4 )).toInt(), 4);  };  
    field.pingIP[3] = limitField( (val.substring(12)).toInt(), 4);  // дописываем последний 
    return true;
  };  
  if( name == timePing ){ field.timePing = limitField( val.toInt(), 5);   return true; };
  if( name == timeoutPing ){ field.timeoutPing = limitField( val.toInt(), 6);   return true; }; 
  if( name == maxLosesFromSent ){ field.maxLosesFromSent = limitField( val.toInt(), 7);   return true; };
  if( name == numPingSent ){ field.numPingSent = limitField( val.toInt(), 8);   return true; };  
  if( name == maxLostPing ){ field.maxLostPing = limitField( val.toInt(), 9);   return true; };  
  if( name == delayBackSwitch ){ field.delayBackSwitch = limitField( val.toInt(), 10);   return true; };
  if( name == delayReturnA ){ field.delayReturnA = limitField( val.toInt(), 11);   return true; };
  if( name == stepDelay){ field.stepDelay  = limitField( val.toInt(), 12);   return true; };
  if( name == maxDelayReturnA ){ field.maxDelayReturnA = limitField( val.toInt(), 13);   return true; };
  if( name == timeServerIP ){ 
    for( byte i = 0; i < 3; i++){ field.timeServerIP[i] = limitField( (val.substring( i*4, (i+1)*4 )).toInt(), 14);  };  
    field.timeServerIP[3] = limitField( (val.substring( 12)).toInt(), 14);  // дописываем последний     
    return true;
  };  
  if( name == portTimeServer ){  field.portTimeServer = limitField( val.toInt(), 15);   return true; };
}


void TSetup::setDefaultField(){   // сбросить установки в значение по умолчанию
    // сетевые адреса    
    field.IP[0]  = 192;  field.IP[1] = 168; field.IP[2] = 1;  field.IP[3] = 222;  //  0-й элемент массива - старший октет
    field.maskIP[0] = 255;    field.maskIP[1] = 255;   field.maskIP[2] = 255;    field.maskIP[3] = 0;  
    field.pingIP[0] = 192;   field.pingIP[1] = 168;  field.pingIP[2] = 1;  field.pingIP[3] = 1;  //  пингуемый адрес        
    field.timePing = 1;  // период повтора ping
    field.timeoutPing = 800;  // тайма-аут пинга в мсек    
    field.maxLosesFromSent = 15; // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    field.numPingSent = 200;      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    field.maxLostPing = 10;  // предельное число подряд (непрерывно) потеряных пакетов
    field.delayBackSwitch = 30;  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    field.delayReturnA = 3600;   // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
    field.stepDelay = 3600;      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    field.maxDelayReturnA = 86400;  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки
    field.timeServerIP[0] = 192;  field.timeServerIP[1] = 168;  field.timeServerIP[2] = 1;   field.timeServerIP[3] = 1;  
    field. portTimeServer = 123;  
    char tmp[] = "admin   ";
    for( byte i=0; i<8; i++){   field.login[i] = tmp[i];    field.password[i] = tmp[i];    };
    crc = CrcDallas::crc8((byte *)&field, sizeof(field));  // sizeof(field)  = рассчитываем контрольную сумму вместе с записанной, в результате должен быть 0  
    
}


bool TSetup::load(){              // чтение установок из EEPROM  , возвращает истину, если контрольная сумма в норме
  byte eeprom_crc;    
  byte *data = (byte*)&field; // data - указатель на field типа Tfield, как на последовательность байт
  word size = sizeof( field );  // определяем длину сетапа в байтах  
  for (int i = 0; i < size; i++) {
    *data = eeprom_read_byte(OFFSET_EEPROM + i);
    data++;
  }
  eeprom_crc = eeprom_read_byte(OFFSET_EEPROM + size);  // дочитываем контрольную сумму из eeprom
  byte thisHash = CrcDallas::crc8((byte *)&field, sizeof(field));  // sizeof(Setup)  = рассчитываем контрольную сумму вместе с записанной, в результате должен быть 0
  if (thisHash == eeprom_crc) {
    crc = eeprom_crc;
    return true;
  } else {  return false;  };  
}

void TSetup::save(){              // запись установок в EEPROM  ДОЛГАЯ ОПЕРАЦИЯ
  byte *data = (byte*)&field; // data - указатель на field типа Tfield, как на последовательность байт 
  word size = sizeof( field );  // определяем длину сетапа в байтах
  //byte bArray = malloc( sizeSetup );  // создаем массив байт для setup
    // считаем контрольную сумму и добавляем в последнее поле структуры crc
  byte eeprom_crc = CrcDallas::crc8((byte *)&field, size);  // рассчитываем контрольную сумму  
  for (byte i = 0; i < size; i++) {
    eeprom_update_byte(OFFSET_EEPROM + i, *data++);
    delay(2);
  };
  // дописываем контрольную сумму
  eeprom_update_byte(OFFSET_EEPROM + size, eeprom_crc);
}

bool TSetup::load( String fileName ){ // чтение установок из файла  , контрольная сумма не формируется, возвращает false если файл не найден или в нем нет всех параметров
  File f;  
  char c;
  bool res = true;
  f = SD.open( fileName, FILE_READ );  // открываем для записи  
  while (f.available() > 0 ){  // пока не конец файла
    String s = "";
    // читать строку из файла
    while ((c = f.read()) != -1) {  // читаем в с пока что то есть в файле    
        // если есть возврат каретки или перевод строки, то выход из цикла
        if ((c == '\r') || (c == '\n')) { break; };
        // если ничего не сработало и дошли сюда то добавляем символ в строку
        if( c !=' ' ){  s = s + c;  };  // если не пробел, то запоминаем символ, проблеы пропускаются
      };
      // пропускаем все возвраты и переводы строки  CR, LF и CR+LF, табуляции для следующего чтения
    while ( (c = f.peek()) != -1 ) {
      if ((c == '\n') || (c == '\r') ) { f.read(); };  //читать символ
    };
    // разбить на имя и параметр    
    int n = s.indexOf("="); 
    String name = s.substring(0, n);  //   выделить до символа = имя
    String val = s.substring(n + 3);
    res = res && setField( name, val );  // устанавливаем параметр и учитываем успешность этого в возвращаемом значении
  };    
  f.close();
  return res;
}

void TSetup::save( String fileName ){ // запись установок в файл
  File f;
  SD.remove( fileName );  // удаляем старую конфигурацию
  f = SD.open( fileName, FILE_WRITE );  // открываем для записи  
  for( byte i = 0; i < MAX_LEN_NAMES; i++) {
      char buf[strlen_P(names[i]) + 1];
      strcpy_P(buf, names[i]); 
      String s = String(buf);  // записали имя
      s = s + " = ";
      s = s + getField( names[i] );
      f.println( s );
  };
  f.close();
}

bool TSetup::check(){             // подсчет контрольной суммы Setup и сравнение с имеющейся
  word size = sizeof( field );  // определяем длину сетапа в байтах
  byte tst_crc = CrcDallas::crc8((byte *)&field, size );  // рассчитываем контрольную сумму
  return tst_crc == crc;
}
