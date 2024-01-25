#include "conf.h"
#include "common.h"
#include "SD.h"
#include "debug.h"

TSetup::TSetup(){// конструктор

}

String TSetup::getField( String name ){     // выдает параметр из Setup по его символьному имени ( например для вставки в вэб страицу). Параметр выдается в виде текстовой строки если параметр не найден то выдает "??????"
  char* c_name = name.c_str();
  // индусский код детектед  
  if( strcmp_P( c_name, login) == 0 ){  return String(field.login);   };
  if( strcmp_P( c_name, password) == 0 ){  return String(field.password);   };
  if( strcmp_P( c_name, IP) == 0 ){  return String(field.IP[0]) + "." + String(field.IP[1]) + "." + String(field.IP[2]) + "." + String(field.IP[3]); };
  if( strcmp_P( c_name, maskIP) == 0 ){ return String(field.maskIP[0]) + "." + String(field.maskIP[1]) + "." + String(field.maskIP[2]) + "." + String(field.maskIP[3]);};
  if( strcmp_P( c_name, gatewayIP) == 0 ){ return String(field.gatewayIP[0]) + "." + String(field.gatewayIP[1]) + "." + String(field.gatewayIP[2]) + "." + String(field.gatewayIP[3]);};
  if( strcmp_P( c_name, pingIP) == 0 ){ return String(field.pingIP[0]) + "." + String(field.pingIP[1]) + "." + String(field.pingIP[2]) + "." + String(field.pingIP[3]); };  
  if (strcmp_P( c_name, timePing) == 0) { return String( field.timePing ); };
  if( strcmp_P( c_name, timeoutPing) == 0 ){ return String( field.timeoutPing ); };
  if( strcmp_P( c_name, maxLosesFromSent) == 0 ){ return String( field.maxLosesFromSent ); };
  if( strcmp_P( c_name, numPingSent) == 0 ){ return String( field.numPingSent ); };  
  if( strcmp_P( c_name, maxLostPing) == 0 ){ return String( field.maxLostPing ); };   
  if( strcmp_P( c_name, delayBackSwitch) == 0 ){ return String( field.delayBackSwitch ); };
  if( strcmp_P( c_name, delayReturnA) == 0 ){ return String( field.delayReturnA ); };  
  if( strcmp_P( c_name, stepDelay) == 0 ){ return String( field.stepDelay ); };    
  if( strcmp_P( c_name, maxDelayReturnA) == 0 ){ return String( field.maxDelayReturnA ); };
  if( strcmp_P( c_name, timeServerIP) == 0 ){ return String(field.timeServerIP[0]) + "." + String(field.timeServerIP[1]) + "." + String(field.timeServerIP[2]) + "." + String(field.timeServerIP[3]);  };  
  if( strcmp_P( c_name, portTimeServer) == 0 ){ return String( field.portTimeServer ); }; 
  
  if( strcmp_P( c_name, time) == 0 ){ return String(field.time); }; 
  if( strcmp_P( c_name, date) == 0 ){ return String(field.date); };  
  //а если не нашли имени
  return NOT_CONFIG_VARIABLE;
}

bool limitField( byte min, byte& val, byte max){
  if( val > max ){ val = max; return true; };
  if( val < min ){ val = min; return true; };
  return false;
};

bool limitField( uint16_t min, uint16_t& val, uint16_t max){
  if( val > max ){ val = max; return true; };
  if( val < min ){ val = min; return true; };
  return false;
};

bool limitField( uint32_t min, uint32_t& val, uint32_t max){
  if( val > max ){ val = max; return true; };
  if( val < min ){ val = min; return true; };
  return false;
};

