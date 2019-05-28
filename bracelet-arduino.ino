#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Nextion.h>
#include <NextionPage.h>
#include <NextionButton.h>
#include <NextionText.h>
#include <SoftwareSerial.h>

/*
 * ATTENTION ! SI VOUS ETES EN LIBRAIRIE ESP8266 2.5.2
 * IL Y A UNE ERREUR SUR LA FONCTION SoftwareSerial.available()
 * IL FAUT DONC MODIFIER SALEMENT LA LIBRAIRIE NEONEXTION
 * 
 * Fichier : C:\Users\YOU\Documents\Arduino\libraries\NeoNextion\Nextion.cpp
 * Ligne : 50
 * 
 * Remplacer par : `if (m_serialPort.available() + 1 >= 6)`
 */

SoftwareSerial nextionSerial(13, 14); // RX, TX

Nextion nex(nextionSerial);
NextionPage pgText(nex, 0, 0, "page0");
NextionPage pgText1(nex, 1, 0, "page1");
NextionButton buttonAlert(nex, 0, 2, "bAlert");
NextionButton buttonTaken(nex, 0, 3, "bTaken");
NextionButton buttonNotTaken(nex, 0, 4, "bNotTaken");
NextionText text(nex, 0, 6, "tText");
NextionText textName(nex, 1, 3, "tName");
NextionText textMed1(nex, 1, 4, "tMedication1");
NextionText textMed2(nex, 1, 5, "tMedication2");
NextionButton buttonProfile(nex, 1, 6, "bProfile");

const char* ssid = "Antoine";
const char* password = "azerty123";
const char* host = "bracelet.nguyenantoine.com";
const uint16_t port = 443;

void setup()
{
  Serial.begin(9600);
  delay(1000);

  // --- WIFI START ---
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  // --- WIFI END ---

  nextionSerial.begin(9600);
  nex.init();
  text.setText("Init OK");

  //  nex.sendCommand("baud=115200");
  //  nextionSerial.end();
  //  nextionSerial.begin(115200);

  // Register the pop event callback function of the components
  buttonAlert.attachCallback(&bAlertPopCallback);
  buttonTaken.attachCallback(&bTakenPopCallback);
  buttonNotTaken.attachCallback(&bNotTakenPopCallback);
  buttonProfile.attachCallback(&bProfilePopCallback);

  //  Serial.println(text.setBackgroundColour(NEX_COL_BLUE));
}

void loop() {
  nex.poll();
  //if(nextionSerial.available()) {Serial.print(nextionSerial.read(),HEX);Serial.print(" ");}
  static unsigned long start = millis();
  if (millis() > start + 1000)
  {
    start = millis();
    Serial.println("tick");
  }
}

bool getResponseByUrl(char* url, char* HTTP_method, bool settingText = false)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("wifi OK");
    BearSSL::WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http; //Object of class HTTPClient
    http.begin(client, host, port, url);

    int httpCode = 0;
    if (HTTP_method == "get") {
      httpCode = http.GET();
    } else if (HTTP_method == "post") {
      httpCode = http.POST("");
    }
    Serial.println(httpCode);

    if (httpCode > 0)
    {
      if (settingText) {
        String httpResponse = http.getString();
        JSONVar myObject = JSON.parse(httpResponse);
        //char charBuf[httpResponse.length() + 1];
        //httpResponse.toCharArray(charBuf, httpResponse.length());
        char myText[80];
        if (myObject.hasOwnProperty("first_name") && myObject.hasOwnProperty("last_name")) {
          sprintf(myText, "%s %s", (const char*)myObject["first_name"], (const char*)myObject["last_name"]);
          textName.setText(myText);
        }
        
        if (myObject.hasOwnProperty("medications")) {
          char myText1[100];
          
          sprintf(myText1, "Drug : %s, Number : %i, Schedule : %s, %s the meal", (const char*)myObject["medications"][0]["patent_medication"]["drug"]["name"], (int)myObject["medications"][0]["patent_medication"]["number"], (const char*)myObject["medications"][0]["patent_medication"]["schedule"], (const char*)myObject["medications"][0]["patent_medication"]["meal_period"]);
          
          Serial.println(myText1);
          textMed1.setText(myText1);
          
          char myText2[100];
          sprintf(myText2, "Drug : %s, Number : %i, Schedule : %s, %s the meal", (const char*)myObject["medications"][0]["patent_medication"]["drug"]["name"], (int)myObject["medications"][0]["patent_medication"]["number"], (const char*)myObject["medications"][0]["patent_medication"]["schedule"], (const char*)myObject["medications"][0]["patent_medication"]["meal_period"]);
          
          Serial.println(myText2);
          textMed2.setText(myText2);
        }
      }
    }
    http.end(); //Close connection

    return true;
  }
  else {
    Serial.println("Wifi NOT OK, Please Restart!");
    text.setText("Wifi NOT OK, Please Restart!");

    return false;
  }
}

void bProfilePopCallback(NextionEventType type, INextionTouchable * widget) {
  Serial.println("Profile");
  if (type == NEX_EVENT_POP)
  {
    getResponseByUrl("/api/v1/patent/1", "get", true);
  }
}

void bAlertPopCallback(NextionEventType type, INextionTouchable * widget) {
  Serial.println("Alert");
  if (type == NEX_EVENT_POP)
  {
    text.setText("2 comprimes \n ABILIFY MAINTENA 300 mg \n apres le repas.");
  }
}

void bTakenPopCallback(NextionEventType type, INextionTouchable * widget) {
  Serial.println("Taken");
  if (type == NEX_EVENT_POP)
  {
    if (getResponseByUrl("/api/v1/medication/1/taken/1", "post")) {
      text.setText("Medicament pris");
    }
  }
}

void bNotTakenPopCallback(NextionEventType type, INextionTouchable * widget) {
  Serial.println("Not Taken");
  if (type == NEX_EVENT_POP)
  {
    if (getResponseByUrl("/api/v1/medication/1/taken/0", "post")) {
      text.setText("Medicament oublie !!");
    }
  }
}
