#pragma once
#include "web.h"
#include "conf.h"
#include "log.h"
#include <Ethernet2.h>
#include "SD.h"
#include "debug.h"
//


TWebServer::TWebServer(String rootWeb) {  // конструктор, параметр - корневая директория вэб сервера, тайм аут соединения в циклах контроллера
  _tag.reserve(MAX_LEN_TAG);  // !!! резервируем место для строки
  _req.reserve(MAX_LEN_REQ);  // !!! резервируем место для строки
  _rootWeb = rootWeb;
  _state = Wait;                   // состояние сервера
  _typeReq = None;                 // тип запроса
  _stateReadReq = Header;          // состояние чтения запроса
  _webRes = "";  // ресурс запрашиваемый на сервере
  _tag = "";    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
  _req = "";
  for(byte i = 0; i < 8; i++){ clientLogin[i] = '*';  clientPassword[i] = '*';  };
  authorized = false;  // признак прохождения авторизации  
  scheduledReset = false;
  _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
  _counterL = 0;  // фактически полученная длина запроса   
}

void TWebServer::reset() {  // сброс соединения, например из-за тайм -аута
  _tag.reserve(MAX_LEN_TAG);  // !!!резервируем место для строки
  _req.reserve(MAX_LEN_REQ);  // !!!резервируем место для строки
  _state = Wait;                   // состояние сервера
  _typeReq = None;                     // тип запроса
  _stateReadReq = Header;           // состояние чтения запроса
  _req = "";  // строка для хранения текущего запроса,  длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
  _webRes = "";  // ресурс запрашиваемый на сервере
  _tag = "";    // для хранения тэга, длинная строка с зарезервированной длинной для оптимизации (255 символов в конструкторе класса)
  for(byte i = 0; i < 8; i++){ clientLogin[i] = '*';  clientPassword[i] = '*';  };
  authorized = false;  // признак прохождения авторизации
  _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
  _counterL = 0;  // фактически полученная длина запроса     
  scheduledReset = false;
}


void TWebServer::startConnection(EthernetClient client) {  // старт обслуживания клиента
  _client = client;
  _state = RecivingRequest;  
}

bool TWebServer::isWait() {  //сервер  в состоянии ожидания запроса
  return _state == Wait;
}
bool TWebServer::isFinished(){  //сервер  в состоянии завершения обслуживания запроса
  return _state == Finished;
}

void TWebServer::run() {  // работа сервера, если ошибка или проблема, то нужно вернуть false, чтоб сбросить соединение
_PRN(" web run ") 
  switch (_state) {
    // case Wait:  -  этого состояния быть не может , из него включается startConnection и идет переход на RecivingRequest      
    case RecivingRequest:
      {
        _PRN(" run - RecivingRequest ")
        if( readReq() ){  // если дочитали запрос, то меняем состояние
            _LOOK( _req.length())
            _LOOK( _content_Length)
            // проверяем на ошибки в запросе        
            if( _counterL > MAX_LEN_REQ ){  // слишком длинный запрос  413; 
              sendStdAnswer( 413 );
              Log.writeMsg( F("err413_1") );   
              _state = Finished;
              return;
            };
            if( _req.length() > _content_Length ){  //  тело больше заявленного  400;
              sendStdAnswer( 400 );
              Log.writeMsg( F("err400_2") );
              _state = Finished;          
              return;         
            };
            if( (_content_Length < 0) && (_typeReq == Post) ){  // если запрос Post, а тела нет то ошибка
              sendStdAnswer( 400 );
              Log.writeMsg( F("err400_4") );
              _PRN("err400_4")       
              _state = Finished;   
              return;
            };
            // ?? а если запрос POST и тело пустое, то надо сообщение 400 Bad Request  ?? пока не реализованно
            // если дошли сюда, то все ОК и переходим к следующему состоянию
              _state = ParsingParam;
              _PRN(" after RecivingRequest state= ")
              _LOOK( _state)
        };  // иначе на следующем такте продолжим чтение        
        break;        
      };
    case ParsingParam:
      {        
        _PRN(" run - parsingparam ")
        // результаты успешности разбора параметров не анализируем из-за сложностей с набором параметров ( например всегда есть submit в строке параметров, который к параметрам не относится и будет давать ошибку в разборе)
        switch (_typeReq) {  // под возможное расширение типов запросов сделано на switch
        case Get: {  setParam(_webRes);   break;  };    // извлекаем параметры и значения из первой строки  запроса                       
        case Post: { setParam(_req);      break;  };    //если body не пустое
        };  
        _state = SendAnswer;
        break;        
      };
    case SendAnswer:  //из-за нагрузки крайне желательно как отдельное состояние
      {        
        _PRN(" run - send http33 ")
        authorized = true;
        //  ????????????    при повторном запросе не правильно идет с авторизацией. видимо reset не соотвтествует конструктору
        if( !authorized ){  //если не пройдена авторизация то на все запросы кроме авторизации выдавать 401 Unauthorized — получение запрашиваемого ресурса доступно только аутентифицированным пользователям. если нет факта авторизации или просто перенаправлять на авторизацию ?    
          sendStdAnswer( 401 );
          Log.writeMsg( F("err401_1") );
          _state = Finished;  // готовимся к след. клиенту
          _LOOK( _state)
          return;
        };  
        sendHTTP();        
        _state = Finished;  // готовимся к след. клиенту        
        break;        
      };
    default:
      { Log.writeMsg( F("err_0010") ); };  // запись о внутренней ошибке
  };  // case
  return;
}

