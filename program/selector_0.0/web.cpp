#pragma once
#include "web.h"
#include "conf.h"
#include "log.h"
#include <Ethernet2.h>
#include "SD.h"
#include "debug.h"
#include "common.h"


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
  _content_Length = 0;  // длина тела запроса в байтах ( без заголовка)
  _counterL = 0;  // фактически полученная длина запроса     
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

bool TWebServer::isLoginProcess(TSetup _setup ){        // проверить, что пользователь ввел сейчас правильную пару логин/пароль на вэб странице устройства
// сравнить логины и пароли
  needSendSessionID = ( strcmp(webConfig.field.login, _setup.field.login) == 0 ) && (strcmp(webConfig.field.password, _setup.field.password) == 0 );  
  webConfig.resetLoginPassword();   // сброс пароля в параметрах конфигурации 
  return needSendSessionID;
}

bool TWebServer::sendNewSessionID(){ // генерируем новый идентификатор сессии  
  if( needSendSessionID && (sessionID == 0) ){  // если сессионный идентификатор сброшен и проверка логин/пароль показала что пара правильна и надо посталь новый идентификтор сессии
    _PRN(F("#### gen and send SessionID"))
      String d = webConfig.getField("date");
      String t = webConfig.getField("time");      
      //	(сек, мин, час, день, мес, год, день_недели)
      uint32_t timeUnix = t.substring( t.lastIndexOf(":") + 1 ).toInt() + t.substring( t.indexOf(":") +1, t.lastIndexOf(":") ).toInt()*60 + t.substring( 0, t.indexOf(":")).toInt()*3600;
      timeUnix += d.substring( d.lastIndexOf("-") + 1 ).toInt() * 3600 * 24 + d.substring( d.indexOf("-") +1, d.lastIndexOf("-") ).toInt() * 3600 * 24 * 30 + d.substring( 0, d.indexOf("-")).toInt() * 3600 * 24 * 365;
      _LOOK(timeUnix)
      uint32_t r = random(0x7FFFFFFF);
      _LOOK(r)
      sessionID = (SESSION_ID_PASSWORD ^ timeUnix ^ r );
      needSendSessionID = false;
      _LOOK(sessionID)
      _client.println("Set-Cookie: LinkSel=" + String(sessionID) + String(F("; SameSite=Strict")) ); // дополнительно определяем SameSite=Strict чтоб не передавать с других страниц, типа безопасность
      return true;
  }else{  return false; };
}

void TWebServer::abortSession(){ // сброс идентификатора сессии - сессия закрыта
_PRN(F(" ### ABORT SESSION"))
    sessionID = 0;
    receivedID = 0;  
    needSendSessionID = false;
}

void TWebServer::run() {  // работа сервера, если ошибка или проблема, то нужно вернуть false, чтоб сбросить соединение
_PRN(" web run ") 
  switch (_state) {
    // case Wait:  -  этого состояния быть не может , из него включается startConnection и идет переход на RecivingRequest      
    case RecivingRequest:
      {
        _PRN(" run - RecivingRequest ")
        if( readReq() ){  // если дочитали запрос, то меняем состояние
            _LOOK(_req.length())
            _LOOK(_content_Length)
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
              _LOOK(_state)
        };  // иначе на следующем такте продолжим чтение        
        break;        
      };
    case ParsingParam:
      {        
        _PRN(F(" run - parsingparam "))
        // результаты успешности разбора параметров не анализируем из-за сложностей с набором параметров ( например всегда есть submit в строке параметров, который к параметрам не относится и будет давать ошибку в разборе)
        switch (_typeReq) {  // под возможное расширение типов запросов сделано на switch
        case Get:     // извлекаем параметры и значения из первой строки  запроса                       
            {
              setParam(_webRes);
              isLoginProcess( config );  // проверяем пытается ли пользователь войти в систему  
              break;  
            };  
        case Post:   //если body не пустое 
            {
              setParam(_req);
              isLoginProcess( config );  // проверяем пытается ли пользователь войти в систему  
              break;
            };    
        };  
        _state = SendAnswer;
        break;        
      };
    case SendAnswer:  //из-за нагрузки крайне желательно как отдельное состояние
      {        
        _PRN(F(" run - send http33 "))
        sendHTTP();        
        _state = Finished;  // готовимся к след. клиенту        
        break;        
      };
    default:
      { Log.writeMsg( F("err_0010") ); };  // запись о внутренней ошибке
  };  // case
  return;
}

