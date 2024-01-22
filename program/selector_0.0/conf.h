#pragma once
#include <Arduino.h>

#define CONFIG_FILE "/config.ini"
#define NOT_CONFIG_VARIABLE "unknow VAR"  // признак того, что не нашли переменную в setup
#define MAX_LEN_NAMES 19  // длина массива параметров names (количество параметров)

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


// массив символьных имен параметров i=  0           1        2      3           4        5          6           7                  8               9                10               11                12           13                14              15               16         17     18
const char* const names[] PROGMEM = {  login,   password,   IP,   maskIP , gatewayIP,   pingIP,   timePing,   timeoutPing,   maxLosesFromSent,   numPingSent,   maxLostPing,   delayBackSwitch,   delayReturnA,   stepDelay,   maxDelayReturnA,   timeServerIP,   portTimeServer, date, time };

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
    char login[9];
    char password[9];
    // текущая дата и время в текстовом формате
    char time[9] = "08:00:00"; 
    char date[11] = "2000-01-01"; 
  };


class TSetup {
public:  
    TSetup();
    Tfield field;
    String getField( String name );            // выдает параметр из Setup по его символьному имени ( например для вставки в вэб страицу). Параметр выдается в виде текстовой строки если параметр не найден то выдает "??????"
    bool setField( String name, String val );  // устанавливает значение поля Setup по символьнму имени, возвращает true , если параметр имеет допустимое значение и был установлен    
    bool load( TSetup _setup);                 // копирование установок из другого объекта конфигурации
    bool load( String fileName );              // чтение установок из файла  , контрольная сумма не формируется, возвращает false если файл не найден или в нем нет всех параметров
    void save( String fileName );              // запись установок в файл
    void setDefaultField();                    // сбросить установки в значение по умолчанию
    String stringIP( byte _IP[4] );            // превращает массив октетов в строку  
    bool verifySetup();                        //  проверка корректности данных в сетапе, нужно для сохранения корректных данных. Если данные не корректны, то они меняются на максимальное/минимальное допустимое значение
    void resetLoginPassword();   // сброс пароля в параметрах конфигурации ( вспомогательно для работы с вэб интерфейсом)

    };


extern TSetup config;    // глобальная структура для сетапа
extern TSetup webConfig;    // глобальная структура для временного хранения данных на вэб странице ( вклчая отредактированные)