/* ??
https://webkyrs.info/post/http-zapros-metodom-post
Если, для того, чтобы обратиться к серверу методом GET, нам достаточно было набрать запрос в URL-адрес, то в методе POST все работает по другому принципу.
Для того, чтобы выполнить этот вид запроса, нам необходимо нажать на кнопку с атрибутом type="submit", которая расположена на веб-странице. Обратите внимание,
 что эта кнопка расположена в элементе <form>, у которого установлен атрибут method со значением post.
*/

bool TWebServer::readReq() {  // чтение запроса от клиента целиком, возвращает true, если запрос прочитан целиком. Устанавливает тип полученного запроса, текст параметра в запросе
     _PRN("read request")  
     _MFREE                                                       
  if (!_client.connected()) {       // если клиент  НЕ подключен    
  _PRN("no connected client")
    return true;
  };
  while (_client.available()) {  // пока есть данные в буфере от клиента?
    char ch = _client.read();
    { _req = _req + String(ch);  };  // для сброса вспомогательных строк из памяти
    //{ _LOOK( _req.length() )  };
    _counterL++;    
    if (_counterL > MAX_LEN_REQ) {
      //аварийное завершение, слишком длинный запрос, не хватает памяти, обработка ошибки в вызывающей процедуре по  _counterL > MAX_LEN_REQ
      _PRN("very long request ERROR ")
      return true;           
    }
  };
_LOOK(_req)
_LOOK(_counterL)
  if (_stateReadReq == Header) {
   /* _LOOK(" now reading the header ","8 char end:")
    for(byte i=8; i>1; i--){
      Serial.print(byte(_req.charAt(_req.length()-1-i)));
    };
    Serial.println();*/
    // если нашлось CR+LF+CR+LF, то заголовок окончился. ищем тип, длину  и переходим к приему тела при необходимости
    if (_req.indexOf("\r\n\r\n") != -1) {      
      _req.trim();  // убрали пробелы
      _PRN(" end title, Request= ")
      _LOOK(_req)
      // ищем тип запроса в НАЧАЛЕ запроса
      //  ??  проверить эту часть +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      int st_webRes;  // начальный индекс 
      int endFirsLine = _req.indexOf("\r\n");
      if (_req.indexOf("GET") == 0) {  //если нашлось GET
        _typeReq = Get;
        st_webRes = 3;
      } else if (_req.indexOf("POST") == 0) {  // если нашлось POST
        _typeReq = Post;
        st_webRes = 4;
      };
      int end_webRes = _req.lastIndexOf(" ",endFirsLine);  // исключаем  доп параметры (например HTTP/1.0) после пробела и до конца первой строки заголовка запроса
      _webRes =  _req.substring( st_webRes, end_webRes ) ;  // запрошенный ресурс между заголовком запроса ( GET, POST) и до первого с конца пробела в первой строке запроса
      _webRes.trim();
      
      _LOOK( _typeReq)
      _LOOK( _webRes)            
      // продолжаем обработку заголовка запроса = найти длину от Content-Length: до CR+LF
      bool res;
      if (_req.indexOf("Content-Length:") != -1) {
        int st_char = _req.indexOf("Content-Length:") + 15 + 1;
        int end_char = _req.indexOf("\r\n", st_char);
        String s = _req.substring(st_char, end_char);
        s.trim();
        _content_Length = s.toInt(); //-1; ?? подгонка
        _LOOK(_content_Length)
       res = false;
      } else {  // нет контента в запросе, значит запрос прочитан целиком
        _content_Length = -1;
        res = true;
      };
      _stateReadReq = Body;   // переходим к чтению тела сообщения, т.к. заголовок уже прочли
      _req.remove(0, _req.indexOf("\r\n\r\n") + 4);// ?????????   + 1);  // убираем из _req заголовок ( +4 чтоб убрать CR+LF+CR+LF и +1 чтоб получить количество символов из позиции- учет что первый символ с 0 индексом)
      return res;
    }
  }
  // в случае если в запросе есть длина, но из-за ошибки мы не можем прочитать тело запроса, то мы вылетим по ошибке из-за тайм-аута.
  else if (_stateReadReq == Body) {  // если уже перешли к чтению тела запроса  
    if( _req.length() >= _content_Length ) {  // если прочли все тело  или даже прочли лишнее то завершили чтение запроса, если слишком много в буфере (длина больше прописанной в запросе), то обработка идет в вызывающей процедуре     
      return true;
    };
  };  
  return false;
}