bool TSetup::verifySetup(){  //  проверка корректности данных в сетапе, нужно для сохранения корректных данных. Если данные не корректны, то они меняются на максимальное/минимальное допустимое значение

    bool flag = false;
    flag = flag | limitField( 1, field.timePing, 120);  // период повтора ping
    flag = flag | limitField( 100, field.timeoutPing, 5000);  // тайма-аут пинга в мсек    
    flag = flag | limitField( 0, field.maxLosesFromSent, 200); // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    flag = flag | limitField( 0, field.numPingSent, 200);      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    flag = flag | limitField( 0, field.maxLostPing, 200);  // предельное число подряд (непрерывно) потеряных пакетов
    flag = flag | limitField( 0, field.delayBackSwitch, 120);  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    flag = flag | limitField( 0, field.delayReturnA, 3600*24);   // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
    flag = flag | limitField( 1, field.stepDelay, 3600*24);      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    flag = flag | limitField( 1, field.maxDelayReturnA, 3600*24);  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки
    flag = flag | limitField( 1, field.portTimeServer, 64000);    

    String s = String(field.date);
    int yyyy = s.substring( 0, s.indexOf("-") ).toInt(); 
    int MM = s.substring( s.indexOf("-")+1, s.lastIndexOf("-") ).toInt();
    int dd = s.substring( s.lastIndexOf("-") ).toInt();
    bool lim = false;  // пока ограничения не срабатывали
    lim = ( yyyy < 2000 ) || ( yyyy > 2100 ) || ( MM < 1 ) || ( MM > 12 ) || ( dd < 1 ) || ( dd > 31 );
    flag = flag | lim;
    if ( lim ){   String s = DEFAULT_DATE;   s.toCharArray(field.date, 11);  };
    
    s = String(field.time);
    int hh = s.substring( 0, s.indexOf(":") ).toInt(); 
    int mm = s.substring( s.indexOf(":")+1, s.lastIndexOf(":") ).toInt();
    int ss = s.substring( s.lastIndexOf(":") ).toInt();
    lim = false;   // пока ограничения не срабатывали
    lim = ( hh < 0 ) || ( hh > 23 ) || ( mm < 0 ) || ( mm > 59 ) || ( ss < 0 ) || ( ss > 59 );
    flag = flag | lim;
    if ( lim ){   String s = DEFAULT_TIME;   s.toCharArray(field.time, 9);  };
    //  login[8]  и    password[8] не проверяются
    // доп проверки
    if( field.timeoutPing > field.timePing * 1000 ){ field.timeoutPing = field.timePing * 1000; flag = true; };
    if( field.maxLosesFromSent < field.maxLostPing ){ field.maxLosesFromSent = field.maxLostPing;  flag = true; };
    if( field.maxDelayReturnA < field.delayReturnA ){ field.maxDelayReturnA = field.delayReturnA;  flag = true; };

    return flag;
 } 

bool TSetup::setField( String name, String val ){ // устанавливает значение поля Setup по символьнму имени, возвращает true , если параметр имеет допустимое значение и был установлен
  char* c_name = name.c_str();
  int st, fin;
  // индусский код детектед  
  if( strcmp_P( c_name, login) == 0 ){ val.toCharArray(field.login, 9); return true; };
  if( strcmp_P( c_name, password) == 0 ){ val.toCharArray(field.password, 9); return true; };
  if( strcmp_P( c_name, IP) == 0 ){    
    st = 0;
    for( byte i = 0; i < 3; i++){ fin = val.indexOf('.', st);  field.IP[i] = val.substring( st, fin ).toInt(); st = fin + 1;  };  
    field.IP[3] = val.substring(st).toInt();  // дописываем последний 
    return true;
  };  
  if( strcmp_P( c_name, maskIP) == 0 ){ 
    st = 0;
    for( byte i = 0; i < 3; i++){ fin = val.indexOf('.', st);   field.maskIP[i] = val.substring( st, fin ).toInt(); st = fin + 1;  };  
    field.maskIP[3] = val.substring(st).toInt();  // дописываем последний 
    return true;
  };
  if( strcmp_P( c_name, gatewayIP) == 0 ){ 
    st = 0;
    for( byte i = 0; i < 3; i++){ fin = val.indexOf('.', st);   field.gatewayIP[i] = val.substring( st, fin ).toInt(); st = fin + 1;  };  
    field.gatewayIP[3] = val.substring(st).toInt();  // дописываем последний 
    return true;
  };
  if( strcmp_P( c_name, pingIP) == 0 ){ 
    st = 0;
    for( byte i = 0; i < 3; i++){ fin = val.indexOf('.', st);   field.pingIP[i] = val.substring( st, fin ).toInt(); st = fin + 1;  };  
    field.pingIP[3] = val.substring(st).toInt();  // дописываем последний 
    return true;
  };  
  if( strcmp_P( c_name, timePing) == 0 ){   field.timePing = val.toInt();    return true; };
  if( strcmp_P( c_name, timeoutPing) == 0 ){   field.timeoutPing = val.toInt();   return true; }; 
  if( strcmp_P( c_name, maxLosesFromSent) == 0 ){ field.maxLosesFromSent = val.toInt();   return true; };
  if( strcmp_P( c_name, numPingSent) == 0 ){ field.numPingSent = val.toInt();   return true; };  
  if( strcmp_P( c_name, maxLostPing) == 0 ){ field.maxLostPing = val.toInt();   return true; };  
  if( strcmp_P( c_name, delayBackSwitch) == 0 ){ field.delayBackSwitch = val.toInt();   return true; };
  if( strcmp_P( c_name, delayReturnA) == 0 ){ field.delayReturnA = val.toInt();   return true; };
  if( strcmp_P( c_name, stepDelay) == 0 ){ field.stepDelay  = val.toInt();   return true; };
  if( strcmp_P( c_name, maxDelayReturnA) == 0 ){ field.maxDelayReturnA = val.toInt();   return true; };
  
  if( strcmp_P( c_name, timeServerIP) == 0 ){ 
    st = 0;
    for( byte i = 0; i < 3; i++){ fin = val.indexOf('.', st);   field.timeServerIP[i] = val.substring( st, fin ).toInt(); st = fin + 1;  };  
    field.timeServerIP[3] = val.substring(st).toInt();  // дописываем последний     
    return true;
  };  
  if( strcmp_P( c_name, portTimeServer) == 0 ){  field.portTimeServer = val.toInt();   return true; };
 
  if( strcmp_P( c_name, time) == 0 ){  val.toCharArray(field.time, 9);   return true; };
  if( strcmp_P( c_name, date) == 0 ){  val.toCharArray(field.date, 11);   return true; };
  
  return false; // если дошли сюда, значит не нашли параметра
  
}