/* 
https://webkyrs.info/post/http-zapros-metodom-post
Если, для того, чтобы обратиться к серверу методом GET, нам достаточно было набрать запрос в URL-адрес, то в методе POST все работает по другому принципу.
Для того, чтобы выполнить этот вид запроса, нам необходимо нажать на кнопку с атрибутом type="submit", которая расположена на веб-странице. Обратите внимание,
 что эта кнопка расположена в элементе <form>, у которого установлен атрибут method со значением post.
*/

bool TWebServer::readReq() {  // чтение запроса от клиента целиком, возвращает true, если запрос прочитан целиком. Устанавливает тип полученного запроса, текст параметра в запросе
     _PRN(F("read request"))  
     _MFREE                                                       
  if (!_client.connected()) {       // если клиент  НЕ подключен    
  _PRN(F("no connected client"))
    return true;
  };
  while (_client.available()) {  // пока есть данные в буфере от клиента?
    char ch = _client.read();
    { _req = _req + String(ch);  };  // для сброса вспомогательных строк из памяти
    //{ _LOOK("len _req", _req.length() )  };
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
    // если нашлось CR+LF+CR+LF, то заголовок окончился. ищем тип, длину  и переходим к приему тела при необходимости
    if (_req.indexOf("\r\n\r\n") != -1) {      
      _req.trim();  // убрали пробелы
      // ищем тип запроса в НАЧАЛЕ запроса
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
        _content_Length = s.toInt(); 
        _LOOK( _content_Length)
       res = false;
      } else {  // нет контента в запросе, значит запрос прочитан целиком
        _content_Length = -1;
        res = true;
      };
      // продолжаем обработку заголовка запроса - найти куку от Cookie: LinkSel= до CR+LF
      if (_req.indexOf("Cookie: LinkSel=") != -1) {
        _PRN(F("#### recived Cookie"))
        _LOOK(_req)
        int st_char = _req.indexOf("Cookie: LinkSel=") + 16;
        int end_char = _req.indexOf("\r\n", st_char);
        String s = _req.substring(st_char, end_char);
        s.trim();
        receivedID = s.toInt();         
      } else {  // нет кука , то ставим 0, авторизации еще не было
        receivedID = 0;
      };
      _stateReadReq = Body;   // переходим к чтению тела сообщения, т.к. заголовок уже прочли
      _req.remove(0, _req.indexOf("\r\n\r\n") + 4); // убираем из _req заголовок 
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
  _LOOK(s)
  if( s.length() == 0 ){ return true; }; // если строка пустая- выходим 
  String val;
  int st=0;  // нач позиция подстроки с параметрами для POST body  
  if (s.indexOf("?") != -1) {  // если в первой строке запроса GET 
    st = _webRes.indexOf("?") + 1;  // выделяем параметры из GET   
  };
  // если нет = , значит нет параметров - выходим  
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
      // костыль, в web двоеточие идет как %3A заменяем в val на : чтоб дальше все нормально работало
      val.replace("%3A",":");
      _LOOK(name)
      _LOOK(val)
      res = res && webConfig.setField(name, val);     
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
    while( ( (ch = f.read()) != -1 ) && (count < MAX_LEN_TAG ) ){      
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
    // проверяем что это начало тэга textarea и в этом случае меняем тип тэга
    if( (tag.indexOf("<textarea ") == 0 ) || (tag.indexOf("< textarea ") == 0 ) )  { typeRes = TagTextArea; };
    // проверяем на слишком длинную строку
    if( count == MAX_LEN_TAG ){ 
      tag[ tag.length()-1 ] = '>'; // принудительно закрываем тэг (хотя это будет неправильный тэг) и выходим с ошибкой
      typeRes = Overflow;
      return typeRes;
    };
  }else{  // чтение просто текста до конца строки
      typeRes = Text;  //тип результата - Text      
      //    пока не конец файла или не слишком много символов
      while( ( (ch = f.read()) != -1 ) &&  (count < MAX_LEN_TAG ) ){          
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
      if( count == MAX_LEN_TAG ){         
        typeRes = Overflow;
        return typeRes;
      };
  };// конец чтения

  return typeRes;  // если сюда дошли, то нормальное завершение
}

bool TWebServer::sendHTML_File(String s){  // отправка HTML файла, если все ОК то возвращает true
  bool res = true;
 if( (s == "") || (s == "/") ){  s = "/index.htm";    };   //  если пусто то главная страница
      s = _rootWeb + s;
      _LOOK(s)
      //ищем файл - ресурс 
      File f = SD.open(s, FILE_READ);  // открываем файл
      if ( !f ){  //     if( !SD.exists( s ) ){  //если файл не найден, то отправляем 
        f.close(); // на всякий случай
        sendStdAnswer( 404 ); //404 Not Found — сервер не смог найти запрашиваемый ресурс. если при открытии файла произошла ошибка
        Log.writeMsg( F("err404_1") );
        res = false;
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
            case TagTextArea:{
              //   отправляем сам тэг, текст из файла отправляется следом
              _PRN("text area tag send")
              // получить количество строк из тэга 
              // выделяем имя параметра
              int16_t numLn;
              byte st = _tag.indexOf("rows=\"");  // пробелы до = и после убраны при чтении тэга
              byte fin = _tag.indexOf("\"", st+6); // ищем закрывающую кавычку
              if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли rows  то определить количество строк
                  String val = _tag.substring( st+6, fin); 
                  numLn = val.toInt();
                  if( numLn <0 ){ numLn = 0;};
                  if( numLn >1000 ){ numLn = 1000;};
              }                   
              else{  numLn = 999; };
              numLn--; // корректируем на -1, чтоб отобразить при необходимости сообщение об очень большой длине файла
              _client.println(_tag);                
              File logFile = SD.open(LOG_FILE, FILE_READ);
              if( !logFile ){  // файл лога не найден
                _client.println(F(" Log file not found.")); 
                logFile.close();  // на всякий, хотя по идее ничего не открыто
                break;
              };
              uint16_t cLn = 0; // кол-во прочитанных строк
              // читаем очередную строку лога в _tag и отправляем клиенту в браузер
              while( readLn( logFile, _tag ) && cLn < numLn ){ _client.println(_tag); cLn++;  };
              logFile.close();
              if( cLn == numLn ){  _client.println(F(" !!! very long LOG file, can't display it !!!")); };
              break;
            };  
            case Text:{
              _client.println(_tag);  //   отправить тэг клиенту
              break;
            };
            case Overflow:{
              sendStdAnswer( 500 ); 
              Log.writeMsg( F("err500_1") ); 
              res = false;
              break;
            };
            default:{  Log.writeMsg( F("err_0011") ); res = false; }
            };// switch
            state = readTag( f, _tag);  //читать следующий тэг 
          }; // while
          f.close();    
      } 
  return res;     
}