bool TWebServer::setParam(String& s) {  // извлекаем параметры и значения из строки и записываем в глобальный setup
  bool res = true;
  _PRN("in setParam ")
  _LOOK(s)
  if( s.length() == 0 ){ return true; }; // если строка пустая- выходим 
  String val;
  int st=0;  // нач позиция подстроки с параметрами для POST body  
  if (s.indexOf("?") != -1) {  // если в первой строке запроса GET 
    st = _webRes.indexOf("?") + 1;  // выделяем параметры из GET   
  };
  // если нет = , значит нет параметров - выходим  
  _LOOK(s.indexOf("="))
  if( s.indexOf("=") == -1 ){  return true;  };  
  // цикл разбора параметров
  do {
      int n = s.indexOf("=", st);
      String name = s.substring(st, n);  //   выделить до символа = имя
      int sep = s.indexOf("&", st);
      // выделяем значение до разделителя & или ( если его нет) - до конца строки
      if (sep != -1) {      
        val = s.substring(n + 1, sep);
      } else {
        val = s.substring(n + 1);
      };
      _LOOK(name)
      _LOOK(val)

      JsonVariant error = webConfig[name];  // проверяем наличие ключа
      if (!error.isNull()) {   webConfig[name] = val;  }    //  записать, флаг не трогать, он остается прежним
      else{ res = false; }; // сброс флага
      _PRN("set OK? ")
      _LOOK(res)
      if (sep != -1) { st = sep + 1; }  // переходим к следующему параметру по разделителю &
      else {  st = -1;   };
  } while ( st > 0 );  // пока не дошли до конца строки  
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


void TWebServer::sendHTTP() {  // отправка ответа от HTTP сервера (работает после полученного запроса _typeReq != None )
// параметры запроса должны быть уже разобраны и сохранены в parsingParam();
  String s=""; 
  _PRN(" in send HTTP, _typeReq= ")
  _LOOK(_typeReq)
  switch(_typeReq) {
  case Get:
    {     
      //ищем , если есть ?, то есть праметры и их пока брать не надо
      if (_webRes.indexOf("?") != -1) {   s = _webRes.substring(0,_webRes.indexOf("?")); } // ресурс
      else{ s = _webRes; }; // иначе вся строка - адрес ресурса
      s.trim();
      _LOOK(s)
      if( s.indexOf("CMD://") == 0 ) { // если это запрос на выполнение команды на сервере
          if( cmd( s ) ){  // то вызвать команду , выполнить и вернуть страницу выполнения ( успех или отказ)
              s = "success.htm";  // подставляем, чтоб отдать страницу об успешном исполнении
          } else {
              s = "error.htm";  // подставляем, чтоб отдать страницу об ошибки
          };    
      };  // если это запрос на выполнение команды на сервере
      _LOOK(s)
      if( (s == "") || (s == "/") ){  s = "/index.htm";    };   //  если пусто то главная страница
      s = _rootWeb + s;
      _LOOK(s)
      //ищем файл - ресурс 
      File f = SD.open(s, FILE_READ);  // открываем файл
      if ( !f ){  //     if( !SD.exists( s ) ){  //если файл не найден, то отправляем 
        f.close(); // на всякий случай
        sendStdAnswer( 404 ); //404 Not Found — сервер не смог найти запрашиваемый ресурс. если при открытии файла произошла ошибка
        Log.writeMsg( F("err404_1") );
      }
      else{  // файл ресурса существует 
          // параметры разобраны ранее и проверены в методе run
          _PRN(" send 4 200 ok ")
          sendHeader( "200 OK" );  //200 OK — запрос принят и корректно обработан веб-сервером. ответ на корректный GET                    
          TstateReadTag state = readTag( f, _tag);  //читать тэг  пропуская лишние пробелы и переводы строки из файла
          while( state != endOfFile ){   //пока не конец файла            
            switch (state){  // в зависимости от того что прочли из файла
            case Tag:{
              tagProcessing( _tag );  // нет реакции если в процессе подстановки не нашли параметра, чтоб нормально обрабатывало кнопки и другие не находящиеся в config объекты
              _client.println(_tag);  //   отправить тэг клиенту              
              break;
            };  
            case Text:{
              _client.println(_tag);  //   отправить тэг клиенту
              break;
            };
            case Overflow:{
              sendStdAnswer( 500 ); 
              Log.writeMsg( F("err500_1") ); 
              break;
            };
            default:{  Log.writeMsg( F("err_0011") ); }
            };// switch
            state = readTag( f, _tag);  //читать следующий тэг 
          }; // while
          f.close();          
    }; 
      // если запланирован reset
      if( scheduledReset ){ resetMCU(); };
      break;
    };  // Get

  case Post:
    {      
      _PRN("post 1")
      s = _webRes; // вся строка - адрес ресурса
      s.trim();
      if( s == "" ){  s = "/index.htm";    };   //  если пусто то главная страница
      s = _rootWeb + s;
      //ищем файл - ресурс 
      if( !SD.exists( s ) ){  //если файл не найден, то отправляем 
        sendStdAnswer( 404 ); //404 Not Found — сервер не смог найти запрашиваемый ресурс. если при открытии файла произошла ошибка
        Log.writeMsg( F("err404_1") );
      }
      else{  // файл ресурса существует 
          // параметры разобраны ранее и проверены в методе run
          sendHeader( "200 OK" );  //200 OK — запрос принят и корректно обработан веб-сервером. ответ на корректный GET
          File f = SD.open(s, FILE_READ);  // открываем файл
          TstateReadTag stateReadTag = readTag( f, _tag);  //читать тэг  пропуская лишние пробелы и переводы строки из файла
          while( stateReadTag != endOfFile ){   //пока не конец файла            
            switch (stateReadTag){  // в зависимости от того что прочли из файла
            case Tag:{
              tagProcessing( _tag );  // нет реакции если в процессе подстановки не нашли параметра, чтоб нормально обрабатывало кнопки и другие не находящиеся в config объекты
              _client.println(_tag);  //   отправить тэг клиенту              
              break;
            };              
            case Text:{
              _client.println(_tag);  //   отправить тэг клиенту
              break;
            };
            case Overflow:{
              sendStdAnswer( 500 ); 
              Log.writeMsg( F("err500_1") ); 
              break;
            };
            default:{  Log.writeMsg( F("err_0011") );  }
            };// switch
            stateReadTag = readTag( f, _tag);  //читать следующий тэг 
          }; // while
          f.close();
    };      
      break;
    };

  case Unknow:
    {
      _PRN("unknow 1")
      sendStdAnswer( 501 );  // 501 Not Implemented — метод запроса не поддерживается сервером и не может быть обработан. Если состояние TypeReq ==Unknow 
      break;
    };

  };  
}

bool TWebServer::tagProcessing(String& tag ){ // если тэг типа input , то подставляем значение параметра из Setup   
  //если тэг типа input то ( с пробелом или нет)  
  if( ( tag.indexOf("<input") >= 0) || ( tag.indexOf("< input") >= 0)  ){    
    _PRN("tagProcessing input- ")
    _LOOK(tag)
      // выделяем имя параметра
      byte st = tag.indexOf("name=\"");  // пробелы до = и после убраны при чтении тэга
      byte fin = tag.indexOf("\"", st+6);            
      if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли name в параметрах config
        String name = tag.substring( st+6, fin );         
        JsonVariant error = webConfig[name];  // проверяем наличие ключа        
        if ( error.isNull() ){  return false; };  // если параметр не нашли в конфиге то выбрасываемся и выше обрабатываем ошибку
        String val = webConfig[name]; 
        _LOOK(name)
        _LOOK(val)               
        st = tag.indexOf("value=\"");    // пробелы до = и после убраны при чтении тэга
        fin = tag.indexOf("\"", st+7);         
        if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли value в тэге
            // к началу строки с value=" добавляем значение и закрывающую часть            
            tag = tag.substring( 0, st+7 ) + val + tag.substring( fin ); 
            _PRN("set param:")
            _LOOK( tag)
        }
        else{  return false;  };       
      }      
      else{  return false;  };    //  не нашли имя параметра - ошибка
  };// конец подстановки параметров 
  return true;  // если дошли сюда, то все ок
}

