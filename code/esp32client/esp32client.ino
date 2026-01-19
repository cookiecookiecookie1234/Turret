#include <WiFi.h>
#include <esp32cam.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>


static const char* WIFI_SSID = "wifi-n";
static const char* WIFI_PASS = "#patagonia";

WiFiUDP udp;

const int DISCOVERY_PORT = 4210;
const char* IDENTIFIER = "COOKIEMEOW";

IPAddress serverIP;
int serverPort = 0;




esp32cam::Resolution initialResolution;

String serverUrl = "http://192.168.0.119:5000/";

void connectWifiOrRestart(){
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi failure %d\n", WiFi.status());
    delay(5000);
    ESP.restart();
  }
  Serial.println("WiFi connected");
}

void cameraSetup(){
    
    using namespace esp32cam;

    initialResolution = Resolution::find(1024, 768);

    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(initialResolution);
    cfg.setJpeg(80);

    bool ok = Camera.begin(cfg);
    if (!ok) {
      Serial.println("camera initialize failure");
      delay(5000);
      ESP.restart();
    }
    Serial.println("camera initialize success");
  

  Serial.println("camera starting");

  Serial.print("http://");
  Serial.println(WiFi.localIP());
}


void udpBroadcastServerDiscovery(){
  bool serverFound = false;
  while(!serverFound){
    Serial.println("Sending UDP broadcast");

    udp.beginPacket("255.255.255.255", DISCOVERY_PORT);

    String msg = String(IDENTIFIER) + "/" + "Hello dear!";

    udp.write((uint8_t*)msg.c_str(), msg.length());
    udp.endPacket();

    delay(600);
    Serial.println("Checking for packets...");
    int packetSize = udp.parsePacket();
    if (packetSize) {
      char buf[64];
      int len = udp.read(buf, sizeof(buf) - 1);
      buf[len] = 0;

      const char* prefix = "TURRET_SERVER:";

      if (strncmp(buf, prefix, 13) == 0) {
        serverIP = udp.remoteIP();
        serverPort = atoi(buf + strlen(prefix));


        serverUrl = "http://" + serverIP.toString() + ":" + serverPort;

        Serial.println("-------------------------------------");
        Serial.print("Found server! IP: ");
        Serial.print(serverIP);
        Serial.print(", Port: ");
        Serial.println(serverPort);
        Serial.print("URL: ");
        Serial.println(serverUrl);
        Serial.print("Reply: ");
        Serial.println(buf);
        Serial.println("-------------------------------------");
      
        serverFound = true;
      }
    }

    
  }
}

void setup() {
  
  Serial.begin(115200);
  esp32cam::setLogger(Serial);

  connectWifiOrRestart();

  cameraSetup();

  udpBroadcastServerDiscovery();
}








void loop() {

  //captures camera
  auto frame = esp32cam::capture();
  if(!frame){
    Serial.println("capture error");
  }
  
  //sends over to uploadurl
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(client, serverUrl)) {
    Serial.println("HTTP upload error");
    delay(2000);
    return;
  }
  int code = http.POST(frame->data(), frame->size());
  if (code == 0){
    Serial.println("HTTP error:");
    Serial.println(http.errorToString(code).c_str());
  }else{
    Serial.print("Posted and received: ");
    Serial.println(http.getString());
  }

  http.end();
  delay(500);
}