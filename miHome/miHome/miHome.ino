#include <WiFi.h>
#include <HTTPUpdate.h>
#include "PubSubClient.h"              //默认，加载MQTT库文件


/******需要修改的地方****************/

#define wifi_name       "wxc"     //WIFI名称，区分大小写，不要写错
#define wifi_password   "853976858"   //WIFI密码
                                     //固件链接，在巴法云控制台复制、粘贴到这里即可
String upUrl = "http://bin.bemfa.com/b/1BcNDJlZWMzYjYyYzg1NDlmNmFjNDQxOGIzYTE1NjA3Nzc=lightEsp32002.bin";

#define ID_MQTT  "42eec3b62c8549f6ac4418b3a1560777"     //用户私钥，控制台获取
const char*  topic = "lightEsp32002";        //主题名字，可在巴法云控制台自行创建，名称随意
// const int B_led = D2;       //单片机LED引脚值，D系列是NodeMcu引脚命名方式，其他esp8266型号将D2改为自己的引脚
/**********************************/
const char* mqtt_server = "bemfa.com"; //默认，MQTT服务器
const int mqtt_server_port = 9501;      //默认，MQTT服务器
WiFiClient espClient;
WiFiClient logClient;
PubSubClient client(espClient);
const int B_led = 13;
const int R_led = 12;
const int G_led = 14;
/**
 * 主函数
 */
void setup() {
  pinMode(B_led, OUTPUT); //设置引脚为输出模式
  digitalWrite(B_led, LOW);//默认引脚上电高电平
  pinMode(R_led, OUTPUT); //设置引脚为输出模式
  digitalWrite(R_led, LOW);//默认引脚上电高电平  
  pinMode(G_led, OUTPUT); //设置引脚为输出模式
  digitalWrite(G_led, LOW);//默认引脚上电高电平

  Serial.begin(115200);                     //波特率115200
  WiFi.begin(wifi_name, wifi_password);     //连接wifi
  while (WiFi.status() != WL_CONNECTED) {   //等待连接wifi
    delay(500);
    Serial.print(".");
  }
  
  setup_wifi();           //设置wifi的函数，连接wifi
  client.setServer(mqtt_server, mqtt_server_port);//设置mqtt服务器
  client.setCallback(callback); //mqtt消息处理
  logClient.connect("192.168.10.18",777);
  updateBin();                              //开始升级
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
void setup_wifi() {
  delay(10);
  esp_print("Connecting to ");
  esp_print(wifi_name);


  esp_print("");
  esp_print("WiFi connected");
  esp_print("IP address: ");
  esp_print(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  esp_print("Topic:");
  esp_print(topic);
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
      esp_print("failed reconnect");
      esp_print(" try again in 5 seconds");
      // Wait 5 seconds before retrying
     delay(5000);
     recount++;
  }
}
/**
 * 循环函数
 */
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
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

//打开灯泡
void turnOnLed(int value) {
  turnOffLed();
  if(value > 80)
  digitalWrite(B_led, HIGH);
  else if(value > 40)
  digitalWrite(R_led, HIGH);
  else if(value >= 0)
  digitalWrite(G_led, HIGH);
  esp_print("turn on light");
}
//关闭灯泡
void turnOffLed() {
  esp_print("turn off light");
  digitalWrite(B_led, LOW);
  digitalWrite(R_led, LOW);
  digitalWrite(G_led, LOW);
}