void TWebServer::sendStdAnswer( uint16_t codeAnswer ){ //отправка стандартного ответа ответа сервера 

//  это обрабатывается отдельно !!! case 200:{ _client.print(VER_HTTP); _client.println(F("200 OK")); break; };
  //  это обрабатывается отдельно !!!  case 201:{ _client.print(VER_HTTP); _client.println(F("201 Created")); break; };
  String msg = "";
  _LOOK( codeAnswer)
  switch(codeAnswer){
// сообщения в ответах сервера  
  case 400:{ msg = F("400 Bad Request"); break; };
  case 401:{ msg = F("401 Unauthorized"); break; };
  case 403:{ msg = F("403 Forbidden"); break; };
  case 404:{ msg = F("404 Not Found"); break; };
  case 406:{ msg = F("406 Not Acceptable"); break; };
  case 408:{ msg = F("408 Request Timeout"); break; };
  case 411:{ msg = F("411 Length Required"); break; };
  case 413:{ msg = F("413 Payload Too Large"); break; };
  case 500:{ msg = F("500 Internal Server Error"); break; };
  case 501:{ msg = F("501 Not Implemented"); break; };
  case 505:{ msg = F("505 HTTP Version Not Supported"); break; };
  default: { msg = F("500 Internal Server Error"); };
  };
  sendHeader( msg );
  _client.println(F("<!DOCTYPE HTML>")); // тело сообщения
  _client.println(F("<html>"));
  _client.print(F("<H1> ")); _client.print( msg ); _client.print(F(" </H1>")); // выбираем крупный шрифт  и печатаем сообщение
  _client.println(F("</html>"));        
  // ??delay(1);
}

