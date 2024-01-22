#pragma once
#include "web.h"
#include "conf.h"
#include "log.h"
//#include "SPI.h"
#include <Ethernet2.h>
#include "SD.h"
#include "debug.h"


TWebServer::TWebServer(String rootWeb, byte timeOutInCycle) {  // конструктор, параметр - корневая директория вэб сервера, тайм аут соединения в циклах контроллера
  _tag.reserve(MAX_LEN_TAG);  // резервируем место для строки
  _req.reserve(MAX_LEN_REQ);  // резервируем место для строки
  _rootWeb = rootWeb;
  _timeOutInCycle = timeOutInCycle;
  _state = Wait;                   // состояние сервера
  _typeReq = None;                 // тип запроса
  _stateReadReq = Header;          // состояние чтения запроса
  _webRes = "";  // ресурс запрашиваемый на сервере
  _body = "";  // тело сообщения
  _tag = "";    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
  _req = "";
  Authorized = false;  // признак прохождения авторизации
  _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
  _counterL = 0;  // фактически полученная длина запроса   
}

void TWebServer::startConnection(EthernetClient client) {  // старт обслуживания клиента
  _client = client;
  _state = RecivingRequest;  
  readReq();  // начинаем читать запрос  
}

bool TWebServer::isWait() {  //сервер  в состоянии ожидания запроса
  return _state == Wait;
}

bool TWebServer::run() {  // работа сервера, если ошибка или проблема, то нужно вернуть false, чтоб сбросить соединение
  switch (_state) {
                  // case Wait:  -  этого состояния быть не может , из него включается startConnection и идет переход на RecivingRequest   
    case RecivingRequest:
      {
        if( readReq() ){  // если дочитали запрос, то меняем состояние
        // проверяем на ошибки в запросе        
        if( _counterL > MAX_LEN_REQ ){  // слишком длинный запрос  413; 
          sendHeaderAnswer( 413 );
          Log.writeMsg( F("err413_1") );          
          _state = Wait;
          return false;  // вышли с ошибкой, вызывающая функция все закроет
          break;  
        };
        if( _req.length() > _content_Length ){  //  тело больше заявленного  400;
          sendHeaderAnswer( 400 );
          Log.writeMsg( F("err400_2") );
          _state = Wait;
          return false;  // вышли с ошибкой, вызывающая функция все закроет
          break;          
        };
        // ?? а если запрос POST и тело пустое, то надо сообщение 400 Bad Request  ?? пока не реализованно
        // если дошли сюда, то все ОК и переходим к следующему состоянию
          _state = ParsingParam;
        };  // иначе на следующем такте продолжим чтение        
        break;        
      };
    case ParsingParam:
      {        
        if( parsingParam() ){  // если успешно разобрали параметры то следующий шаг
            _state = SendAnswer;
        }
        else{   //неправильный запрос, послать ответ с сообщением об ошибке 
          sendHeaderAnswer( 400 );
          Log.writeMsg( F("err400_1") );
          _state = Wait;
          return false;  // вышли с ошибкой, вызывающая функция все закроет
          break;
        }
        break;        
      };
    case SendAnswer:  //из-за нагрузки крайне желательно как отдельное состояние
      {        
        sendHTTP();        
        _state = Wait;  // готовимся к след. клиенту
        return false;   // в конце тоже false чтоб внешняя процедура закрыла соединение
        break;        
      };
    default:
      { Log.writeMsg( F("err_0010") ); };  // запись о внутренней ошибке
  };  // case
  return true; // все ОК, при ошибках вышли из функции раньше
}

void TWebServer::reset() {  // сброс соединения, например из-за тайм -аута
//   EthernetClient _client не сбрасываем, только по тайм ауту
  _state = Wait;                   // состояние сервера
  _typeReq = None;                     // тип запроса
  _stateReadReq = Header;           // состояние чтения запроса
  _req = "";  // строка для хранения текущего запроса,  длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
  _webRes = "";  // ресурс запрашиваемый на сервере
  _body = "";  // тело сообщения
  _tag = "";    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
  Authorized = false;  // признак прохождения авторизации
  _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
  _counterL = 0;  // фактически полученная длина запроса     
}


String TWebServer::getWebRes(String req, String reqName) {  // получает параметр из запроса req. reqName - строка с названием запроса GET, POST....
  req.remove(_req.indexOf("\r\n"));                         // получаем первую строку
  req.remove(0, reqName.length());                          // вырезаем само название запроса
  req.trim();                                               // и пробелы
  // удаляем доп параметры (например HTTP/1.0) после пробела и до конца строки, если такие есть
  int st_char = req.lastIndexOf(" ");  // ищем пробел
  if (st_char != -1) {
    req.remove(st_char);
    req.trim();
  }
  return req;
}

