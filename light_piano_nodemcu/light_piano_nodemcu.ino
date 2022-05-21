#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK  "thereisnospoon"
#endif

#define MAX_FILES_COUNT 100
#define MAX_FILENAME_LENGTH 15

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

SoftwareSerial espSerial(D6, D7);

AsyncWebServer server(80);

char files[MAX_FILES_COUNT][MAX_FILENAME_LENGTH];
int file_count = 0;

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/

char *get_index_page() {
  char *result = (char *)malloc(1000);
  strcpy(result, "<iframe name=\"dummyframe\" id=\"dummyframe\" style=\"display: none;\"></iframe>");
  strcat(result, "<form action=\"play\" method=\"POST\" target=\"dummyframe\">");
  strcat(result, "<select name=\"songs\" id=\"songs\">");

  for (int i = 0; i < file_count; i++) {
    char aux[100];
    if (strstr(files[i], "TRASH")) continue;
    sprintf(aux, "<option value=\"%s\">%s</option>", files[i], files[i]);
    strcat(result, aux);
  }
  strcat(result, "</option>");
  strcat(result, "<input type=\"submit\" value=\"Play!\">");
  strcat(result, "</form>");
  return result;
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  espSerial.begin(9600);
  Serial.println();
  Serial.print("Configuring wifi point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.mode(WIFI_STA);
  WiFi.begin("AndroidAP49C7", "cxgx6019");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    char *index = get_index_page();
    request->send(200, "text/html", index);
    free(index);
  });

  server.on("/play", HTTP_POST, [](AsyncWebServerRequest *request) {
    for (char c : request->getParam(0)->value()) {
      espSerial.write(c);  
    }
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  while(espSerial.available()) {
    espSerial.readBytesUntil('\n', files[file_count++], MAX_FILENAME_LENGTH);
    char *newline = strchr(files[file_count - 1], '\n');
    if (newline) *newline = '\0';
    Serial.println(files[file_count - 1]);
  }
}
