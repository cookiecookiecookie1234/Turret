#include <WiFi.h>
#include <esp32cam.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <Esp32Bridge.h>

#define TX_PIN 14
#define RX_PIN 15
#define LED_BUILTIN 33

static const char* WIFI_SSID = "wifi-n";
static const char* WIFI_PASS = "#patagonia";

WiFiClient client;
HTTPClient http;

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
    
    {
    using namespace esp32cam;

    initialResolution = Resolution::find(768,576);

    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(initialResolution);
    cfg.setJpeg(65);

    bool ok = Camera.begin(cfg);
    if (!ok) {
      Serial.println("camera initialize failure");
      delay(5000);
      ESP.restart();
    }
    Serial.println("camera initialize success");
  }
  

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

    for(int n = 0; n < 3; n++){
      digitalWrite(4, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      delay(100);
    }

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
  pinMode(LED_BUILTIN, OUTPUT);

  setupBridge(TX_PIN, RX_PIN, 5);
  
  Serial.begin(115200);
  esp32cam::setLogger(Serial);

  connectWifiOrRestart();

  cameraSetup();

  udpBroadcastServerDiscovery();

  http.setReuse(true); 
  http.setTimeout(15000);
}


void sendData(String packet) {
  auto packetC = packet.c_str();

  Serial.print("Parsing ");
  Serial.print(packet);
  Serial.print(":");

  int Ch = 0;

  for (int i = 0; i < packet.length(); i++){
    if (packetC[i] == "e"[0]){
      Serial.print(") Ch ");
      Serial.print(Ch);
      Serial.print(": ()");    
      int value = atoi(packetC + i + 1);
      Serial.print(value);
      setPacketData(value + 128, Ch);
      Ch++;
    }
  }

  Serial.println();
}


void loop() {
  auto frame = esp32cam::capture();
  if(!frame){
    Serial.println("capture error");
  }

  if (!http.begin(client, serverUrl)) {
    Serial.println("HTTP upload error");
    delay(2000);
    return;
  }
  int code = http.POST(frame->data(), frame->size());
  if (code == 0){
    Serial.println("HTTP error:");
    Serial.println(http.errorToString(code).c_str());

      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);

  }else{
    String value = http.getString();
    Serial.println(value);

      digitalWrite(LED_BUILTIN, HIGH);
      delay(10);
      digitalWrite(LED_BUILTIN, LOW);


    sendData(value);
  }
  http.end();
  
  tickBridge();

  delay(40);
}