/* ??
https://webkyrs.info/post/http-zapros-metodom-post
Если, для того, чтобы обратиться к серверу методом GET, нам достаточно было набрать запрос в URL-адрес, то в методе POST все работает по другому принципу.
Для того, чтобы выполнить этот вид запроса, нам необходимо нажать на кнопку с атрибутом type="submit", которая расположена на веб-странице. Обратите внимание,
 что эта кнопка расположена в элементе <form>, у которого установлен атрибут method со значением post.
*/

bool TWebServer::readReq() {  // чтение запроса от клиента целиком, возвращает true, если запрос прочитан целиком. Устанавливает тип полученного запроса, текст параметра в запросе
                                                              
  if (!_client.connected()) {       // если клиент  НЕ подключен    
    return true;
  };
  while (_client.available()) {  // пока есть данные в буфере от клиента?
    _req = _req + _client.read();
    _counterL++;
    if (_counterL > MAX_LEN_REQ) {
      //аварийное завершение, слишком длинный запрос, не хватает памяти, обработка ошибки в вызывающей процедуре по  _counterL > MAX_LEN_REQ
      return true;           
    }
  };

  if (_stateReadReq == Header) {
    // если нашлось CR+LF+CR+LF, то заголовок окончился. ищем тип, длину  и переходим к приему тела при необходимости
    if (_req.indexOf("\r\n\r\n") != -1) {
      _req.trim();  // убрали пробелы
      // ищем тип запроса в НАЧАЛЕ запроса
      if (_req.indexOf("HEAD") == 0) {  // если нашлось HEAD
        _typeReq = Head;
        _webRes = "";  // надо не читать параметры или тел
        return true;
      } else if (_req.indexOf("GET") == 0) {  //если нашлось GET
        _typeReq = Get;
        _webRes = getWebRes(_req, "GET");
      } else if (_req.indexOf("POST") == 0) {  // если нашлось POST
        _typeReq = Post;
        _webRes = getWebRes(_req, "POST");
      };

      // продолжаем обработку заголовка запроса = найти длину от Content-Length: до CR+LF
      if (_req.indexOf("Content-Length:") != -1) {
        int st_char = _req.indexOf("Content-Length:") + 1;
        int end_char = _req.indexOf("\r\n", st_char);
        String s = _req.substring(st_char, end_char);
        s.trim();
        _content_Length = s.toInt();
        _req.remove(0, _req.indexOf("\r\n\r\n") + 4 + 1);  // убираем из _req заголовок ( +4 чтоб убрать CR+LF+CR+LF и +1 чтоб получить количество символов из позиции- учет что первый символ с 0 индексом)
        _stateReadReq = Body;                           // переходим к чтению тела сообщения
        return false;
      } else {  // нет контента в запросе
        _content_Length = -1;
        return true;
      };
    }
  }
  // в случае если в запросе есть длина, но из-за ошибки мы не можем прочитать тело запроса, то мы вылетим по ошибке из-за тайм-аута.
  else if (_stateReadReq == Body) {  // если уже перешли к чтению тела запроса
    if( _req.length() == _content_Length ) {  // если прочли все тело то
      _body = _req;                          // запоминаем
      return true;
    } else if (_req.length() > _content_Length) {   //  ошибка, слишком много в буфере, обработка в вызывающей процедуре      
      return true;
    };
  };
}

String TWebServer::readLnSkippTrivial(File f) {  // чтение символов из файла до конца строки n\  с исключением повторяющихся пробелов, табуляции \n и т.д.
  String s = "";
  char c = ' ';
  bool space = false;
  while ((c = f.read()) != -1) {  // читаем в с пока что то есть в файле
    // если табуляция, то пропускаем
    if (c == '\t') { continue; };
    // если повтор пробела то пропускать  и переходить к следующему
    if (c == ' ') {
      if (space) {
        continue;
      } else {
        space = true;
      };
    } else {  // это не пробел
      space = false;
    };
    // если есть возврат каретки или перевод строки, то выход из цикла
    if ((c == '\r') || (c == '\n')) { break; };
    // если ничего не сработало и дошли сюда то добавляем символ в строку
    s = s + c;
  };
  // пропускаем все возвраты и переводы строки  CR, LF и CR+LF, табуляции для следующего чтения
  while (f.available()>0 ) {
    c = f.peek();
    if ((c == '\n') || (c == '\r') || (c == '\t')) { f.read(); };  //читать символ
  };

  return s;
}