void TWebServer::sendHeader( String msg ){ //отправка заголовка ответа сервера 
  _client.print(F("HTTP/1.1 ")); _client.println( msg ); // стартовая строка с кодом ошибки
  _client.println(F("Content-Type: text/html; charset=utf-8")); // тело передается в коде HTML, кодировка UTF-8
  _client.println(F("Connection: close")); // закрыть сессию после ответа
  _client.println(); // пустая строка отделяет тело сообщения
}

bool TWebServer::cmd( String s ){                         // функция выполнения команд на сервере
  s.remove(0,6); // выделить название команды
  /*  ?????????????????????????  временно отключено
  if( s == reboot ){  scheduledReset = true; return true; };
  if( s == save ){  webConfig.save("/conf2.ini");  scheduledReset = true;  return true; };
  if( s == defConf ){ webConfig.setDefaultField();  return true; };
  if( s == clearLog ){ Log.clear();  return true; };
  if( s == logOn ){ // проверка пароля и вход 
      if( !strcmp( config[login, clientLogin ) && !strcmp( config[password, clientPassword ) ){  // если логин и пароль совпадают (strcmp выдает 0) 
            authorized = true;
            webConfig.load( config );  // загружаем текущую конфигурацию
            return true;  
      };
  }
  else{
    authorized = false;
    Log.writeMsg( F("a_denied") );  //сообщение об отказе в доступе в лог
    return false;  
  };
  if( s == changeP ){ changePort( D ); return true; };

  */
  return false;  
}

static TWebServer webServer = TWebServer( "/web" );  // объект вэб-сервер