#include "conf.h"
#include "common.h"
#include "SD.h"
#include "debug.h"

bool limitIP( byte IP[4]){    // ограничение значений в IP адресе  возвращает истину, если проводилась коррекция значений
  bool flag = false;
  for( byte i = 0; i < 4; i++){   
    if( IP[i] > 255 ){ IP[i] = 255; flag = true; };    
  };
  return flag;
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

bool verifyConfig(){  //  проверка корректности данных в сетапе, нужно для сохранения корректных данных. Если данные не корректны, то они меняются на максимальное/минимальное допустимое значение
    bool flag = false;

    /*  ?????????????????????????????????    временно закрыто  ??????????????????????
    flag = flag | limitIP(field.IP);
    flag = flag | limitIP(field.maskIP);  
    flag = flag | limitIP(field.gatewayIP);  
    flag = flag | limitIP(field.pingIP);  //  пингуемый адрес
    flag = flag | limitField( 1, field.timePing, 120);  // период повтора ping
    flag = flag | limitField( 100, field.timeoutPing, 5000);  // тайма-аут пинга в мсек    
    flag = flag | limitField( 0, field.maxLosesFromSent, 200); // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    flag = flag | limitField( 0, field.numPingSent, 200);      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    flag = flag | limitField( 0, field.maxLostPing, 200);  // предельное число подряд (непрерывно) потеряных пакетов
    flag = flag | limitField( 0, field.delayBackSwitch, 120);  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    flag = flag | limitField( 0, field.delayReturnA, 3600*24);   // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
    flag = flag | limitField( 1, field.stepDelay, 3600*24);      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    flag = flag | limitField( 1, field.maxDelayReturnA, 3600*24);  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки
    flag = flag | limitIP(field.timeServerIP);  
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
*/
    return flag;
 } 


void setDefaultConfig(){   // сбросить установки в значение по умолчанию

// ???? временно для проверки
char defConfig[] = R"({"IP":"10.140.33.147","maskIP":"255.255.255.0","gatewayIP":"10.140.33.1","pingIP":"10.140.33.1","timePing":10,"timeoutPing":800,"maxLosesFromSent":15,"numPingSent":10,"maxLostPing":5,"delayBackSwitch":20,"delayReturnA":30,"stepDelay":10,"maxDelayReturnA":120,"timeServerIP":"89.109.251.21","portTimeServer":"123", "login":"admin___","password":"12345678","time":"08:00:00","date":"2000-01-01"})";
//"{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";
DeserializationError error = deserializeJson(config, defConfig);
  if (error) {
    //  ??? временная печать
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());    
  }
}


String stringIP( byte _IP[4] ){  // превращает массив октетов в строку
  String res = String(_IP[0]) + ".";
  res += String(_IP[1]) + ".";
  res += String(_IP[2]) + ".";
  res += String(_IP[3]);  
  return res;
}

bool copyConfig( StaticJsonDocument<lenJSON> source, StaticJsonDocument<lenJSON> dist ){ // копирование конфига
return false;
}

IPAddress charsToIP( const char* chars ){            // превращает строку в массив октетов
  IPAddress IP;
  String val = String( chars );
  int st = 0;
  int fin;
  for( byte i = 0; i < 3; i++){ fin = val.indexOf('.', st);   IP[i] = val.substring( st, fin ).toInt(); st = fin + 1;  };    
  IP[3] = val.substring(st).toInt();  // дописываем последний  
  return IP; 
  // проверки не делаем, т.к. IPadress состоит из байт и выйти за границы не может
};

StaticJsonDocument<lenJSON> config;  // создаем статический документ- глобальную структура для сетапа
StaticJsonDocument<lenJSON> webConfig;    // глобальная структура для временного хранения данных на вэб странице ( вклчая отредактированные)