void TSetup::setDefaultField(){   // сбросить установки в значение по умолчанию
    field.IP[0]  = 10;  field.IP[1] = 140; field.IP[2] = 33;  field.IP[3] = 147;  //  0-й элемент массива - старший октет
    field.gatewayIP[0]  = 10;  field.gatewayIP[1] = 140; field.gatewayIP[2] = 33;  field.gatewayIP[3] = 1;  //  0-й элемент массива - старший октет
    field.maskIP[0] = 255;    field.maskIP[1] = 255;   field.maskIP[2] = 255;    field.maskIP[3] = 0;  
    field.pingIP[0] = 10;   field.pingIP[1] = 140;  field.pingIP[2] = 33;  field.pingIP[3] = 1;  //  пингуемый адрес        
    field.timePing = 10;  // период повтора ping
    field.timeoutPing = 800;  // тайма-аут пинга в мсек    
    field.maxLosesFromSent = 15; // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    field.numPingSent = 10;      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    field.maxLostPing = 5;  // предельное число подряд (непрерывно) потеряных пакетов
    field.delayBackSwitch = 20;  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    field.delayReturnA = 30;   // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
    field.stepDelay = 10;      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    field.maxDelayReturnA = 120;  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки
    field.timeServerIP[0] = 89;  field.timeServerIP[1] = 109;  field.timeServerIP[2] = 251;   field.timeServerIP[3] = 21;  
    field.portTimeServer = 123;  

    String s = "08:00:00";
    s.toCharArray(field.time, 9);
    s = "2000-01-01"; 
    s.toCharArray(field.date, 11);
    s = "admin___";
    s.toCharArray(field.login, 9);
    s = "12345678";
    s.toCharArray(field.password, 9);    
}

void TSetup::resetLoginPassword(){   // сброс пароля в параметрах конфигурации ( вспомогательно для работы с вэб интерфейсом)
    for(uint8_t i = 0; i < 9; i++){ field.login[i] = 0;  field.password[i] = 0;  };
}

bool TSetup:: load( TSetup _setup){ // копирование установок из другого объекта конфигурации
  byte *data = (byte*)&_setup.field; // data - указатель на field типа Tfield, как на последовательность байт  в источнике данных
  byte *dest = (byte*)&field;
  word size = sizeof( _setup.field );  // определяем длину сетапа в байтах
  if( size != sizeof( field ) ){  return false; };  // если размеры не равны, то ошибка
  for (byte i = 0; i < size; i++) {
    *dest = *data;
    dest++;
    data++;      
  };
  return true;
}


bool TSetup::load( String fileName ){ // чтение установок из файла  , контрольная сумма не формируется, возвращает false если файл не найден или в нем нет всех параметров
// файл из текстовых строк <формата имя параметра>  = <значение параметра>
// порядок строк не имеет значения
  File f;  
  char c;
  bool res = true;  
  f = SD.open( fileName, FILE_READ );
  if ( !f ){ return false; };  // не смогли ничего прочитать
  while (f.available() > 0 ){  
    String s = "";
    // читать строку из файла
    while( (c = f.read() ) != -1 ) {  // читаем в с пока что то есть в файле    
        // если есть возврат каретки или перевод строки, то выход из цикла
        if ((c == '\r') || (c == '\n')) { break; };
        // если ничего не сработало и дошли сюда то добавляем символ в строку
        if( c !=' ' ){  s = s + c;  };  // если не пробел, то запоминаем символ, проблеы пропускаются
      };
      // пропускаем все возвраты и переводы строки  CR, LF и CR+LF для следующего чтения
    while ( (c = f.peek()) != -1 ) {
      if ((c == '\n') || (c == '\r') || (c == ' ') ) { f.read(); }  //читать символ
      else{ break; };  // выходим для дальнейшего чтения
    };
    // разбить на имя и параметр        
    int n = s.indexOf("="); 
    String name = s.substring(0, n);  //   выделить до символа = имя
    String val = s.substring(n + 1);
    name.trim();
    val.trim();
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
      String param = String((const __FlashStringHelper*) (const char*)pgm_read_word(names + i));  // записали имя  == танцы с бубном
      String val = getField( param );
      _PRN("write cfg file")
      _LOOK(param + " = " + val)
      f.println( param + " = " + val );
  };
  f.close();
}

String TSetup::stringIP( byte _IP[4] ){  // превращает массив октетов в строку
  String res = String(_IP[0]) + ".";
  res += String(_IP[1]) + ".";
  res += String(_IP[2]) + ".";
  res += String(_IP[3]);  
  return res;
}

static TSetup config = TSetup();    // глобальная структура для сетапа
static TSetup webConfig = TSetup();    // глобальная структура для временного хранения данных на вэб странице ( вклчая отредактированные)
