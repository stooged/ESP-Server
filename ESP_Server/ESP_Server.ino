
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>


String AP_SSID = "PS4_WEB_AP";
String AP_PASS = "password";
int DNS_PORT = 53;
int WEB_PORT = 80;
IPAddress Server_IP(10,1,1,1);
IPAddress Subnet_Mask(255,255,255,0);

DNSServer dnsServer;
ESP8266WebServer webServer;


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




void setup(void) {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.print("\n");

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
  }
  
}

void loop(void) {
  dnsServer.processNextRequest();
  webServer.handleClient();
}
