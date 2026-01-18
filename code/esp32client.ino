#include <WiFi.h>
#include <esp32cam.h>
#include <HTTPClient.h>


static const char* WIFI_SSID = "CLARO_4B0DF9-IoT";
static const char* WIFI_PASS = "@Cookie2025setembro";


esp32cam::Resolution initialResolution;

String uploadurl = "http://192.168.0.119:5000/";

void setup() {
  Serial.begin(115200);
  Serial.println();
  esp32cam::setLogger(Serial);
  delay(1000);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi failure %d\n", WiFi.status());
    delay(5000);
    ESP.restart();
  }
  Serial.println("WiFi connected");
  delay(1000);

  {
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
  }

  Serial.println("camera starting");

  Serial.print("http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  auto frame = esp32cam::capture();
  if(!frame){
    Serial.println("capture error");
  }
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(client, uploadurl)) {
    Serial.println("HTTP upload error");
    delay(2000);
    return;
  }
  int code = http.POST(frame->data(), frame->size());
  if (code == 0){
    Serial.println("HTTP error:");
    Serial.println(http.errorToString(code).c_str());
  }else{
    Serial.println(http.getString());
  }
  http.end();
  delay(500);




}