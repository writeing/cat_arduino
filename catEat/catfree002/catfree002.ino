/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */
#include <WiFi.h>
#include <HTTPUpdate.h>
#include "PubSubClient.h"              //默认，加载MQTT库文件

/******需要修改的地方****************/

#define wifi_name       "wxc"     //WIFI名称，区分大小写，不要写错
#define wifi_password   "853976858"   //WIFI密码
                                     //固件链接，在巴法云控制台复制、粘贴到这里即可
                                     
String upUrl = "http://bin.bemfa.com/b/1BcNDJlZWMzYjYyYzg1NDlmNmFjNDQxOGIzYTE1NjA3Nzc=catfree002.bin";
#define ID_MQTT  "42eec3b62c8549f6ac4418b3a1560777"     //用户私钥，控制台获取
const char*  topic = "catfree002";        //主题名字，可在巴法云控制台自行创建，名称随意
/**********************************/
const char* mqtt_server = "bemfa.com"; //默认，MQTT服务器
const int mqtt_server_port = 9501;      //默认，MQTT服务器
WiFiClient espClient;
PubSubClient client(espClient);

const int motorPin = 10;
const int LimtePin = 22;
const int HOTPin = 14;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
int fullCount = 0;
int openTime = 0;
int controlStatus = 0;
int rpy;
int limteRpy;
WiFiClient logClient;
int proOpenTime = 0;
/***************************************************/




void ARDUINO_ISR_ATTR onTimer(){
  if(client.connected())
  {
    static int status = HIGH;
    digitalWrite(2, status);
    status = 1 - status;
  }
  else
  {
    digitalWrite(2, 1);
  }
}
void initHWTimer()
{
    // Use 1st timer of 4 (counted from zero).
    // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
    // info).
    timer = timerBegin(0, 80, true);

    // Attach onTimer function to our timer.
    timerAttachInterrupt(timer, &onTimer, true);

    // Set alarm to call onTimer function every second (value in microseconds).
    // Repeat the alarm (third parameter)
    timerAlarmWrite(timer, 500000, true);
}
void esp_print(String data)
{
  Serial.println(data);
  if(logClient.connected())
    logClient.println(data);
}
void esp_print(int data)
{
  Serial.println(data);
  if(logClient.connected())
    logClient.println(data);
}

void setup()
{
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.begin(115200);                     //波特率115200
    WiFi.begin(wifi_name, wifi_password);     //连接wifi
    while (WiFi.status() != WL_CONNECTED) {   //等待连接wifi
      delay(500);
      Serial.print(".");
    }
    setup_wifi();           //设置wifi的函数，连接wifi
    client.setServer(mqtt_server, mqtt_server_port);//设置mqtt服务器
    client.setCallback(callback); //mqtt消息处理
    logClient.connect("192.168.10.18",888);

    updateBin();                              //开始升级

    initHWTimer();
    pinMode(2, OUTPUT);
    pinMode(HOTPin, INPUT);
    // pinMode(LimtePin, INPUT);
    digitalWrite(2, LOW);
    pinMode(motorPin, OUTPUT);
    digitalWrite(motorPin, LOW);
        // Start an alarm
    timerAlarmEnable(timer);
    pinMode(LimtePin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LimtePin), LimteCallBack, CHANGE);
}
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_name);


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Topic:");
  Serial.println(topic);
  String msg = "";
  String msgValue = "";
  int flag = 0;
  for (int i = 0; i < length; i++) {
    if((char)payload[i] == '#')  
      {
        flag = 1;
        continue;
      }  
    if(flag == 1)
    msgValue += (char)payload[i];
    else     
    msg += (char)payload[i];
     
  }
  esp_print("Msg:");
  esp_print(msg+"#"+msgValue);
  if (msg == "on") {//如果接收字符on，亮灯
      turnOnLed(msgValue.toInt());//开灯函数      
    // Serial.println("open the light");    
  } else if (msg == "off") {//如果接收字符off，亮灯
    turnOffLed();//关灯函数
    // Serial.println("close the light");    
  }
  msg = "";
}
int recount = 0;
void reconnect() {
  // Loop until we're reconnected
  recount = 0;
  while (!client.connected() && recount < 5) {
    esp_print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID_MQTT)) {
      esp_print("connected");
      esp_print("subscribe:");
      esp_print(topic);
      //订阅主题，如果需要订阅多个主题，可发送多条订阅指令client.subscribe(topic2);client.subscribe(topic3);
      client.subscribe(topic);
    } else {
      esp_print("failed, rc=");
      esp_print(client.state());
     esp_print(" try again in 5 seconds");
      // Wait 5 seconds before retrying
     delay(5000);
    }
    recount++;
  }
  recount = 0;
  while (!logClient.connected() && recount < 1) {
      logClient.connect("192.168.10.18",888);
      esp_print("failed reconnect");
      esp_print(" try again in 5 seconds");
      // Wait 5 seconds before retrying
     delay(5000);
     recount++;
  }
}

