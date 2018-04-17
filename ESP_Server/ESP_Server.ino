#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>

MD5Builder md5;
DNSServer dnsServer;
ESP8266WebServer webServer;

SoftwareSerial uartSerial(0, 2);  // pins 0 and 2 are used here as they translate to pins D3 and D4 on the "D1 Mini board"
WiFiServer uartServer(24);       // you will need to change these pins to suit pins you select on your esp board for uart RX/TX
WiFiClient uartClient;          // the uart socket port is 24 its best to just leave this unless you need to change it for some reason

String firmwareFile = "fwupdate.bin"; //update filename
String firmwareVer = "1.04"; 

//------default settings if config.ini is missing------//
String AP_SSID = "PS4_WEB_AP";
String AP_PASS = "password";
int DNS_PORT = 53;
int WEB_PORT = 80;
int UART_PORT = 24;
IPAddress Server_IP(10,1,1,1);
IPAddress Subnet_Mask(255,255,255,0);
//-----------------------------------------------------//




String split(String str, String from, String to)
{
  String tmpstr = str;
  tmpstr.toLowerCase();
  from.toLowerCase();
  to.toLowerCase();
  int pos1 = tmpstr.indexOf(from);
  int pos2 = tmpstr.indexOf(to, pos1 + from.length());   
  String retval = str.substring(pos1 + from.length() , pos2);
  return retval;
}


bool instr(String str, String search)
{
int result = str.indexOf(search);
if (result == -1)
{
  return false;
}
return true;
}


void returnOK() {
  webServer.send(200, "text/plain", "");
}


void returnFail(String msg) {
  webServer.send(500, "text/plain", msg + "\r\n");
}


bool loadFromSdCard(String path) {
  String dataType = "text/plain";
  if (instr(path,"/document/"))
  {
    path.replace("/document/" + split(path,"/document/","/ps4/") + "/ps4/", "/");
  }
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  if (path.endsWith(".html")) {
    path.replace(".html",".htm");
  }
  if (path.endsWith(".src")) {
    path = path.substring(0, path.lastIndexOf("."));
  } else if (path.endsWith(".htm")) {
    dataType = "text/html";
  } else if (path.endsWith(".css")) {
    dataType = "text/css";
  } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
  } else if (path.endsWith(".png")) {
    dataType = "image/png";
  } else if (path.endsWith(".gif")) {
    dataType = "image/gif";
  } else if (path.endsWith(".jpg")) {
    dataType = "image/jpeg";
  } else if (path.endsWith(".ico")) {
    dataType = "image/x-icon";
  } 

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile) {
    return false;
  }

  if (webServer.hasArg("download")) {
    dataType = "application/octet-stream";
  }

  if (webServer.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}




void handleNotFound() {
  if (loadFromSdCard(webServer.uri())) {
    return;
  }
  String message = "\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += " NAME:" + webServer.argName(i) + "\n VALUE:" + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
  Serial.print(message);
}



void handleUart()
{
  if(!uartClient.connected())
  {
    uartClient = uartServer.available();
  }
  else
  {
    if(uartClient.available() > 0)
    {
      char data[1024];
      int cnt = 0;
      while(uartClient.available())
      {
        data[cnt] = uartClient.read();
        cnt++;
      }    
      uartClient.flush();
      String cBuffer = String(data);
      Serial.println(cBuffer);
      //process commands to send
      //uartSerial.print(cBuffer);
    }
    
    while (uartSerial.available() > 0) {
       String sData = uartSerial.readString();   
       Serial.print(sData);  
       uartClient.print(sData);  
    }
    }
}


void updateFw()
{
  if (SD.exists(firmwareFile)) {
  File updateFile;
  Serial.println("Update file found");
  updateFile = SD.open(firmwareFile, FILE_READ);
 if (updateFile) {
  size_t updateSize = updateFile.size();
   if (updateSize > 0) {   
    md5.begin();
    md5.addStream(updateFile,updateSize);
    md5.calculate();
    String md5Hash = md5.toString();
    Serial.println("Update file hash: " + md5Hash);
    updateFile.close();
    updateFile = SD.open(firmwareFile, FILE_READ);
  if (updateFile) {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace, U_FLASH)) {
    Update.printError(Serial);
    digitalWrite(BUILTIN_LED, HIGH);
	updateFile.close();
    return;
    }
    int md5BufSize = md5Hash.length() + 1;
    char md5Buf[md5BufSize];
    md5Hash.toCharArray(md5Buf, md5BufSize) ;
    Update.setMD5(md5Buf);
    Serial.println("Updating firmware...");
   long bsent = 0;
   int cprog = 0;
    while (updateFile.available()) {
    uint8_t ibuffer[1];
    updateFile.read((uint8_t *)ibuffer, 1);
    Update.write(ibuffer, sizeof(ibuffer));
      bsent++;
      int progr = ((double)bsent /  updateSize)*100;
      if (progr >= cprog) {
        cprog = progr + 10;
      Serial.println(String(progr) + "%");
      }
    }
    updateFile.close(); 
   if (Update.end(true))
  {
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.println("Installed firmware hash: " + Update.md5String()); 
  Serial.println("Update complete");
  SD.remove(firmwareFile);
  ESP.restart();
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.println("Update failed");
     Update.printError(Serial);
    }
  }
  }
  else {
  Serial.println("Error, file is invalid");
  updateFile.close(); 
  digitalWrite(BUILTIN_LED, HIGH);
  SD.remove(firmwareFile);
  return;    
  }
  }
  }
  else
  {
    Serial.println("No update file found");
  }
}



