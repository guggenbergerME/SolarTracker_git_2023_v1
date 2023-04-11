#include <Arduino.h>
#include <ArduinoOTA.h>
#include "time.h"
#include "WiFi.h"

/////////////////////////////////////////////////////////////////////////// Pin output zuweisen
#define M1_re 2  // D2
#define M1_li 4  // D4
#define M2_re 5  // D5
#define M2_li 18  // D18

/////////////////////////////////////////////////////////////////////////// Interrup
int sturmschutzschalterpin =  13;
int panelsenkrechtpin =  12;


/////////////////////////////////////////////////////////////////////////// ADC zuweisen
const int adc_NO = 34; //ADC1_6 - Fotowiderstand 
const int adc_NW = 35; //ADC1_7 - Fotowiderstand 
const int adc_SO = 33; //ADC1_8 - Fotowiderstand
const int adc_SW = 32; //ADC1_9 - Fotowiderstand 

int sensorSonne_NO, sensorSonne_NW, sensorSonne_SO, sensorSonne_SW;
int horizontal_hoch, horizontal_runter, vertikal_rechts, vertikal_links; 
int differenz_neigen, differenz_drehen, sonne_quersumme, neigen_fahrt;
int traker_tolleranz = 45; // Getestet mit 300
int helligkeit_schwellwert = 200; // Wolkenschwellwert

/////////////////////////////////////////////////////////////////////////// Windsensor Variablen
int wind_zu_stark = 0;
int sturmschutz_pause = 50000;

/////////////////////////////////////////////////////////////////////////// NTP Daten
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

/////////////////////////////////////////////////////////////////////////// Schleifen verwalten
unsigned long previousMillis_Sturmcheck = 0; // Windstärke prüfen
unsigned long interval_Sturmcheck = 50000; 

unsigned long previousMillis_sonnensensor = 0; // Sonnenstand prüfen
unsigned long interval_sonnensensor = 1100; 

unsigned long previousMillis_sturmschutzschalter = 0; // Sturmschutz Schalter prüfen
unsigned long interval_sturmschutzschalter = 1200; 

unsigned long previousMillis_panelsenkrecht = 0; // Sturmschutz Schalter prüfen
unsigned long interval_panelsenkrecht = 1300; 

/////////////////////////////////////////////////////////////////////////// Funktionsprototypen
void loop                       ();
void m1                         (int); // Panel neigen
void m2                         (int); // Panel drehen
void sturmschutz                ();
void panel_senkrecht            ();
void sonnenaufgang              ();
void sonnensensor               ();
void wifi_setup                 ();
void ArduinoOTAsetup            ();
void LokaleZeit                 ();
void sturmschutzschalter        ();


/////////////////////////////////////////////////////////////////////////// SETUP - Wifi
void wifi_setup() {

// WiFi Zugangsdaten
const char* WIFI_SSID = "GuggenbergerLinux";
const char* WIFI_PASS = "Isabelle2014samira";

// Static IP
IPAddress local_IP(192, 168, 13, 55);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 0, 0, 0);  
IPAddress dns(192, 168, 1, 1); 

// Verbindung zu SSID
Serial.print("Verbindung zu SSID - ");
Serial.println(WIFI_SSID); 

// IP zuweisen
if (!WiFi.config(local_IP, gateway, subnet, dns)) {
   Serial.println("STA fehlerhaft!");
  }

// NTP Setup
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  LokaleZeit();

// WiFI Modus setzen
WiFi.mode(WIFI_OFF);
WiFi.disconnect();
delay(100);

WiFi.begin(WIFI_SSID, WIFI_PASS);
Serial.println("Verbindung aufbauen ...");

while (WiFi.status() != WL_CONNECTED) {

  if (WiFi.status() == WL_CONNECT_FAILED) {
     Serial.println("Keine Verbindung zum SSID möglich : ");
     Serial.println();
     Serial.print("SSID: ");
     Serial.println(WIFI_SSID);
     Serial.print("Passwort: ");
     Serial.println(WIFI_PASS);
     Serial.println();
    }
  delay(2000);
}
    Serial.println("");
    Serial.println("Mit Wifi verbunden");
    Serial.println("IP Adresse: ");
    Serial.println(WiFi.localIP());

}

/////////////////////////////////////////////////////////////////////////// SETUP - OTA
void ArduinoOTAsetup(){

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

}

