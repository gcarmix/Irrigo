
#include "Arduino.h"
#include <EEPROM.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include <PubSubClient.h>
#include "time.h"
#include <Ticker.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include "files.h"
#define EEPROM_SIZE 512
#define MAX_TIME 3600

#define U_PART U_SPIFFS
int mqtt_status = -1;
int channels[4] = {0,0,0,0};
int ontime[4] = {0,0,0,0};
void control();
AsyncWebServer server(80);
WiFiMulti wiFiMulti;
#define FW_VERSION "1.0.0"
bool updateRequested = false;
typedef struct CONFIG_S{
  char essid[32] = "";
  char password[32] = "";
  char mqtt_broker[32] = "";
  char mqtt_client_name[32] = "irrigo_client";
  char mqtt_tele_topic[32] = "tele/irrigation";
  char mqtt_cmnd_topic[32] = "cmnd/irrigation";
  char mqtt_user[32] = "";
  char mqtt_password[32] = "";
  uint16_t mqtt_port = 1883;
} CONFIG_S;

CONFIG_S Config;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
int last_hour = -1;
long last_conn_check = 0;
size_t content_len;
#define RELE1 22 
#define RELE2 19 
#define RELE3 21
#define RELE4 18
#define DHTPIN 23
#define DHTTYPE DHT11   // DHT 11
//DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient mqttClient(espClient);