void TWebServer::sendHTTP() {  // отправка ответа от HTTP сервера (работает после полученного запроса _typeReq != None )
// параметры запроса должны быть уже разобраны и сохранены в parsingParam();
  String s=""; 
  _LOOK(_typeReq)
  if( !needSendSessionID && ((sessionID == 0) || (receivedID == 0) || (sessionID != receivedID)) ){  // если кука нет или не совпадает значение , то выдать окно запроса пароля
      sendHTML_File(F("/access.htm"));
      return;
  };

  switch(_typeReq) {
  case Get:
    {     
      //ищем , если есть ?, то есть праметры и их пока брать не надо
      if (_webRes.indexOf("?") != -1) {   s = _webRes.substring(0,_webRes.indexOf("?")); } // ресурс
      else{ s = _webRes; }; // иначе вся строка - адрес ресурса
      s.trim();
      _LOOK(s)
      if( s.indexOf( POSTFIX_CMD ) > 0 ) { // если это запрос на выполнение команды на сервере
          if( cmd.execute( s ) ){  // то вызвать команду , выполнить и вернуть страницу выполнения ( успех или отказ)
              s = "/success.htm";  // подставляем, чтоб отдать страницу об успешном исполнении
          } else {
              s = "/error.htm";  // подставляем, чтоб отдать страницу об ошибки
          };    
      };  // если это запрос на выполнение команды на сервере
      _LOOK(s)
      sendHTML_File( s );  // отправляем файл s
      // если запланирован reset
      if( D.scheduledReset ){ resetMCU(); };
      break;
    };  // Get

  case Post:
    {      
      _PRN(F("post 1"))
      s = _webRes; // вся строка - адрес ресурса
      s.trim();
      sendHTML_File( s );  // отправляем файл s
      // если запланирован reset
      if( D.scheduledReset ){ resetMCU(); };
      break;
    };

  case Unknow:
    {
      _PRN(F("unknow 1"))
      sendStdAnswer( 501 );  // 501 Not Implemented — метод запроса не поддерживается сервером и не может быть обработан. Если состояние TypeReq ==Unknow 
      break;
    };

  };  
}