bool TWebServer::setParam(String s) {  // извлекаем параметры и значения из строки и записываем в глобальный setup
  bool res = true;
  String val;
  //ищем , если есть, то
  if (s.indexOf("?") != -1) {
    s = s.substring(_webRes.indexOf("?") + 1);  // выделяем параметры
    s.trim();
    do {
      int n = s.indexOf("=");
      String name = s.substring(0, n);  //   выделить до символа = имя
      int sep = s.indexOf("&");
      // выделяем значение до разделителя & или ( если его нет) - до конца строки
      if (sep != -1) {
        val = s.substring(n + 1, sep);
      } else {
        val = s.substring(n + 1);
      };
      res = res && config.setField(name, val);      //      записать в Setup и установить флаг
      if (sep != -1) { s = s.substring(sep + 1); }  //убираем из строки то , что разобрали
      else {
        s = "";
      };
    } while (s.length() != 0);
  };
  return res;
}

bool TWebServer::parsingParam() {  // разбор параметров и сохранение в Setup в зависимости от типа принятого типа запроса
  bool res = true;
  switch (_typeReq) {
Get:
    {
      res = res && setParam(_webRes);  // извлекаем параметры и значения из первой строки  запроса
      if (_body.length() != 0) { res = res && setParam(_body); };
      break;
    };
Post:
    {
      //если body не пустое
      if (_body.length() != 0) { res = res && setParam(_body); };      
      break;
    };
  };
  return res;
}

// чтение тэга из файла до конца строки n\  с исключением повторяющихся пробелов \n и т.д. Если это текст, то он тоже читается, но пробелы не пропускаются
// тэг или текст длинее 255 символов обрезается и дополняетя >
TstateReadTag TWebServer::readTag( File f, String& tag){    // возвращает тип результата чтения TstateReadTag
  TstateReadTag typeRes;
  byte count = 0; // счетчик для ограничения длины строки
  char ch;
  tag = ""; // сбрасываем строку
  // если читать нечего или файл не открыт - состояние Конец файла и выход
  if( !f.available() ){ typeRes = endOfFile; return typeRes; };
  // проверить первый символ
  // если < то начинаем читать тэг
  if( f.peek() == '<' ){
    typeRes = Tag;  //тип результата - тэг
    //    пока не конец файла или не слишком много символов
    while( ( (ch = f.read()) != -1 ) && (count < 255) ){      
      //если символ НЕ повторный пробел и НЕ( перевод строки или возврат каретки или табуляция)
      // и не пробел до или после "=", то       
      if( !((ch == ' ') && ( tag[ tag.length()-1 ] == ' ' ))  && 
          !( (ch == '\r') || (ch == '\n') || (ch == '\t') ) &&
          !((ch == ' ') && ( tag[ tag.length()-1 ] == '=' )) &&
          !((ch == '=') && ( tag[ tag.length()-1 ] == ' ' ))        ){
          tag = tag + ch; //запомнить символ  
          count++;
          if( ch == '>' ){ break; }  // если прочитали закрытие тэга, то выбрасываемся из цикла
      };    
    };  
    // проверяем на слишком длинную строку
    if( count == 255 ){ 
      tag[ tag.length()-1 ] = '>'; // принудительно закрываем тэг (хотя это будет неправильный тэг) и выходим с ошибкой
      typeRes = Overflow;
      return typeRes;
    };
  }else{  // чтение просто текста до конца строки
      typeRes = Text;  //тип результата - Text      
      //    пока не конец файла или не слишком много символов
      while( ( (ch = f.read()) != -1 ) &&  (count < 255) ){          
        if( ch != '<' ){ 
            tag = tag + ch; //запомнить символ  
            count++;
        }
        else{  // начинается тэг
          // возвращаемся на символ назад чтоб потом полностью читать тэг
          f.seek( f.position()-1 ); 
          break; 
        }  // если прочитали закрытие тэга, то выбрасываемся из цикла
      };            
      // проверяем на слишком длинную строку
      if( count == 255 ){         
        typeRes = Overflow;
        return typeRes;
      };
  };// конец чтения

  return typeRes;  // если сюда дошли, то нормальное завершение

}