void setup(void) {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Version: " + firmwareVer);

  if (SD.begin(SS)) {
  File iniFile;
  
  if (SD.exists("config.ini")) {
  iniFile = SD.open("config.ini", FILE_READ);
  if (iniFile) {
  String iniData;
    while (iniFile.available()) {
      char chnk = iniFile.read();
      iniData += chnk;
    }
   iniFile.close();


   if(instr(iniData,"SSID="))
   {
   AP_SSID = split(iniData,"SSID=","\r\n");
   AP_SSID.trim();
   }
   
   if(instr(iniData,"PASSWORD="))
   {
   AP_PASS = split(iniData,"PASSWORD=","\r\n");
   AP_PASS.trim();
   }
   
   if(instr(iniData,"WEBSERVER_PORT="))
   {
   String strWprt = split(iniData,"WEBSERVER_PORT=","\r\n");
   strWprt.trim();
   WEB_PORT = strWprt.toInt();
   }
   
   if(instr(iniData,"DNSSERVER_PORT="))
   {
   String strDprt = split(iniData,"DNSSERVER_PORT=","\r\n");
   strDprt.trim();
   DNS_PORT = strDprt.toInt();
   }

   if(instr(iniData,"WEBSERVER_IP="))
   {
    String strwIp = split(iniData,"WEBSERVER_IP=","\r\n");
    strwIp.trim();
    Server_IP.fromString(strwIp);
   }

   if(instr(iniData,"SUBNET_MASK="))
   {
    String strsIp = split(iniData,"SUBNET_MASK=","\r\n");
    strsIp.trim();
    Subnet_Mask.fromString(strsIp);
   }
   
    }
  }
  else
  {
    iniFile = SD.open("config.ini", FILE_WRITE);
    if (iniFile) {
    iniFile.print("\r\nSSID=" + AP_SSID + "\r\nPASSWORD=" + AP_PASS + "\r\n\r\nWEBSERVER_IP=" + Server_IP.toString() + "\r\nWEBSERVER_PORT=" + String(WEB_PORT) + "\r\n\r\nDNSSERVER_PORT=" + String(DNS_PORT) + "\r\n\r\nSUBNET_MASK=" + Subnet_Mask.toString() + "\r\n");
    iniFile.close();
    }
  }

  updateFw();
  Serial.println("SSID: " + AP_SSID);
  Serial.println("Password: " + AP_PASS);
  Serial.print("\n");
  Serial.println("WEB Server IP: " + Server_IP.toString());
  Serial.println("Subnet: " + Subnet_Mask.toString());
  Serial.println("WEB Server Port: " + String(WEB_PORT));
  Serial.println("DNS Server IP: " + Server_IP.toString());
  Serial.println("DNS Server Port: " + String(DNS_PORT));
  Serial.print("\n\n");


  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(Server_IP, Server_IP, Subnet_Mask);
  WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str());
  Serial.println("WIFI AP started");

  dnsServer.setTTL(30);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "*", Server_IP);
  Serial.println("DNS server started");

  webServer.onNotFound(handleNotFound);
  webServer.begin(WEB_PORT);
  Serial.println("HTTP server started");
  }
  else
  {
     Serial.println("No SD card found");
     Serial.println("Starting Wifi AP without webserver");
     WiFi.mode(WIFI_AP);
     WiFi.softAPConfig(Server_IP, Server_IP, Subnet_Mask);
     WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str());
     Serial.println("WIFI AP started");

     dnsServer.setTTL(30);
     dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
     dnsServer.start(DNS_PORT, "*", Server_IP);
     Serial.println("DNS server started");
  }
  
  uartSerial.setTimeout(100);
  uartSerial.begin(115200);
  uartServer.begin();
  Serial.println("UART server started");
  
}



void loop(void) {
  dnsServer.processNextRequest();
  webServer.handleClient();
  handleUart();
}