bool TWebServer::tagProcessing(String& tag ){ // если тэг типа input , то подставляем значение параметра из Setup  или State 
  //если тэг типа input то ( с пробелом или нет)  
  if( ( tag.indexOf("<input") >= 0) || ( tag.indexOf("< input") >= 0)  ){    
    _PRN(tag)
      // выделяем имя параметра
      byte st = tag.indexOf("name=\"");  // пробелы до = и после убраны при чтении тэга
      byte fin = tag.indexOf("\"", st+6);            
      if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли name в параметрах config
        String name = tag.substring( st+6, fin ); 
        String val;
        val = webConfig.getField( name ); 
        _LOOK(name)
        _LOOK(val)       
        // старое if( val == NOT_CONFIG_VARIABLE ){ return false; };  // если параметр не нашли в конфиге то выбрасываемся и выше обрабатываем ошибку
        if( val == NOT_CONFIG_VARIABLE ){ 
              val = D.getParam( name ); 
              if( val == NOT_STATE_VARIABLE ){ return false; };  // если параметр не нашли в конфиге и в структуре состояния устройтсва D, то выбрасываемся и выше обрабатываем ошибку (или не обрабатываем :) )
        }; 
        st = tag.indexOf("value=\"");    // пробелы до = и после убраны при чтении тэга
        fin = tag.indexOf("\"", st+7);         
        if( (st != -1) && (fin != -1) && ( st < fin) ){ //  если нашли value в тэге
            // к началу строки с value=" добавляем значение и закрывающую часть            
            tag = tag.substring( 0, st+7 ) + val + tag.substring( fin ); 
            _LOOK(tag)
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
  _LOOK(codeAnswer)
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
  delay(1);
}

void TWebServer::sendHeader( String msg ){ //отправка заголовка ответа сервера 
  _client.print(F("HTTP/1.1 ")); _client.println( msg ); // стартовая строка с кодом ошибки
  _client.println(F("Content-Type: text/html; charset=utf-8")); // тело передается в коде HTML, кодировка UTF-8
  //  ?? нужно ли это и для каких страниц?   client.println("Refresh: 5"); // время обновления страницы !!!! работает и обновляет  
  sendNewSessionID(); // при необходимости посылаем куку ( один раз за сеанс)
  _client.println(F("Connection: close")); // закрыть сессию после ответа
  _client.println(); // пустая строка отделяет тело сообщения
}

static TWebServer webServer = TWebServer( "/web" );  // объект вэб-сервер