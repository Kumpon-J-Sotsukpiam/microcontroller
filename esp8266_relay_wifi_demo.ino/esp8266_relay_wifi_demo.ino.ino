/*
  Simple.ino, Example for the AutoConnect library.
  Copyright (c) 2018, Hieromon Ikasamo
  https://github.com/Hieromon/AutoConnect

  This software is released under the MIT License.
  https://opensource.org/licenses/MIT
*/

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <time.h>
#include <AutoConnect.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

char SN[] = "85:39:73:23:23:03:51:91:B1:D1"; // Tools > Get Board Info
//ESP.getChipId()
IPAddress server(115,31,187,179);
char server_username[] = "seen-digital";
char server_password[] = "Seen_2021";
int Pins[] = { 0 }; // an array of pin numbers to which LEDs are attached
int pinCount = 1; // the number of pins (i.e. the length of the array)
long lastReconnectAttempt = 0;

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

static const char AUX_TIMEZONE[] PROGMEM = R"(
{
  "title": "TimeZone",
  "uri": "/timezone",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "Sets the time zone to get the current local time.",
      "style": "font-family:Arial;font-weight:bold;text-align:center;margin-bottom:10px;color:DarkSlateBlue"
    },
    {
      "name": "timezone",
      "type": "ACSelect",
      "label": "Select TZ name",
      "option": [],
      "selected": 10
    },
    {
      "name": "newline",
      "type": "ACElement",
      "value": "<br>"
    },
    {
      "name": "start",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/start"
    }
  ]
}
)";

typedef struct {
  const char* zone;
  const char* ntpServer;
  int8_t      tzoff;
} Timezone_t;

static const Timezone_t TZ[] = {
  { "Europe/London", "europe.pool.ntp.org", 0 },
  { "Europe/Berlin", "europe.pool.ntp.org", 1 },
  { "Europe/Helsinki", "europe.pool.ntp.org", 2 },
  { "Europe/Moscow", "europe.pool.ntp.org", 3 },
  { "Asia/Dubai", "asia.pool.ntp.org", 4 },
  { "Asia/Karachi", "asia.pool.ntp.org", 5 },
  { "Asia/Dhaka", "asia.pool.ntp.org", 6 },
  { "Asia/Jakarta", "asia.pool.ntp.org", 7 },
  { "Asia/Manila", "asia.pool.ntp.org", 8 },
  { "Asia/Tokyo", "asia.pool.ntp.org", 9 },
  { "Australia/Brisbane", "oceania.pool.ntp.org", 10 },
  { "Pacific/Noumea", "oceania.pool.ntp.org", 11 },
  { "Pacific/Auckland", "oceania.pool.ntp.org", 12 },
  { "Atlantic/Azores", "europe.pool.ntp.org", -1 },
  { "America/Noronha", "south-america.pool.ntp.org", -2 },
  { "America/Araguaina", "south-america.pool.ntp.org", -3 },
  { "America/Blanc-Sablon", "north-america.pool.ntp.org", -4},
  { "America/New_York", "north-america.pool.ntp.org", -5 },
  { "America/Chicago", "north-america.pool.ntp.org", -6 },
  { "America/Denver", "north-america.pool.ntp.org", -7 },
  { "America/Los_Angeles", "north-america.pool.ntp.org", -8 },
  { "America/Anchorage", "north-america.pool.ntp.org", -9 },
  { "Pacific/Honolulu", "north-america.pool.ntp.org", -10 },
  { "Pacific/Samoa", "oceania.pool.ntp.org", -11 }
};

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer Server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer Server;
#endif

AutoConnect       Portal(Server);
AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4
AutoConnectAux    Timezone;

WiFiClient espClient;
PubSubClient client(espClient);
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
void rootPage() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<script type=\"text/javascript\">"
    "setTimeout(\"location.reload()\", 1000);"
    "</script>"
    "</head>"
    "<body>"
    "<h2 align=\"center\" style=\"color:blue;margin:20px;\">Hello, world</h2>"
    "<h3 align=\"center\" style=\"color:gray;margin:10px;\">{{DateTime}}</h3>"
    "<p style=\"text-align:center;\">Reload the page to update the time.</p>"
    "<p></p><p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";
  static const char *wd[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
  struct tm *tm;
  time_t  t;
  char    dateTime[26];
  t = time(NULL);
  tm = localtime(&t);
  sprintf(dateTime, "%04d/%02d/%02d(%s) %02d:%02d:%02d.",
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
    wd[tm->tm_wday],
    tm->tm_hour, tm->tm_min, tm->tm_sec);
  content.replace("{{DateTime}}", String(dateTime));
  Server.send(200, "text/html", content);
}

void startPage() {
  String  tz = Server.arg("timezone");

  for (uint8_t n = 0; n < sizeof(TZ) / sizeof(Timezone_t); n++) {
    String  tzName = String(TZ[n].zone);
    if (tz.equalsIgnoreCase(tzName)) {
      configTime(TZ[n].tzoff * 3600, 0, TZ[n].ntpServer);
      Serial.println("Time zone: " + tz);
      Serial.println("ntp server: " + String(TZ[n].ntpServer));
      break;
    }
  }
  Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/"));
  Server.send(302, "text/plain", "");
  Server.client().flush();
  Server.client().stop();
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  client.setServer(server, 1883);
  client.setCallback(callback);
  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    pinMode(Pins[thisPin], OUTPUT);
    digitalWrite(Pins[thisPin],(HIGH));
  }
  delay(1000);
  lastReconnectAttempt = 0;
  Serial.println();
  Config.autoReconnect = true;
  Portal.config(Config);
  Timezone.load(AUX_TIMEZONE);
  AutoConnectSelect&  tz = Timezone["timezone"].as<AutoConnectSelect>();
  for (uint8_t n = 0; n < sizeof(TZ) / sizeof(Timezone_t); n++) {
    tz.add(String(TZ[n].zone));
  }
  Portal.join({ Timezone });
  Server.on("/", rootPage);
  Server.on("/start", startPage);   // Set NTP server trigger handler
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
}

void loop() {
  Portal.handleClient();
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
    digitalWrite(_pin,!_digital);
    if(_delay > 0){
      delay(_delay);
      digitalWrite(_pin,(_digital));
    }
  }
  if (String(topic) == "status") {
    client.publish("heartbeat", SN);
  }
}