bool TWebServer::sendHTTP() {  // отправка ответа от HTTP сервера (работает после полученного запроса _typeReq != None )
// параметры запроса должны быть уже разобраны и сохранены в parsingParam();
  String s="";

  if( !Authorized ){  //если не пройдена авторизация то на все запросы кроме авторизации выдавать 401 Unauthorized — получение запрашиваемого ресурса доступно только аутентифицированным пользователям. если нет факта авторизации или просто перенаправлять на авторизацию ?
      sendHeaderAnswer( 401 );
      Log.writeMsg( F("err401_1") );
      return true;
  };
  
  switch(_typeReq) {
  Get:
    {     
      //ищем , если есть ?, то есть праметры и их пока брать не надо
      if (_webRes.indexOf("?") != -1) {   s = _webRes.substring(0,_webRes.indexOf("?")); } // ресурс
      else{ s = _webRes; }; // иначе вся строка - адрес ресурса
      s.trim();
      if( s == "" ){  s = "index.html";    };   //  если пусто то главная страница
      s = "/" + s;
      s = _rootWeb + s;
      //ищем файл - ресурс 
      if( !SD.exists( s ) ){  //если файл не найден, то отправляем 
        sendHeaderAnswer( 404 ); //404 Not Found — сервер не смог найти запрашиваемый ресурс. если при открытии файла произошла ошибка
        Log.writeMsg( F("err404_1") );
      }
      else{  // файл ресурса существует 
          // параметры разобраны ранее и проверены в методе run
          sendHeaderAnswer( 200 );  //200 OK — запрос принят и корректно обработан веб-сервером. ответ на корректный GET
          _client.println("Content-type: text/html");
          // ?? _client.println("Content-length: " +  );
          File f = SD.open(s, FILE_READ);  // открываем файл
          TstateReadTag state = readTag( f, _tag);  //читать тэг  пропуская лишние пробелы и переводы строки из файла
          while( state != endOfFile ){   //пока не конец файла            
            switch (state){  // в зависимости от того что прочли из файла
            case Tag:{
              // вставляем значение параметров , если тэг input  и подстановка праметра прошла успешно
              if( tagProcessing( _tag ) ){ 
                _client.println(_tag);  //   отправить тэг клиенту
              }
              else{  // ошибка при подстановке параметров                
                sendHeaderAnswer( 400 );     
                Log.writeMsg( F("err400_3") );        
                return false;
              };
              break;
            };
            case Text:{
              _client.println(_tag);  //   отправить тэг клиенту
              break;
            };
            case Overflow:{
              sendHeaderAnswer( 500 ); 
              Log.writeMsg( F("err500_1") ); 
              return false;
              break;
            };
            default:{  Log.writeMsg( F("err_0011") ); }
            };// switch
            state = readTag( f, _tag);  //читать следующий тэг 
          }; // while
          f.close();          
    }; 
      break;
    };  // Get

  Post:
    {      
      s = _webRes; // вся строка - адрес ресурса
      s.trim();
      if( s == "" ){  s = "index.html";    };   //  если пусто то главная страница
      s = "/" + s;
      s = _rootWeb + s;
      //ищем файл - ресурс 
      if( !SD.exists( s ) ){  //если файл не найден, то отправляем 
        sendHeaderAnswer( 404 ); //404 Not Found — сервер не смог найти запрашиваемый ресурс. если при открытии файла произошла ошибка
        Log.writeMsg( F("err404_1") );
      }
      else{  // файл ресурса существует 
          // параметры разобраны ранее и проверены в методе run
          sendHeaderAnswer( 200 );  //200 OK — запрос принят и корректно обработан веб-сервером. ответ на корректный GET
          _client.println("Content-type: text/html");
          // ?? _client.println("Content-length: " +  );
          File f = SD.open(s, FILE_READ);  // открываем файл
          TstateReadTag state = readTag( f, _tag);  //читать тэг  пропуская лишние пробелы и переводы строки из файла
          while( state != endOfFile ){   //пока не конец файла            
            switch (state){  // в зависимости от того что прочли из файла
            case Tag:{
              // вставляем значение параметров , если тэг input  и подстановка праметра прошла успешно
              if( tagProcessing( _tag ) ){ 
                _client.println(_tag);  //   отправить тэг клиенту
              }
              else{  // ошибка при подстановке параметров                
                sendHeaderAnswer( 400 );           
                Log.writeMsg( F("err400_3") );    
              };
              break;
            };
            case Text:{
              _client.println(_tag);  //   отправить тэг клиенту
              break;
            };
            case Overflow:{
              sendHeaderAnswer( 500 ); 
              Log.writeMsg( F("err500_1") ); 
              break;
            };
            default:{  Log.writeMsg( F("err_0011") );  }
            };// switch
            state = readTag( f, _tag);  //читать следующий тэг 
          }; // while
          f.close();
    };      
      break;
    };

  Head:
    {     
      s = _webRes; // вся строка - адрес ресурса
      s.trim();
      if( s == "" ){  s = "index.html";    };   //  если пусто то главная страница
      s = "/" + s;
      s = _rootWeb + s;
      //ищем файл - ресурс 
      if( !SD.exists( s ) ){  //если файл не найден, то отправляем 
        sendHeaderAnswer( 404 ); //404 Not Found — сервер не смог найти запрашиваемый ресурс. если при открытии файла произошла ошибка
        return false;
      }
      else{  // файл ресурса существует 
          File f = SD.open(s, FILE_READ);  // открываем файл
          sendHeaderAnswer( 200 );  //200 OK — запрос принят и корректно обработан веб-сервером. ответ на корректный GET
          _client.println("Content-type: text/html");          
          _client.print("Content-length: " );   
          _client.println( f.size() );   //определить длину
          f.close();
      };      
      break;
    };

  Unknow:
    {
      sendHeaderAnswer( 501 );  // 501 Not Implemented — метод запроса не поддерживается сервером и не может быть обработан. Если состояние TypeReq ==Unknow 
      return false;
      break;
    };

  };
}

