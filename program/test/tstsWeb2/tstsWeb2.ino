#include <SPI.h>
#include <Ethernet2.h>

// РАБОТАЕТ ПРИ ОТКЛЮЧЕННОМ SD CARD !!!!!

//SPI
#define MOSI 51
#define MISO 50
#define SCK 52
#define CS_W5500 10
#define CS_SD_CARD 42


 
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEA};
  IPAddress ip(10, 140, 33, 147);// собственный алрес Ethernet интерфейса
  IPAddress ip_dns(10, 140, 33, 1);
  IPAddress gateway(10, 140, 33, 1);
  IPAddress subnet(255, 255, 255, 0);

  EthernetServer server(80);
 
void setup() { 
  Serial.begin(9600);
 // Ethernet.begin(mac, ip);
//Ethernet.init(10);
  //Ethernet.begin(mac, ip, ip_dns, gateway, subnet);
  Ethernet.begin(mac, ip);
  delay(1000);
    //Ethernet.init(53);
  
  server.begin();
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
    Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
    Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());
    //    for(;;){};
  //char* dnsDomainName();
  //char* hostName();

  //дает 0 вместо адреса
  //что то с SPI ?

}
 
 
void loop() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    boolean ok = true;
 
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
 
        if (c == '\n' && ok) {
 
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println("Refresh: 5"); // время обновления страницы 
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html><meta charset='UTF-8'>");
 
          client.println("<h1>Привет МИР!!!</h1>");
 
          client.println("</html>");
 
          break;
        }
        if (c == '\n'){ok = true;}else if(c != '\r'){ok = false;}
      }
    }
 
    delay(1);
    client.stop();
    Serial.println("client disconnected");
  }
}
