#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

char SN[] = "85:39:73:23:23:03:51:91:B1:D0"; // Tools > Get Board Info
byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
//IPAddress ip(192, 168, 1, 200);
//IPAddress dns(8, 8, 8, 8);
//IPAddress gateway(192, 168, 1, 1);
//IPAddress subnet(255, 255, 255, 0);

IPAddress server(115,31,187,179);
char server_username[] = "seen-digital";
char server_password[] = "Seen_2021";
byte server_ping[] = { 64, 233, 187, 99 }; // Google
int Pins[] = { 2, 7, 4, 6, 5, 3 }; // an array of pin numbers to which LEDs are attached
int pinCount = 6; // the number of pins (i.e. the length of the array)

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;
boolean reconnect() {
  if (client.connect(SN, server_username, server_password)) {
    // Once connected, publish an announcement...
    client.publish("connect", SN);
    // ... and resubscribe
    client.subscribe(SN);
    client.subscribe("status");
  }
  return client.connected();
}
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  client.setServer(server, 1883);
  client.setCallback(callback);
  // initialize the Ethernet device
  //Ethernet.begin(mac, ip, dns, gateway, subnet);
  Ethernet.begin(mac);
  Serial.print("[ip]: ");
  Serial.println(Ethernet.localIP());
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  auto link = Ethernet.linkStatus();
  Serial.print("Link status: ");
  switch (link) {
    case Unknown:
      Serial.println("Unknown");
      break;
    case LinkON:
      Serial.println("Ethernet cable is connected.");
      break;
    case LinkOFF:
      Serial.println("Ethernet cable is not connected.");
      break;
  }
  Serial.println("internet connecting");
  if (ethClient.connect(server_ping, 80)) {
    Serial.println("connected");
  } else {
    Serial.println("connection failed");
  }
  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    pinMode(Pins[thisPin], OUTPUT);
  }
  delay(1000);
  lastReconnectAttempt = 0;
}

void loop() {
  if (!client.connected()) {
    if(lastReconnectAttempt == 0){
      Serial.print("mqtt broket connect ");
    }
    Serial.print(".");
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(1500);
  } else {
    // Client connected
    delay(1000);
    client.loop();
  }
}
// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  // Allocate the correct amount of memory for the payload copy
  if (String(topic) == String(SN)) {
    //char msg[length - 3];
    //for (int i = 0; i < length; i++) {
    //  msg[i] = (char)payload[i];
    //}
    //Serial.println(msg);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    const int _pin = doc["pin"];
    const int _digital = doc["digital"];
    const int _delay = doc["delay"];
    Serial.println(_pin);
    Serial.println(_digital);
    Serial.println(_delay);
    digitalWrite(_pin,_digital);
    if(_delay > 0){
      delay(_delay);
      digitalWrite(_pin,(!_digital));
    }
  }
  if (String(topic) == "status") {
    client.publish("heartbeat", SN);
  }
}
