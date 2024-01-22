#include "conf.h"
#include "common.h"
#include "SD.h"
#include "debug.h"

bool limitIP( StaticJsonDocument<lenJSON>& source, const char* param ){    // ограничение значений в IP адресе  возвращает истину, если проводилась коррекция значений  
  IPAddress IP = charsToIP( source[PGM param] );
  // октеты уже ограничены типом byte , надо только сформировать строку и проверить изменилась ли она по сравнению с первоначальной
  String s = String( IP[0] ) + '.' + String( IP[1] ) + '.' + String( IP[2] ) + '.' + String( IP[3] );  
  bool res = !EQ( source[PGM param ], s.c_str() );  // проверяем совпадает ли, если да, то коррекции не было и функция вернет false
  source[PGM param] = s;  // устанавливаем откорректированное
  return res;
}

bool limitI32( uint32_t min, StaticJsonDocument<lenJSON>& conf, const char* param, uint32_t max){
  uint32_t val = conf[PGM param];
  if( val > max ){ conf[PGM param] = max; return true; };
  if( val < min ){ conf[PGM param] = min; return true; };
  return false;
};


bool verifyConfig( StaticJsonDocument<lenJSON>& source ){  //  проверка корректности данных в сетапе (результат - true), нужно для сохранения корректных данных. Если данные не корректны, то они меняются на максимальное/минимальное допустимое значение
    bool flag = false;
    flag = flag | limitIP(source, IP);
    flag = flag | limitIP(source, maskIP);  
    flag = flag | limitIP(source, gatewayIP);  
    flag = flag | limitIP(source, pingIP);  //  пингуемый адрес
    flag = flag | limitI32( 1, source, timePing, 120);  // период повтора ping
    flag = flag | limitI32( 100, source, timeoutPing, 5000);  // тайма-аут пинга в мсек    
    flag = flag | limitI32( 0, source, maxLosesFromSent, 200); // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    flag = flag | limitI32( 0, source, numPingSent, 200);      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    flag = flag | limitI32( 0, source, maxLostPing, 200);  // предельное число подряд (непрерывно) потеряных пакетов
    flag = flag | limitI32( 0, source, delayBackSwitch, 120);  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    flag = flag | limitI32( 0, source, delayReturnA, 3600*24);   // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
    flag = flag | limitI32( 1, source, stepDelay, 3600*24);      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    flag = flag | limitI32( 1, source, maxDelayReturnA, 3600*24);  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки
    flag = flag | limitIP(source, timeServerIP);  
    flag = flag | limitI32( 1, source, portTimeServer, 64000);    
    String s1 = source[PGM date];
    int yyyy = s1.substring( 0, s1.indexOf("-") ).toInt(); 
    int MM = s1.substring( s1.indexOf("-")+1, s1.lastIndexOf("-") ).toInt();
    int dd = s1.substring( s1.lastIndexOf("-")+1 ).toInt();
    bool lim = false;  // пока ограничения не срабатывали
    lim = ( yyyy < 2000 ) || ( yyyy > 2100 ) || ( MM < 1 ) || ( MM > 12 ) || ( dd < 1 ) || ( dd > 31 );
    flag = flag | lim;
    if ( lim ){  source[PGM date] = DEFAULT_DATE;  };  
    String s2 = source[PGM time];
    int hh = s2.substring( 0, s2.indexOf(":") ).toInt(); 
    int mm = s2.substring( s2.indexOf(":")+1, s2.lastIndexOf(":") ).toInt();
    int ss = s2.substring( s2.lastIndexOf(":")+1 ).toInt();
    lim = false;   // пока ограничения не срабатывали
    lim = ( hh < 0 ) || ( hh > 23 ) || ( mm < 0 ) || ( mm > 59 ) || ( ss < 0 ) || ( ss > 59 );
    flag = flag | lim;
    if ( lim ){  source[PGM time] = DEFAULT_TIME;   };  
   
    //  login[8]  и    password[8] не проверяются
    // доп проверки
    if( source[timeoutPing].as<unsigned long>() > (source[timePing].as<unsigned long>() * 1000) ){ 
      source[timeoutPing] = source[timePing].as<unsigned long>() * 1000; 
      flag = true; 
    };
    if( source[maxLosesFromSent].as<unsigned long>() < source[maxLostPing].as<unsigned long>() ){ 
      source[maxLosesFromSent] = source[maxLostPing].as<unsigned long>();
      flag = true; 
    };
    if( source[maxDelayReturnA].as<unsigned long>() < source[delayReturnA].as<unsigned long>() ){ 
      source[maxDelayReturnA] = source[delayReturnA].as<unsigned long>();
      flag = true;
    };
    return !flag;  // инвертируем, чтоб возвращало 1 если данные корректны и не менялись или 0- если данные изменились
 } 


void setDefaultConfig( StaticJsonDocument<lenJSON>& source ){   // сбросить установки в значение по умолчанию

// ???? временно для проверки
//char defConfig[] 
//String defConfig = String(R"({"IP":"10.140.33.147","maskIP":"255.255.255.0","gatewayIP":"10.140.33.1","pingIP":"10.140.33.1","timePing":10,"timeoutPing":800,"maxLosesFromSent":15,"numPingSent":10,"maxLostPing":5,"delayBackSwitch":20,"delayReturnA":30,"stepDelay":10,"maxDelayReturnA":120,"timeServerIP":"89.109.251.21","portTimeServer":"123", "login":"admin___","password":"12345678","time":"08:00:00","date":"2000-01-01"})");
// рабочий вариант const __FlashStringHelper* defConfig = F(R"({"IP":"10.140.33.147","maskIP":"255.255.255.0","gatewayIP":"10.140.33.1","pingIP":"10.140.33.1","timePing":10,"timeoutPing":800,"maxLosesFromSent":15,"numPingSent":10,"maxLostPing":5,"delayBackSwitch":20,"delayReturnA":30,"stepDelay":10,"maxDelayReturnA":120,"timeServerIP":"89.109.251.21","portTimeServer":"123", "login":"admin___","password":"12345678","time":"08:00:00","date":"2000-01-01"})");
//String s = String(defConfig);
DeserializationError error = deserializeJson( source, PGM defConfig);
  if (error) {
    //  ??? временная печать
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());    
  }
}


bool copyConfig( StaticJsonDocument<lenJSON>& source, StaticJsonDocument<lenJSON>& dist ){ // копирование конфига
  String json;
  serializeJson(source, json);
  DeserializationError error = deserializeJson(dist, json);
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str()); 
    if ( error ){ return false; }  // с выверотм вместо прямого присваивания из-за заморочек библиотеки JSON и С++
    else{ return true; };
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

bool EQ( const char* val1, const char* val2){    // проверяет на равенство значений два параметра
  return strcmp( val1, val2 ) == 0 ;
}

static StaticJsonDocument<lenJSON> config;  // создаем статический документ- глобальную структура для сетапа
static StaticJsonDocument<lenJSON> webConfig;    // глобальная структура для временного хранения данных на вэб странице ( вклчая отредактированные)