bool TWebServer::tagProcessing(String& tag ){ // если тэг типа input , то подставляем значение параметра из Setup 
  //если тэг типа input то ( с пробелом или нет)
  if( ( tag.indexOf("<input") > 0) || ( tag.indexOf("< input") > 0)  ){
      // выделяем имя параметра
      byte st = tag.indexOf("name=\"");  // пробелы до = и после убраны при чтении тэга
      byte fin = tag.indexOf("\"", st);      
      if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли name в параметрах config
        String name = tag.substring( st+6, fin ); 
        String val = config.getField( name ); 
        if( val == NOT_CONFIG_VARIABLE ){ return false; };  // если параметр не нашли в конфиге то выбрасываемся и выше обрабатываем ошибку
        st = tag.indexOf("value=\"");    // пробелы до = и после убраны при чтении тэга
        fin = tag.indexOf("\"", st); 
        if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли value в тэге
            // к началу строки с value=" добавляем значение и закрывающую часть
            tag = tag.substring( 0, st+8 ) + val + tag.substring( fin ); 
        }
        else{  return false;  };       
      }      
      else{  return false;  };    //  не нашли имя параметра - ошибка
  };// конец подстановки параметров 
  return true;  // если дошли сюда, то все ок
}

void TWebServer::sendHeaderAnswer( uint16_t codeAnswer ){ //отправка заголовка ответа сервера ( при ошибках тела может и не быть)
#define VER_HTTP "HTTP/1.1"   // версия поддерживаемого протокола
  switch(codeAnswer){
// сообщения в ответах сервера
  case 200:{ _client.print(VER_HTTP); _client.println(F("200 OK")); break; };
  case 201:{ _client.print(VER_HTTP); _client.println(F("201 Created")); break; };
  case 400:{ _client.print(VER_HTTP); _client.println(F("400 Bad Request")); break; };
  case 401:{ _client.print(VER_HTTP); _client.println(F("401 Unauthorized")); break; };
  case 403:{ _client.print(VER_HTTP); _client.println(F("403 Forbidden")); break; };
  case 404:{ _client.print(VER_HTTP); _client.println(F("404 Not Found")); break; };
  case 406:{ _client.print(VER_HTTP); _client.println(F("406 Not Acceptable")); break; };
  case 408:{ _client.print(VER_HTTP); _client.println(F("408 Request Timeout")); break; };
  case 411:{ _client.print(VER_HTTP); _client.println(F("411 Length Required")); break; };
  case 413:{ _client.print(VER_HTTP); _client.println(F("413 Payload Too Large")); break; };
  case 500:{ _client.print(VER_HTTP); _client.println(F("500 Internal Server Error")); break; };
  case 501:{ _client.print(VER_HTTP); _client.println(F("501 Not Implemented")); break; };
  case 505:{ _client.print(VER_HTTP); _client.println(F("505 HTTP Version Not Supported")); break; };
  default: { _client.print(VER_HTTP); _client.println(F("500 Internal Server Error")); };
  };
}