void LimteCallBack() {
 limteRpy = digitalRead(LimtePin);
}

static int connectLoop = 0;
void loop()
{  
    if(proOpenTime > millis())
      proOpenTime = millis();
    if(millis() - proOpenTime > 1000*60*60*6)
    {
      esp_print("timeout,free 6h");
      proOpenTime = millis();
      fullCount = 3;
      controlStatus = 1;   
    }
    // Read all the lines of the reply from server and print them to Serial
    if((!client.connected() || !logClient.connected()) && millis() - connectLoop > 1000*10)
    {
      connectLoop = millis();
      reconnect();
    }
    client.loop();    
    switch(controlStatus)
    {
      case 0:
        // rpy = digitalRead(HOTPin);  
        // if(openTime != 0)
        // {
        //   controlStatus = 5;          
        // } 
        // else if(rpy == HIGH)
        // {
        //   openTime = millis(); 
        //   controlStatus = 4;   
        // }        
        break;
      case 1:
        digitalWrite(motorPin, HIGH);  
        controlStatus = 2;    
        openTime = millis();         
        break;
      case 2:    
        if(millis() - openTime > fullCount*1000)
        {
          controlStatus = 3;       
        }
        break;
      case 3:
        digitalWrite(motorPin, LOW);
        controlStatus = 0;   
      break;
      case 4:
      break;
      case 5:
      break;
    }
    delay(100);    
}

//当升级开始时，打印日志
void update_started() {
  esp_print("CALLBACK:  HTTP update process started");
}

//当升级结束时，打印日志
void update_finished() {
  esp_print("CALLBACK:  HTTP update process finished");
}

//当升级中，打印日志
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

//当升级失败时，打印日志
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

/**
 * 固件升级函数
 * 在需要升级的地方，加上这个函数即可，例如setup中加的updateBin(); 
 * 原理：通过http请求获取远程固件，实现升级
 */
void updateBin(){
  Serial.println("start update");    
  WiFiClient UpdateClient;
  
  //如果是旧版esp32 SDK，需要删除下面四行，旧版不支持，不然会报错
  httpUpdate.onStart(update_started);//当升级开始时
  httpUpdate.onEnd(update_finished);//当升级结束时
  httpUpdate.onProgress(update_progress);//当升级中
  httpUpdate.onError(update_error);//当升级失败时
  
  t_httpUpdate_return ret = httpUpdate.update(UpdateClient, upUrl);
  switch(ret) {
    case HTTP_UPDATE_FAILED:      //当升级失败
        esp_print("[update] Update failed.");
        break;
    case HTTP_UPDATE_NO_UPDATES:  //当无升级
        esp_print("[update] Update no Update.");
        break;
    case HTTP_UPDATE_OK:         //当升级成功
        esp_print("[update] Update ok.");
        break;
  }
}
void(* resetFunc) (void) = 0;

//打开灯泡
void turnOnLed(int value) {
  Serial.println("turn on free");
  if(value / 10 == 0)
  {
    fullCount = 1;
    controlStatus = 1;     
  }
  else 
  {
    value /= 10;  
    fullCount = value ;
    controlStatus = 1;
  }
  proOpenTime = millis();
}
//关闭灯泡
void turnOffLed() {
  Serial.println("turn off free");
  resetFunc();
  controlStatus = 0;
  
}