/////////////////////////////////////////////////////////////////////////// SETUP
void setup() {

  // Serielle Kommunikation starten
  Serial.begin(115200);

// Sturmschutzschalter init
pinMode(sturmschutzschalterpin, INPUT);

// Panel senkrecht init
pinMode(panelsenkrechtpin, INPUT);

// Wifi setup
wifi_setup();

// OTA Setup
ArduinoOTAsetup();

//Pins deklarieren
  pinMode(M1_re,OUTPUT);
  pinMode(M1_li,OUTPUT);
  pinMode(M2_re,OUTPUT);
  pinMode(M2_li,OUTPUT);
}

/////////////////////////////////////////////////////////////////////////// Sonnensensor - Fotowiderstände
void sonnensensor(){

sensorSonne_NO = analogRead(adc_NO);  
sensorSonne_NW = analogRead(adc_NW);
sensorSonne_SO = analogRead(adc_SO);
sensorSonne_SW = analogRead(adc_SW);


// Werte Seriell ausgeben
Serial.print("Wert sensorSonne_NO : ");
Serial.println(sensorSonne_NO);
Serial.print("Wert sensorSonne_NW : ");
Serial.println(sensorSonne_NW);
Serial.print("Wert sensorSonne_SO : ");
Serial.println(sensorSonne_SO);
Serial.print("Wert sensorSonne_SW : ");
Serial.println(sensorSonne_SW);

// Quersumme aller Werte
sonne_quersumme = (sensorSonne_NO + sensorSonne_NW + sensorSonne_SO + sensorSonne_SW) / 4;

Serial.print("Sonne Quersumme: ");
Serial.println(sonne_quersumme);
Serial.print("Sonne Quersumme max Wert ");
Serial.println(helligkeit_schwellwert);

// Justierung stoppen Wolken
if (sonne_quersumme < helligkeit_schwellwert) {
Serial.println("Helligkeit - Ausrichten ");

horizontal_hoch   = (sensorSonne_NO + sensorSonne_NW)/2;
horizontal_runter = (sensorSonne_SO + sensorSonne_SW)/2;
vertikal_rechts   = (sensorSonne_NO + sensorSonne_SO)/2;
vertikal_links    = (sensorSonne_NW + sensorSonne_SW)/2;

    // Sonnentraking Drehen
    differenz_drehen = (sensorSonne_NW + sensorSonne_SW)/2 - (sensorSonne_NO + sensorSonne_SO)/2;
    Serial.print("Differenz Drehen: ");
    Serial.println(differenz_drehen);
    // Motor stoppen
    m2(3);

    if (vertikal_rechts > vertikal_links && (vertikal_rechts-vertikal_links) > traker_tolleranz) {
      Serial.println("Motor drehen - RECHTS");
      m2(2);  
    }
    
    if (vertikal_links > vertikal_rechts && (vertikal_links-vertikal_rechts) > traker_tolleranz) {
      Serial.println("Motor drehen - LINKS");
      m2(1); 
    }
       

    // Schwellwert überschritten Ausrichten
    // Sonnentraking Neigen
    differenz_neigen = (sensorSonne_NO + sensorSonne_NW)/2 - (sensorSonne_SO + sensorSonne_SW)/2;
    Serial.print("Differenz Neigen: ");
    Serial.println(differenz_neigen);

    // Motor stoppen
    m1(3);

if (horizontal_hoch > horizontal_runter && (horizontal_hoch-horizontal_runter) > traker_tolleranz) {
      Serial.println("Motor neigen - RUNTER");
      m1(1); 
}
 
if (horizontal_runter > horizontal_hoch && (horizontal_runter-horizontal_hoch) > traker_tolleranz) {
      Serial.println("Motor neigen - HOCH");
      m1(2);       
}


} else {
Serial.println("Helligkeit - Nichts tun ");
// Platte horizontal stellen
m1(2);

}

}

/////////////////////////////////////////////////////////////////////////// NTP Local Time
void LokaleZeit(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
 /* Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
  */
}

