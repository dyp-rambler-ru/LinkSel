#pragma once
#include "cmd.h"
#include "conf.h"
#include "log.h"
#include "common.h"
#include "web.h"
#include "counters.h"

Tcmd::Tcmd(){
}

bool Tcmd::execute(String line){
 line.remove( 0,1 ); // убираем "/"" в начале
 line.remove( line.indexOf( POSTFIX_CMD ) ); // выделить название команды
  _SEE(line)
 char* c_name = line.c_str();
  if( strcmp_P( c_name, reboot ) == 0 ){   D.scheduledReset = true;  return true; };
  if( strcmp_P( c_name, save ) == 0 ){  webConfig.save(CONFIG_FILE);  D.scheduledReset = true;  return true; };
  if( strcmp_P( c_name, defConf ) == 0 ){  webConfig.setDefaultField();  return true; };
  if( strcmp_P( c_name, clearLog ) == 0 ){ Log.clear();  return true; };
  if( strcmp_P( c_name, logOut ) == 0 ){ // выход из вэб интерфейса - забываем пароль от клиента        
        webServer.abortSession();
        Log.writeMsg( F("exit") );  //сообщение об отказе в доступе в лог
        return true;  
  };  
  if( strcmp_P( c_name, changeP ) == 0  ){ 
    D.autoReturnA = false; // автовозврат так же сбрасывается при каждом успешном пинге, чтоб исключить приращение времени автовозврата при обычной работе и отсутствии постоянного отказа канала А
    D.calcDelayReturnA = config.field.delayReturnA;  // задержка возврата на А устанавливается по конфигурации, без учета возможных приращений возникших из-за неисправности канала А
    counters.load( returnA, D.calcDelayReturnA * 1000);
    changePort( D );
    return true; 
  };
  return false;  
}


static Tcmd cmd = Tcmd();