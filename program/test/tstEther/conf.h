#pragma once
#include <Arduino.h>

#define OFFSET_EEPROM 0  // смещение для чтения Setup из энергонезависимой памяти
#define NOT_CONFIG_VARIABLE "??????"  // признак того, что не нашли переменную в setup
#define MAX_LEN_NAMES 16  // длина массива параметров names (количество параметров)
// символьные имена параметров
const char login[] PROGMEM = "login";  
const char password[] PROGMEM = "password";
const char IP[] PROGMEM = "IP";  
const char maskIP[] PROGMEM = "maskIP";
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

// массив символьных имен параметров i=  0           1        2      3           4        5            6               7                  8               9                10               11           12              13                14              15
const char* const names[] PROGMEM = {  login,   password,   IP,   maskIP ,   pingIP,   timePing,   timeoutPing,   maxLosesFromSent,   numPingSent,   maxLostPing,   delayBackSwitch,   delayReturnA,   stepDelay,   maxDelayReturnA,   timeServerIP,   portTimeServer };
const uint32_t maxVal[] PROGMEM   = {  255,     255,        255,   255,      255,      120,        5000,          200,                200,           200,           120,               3600*24,        3600*24,     3600*24,             255,            64000        };
const uint32_t minVal[] PROGMEM   = {    0,     0,          0,     0,        0,        1,          100,           0,                  0,             0,             0,                 0,              1,           1,                   0,              1            };

struct Tfield{
    // сетевые адреса
    byte IP[4];  //  0-й элемент массива - старший октет
    byte maskIP[4];  
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
  };


class TSetup {
public:  
    TSetup();
    Tfield field;
    String getField( String name );     // выдает параметр из Setup по его символьному имени ( например для вставки в вэб страицу). Параметр выдается в виде текстовой строки если параметр не найден то выдает "??????"
    bool setField( String name, String val );  // устанавливает значение поля Setup по символьнму имени, возвращает true , если параметр имеет допустимое значение и был установлен    
    bool load();              // чтение установок из EEPROM  , возвращает истину, если контрольная сумма в норме
    void save();              // запись установок в EEPROM
    bool load( String fileName ); // чтение установок из файла  , контрольная сумма не формируется, возвращает false если файл не найден или в нем нет всех параметров
    void save( String fileName ); // запись установок в файл
    void setDefaultField();   // сбросить установки в значение по умолчанию
    bool check();             // подсчет контрольной суммы Setup и сравнение с имеющейся
    
    

private:
    byte crc;                        // вспомогательная переменная для расчета контрольной суммы    
    uint32_t TSetup::limitField( uint32_t val, byte n);  // ограничение значения в соотвтествии с данными массивов maxVal и minVal
    };


static TSetup config = TSetup();    // глобальная структура для сетапа