/////////////////////////////////////////////////////////////////////////// m1 Neigen
void m1(int x) {
  // x = 1 senken | x = 2 heben | x = 3 aus

  if (x == 1) {
    // Panele senken
    //Serial.println("Panele senken");
    digitalWrite(M1_re, HIGH);
    digitalWrite(M1_li, LOW);
  }

  if (x == 2) {
    // Panele heben
    //Serial.println("Panele heben");
    digitalWrite(M1_re, LOW);
    digitalWrite(M1_li, HIGH);
  }

  if (x == 3) {
    // Neigen stop
    //Serial.println("Neigen stop");
    digitalWrite(M1_re, LOW);
    digitalWrite(M1_li, LOW);
  }  

}

/////////////////////////////////////////////////////////////////////////// m1 Drehen
void m2(int x) {
  // x = 1 links | x = 2 rechts | x = 3 aus

  if (x == 1) {
    // Panel links drehen
    //erial.println("Panel links drehen");
    digitalWrite(M2_re, HIGH);
    digitalWrite(M2_li, LOW);
  }

  if (x == 2) {
    // Panel rechts drehen
    //Serial.println("Panel rechts drehen");
    digitalWrite(M2_re, LOW);
    digitalWrite(M2_li, HIGH);
  }

  if (x == 3) {
    // Drehen stop
    //Serial.println("Drehen stop");
    digitalWrite(M2_re, LOW);
    digitalWrite(M2_li, LOW);
  }  

}

/////////////////////////////////////////////////////////////////////////// Sturmschutz - Solarpanel waagerecht ausrichten 
void sturmschutz() {

  // Prüfen auf Messwert des Sensors

  // Wenn zu stark wind_zu_stark auf 1 setzen
  Serial.println("Sturmschutz fahren");
  // Motor m1 Panel waagerecht ausrichten bis Endlage
 

  // Motor m2 Panel drehen Osten bis Endlage
  
  //Sturmschutz pause
  delay(sturmschutz_pause);

}

/////////////////////////////////////////////////////////////////////////// Schneelast / Reinigen - Solarpanel senkrecht ausrichten 
void panel_senkrecht() {

  // Schalter abfragen
  	while( digitalRead(panelsenkrechtpin) == 1 ) //while the button is pressed
      {
        //blink
        Serial.println("Panele senkrecht stellen");
        m1(1);

        delay(500);
      }

}

/////////////////////////////////////////////////////////////////////////// Sonnenaufgang - Panele ausrichten 
void sonnenaufgang() {

  // Motor m1 Panel senkrecht ausrichten bis Endlage
  m1(1);

  // Motor m2 Panel drehen Osten bis Endlage
  m2(1); 

}

/////////////////////////////////////////////////////////////////////////// Sonnenaufgang - Panele ausrichten 
void sturmschutzschalter() {

  // Schalter abfragen
  	while( digitalRead(sturmschutzschalterpin) == 1 ) //while the button is pressed
      {
        //blink
        Serial.println("Alles unterbrechen wegen Windschutz!");
        m1(2);
        delay(500);
      }

}

/////////////////////////////////////////////////////////////////////////// LOOP
void loop() {

//OTA Handler
ArduinoOTA.handle();

/*
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Auf Sturm prüfen
  if (millis() - previousMillis_Sturmcheck > interval_Sturmcheck) {
      previousMillis_Sturmcheck = millis(); 
      // Windstärke prüfen
      Serial.println("Windstärke prüfen");
    }
*/

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Sturmschutzschalter abfragen
  if (millis() - previousMillis_sturmschutzschalter > interval_sturmschutzschalter) {
      previousMillis_sturmschutzschalter = millis(); 
      // Windstärke prüfen
      Serial.println("Sturmschutzschalter Prüfen");
      sturmschutzschalter();
    }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Panele senkrecht
  if (millis() - previousMillis_panelsenkrecht > interval_panelsenkrecht) {
      previousMillis_panelsenkrecht = millis(); 
      // Windstärke prüfen
      Serial.println("Panele senkrecht stellen");
      panel_senkrecht();
    }


  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Auf Sturm prüfen
  if (millis() - previousMillis_sonnensensor > interval_sonnensensor) {
      previousMillis_sonnensensor = millis(); 
      // Sonnenposition prüfen wenn windstärke okay
      if (wind_zu_stark != 1) {
      //Serial.println("Position der Sonne prüfen.");
      sonnensensor();
      } else {
        Serial.println("Keine Ausrichtung, da Wind zu stark!");
      }
  
    }

}