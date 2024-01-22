#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>

#define PGM (const __FlashStringHelper*) // макрос для правильного обращения к именам параметров  в виде  PROGMEM  строк
#define CONFIG_FILE "/config.ini"
#define MAX_LEN_NAMES 19  // количество параметров

#define DEFAULT_TIME "08:00:00"
#define DEFAULT_DATE "2000:01:01"

// символьные имена параметров
const char login[] PROGMEM = "login";  
const char password[] PROGMEM = "password";
const char IP[] PROGMEM = "IP";  
const char maskIP[] PROGMEM = "maskIP";
const char gatewayIP[] PROGMEM = "gatewayIP";
const char pingIP[] PROGMEM = "pingIP";  
const char timePing[] PROGMEM = "timePing";
const char timeoutPing[] PROGMEM = "timeoutPing";  
const char maxLosesFromSent[] PROGMEM = "maxLosesFromSent";
const char numPingSent[] PROGMEM = "numPingSent";  
const char maxLostPing[] PROGMEM = "maxLostPing";
const char delayBackSwitch[] PROGMEM = "delayBackSwitch";  
const char delayReturnA[] PROGMEM = "delayReturnA";
const char stepDelay[] PROGMEM = "stepDelay";  
const char maxDelayReturnA[] PROGMEM = "maxDelayReturnA";
const char timeServerIP[] PROGMEM = "timeServerIP";  
const char portTimeServer[] PROGMEM = "portTimeServer";
const char time[] PROGMEM = "time";
const char date[] PROGMEM = "date";

#define N_PARAM 20 // с запасом
const int lenJSON = 500; //JSON_OBJECT_SIZE(N_PARAM);   минимально подбором 445

const char defConfig[] PROGMEM = R"({"IP":"10.140.33.147","maskIP":"255.255.255.0","gatewayIP":"10.140.33.1","pingIP":"10.140.33.1","timePing":10,"timeoutPing":800,"maxLosesFromSent":15,"numPingSent":10,"maxLostPing":5,"delayBackSwitch":20,"delayReturnA":30,"stepDelay":10,"maxDelayReturnA":120,"timeServerIP":"89.109.251.21","portTimeServer":"123", "login":"admin___","password":"12345678","time":"08:00:00","date":"2000-01-01"})";

// старая стркутура config
/*
struct Tfield{
    // сетевые адреса
    byte IP[4];  //  0-й элемент массива - старший октет
    byte maskIP[4];  
    byte gatewayIP[4];  
    byte pingIP[4];  //  пингуемый адрес
    uint16_t timePing;  // период повтора ping
    uint16_t timeoutPing;  // тайма-аут пинга в мсек    
    uint16_t maxLosesFromSent; // порог потерь пингов из общего количества посланных numPingSent для переключения на другой канал
    uint16_t numPingSent;      // количество посылаемых пингов для вычисления критерия переключения на другой канал    
    byte  maxLostPing;  // предельное число подряд (непрерывно) потеряных пакетов
    uint16_t delayBackSwitch;  // задержка переключения обратно в сек, если новый канал тоже не рабочий, необходимо, чтоб в случае отказа обоих каналов не было непрерывного переключения туда-сюда
    uint32_t delayReturnA;   // время до попытки автоматического возврата на порт А  в сек      по умолчанию 1 час
    uint32_t stepDelay;      // шаг приращения задержки до автоматического возврата на порт А , если предыдущая попытка возврата быле НЕ успешной
    uint32_t maxDelayReturnA;  // максимально возможное время до попытки автоматического возврата на порт А  в сек ( с учетом циклического увеличения на stepDelay), по умолчанию 1 сутки
    byte timeServerIP[4];  
    uint16_t portTimeServer;                                                         
    char login[8];
    char password[8];
    // текущая дата и время в текстовом формате
    char time[9] = "08:00:00"; 
    char date[11] = "2000-01-01"; 
  };
*/

    void setDefaultConfig( StaticJsonDocument<lenJSON>& source );   // сбросить установки в значение по умолчанию
    IPAddress charsToIP( const char* chars );            // превращает строку в массив октетов
    bool EQ( const char* val1, const char* val2);    // проверяет на равенство значений два параметра 
    bool verifyConfig( StaticJsonDocument<lenJSON>& source );                        //  проверка корректности данных в сетапе, нужно для сохранения корректных данных. Если данные не корректны, то они меняются на максимальное/минимальное допустимое значение
    bool copyConfig( StaticJsonDocument<lenJSON>& source, StaticJsonDocument<lenJSON>& dist ); // копирование конфига

extern StaticJsonDocument<lenJSON> config;  // создаем статический документ- глобальную структура для сетапа
extern StaticJsonDocument<lenJSON> webConfig;    // глобальная структура для временного хранения данных на вэб странице ( вклчая отредактированные)
