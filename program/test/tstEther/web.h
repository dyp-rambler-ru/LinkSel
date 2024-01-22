#pragma once
//  Подключаем стандартную библиотеку для работы с Ethernet
#include <Ethernet2.h>
#include "SD.h"

#define MAX_LEN_REQ 255  // макстмальная длина запроса  в байтах 
#define MAX_LEN_TAG 255  // максимальная длина для одиночного тэга или строки текста (отправляемого за один раз клиенту, иначе в строка при отправке будет разбита на 2)

enum TypeReq { None, Get, Post, Head, Unknow };  // тип принятого запроса PUT и DELETE не используются
enum TstateWebServer { Wait, RecivingRequest, ParsingParam, SendAnswer };  //  состояние вэбсервера    400 Bad Request  404 Not Found  405 Method Not Allowed  413 Request Entity Too Large 501 Not Implemented  406 Not Acceptable	 411 Length Required 
enum TstateReadReq { Header, Body };  // состояние чтения запроса, ждем заголовок или ждем тело
enum TstateReadTag { Tag, Text, Overflow, endOfFile };  // результаты чтения тэга из файла

class TWebServer {

    public:
    TWebServer( String rootWeb, byte timeOutInCycle );  // конструктор, параметр - корневая директория вэб сервера, тайм аут соединения в циклах контроллера
    void startConnection(EthernetClient client); // старт обслуживания клиента
    bool isWait(); //  сервер  в состоянии ожидания запроса
    bool run();  // работа сервера
    void reset();  // сброс соединения, например из-за тайм -аута
    void sendHeaderAnswer( uint16_t codeAnswer );   // отправка заголовка ответа сервера ( при ошибках тела в ответе может и не быть)

    private:
    EthernetClient _client;
    String _rootWeb;
    byte _timeOutInCycle; 
    TstateWebServer _state = Wait;  // состояние сервера
    TypeReq _typeReq = None;  // тип запроса
    TstateReadReq _stateReadReq = Header;  // состояние чтения строки
    String _req;  // строка для хранения текущего запроса,  длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
    String _webRes;  // ресурс запрашиваемый на сервере
    String _body;  // тело сообщения
    String _tag;    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
    bool Authorized = false;  // признак прохождения авторизации
    int _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
    int _counterL = 0;  // фактически полученная длина запроса       
    
    String getWebRes(String req, String reqName); // получает параметр  запроса web ресурса из запроса req. reqName - строка с названием запроса GET, POST....        
    bool readReq();                               // чтение запроса от клиента целиком, возвращает true, если запрос прочитан целиком. Устанавливает тип полученного запроса, текст параметра в запросе    
    bool parsingParam();                          // разбор параметров и сохранение в Setup в зависимости от типа принятого типа запроса    
    bool sendHTTP();                              // отправка ответа от HTTP сервера
    bool setParam( String s );                    // извлекаем параметры и значения из строки и записываем в глобальный setup, вспомогательная функция
    bool tagProcessing(String& tag );             // если тэг типа input , то подставляем значение параметра из Setup, True - если обработка была успешной ( с заменой параметров или без) 
    String readLnSkippTrivial( File f);           // чтение символов из файла до конца строки n\  с исключением повторяющихся пробелов \n и т.д. 
    // чтение тэга из файла до конца строки n\  с исключением повторяющихся пробелов \n и т.д. Если это текст, то он тоже читается, но пробелы не пропускаются
    // тэг или текст длинее 255 символов обрезается и дополняетя >
    TstateReadTag readTag( File f, String& tag);    // возвращает тип результата чтения TstateReadTag    
};

static TWebServer webServer = TWebServer( "/web", 5);  // переменная для состояния вэб сервера
