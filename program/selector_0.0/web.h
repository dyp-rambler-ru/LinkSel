#pragma once
#include <Ethernet2.h>
#include "SD.h"
#include "conf.h"
#include "cmd.h"

#define MAX_LEN_REQ 1024  // макстмальная длина запроса  в байтах 
#define MAX_LEN_TAG 255  // максимальная длина для одиночного тэга или строки текста (отправляемого за один раз клиенту, иначе в строка при отправке будет разбита на 2)
#define SESSION_ID_PASSWORD 0x9D3F8E0A  // пароль шифрующий по XOR идентификатор сессии с вэб интерфейсом

enum TypeReq { None=0, Get=1, Post=2, Unknow=3 };  // тип принятого запроса PUT, HEAD и DELETE не используются
enum TstateWebServer { Wait, RecivingRequest, ParsingParam, SendAnswer, Finished };  //  состояние вэбсервера  Ожидание/прием запроса/разбор параметров/посылка ответа/ завершение обслуживания запроса  
enum TstateReadReq { Header, Body };  // состояние чтения запроса, ждем заголовок или ждем тело
enum TstateReadTag { Tag, Text, TagTextArea, Overflow, endOfFile };  // результаты чтения тэга из файла

class TWebServer {

    public:
    TWebServer( String rootWeb );  // конструктор, параметр - корневая директория вэб сервера, тайм аут соединения в циклах контроллера
    void startConnection(EthernetClient client); // старт обслуживания клиента
    void run();  // работа сервера    
    void reset();  // сброс соединения, например из-за тайм -аута    
    bool isWait(); //  сервер  в состоянии ожидания запроса
    bool isFinished();  //сервер  в состоянии завершения обслуживания запроса            
    void abortSession(); // сброс сессии - сессия закрыта

    private:
    EthernetClient _client;
    String _rootWeb;
    TstateWebServer _state = Wait;  // состояние сервера
    TypeReq _typeReq = None;  // тип запроса
    TstateReadReq _stateReadReq = Header;  // состояние чтения строки
    String _req;  // строка для хранения текущего запроса,  длинная строка с зарезервированной длинной для оптимизации (MAX_LEN_REQ символов в конструкторе класса). После разбора заголовка здесь храниться тело запроса!!!
    String _webRes;  // ресурс запрашиваемый на сервере
    String _tag;    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (MAX_LEN_TAG символов в конструкторе класса)
    int _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
    int _counterL = 0;  // фактически полученная длина запроса       
    uint32_t sessionID = 0;
    uint32_t receivedID = 0;  // полученный ID сессии в куке, обновляется при получении каждого запроса
    bool needSendSessionID = false;// пользователь в процессе входа в систему и ввел правильную пару логин/пароль  и ему надо отправить куку с уникальным ID сеанса
    
    bool isLoginProcess( TSetup _setup );         // проверить, что пользователь в процессе авторизации ввел сейчас правильную пару логин/пароль на вэб странице устройства
    bool readReq();                               // чтение запроса от клиента целиком, возвращает true, если запрос прочитан целиком. Устанавливает тип полученного запроса, текст параметра в запросе  
    bool sendHTML_File(String s);                 // отправка HTML файла (вспмогательно для sendHTML)  
    void sendHTTP();                              // отправка ответа от HTTP сервера
    bool setParam( String& s );                    // извлекаем параметры и значения из строки и записываем в глобальный setup, вспомогательная функция
    bool tagProcessing(String& tag );             // если тэг типа input , то подставляем значение параметра из Setup или State, True - если обработка была успешной ( с заменой параметров или без) 
    // чтение тэга из файла до конца строки n\  с исключением повторяющихся пробелов \n и т.д. Если это текст, то он тоже читается, но пробелы не пропускаются
    // тэг или текст длинее 255 символов обрезается и дополняетя >
    TstateReadTag readTag( File f, String& tag);  // возвращает тип результата чтения TstateReadTag        
    void sendStdAnswer( uint16_t codeAnswer );    //отправка стандартного ответа ответа сервера 
    void sendHeader( String msg );                //отправка заголовка ответа сервера 
    bool sendNewSessionID();                      // генерируем новый идентификатор сессии 
};

extern TWebServer webServer;  // объект вэб-сервер
//static TWebServer webServer = TWebServer( "/web" );  // объект вэб-сервер