int count = 0;
int mytime = 0;
int mqttTime = 0;

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void handleUpdate(AsyncWebServerRequest *request) {
  char* html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index){
    Serial.println("Update");
    content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
#ifdef ESP8266
    Update.runAsync(true);
    if (!Update.begin(content_len, cmd)) {
#else
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#endif
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
#ifdef ESP8266
  } else {
    Serial.printf("Progress: %d%%\n", (Update.progress()*100)/Update.size());
#endif
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      Serial.println("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
}

void printProgress(size_t prg, size_t sz) {
  Serial.printf("Progress: %d%%\n", (prg*100)/content_len);
}

int mqttReconnect()
{
  if(!mqttClient.connected()){
    if(mqttClient.connect(Config.mqtt_client_name,Config.mqtt_user,Config.mqtt_password)){
      Serial.println("connected");
      char msg[64];
      sprintf(msg,"%s/+",Config.mqtt_cmnd_topic);
      mqttClient.subscribe(msg);
      return 0;
    }
    else
    {
      Serial.println("Error connecting to MQTT");
      delay(5000);
      return -1;
    }
  }
  return 0;
}

void mqttCallback(char* topic,uint8_t* payload,unsigned int length){
  char test_topic[64];
  int i=0;
  Serial.println("MSG:\r\n");
  Serial.println(topic);
  payload[length] = 0;
  Serial.println((char*)payload);
  
  for (i=0;i<4;i++)
  {
    sprintf(test_topic,"%s/%d",Config.mqtt_cmnd_topic,i+1);
    if(!strcmp(topic,test_topic))
    {
      sprintf(test_topic,"%s/%d",Config.mqtt_tele_topic,i+1);
      if(!strcmp((char*)payload,"ON"))
      {
        channels[i] = 1;
        mqttClient.publish(test_topic,"ON");
      }
      else
      {
        channels[i] = 0;
        mqttClient.publish(test_topic,"OFF");
      }
      control();

      break;
    }

  }



}

String mqttProcessor(String var)
{
  if(var == "MQTT_BROKER")
  {
    return String(Config.mqtt_broker);
  }
  if(var == "MQTT_PORT")
  {
    return String(Config.mqtt_port);
  }
  if(var == "MQTT_CLIENT_NAME")
  {
    return String(Config.mqtt_client_name);
  }
  if(var == "MQTT_USER")
  {
    return String(Config.mqtt_user);
  }
  if(var == "MQTT_PASSWORD")
  {
    return String(Config.mqtt_password);
  }
  if(var == "MQTT_TELE_TOPIC")
  {
    return String(Config.mqtt_tele_topic);
  }
  if(var == "MQTT_CMND_TOPIC")
  {
    return String(Config.mqtt_cmnd_topic);
  }
  if(var == "MQTT_CONNECTED")
  {
    if(mqttClient.connected())
    {
      return String("Connected");
    }
    else{
      return String("Disconnected");
    }
  }

  return String();
}

String netProcessor(String var)
{
  if(var == "WIFI_SSID")
  {
    return String(Config.essid);
  }
  if(var == "WIFI_PASSWD")
  {
    return String(Config.password);
  }
  return String();
}

String indexProcessor(String var)
{
  if(var == "CHAN1_STATE")
  {
    if(channels[0])
      return String("checked");
    else
      return String("");
  }
  if(var == "CHAN2_STATE")
  {
    if(channels[1])
      return String("checked");
    else
      return String("");
  }
  if(var == "CHAN3_STATE")
  {
    if(channels[2])
      return String("checked");
    else
      return String("");
  }
  if(var == "CHAN4_STATE")
  {
    if(channels[3])
      return String("checked");
    else
      return String("");
  }

  return String();
}

String updateProcessor(String var)
{
  if(var == "FW_VER")
  {
    return String(FW_VERSION);
  }
  
  if(var == "REMOTE_FW_VER")
  {
    return String("0.0");
  }

  
  return "";
}

void loadConfig(){
  uint8_t * configptr = (uint8_t *)&Config;
  for(int i=0;i<sizeof(CONFIG_S);i++)
  {
    configptr[i]=EEPROM.read(i);
  }
}
void saveConfig(){
  Serial.print("Saving config...");
  uint8_t *configptr = (uint8_t *)&Config;
  for(int i = 0; i < sizeof(CONFIG_S); i++)
  {
       EEPROM.write(i, configptr[i] );
  }
  EEPROM.write(511, 0x55);
  EEPROM.commit();
  Serial.println("done.");
  delay(500);
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}


void control()
{
  int i;
    for(i=0;i<4;i++)
    {
      if(channels[i]>0)
      {
        ontime[i]+=10;
      }
      else
      {
        ontime[i]=0;
      }

      if(ontime[i]> MAX_TIME)
      {
        ontime[i] = 0;
        channels[i] = 0;
      }
    }
    digitalWrite(RELE1,!channels[0]);
    digitalWrite(RELE2,!channels[1]);
    digitalWrite(RELE3,!channels[2]);
    digitalWrite(RELE4,!channels[3]);

}



void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
Serial.printf("\r\nIrrigo v %s\n",FW_VERSION);
if(EEPROM.read(511) != 0x55)
  {
    Serial.println("No config found, loading default.");
      saveConfig();
  }
  else
  {
    loadConfig();
  }
  pinMode(RELE1,OUTPUT);
  pinMode(RELE2,OUTPUT);
  pinMode(RELE3,OUTPUT);
  pinMode(RELE4,OUTPUT);
  count = 0;
  mytime = millis();
  Serial.println("Inizio");
  digitalWrite(RELE1,1);
  digitalWrite(RELE2,1);
  digitalWrite(RELE3,1);
  digitalWrite(RELE4,1);


if(strlen(Config.essid) == 0 )
  {
    Serial.println("Starting WiFi AP with name Irrigo");
    WiFi.softAP("Irrigo");
  }
  else
  {
    Serial.println("Starting as WiFi Client");
    WiFi.mode(WIFI_STA);
    WiFi.hostname("Irrigo");
    wiFiMulti.addAP(Config.essid, Config.password);
    //WiFi.begin("TP-LINK_1EF8","supercalifragilisti12345");
    // Wait for connection
    while (wiFiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    if(!MDNS.begin("Irrigo"))
    {
      Serial.println("Error setting up MDNS responder");
    }
    else
      MDNS.addService("http", "tcp", 80);
  }


// Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);




 // put your setup code here, to run once:
  // Route for root / web page
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(LittleFS, "/style.css", String());
    const char* dataType = "text/css";
    request->send_P(200,dataType, style_css,style_css_len,NULL);
  });
  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(LittleFS, "/jquery.min.js", String());
    const char* dataType = "application/javascript";
    request->send_P(200,dataType, jquery_min_js,jquery_min_js_len);

  });

     server.on("/action.js", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(LittleFS, "/action.js", String());
    const char* dataType = "application/javascript";
    request->send_P(200,dataType,action_js,action_js_len,NULL);
  });

  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    
    //request->send(LittleFS, "/index.html", String());
    const char* dataType = "text/html";
    request->send_P(200,dataType, index_html,index_html_len,indexProcessor);
  });
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(LittleFS, "/wifi.html", String(),false,netProcessor);
    const char* dataType = "text/html";
    request->send_P(200,dataType, wifi_html,wifi_html_len,netProcessor);
  });
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    const char* dataType = "text/html";
    AsyncWebParameter * chan = request->getParam(0);
    AsyncWebParameter * val = request->getParam(1);
    Serial.println(chan->value().substring(0,3));
    int i;
    char msg[64];
    if(chan->value().substring(0,4) == "chan")
    {
      Serial.println("CHAN toggle request");
      for(i=0;i<4;i++)
      {
        if(chan->value().charAt(4) == ('1'+i))
        {
          sprintf(msg,"%s/%d",Config.mqtt_tele_topic,i+1);
          if(val->value() == "true")
                {
                  channels[i] = 1;
                  mqttClient.publish(msg,"ON");
                }
                else
                {
                  channels[i] = 0;
                  mqttClient.publish(msg,"OFF");
                }
        }

      }
      
    control();
    }
    request->send_P(200,dataType, wifi_html,wifi_html_len,netProcessor);
  });
