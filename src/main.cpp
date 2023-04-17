#include <Arduino.h>
#include <PubSubClient.h>  
#include <ArduinoOTA.h>
#include "WiFi.h"

/////////////////////////////////////////////////////////////////////////// Schleifen verwalten
unsigned long previousMillis_mqttCHECK = 0; // Windstärke prüfen
unsigned long interval_mqttCHECK = 450; 

unsigned long previousMillis_LDR_auslesen = 0; // Sonnenstand prüfen
unsigned long interval_LDR_auslesen = 2500;

unsigned long previousMillis_sonnentracking = 0; // Sonnenstand prüfen
unsigned long interval_sonnentracking = 2500;

unsigned long previousMillis_sturmschutzschalter = 0; // Sturmschutz Schalter prüfen
unsigned long interval_sturmschutzschalter = 1000; 

unsigned long previousMillis_panelsenkrecht = 0; // Sturmschutz Schalter prüfen
unsigned long interval_panelsenkrecht = 1000; 

unsigned long previousMillis_nachtstellung_pruefen = 0; // Sturmschutz Schalter prüfen
unsigned long interval_nachtstellung_pruefen = 2500; 

/////////////////////////////////////////////////////////////////////////// Pin Input
int sturmschutzschalterpin =  13;
int panelsenkrechtpin =  12;

/////////////////////////////////////////////////////////////////////////// Schwellwerte
int nachtstellung_aktiv = 0;
int schwellwert_nachtstellung = 1800 ;

/////////////////////////////////////////////////////////////////////////// Pin output zuweisen
#define M1_re 2   // D2  - grau weiss - Pin 7
#define M1_li 4   // D4  - grün - Pin 6
#define M2_re 5   // D5  - violett - Pin 5
#define M2_li 18  // D18 - schwarz - Pin 4

/////////////////////////////////////////////////////////////////////////// mqtt
char buffer1[10];


/////////////////////////////////////////////////////////////////////////// ADC Pin und Variablen zuweisen
/*
Sensor-Leitung orange +5V
Sensor-Leitung weis Masse
*/
int oben_links;
const int ldr_oben_links = 33; //ADC1_6   - LDR OL - Sensor-Leitung blau  (NO)
int oben_rechts;
const int ldr_oben_rechts = 32; //ADC1_7  - LDR OR - Sensor-Leitung braun (NW)
int unten_links;
const int ldr_unten_links = 34; //ADC1_8  - LDR UL - Sensor-Leitung grün  (SO)
int unten_rechts;
const int ldr_unten_rechts = 35; //ADC1_9 - LDR UR - Sensor-Leitung weiss (SW)

/////////////////////////////////////////////////////////////////////////// Funktionsprototypen
void loop                       ();
void callback                   (char* topic, byte* payload, unsigned int length);
void m1                         (int); // Panel neigen
void m2                         (int); // Panel drehen
void reconnect                  ();
void wifi_setup                 ();
void OTA_update                 ();
void fotosensoren_auslesen      ();
void tracking                   ();
void sturmschutzschalter        ();
void panel_senkrecht            ();
void nachtstellung              ();
void mqtt_connected             ();


/////////////////////////////////////////////////////////////////////////// Kartendaten 
const char* kartenID = "SunTracker2023_v1";

/////////////////////////////////////////////////////////////////////////// MQTT 
WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "192.168.150.1";

