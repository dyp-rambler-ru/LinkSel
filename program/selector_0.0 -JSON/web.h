#pragma once
#include <Ethernet2.h>
#include "SD.h"
#include "cmd.h"

#define MAX_LEN_REQ 1024  // макстмальная длина запроса  в байтах 
#define MAX_LEN_TAG 255  // максимальная длина для одиночного тэга или строки текста (отправляемого за один раз клиенту, иначе в строка при отправке будет разбита на 2)

enum TypeReq { None=0, Get=1, Post=2, Unknow=3 };  // тип принятого запроса PUT, HEAD и DELETE не используются
enum TstateWebServer { Wait, RecivingRequest, ParsingParam, SendAnswer, Finished };  //  состояние вэбсервера  Ожидание/прием запроса/разбор параметров/посылка ответа/ завершение обслуживания запроса  
enum TstateReadReq { Header, Body };  // состояние чтения запроса, ждем заголовок или ждем тело
enum TstateReadTag { Tag, Text, Overflow, endOfFile };  // результаты чтения тэга из файла

class TWebServer {

    public:
    TWebServer( String rootWeb );  // конструктор, параметр - корневая директория вэб сервера, тайм аут соединения в циклах контроллера
    void startConnection(EthernetClient client); // старт обслуживания клиента
    void run();  // работа сервера    
    void reset();  // сброс соединения, например из-за тайм -аута    
    bool isWait(); //  сервер  в состоянии ожидания запроса
    bool isFinished();  //сервер  в состоянии завершения обслуживания запроса        

    private:
    EthernetClient _client;
    String _rootWeb;
    TstateWebServer _state = Wait;  // состояние сервера
    TypeReq _typeReq = None;  // тип запроса
    TstateReadReq _stateReadReq = Header;  // состояние чтения строки
    String _req;  // строка для хранения текущего запроса,  длинная строка с зарезервированной длинной для оптимизации (MAX_LEN_REQ символов в конструкторе класса). После разбора заголовка здесь храниться тело запроса!!!
    String _webRes;  // ресурс запрашиваемый на сервере
    String _tag;    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (MAX_LEN_TAG символов в конструкторе класса)
    char clientLogin[8];  // логин и пароль для текущего клиента
    char clientPassword[8];
    bool authorized = false;  // признак прохождения авторизации
    bool scheduledReset = false;
    int _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
    int _counterL = 0;  // фактически полученная длина запроса       
    
    bool readReq();                               // чтение запроса от клиента целиком, возвращает true, если запрос прочитан целиком. Устанавливает тип полученного запроса, текст параметра в запросе    
    void sendHTTP();                              // отправка ответа от HTTP сервера
    bool setParam( String& s );                    // извлекаем параметры и значения из строки и записываем в глобальный setup, вспомогательная функция
    bool tagProcessing(String& tag );             // если тэг типа input , то подставляем значение параметра из Setup, True - если обработка была успешной ( с заменой параметров или без) 
    // чтение тэга из файла до конца строки n\  с исключением повторяющихся пробелов \n и т.д. Если это текст, то он тоже читается, но пробелы не пропускаются
    // тэг или текст длинее 255 символов обрезается и дополняетя >
    TstateReadTag readTag( File f, String& tag);  // возвращает тип результата чтения TstateReadTag    
    bool cmd( String s );                         // функция выполнения команд на сервере    
    void sendStdAnswer( uint16_t codeAnswer );    //отправка стандартного ответа ответа сервера 
    void sendHeader( String msg );                //отправка заголовка ответа сервера 
};

extern TWebServer webServer;  // объект вэб-сервер
//static TWebServer webServer = TWebServer( "/web" );  // объект вэб-сервер