/*handling uploading firmware file */
server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(LittleFS, "/update.html", String(),false,updateProcessor);
    const char* dataType = "text/html";
    request->send_P(200,dataType, update_html,update_html_len,updateProcessor);
  });
  
  server.on("/doUpdate", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                  size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
  );


  server.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send(LittleFS, "/mqtt.html", String(),false,mqttProcessor);
    const char* dataType = "text/html";
    request->send_P(200,dataType, mqtt_html,mqtt_html_len,mqttProcessor);
  });

    server.on("/set_mqtt", HTTP_POST, [](AsyncWebServerRequest *request){
    String broker;
    String mqtt_client_name;
    String mqtt_user;
    String mqtt_password;
    String mqtt_cmnd_topic;
    String mqtt_tele_topic;
    int port;
    int params = request->params();
    Serial.println(params);
    if(params < 2)
    {
      Serial.println("Error, not enough params");
    }
    else
    {
      for (int i=0;i<params;i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if(p->isPost())
      {
        if(p->name() == "broker_ip")
        {
          broker = p->value(); 
        }
        if(p->name() == "broker_port")
        {
          port = p->value().toInt(); 
        }
        if(p->name() == "mqtt_client_name")
        {
          mqtt_client_name = p->value(); 
        }
        if(p->name() == "mqtt_user")
        {
          mqtt_user = p->value(); 
        }
        if(p->name() == "mqtt_password")
        {
          mqtt_password = p->value(); 
        }
        if(p->name() == "mqtt_tele_topic")
        {
          mqtt_tele_topic = p->value(); 
        }
        if(p->name() == "mqtt_cmnd_topic")
        {
          mqtt_cmnd_topic = p->value(); 
        }
        
        
      }
    }

      Serial.println(broker);
      Serial.println(port);
      strncpy(Config.mqtt_broker,broker.c_str(),32);
      Config.mqtt_port = port;
      strncpy(Config.mqtt_client_name,mqtt_client_name.c_str(),32);
      strncpy(Config.mqtt_user,mqtt_user.c_str(),32);
      strncpy(Config.mqtt_password,mqtt_password.c_str(),32);
      strncpy(Config.mqtt_tele_topic,mqtt_tele_topic.c_str(),32);
      strncpy(Config.mqtt_cmnd_topic,mqtt_cmnd_topic.c_str(),32);

      mqttClient.setServer(Config.mqtt_broker,Config.mqtt_port);
      mqttClient.setCallback(mqttCallback);
      mqttReconnect();
      saveConfig();

    }
    const char* dataType = "text/html";
    request->send_P(200,dataType, mqtt_html,mqtt_html_len,mqttProcessor);
    
    //request->send(LittleFS, "/index.html", String());
  });



  server.on("/set_wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    String ssid;
    String password;
    int params = request->params();
    Serial.println(params);
    if(params != 3)
    {
      Serial.println("Error, not enough params");
    }
    else
    {
      for (int i=0;i<params;i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if(p->isPost())
      {
        if(p->name() == "wifi_ssid")
        {
          ssid = p->value(); 
        }
        if(p->name() == "wifi_pwd")
        {
          password = p->value(); 
        }
      }
    }

      Serial.println(ssid);
      Serial.println(password);
      strncpy(Config.essid,ssid.c_str(),32);
      strncpy(Config.password,password.c_str(),32);
      saveConfig();

    }
    const char* dataType = "text/html";
    request->send_P(200,dataType, wifi_html,wifi_html_len,netProcessor);
  });

  server.on("/start_update",HTTP_GET,[](AsyncWebServerRequest *request){
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      updateRequested = true;
      int ret = 0;
      DynamicJsonDocument json(4096);
      if (ret == 0)
      {
        json["status"] = "Done";
      }
      else
      {
        json["status"] = "Error";
      }
      
      
      serializeJson(json, *response);
    request->send(response);
  });



server.onNotFound(notFound);
 

 
  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(Config.mqtt_broker,Config.mqtt_port);
  mqttClient.setCallback(mqttCallback);


esp_task_wdt_init(10, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

}

void loop() {
 esp_task_wdt_reset();
  if(WiFi.status() == WL_CONNECTED)
  {

  if(!mqttClient.connected()){
    mqtt_status = mqttReconnect();
    
    
  }
  else
  {
      mqttClient.loop();
      mqtt_status = 0;

  }
  long now = millis();
 
  if(updateRequested == true)
  {
    ESP.restart();
    updateRequested = false;

  }


  
  if((now > mqttTime + 30000 )&& mqtt_status == 0 && WiFi.status()==WL_CONNECTED)
  {
    mqttTime = now;
    Serial.println("Publishing MQTT...");
    digitalWrite(BUILTIN_LED,1);
    //sprintf(msg,"{\"Channels\":[%d,%d,%d,%d]}",channels[0],channels[1],channels[2],channels[3]);
    int i;
    for(i=0;i<4;i++)
    {
      char topic[64];
      sprintf(topic,"%s/%d",Config.mqtt_tele_topic,i+1);
      if(channels[i])
        mqttClient.publish(topic,"ON");
      else
        mqttClient.publish(topic,"OFF");
    }
    
  }

  if(now > mytime + 10000)
  {
    control();

    mytime = now; 
 }
 
  }
  delay(1000);
 digitalWrite(BUILTIN_LED,0);
}