/////////////////////////////////////////////////////////////////////////// SETUP - OTA Update
void OTA_update(){

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

/////////////////////////////////////////////////////////////////////////// SETUP - Wifi
void wifi_setup() {

// WiFi Zugangsdaten
const char* WIFI_SSID = "GuggenbergerLinux";
const char* WIFI_PASS = "Isabelle2014samira";

// Static IP
IPAddress local_IP(192, 168, 55, 30);
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

/////////////////////////////////////////////////////////////////////////// VOID mqtt reconnected
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Baue Verbindung zum mqtt Server auf. IP: ");
    // Attempt to connect
    if (client.connect(kartenID,"zugang1","43b4134735")) {
      //Serial.println("connected");
      ////////////////////////////////////////////////////////////////////////// SUBSCRIBE Eintraege
      //client.subscribe("SolarTracker/001//IN");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

/////////////////////////////////////////////////////////////////////////// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {

/*
  /////////////////////////////////////////////////////////////////////////// Relais 0
      if (strcmp(topic,"Vogelhaus/LED_Licht/IN")==0) {

          // ON und OFF Funktion auslesen
          if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {  
                  Serial.println("LED Licht -> AN");

                }

          if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {  
                  Serial.println("LED Licht -> AUS");

                }
        } 
*/
}

/////////////////////////////////////////////////////////////////////////// SETUP
void setup() {

// Serielle Kommunikation starten
Serial.begin(38400);

// Wifi setup
wifi_setup();

// OTA update 
OTA_update();

// MQTT Broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

// Sturmschutzschalter init
pinMode(sturmschutzschalterpin, INPUT);

// Panel senkrecht init
pinMode(panelsenkrechtpin, INPUT);  

//Pins deklarieren Stellmotoren
  pinMode(M1_re,OUTPUT);
  pinMode(M1_li,OUTPUT);
  pinMode(M2_re,OUTPUT);
  pinMode(M2_li,OUTPUT);

}

/////////////////////////////////////////////////////////////////////////// LDR auslesen
void fotosensoren_auslesen() {
// Analogwerte auslesen
oben_links = analogRead(ldr_oben_links);  

oben_rechts = analogRead(ldr_oben_rechts);

unten_links = analogRead(ldr_unten_links);

unten_rechts = analogRead(ldr_unten_rechts);


Serial.print("Wert LDR oben links : ");
Serial.println(oben_links);
Serial.print("Wert LDR oben rechts : ");
Serial.println(oben_rechts);
Serial.print("Wert LDR unten links : ");
Serial.println(unten_links);
Serial.print("Wert LDR unten rechts: ");
Serial.println(unten_rechts);

Serial.println("------------------------------------------");


}

/////////////////////////////////////////////////////////////////////////// Dunkelheit feststellen
void nachtstellung(){
  
// Quersumme aller Sensoren berechnen
int durchschnitt_nachtstellung = (oben_links + oben_rechts + unten_links + unten_rechts) / 4;

Serial.print("Nachstellung Wert Quersumme : ");
Serial.println(durchschnitt_nachtstellung);

// Werte senden
          dtostrf(durchschnitt_nachtstellung,2, 1, buffer1); 
  
          client.publish("Solarpanel/001/sonnenQuersumme", buffer1); 


if (durchschnitt_nachtstellung <= schwellwert_nachtstellung)
{
// Nachtstellung fahren

nachtstellung_aktiv = 1;

client.publish("Solarpanel/001/meldung", "Nachtstellung aktiv"); 

} else {

nachtstellung_aktiv = 0;

}

}

/////////////////////////////////////////////////////////////////////////// Sonne tracken
void tracking(){
  int durchschnitt_oben = (oben_links + oben_rechts) ; //Durchschnitt von rauf 
  int durchschnitt_unten = (unten_links + unten_rechts) ; //Durchschnitt von runter 
  int durchschnitt_links = (oben_links + unten_links) ; //Durchschnitt von links 
  int durchschnitt_rechts = (oben_rechts + unten_rechts) ; //Durchschnitt von rechts 

  client.publish("Solarpanel/001/meldung", "Panele ausrichten"); 

  // Oben Unten ausrichten
  if (durchschnitt_oben < durchschnitt_unten)
  {
        // Nach unten ausrichten
        Serial.println("BEWEGEN ---- UNTEN");
  }
  else if (durchschnitt_unten < durchschnitt_oben)
  {
        // Nach oben ausrichten
        Serial.println("BEWEGEN ---- OBEN");
  }
  else 
  {
        Serial.println("BEWEGEN OBEN/UNTEN ---- NICHTS");
  }
  

  // Rechts / Links ausrichten
  if (durchschnitt_links > durchschnitt_rechts)
  {
        // Links
        Serial.println("BEWEGEN ---- LINKS");

  }
  else if (durchschnitt_rechts > durchschnitt_links)
  {
        // Rechts
        Serial.println("BEWEGEN ---- RECHTS");

  }
  else 
  {
        Serial.println("BEWEGEN RECHTS/UNTEN ---- NICHTS");
  }
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

/////////////////////////////////////////////////////////////////////////// m1 Neigen
void m1(int x) {
  // x = 1 senken | x = 2 heben | x = 3 aus

  if (x == 1) {
    // Panele senken
    //Serial.println("Panele senken");
    digitalWrite(M1_re, HIGH); // D2 - weiss - Pin7
    digitalWrite(M1_li, LOW); // D4 - grün + Pin 6
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
    digitalWrite(M2_re, HIGH); // D5 - violett - Pin 5
    digitalWrite(M2_li, LOW); // D18 - schwarz - Pin 4
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

/////////////////////////////////////////////////////////////////////////// mqtt connected
void mqtt_connected(){

    // mqtt Daten senden     
  if (!client.connected()) {
      reconnect();
    }
    client.loop(); 

}

/////////////////////////////////////////////////////////////////////////// LOOP
void loop() {

ArduinoOTA.handle();  

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ mqtt Checken
  if (millis() - previousMillis_mqttCHECK > interval_mqttCHECK) {
      previousMillis_mqttCHECK = millis(); 
      mqtt_connected();
    }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Nachtstellung prüfen
  if (millis() - previousMillis_nachtstellung_pruefen > interval_nachtstellung_pruefen) {
      previousMillis_nachtstellung_pruefen = millis(); 
      nachtstellung();
    }


  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ LDR auslesen
  if (millis() - previousMillis_LDR_auslesen > interval_LDR_auslesen) {
      previousMillis_LDR_auslesen = millis(); 
      fotosensoren_auslesen();
    }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Sonne tracking
  // auf Nachstellung prüfen wenn 1 kein Tracking
  if (nachtstellung_aktiv == 0) { 
      if (millis() - previousMillis_sonnentracking > interval_sonnentracking) {
          previousMillis_sonnentracking = millis(); 
          tracking();        
        }
  } else {};

/*
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Sturmschutzschalter abfragen
  if (millis() - previousMillis_sturmschutzschalter > interval_sturmschutzschalter) {
      previousMillis_sturmschutzschalter = millis(); 
      // Windstärke prüfen
      //Serial.println("Sturmschutzschalter Prüfen");
     sturmschutzschalter();
    }    

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Panele senkrecht
  if (millis() - previousMillis_panelsenkrecht > interval_panelsenkrecht) {
      previousMillis_panelsenkrecht = millis(); 
      // Windstärke prüfen
      //Serial.println("Panele senkrecht stellen");
      panel_senkrecht();
    }
*/
}