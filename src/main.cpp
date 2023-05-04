#include <Arduino.h>
#include <PubSubClient.h>  
#include <ArduinoOTA.h>
#include "WiFi.h"

/////////////////////////////////////////////////////////////////////////// Schleifen verwalten
unsigned long previousMillis_mqttCHECK = 0; // Windstärke prüfen
unsigned long interval_mqttCHECK = 500; 

unsigned long previousMillis_LDR_auslesen = 0; // Sonnenstand prüfen
unsigned long interval_LDR_auslesen = 5000; //5000

unsigned long previousMillis_sonnentracking = 0; // Sonnenstand prüfen
unsigned long interval_sonnentracking = 10; //5000

unsigned long previousMillis_sturmschutzschalter = 0; // Sturmschutz Schalter prüfen
unsigned long interval_sturmschutzschalter = 1000; 

unsigned long previousMillis_panelsenkrecht = 0; // Sturmschutz Schalter prüfen
unsigned long interval_panelsenkrecht = 1000; 

unsigned long previousMillis_nachtstellung_pruefen = 0; // Sturmschutz Schalter prüfen
unsigned long interval_nachtstellung_pruefen = 5000; 

unsigned long previousMillis_mqttbewegung_pruefen = 0; // Sturmschutz Schalter prüfen
unsigned long interval_mqttbewegung_pruefen = 1000; 

/////////////////////////////////////////////////////////////////////////// Systemvariablen
int nachstellung_merker = 0; // Registriert die Nachstellung und wird morgens resetet.
int mqtt_sturmschutz_status = 0; // Sturmschutz wird per mqtt aktiviert
int mqtt_panel_senkrecht = 0; // Panel Senkrecht schalten
int mqtt_panel_links = 0; // Panel links fahren
int mqtt_panel_rechts = 0; // Panel rechts fahren

/////////////////////////////////////////////////////////////////////////// Pin Input
int sturmschutzschalterpin =  13;
int panelsenkrechtpin =  12;

/////////////////////////////////////////////////////////////////////////// Schwellwerte
int schwellwert_nachtstellung = 1500 ;  // 600Ab diesem Wert wird auf Nachtstellung gefahren
int schwellwert_bewoelkt = 100 ;          // Schwellwert für Bewölkung
int schwellwert_morgen_aktivieren = 500;  // Schwellwert von Sensor oben_links der die ersten
                                        // Sonnenstrahlen registriert
int ausrichten_tolleranz_oben_unten = 250; // Ausgleichen von Schwankungen!
int ausrichten_tolleranz_rechts_links = 250; // Ausgleichen von Schwankungen!

int durchschnitt_oben;
int durchschnitt_unten;
int durchschnitt_links;
int durchschnitt_rechts;

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
const int ldr_oben_links = 33; //ADC1_6   - LDR OL - Sensor-Leitung blau  (NW) OR 33
int oben_rechts;
const int ldr_oben_rechts = 32; //ADC1_7  - LDR OR - Sensor-Leitung braun (NO)  32
int unten_links;
const int ldr_unten_links = 34; //ADC1_8  - LDR UL - Sensor-Leitung grün  (SW) BR 34
int unten_rechts;
const int ldr_unten_rechts = 35; //ADC1_9 - LDR UR - Sensor-Leitung weiss (SO ) WS 35

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
void mqtt_Sturmschutz           ();
void mqtt_panele_senkrecht      ();
void mqtt_panele_links          ();
void mqtt_panele_rechts         ();


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
      client.subscribe("Solarpanel/001/steuerung/sturmschutz");
      client.subscribe("Solarpanel/001/steuerung/senkrecht");
      client.subscribe("Solarpanel/001/steuerung/links");
      client.subscribe("Solarpanel/001/steuerung/rechts");

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

  /////////////////////////////////////////////////////////////////////////// Sturmschutz schalten
      if (strcmp(topic,"Solarpanel/001/steuerung/sturmschutz")==0) {

          // Sturmschutz aktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {  
                client.publish("Solarpanel/001/codemeldung", "Sturmschutz AKTIV");  
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_sturmschutz_status = 1;

                }
          // Sturmschutz deaktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {  
                client.publish("Solarpanel/001/codemeldung", "Sturmschutz AUS");
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_sturmschutz_status = 0;

                }
        } 

  /////////////////////////////////////////////////////////////////////////// Panel senkrecht
      if (strcmp(topic,"Solarpanel/001/steuerung/senkrecht")==0) {

          // Sturmschutz aktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {  
                client.publish("Solarpanel/001/codemeldung", "Panel Senkrecht AN");  
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_panel_senkrecht = 1;

                }
          // Sturmschutz deaktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {  
                client.publish("Solarpanel/001/codemeldung", "Panel Senkrecht AUS");
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_panel_senkrecht = 0;

                }
        }         

  /////////////////////////////////////////////////////////////////////////// Panel links
      if (strcmp(topic,"Solarpanel/001/steuerung/links")==0) {

          // Sturmschutz aktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {  
                client.publish("Solarpanel/001/codemeldung", "Panel Links AN");  
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_panel_links = 1;

                }
          // Sturmschutz deaktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {  
                client.publish("Solarpanel/001/codemeldung", "Panel Links AUS");
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_panel_links = 0;

                }
        } 

  /////////////////////////////////////////////////////////////////////////// Panel rechts
      if (strcmp(topic,"Solarpanel/001/steuerung/rechts")==0) {

          // Sturmschutz aktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {  
                client.publish("Solarpanel/001/codemeldung", "Panel Rechts AN");  
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_panel_rechts = 1;

                }
          // Sturmschutz deaktivieren
          if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {  
                client.publish("Solarpanel/001/codemeldung", "Panel Rechts AUS");
                // mqtt_sturmschutz_status - per FHEM aktivieren
                mqtt_panel_rechts = 0;

                }
        } 


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


// Quersumme aller Sensoren berechnen
int durchschnitt_bewoelkt = (oben_links + oben_rechts + unten_links + unten_rechts) / 4;

// Daten LDR auf mqtt ausgeben


dtostrf(durchschnitt_bewoelkt,2, 1, buffer1); 
client.publish("Solarpanel/001/bewoelkung", buffer1);

dtostrf(oben_links,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_wert_oben_links", buffer1); 

dtostrf(oben_rechts,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_wert_oben_rechts", buffer1); 

dtostrf(unten_links,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_wert_unten_links", buffer1); 

dtostrf(unten_rechts,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_wert_unten_rechts", buffer1); 


// Durchschnitt

durchschnitt_oben = (oben_links + oben_rechts)/2; //Durchschnitt von rauf 
durchschnitt_unten = (unten_links + unten_rechts)/2 ; //Durchschnitt von runter 
durchschnitt_links = (oben_links + unten_links)/2; //Durchschnitt von links 
durchschnitt_rechts = (oben_rechts + unten_rechts)/2; //Durchschnitt von rechts 

  dtostrf(durchschnitt_oben,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_oben", buffer1); 

dtostrf(durchschnitt_unten,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_unten", buffer1); 

dtostrf(durchschnitt_links,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_links", buffer1); 

dtostrf(durchschnitt_rechts,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_rechts", buffer1); 

dtostrf(nachstellung_merker,2, 1, buffer1); 
client.publish("Solarpanel/001/codemeldung", buffer1);

/*

Serial.print("Wert LDR oben links : ");
Serial.println(oben_links);
Serial.print("Wert LDR oben rechts : ");
Serial.println(oben_rechts);
Serial.print("Wert LDR unten links : ");
Serial.println(unten_links);
Serial.print("Wert LDR unten rechts: ");
Serial.println(unten_rechts);
Serial.println("------------------------------------------");
*/

}

/////////////////////////////////////////////////////////////////////////// Dunkelheit feststellen
void nachtstellung(){
//Serial.println("FUNCTION ################################### Nachtstellung");

  
// Quersumme aller Sensoren berechnen
int durchschnitt_nachtstellung = (oben_links + oben_rechts + unten_links + unten_rechts) / 4;

//Serial.print("Nachstellung Wert Quersumme : ");
//Serial.println(durchschnitt_nachtstellung);

// Sonnenstärke auslesen
int mqtt_sonnenstaerke = durchschnitt_nachtstellung;
dtostrf(mqtt_sonnenstaerke,2, 1, buffer1); 
client.publish("Solarpanel/001/sonnenQuersumme", buffer1); 


if (durchschnitt_nachtstellung >= schwellwert_nachtstellung)
{
// Nachtstellung fahren
Serial.println("Nachtstellung ---------------------------------- AKTIV");
//Serial.println("##########################   -- > Nachtstellung AKTIV");
client.publish("Solarpanel/001/meldung", "Nachtstellung aktiv"); 

// Variabl setzen für merker
nachstellung_merker = 1;

// Platten stellen
m1(2); // Oben
m2(1); //Links

} else {

//nachtstellung_aktiv = 0;

}

}

/////////////////////////////////////////////////////////////////////////// Sonne tracken
void tracking(){
 // Serial.println("FUNCTION ################################### Tracking");
durchschnitt_oben = (oben_links + oben_rechts); //Durchschnitt von rauf 
durchschnitt_unten = (unten_links + unten_rechts) ; //Durchschnitt von runter 
durchschnitt_links = (oben_links + unten_links); //Durchschnitt von links 
durchschnitt_rechts = (oben_rechts + unten_rechts); //Durchschnitt von rechts 


/*
  dtostrf(durchschnitt_oben,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_oben", buffer1); 

dtostrf(durchschnitt_unten,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_unten", buffer1); 

dtostrf(durchschnitt_links,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_links", buffer1); 

dtostrf(durchschnitt_rechts,2, 1, buffer1); 
client.publish("Solarpanel/001/LDR_ds_rechts", buffer1); 

dtostrf(nachstellung_merker,2, 1, buffer1); 
client.publish("Solarpanel/001/codemeldung", buffer1);

/*
Serial.print("Durchschnitt Bewölkt ");
Serial.println(durchschnitt_bewoelkt);
*/

// Quersumme aller Sensoren berechnen
int durchschnitt_bewoelkt = (oben_links + oben_rechts + unten_links + unten_rechts) / 4;

// Ausrichten des Panel am morgen
if (oben_links < schwellwert_morgen_aktivieren && nachstellung_merker == 1)
{
  // client.publish("Solarpanel/001/meldung", "Morgen ausrichten");
  // Sobald der Sensor oben_links unter den Schwellwert fällt, Panele in morgenstellung bringen
  m1(1); // senkrecht
  m2(2); //rechts
  //client.publish("Solarpanel/001/codemeldung", "Morgensetup - Ausrichten");
  // Warten das alle Positionen angefahren werden
  delay(12000);
  // nachstellung_merker zurücksetzten
  nachstellung_merker = 0;
 
} else
{
 //client.publish("Solarpanel/001/codemeldung", "Morgensetup - NICHTS");
}

// Messen des Schwellwertes für Bewölkung
if (durchschnitt_bewoelkt < schwellwert_bewoelkt) {

  //Serial.println("------------------------> Sonne ausreichend --- Panel justieren");

    if (((durchschnitt_oben + durchschnitt_unten) / 2) > ausrichten_tolleranz_oben_unten) { // Durchschnitt

    // client.publish("Solarpanel/001/meldung", "Regelung");

          // Oben Unten ausrichten
          if (durchschnitt_oben < durchschnitt_unten)
          {
                // Nach unten ausrichten
                Serial.println("BEWEGEN ---- unten");
               // client.publish("Solarpanel/001/bewegungsmeldung", "Panel unten");
                m1(1); // Unten
                //delay(600);
                //m1(3);
                
          }
          else if (durchschnitt_unten < durchschnitt_oben)
          {
                // Nach oben ausrichten
                Serial.println("BEWEGEN ---- oben");
              //  client.publish("Solarpanel/001/bewegungsmeldung", "Panel oben");
                m1(2); // Oben
                //delay(600);
                //m1(3);
          }
          else 
          {
                //Serial.println("BEWEGEN OBEN/UNTEN ---- NICHTS");
                m1(3);
                
                //delay(3500);
          }
        
    } else {

    //client.publish("Solarpanel/001/bewegungsmeldung", "Durch oben/unten < Schwellwert");
    Serial.println("KEINE BEWEGUNG  oben/unten < Schwellwert");

    }// Durchschnitt


    if (((durchschnitt_links + durchschnitt_rechts) / 2) > ausrichten_tolleranz_rechts_links) { // Durchschnitt

        // Rechts / Links ausrichten
        if (durchschnitt_links > durchschnitt_rechts)
        {
              // Rechts
              Serial.println("BEWEGEN ---- rechts");
            //  client.publish("Solarpanel/001/bewegungsmeldung", "Panel rechts");
              m2(2); // Rechts
              //delay(600);
              //m2(3);

        }
        else if (durchschnitt_rechts > durchschnitt_links)
        {
              // Links
              Serial.println("BEWEGEN ---- links");
            //  client.publish("Solarpanel/001/bewegungsmeldung", "Panel links");
              m2(1); //Links
              //delay(600);
              //m2(2);

        }
        else 
        {
              //Serial.println("BEWEGEN RECHTS/UNTEN ---- NICHTS");
            // client.publish("Solarpanel/001/bewegungsmeldung", "Ausgerichtet!");
             Serial.println("BEWEGEN ---- AUSGERICHTET");
            
              m2(3);
              //delay(1000);
        }

    } else {

   //  client.publish("Solarpanel/001/bewegungsmeldung", "Durch rechts/links < Schwellwert");
     Serial.println("KEINE BEWEGUNG rechts/links < Schwellwert");

    } // Durchschnitt    

} else { // schwellwert prüfen

              Serial.println("##########################   -- > Keine Aktion - zuviel Wolken");
             // client.publish("Solarpanel/001/bewegungsmeldung", "Stop - zu viele Wolken");
            //  client.publish("Solarpanel/001/meldung", "Zu viele Wolken!"); 
              m1(3);
              m2(3);

}


}

/////////////////////////////////////////////////////////////////////////// Sonnenaufgang - Panele ausrichten 
void sturmschutzschalter() {
  //Serial.println("FUNCTION ################################### Sturmschalter");

  // Schalter abfragen
  	while( digitalRead(sturmschutzschalterpin) == 1) //while the button is pressed
      {
        //Serial.println("Alles unterbrechen wegen Windschutz!");
        client.publish("Solarpanel/001/bewegungsmeldung", "STURMSCHUTZ AKTIV!2");
        m1(2);
        delay(300);
        
      }

}

/////////////////////////////////////////////////////////////////////////// Schneelast / Reinigen - Solarpanel senkrecht ausrichten 
void panel_senkrecht() {
  //Serial.println("FUNCTION ################################### Panele senkrecht");

  // Schalter abfragen
  	while( digitalRead(panelsenkrechtpin) == 1) //while the button is pressed
      {
        //blink
        client.publish("Solarpanel/001/bewegungsmeldung", "Panele senkrecht");
        m1(1);

        delay(300);
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

/////////////////////////////////////////////////////////////////////////// mqtt Sturmschutz ausführen
void mqtt_sturmschutz(){
  //Serial.println("FUNCTION ################################### mqtt Sturmschutz");

if (mqtt_sturmschutz_status == 1) {

        client.publish("Solarpanel/001/bewegungsmeldung", "mqtt Sturmschutz aktiv");
        m1(2);

} else {

m1(3);

}

}

/////////////////////////////////////////////////////////////////////////// mqtt Panele senkrecht
void mqtt_panele_senkrecht(){
  //Serial.println("FUNCTION ################################### mqtt Panele senkrecht");

if (mqtt_panel_senkrecht == 1) {

        client.publish("Solarpanel/001/bewegungsmeldung", "mqtt Panel senkrecht");
        m1(1);

} else {

m1(3);

}

}

/////////////////////////////////////////////////////////////////////////// mqtt Panele links
void mqtt_panele_links(){

  //Serial.println("FUNCTION ################################### mqtt Panele links");

if (mqtt_panel_links == 1) {

        client.publish("Solarpanel/001/bewegungsmeldung", "mqtt Panel links");
        m2(1);

} else {

  m2(3);
  
  }

}

/////////////////////////////////////////////////////////////////////////// mqtt Panele senkrecht
void mqtt_panele_rechts(){
  
  //Serial.println("FUNCTION ################################### mqtt Panele rechts");

if (mqtt_panel_rechts == 1) {

        client.publish("Solarpanel/001/bewegungsmeldung", "mqtt Panel rechts");
        m2(2);

} else {

  m2(3);

}

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
  if (nachstellung_merker == 0 && mqtt_sturmschutz_status == 0 && mqtt_panel_senkrecht == 0 && mqtt_panel_links == 0 && mqtt_panel_rechts == 0) { 
      if (millis() - previousMillis_sonnentracking > interval_sonnentracking) {
          previousMillis_sonnentracking = millis(); 
          tracking();        
        }
  }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Sturmschutzschalter abfragen
  if (millis() - previousMillis_sturmschutzschalter > interval_sturmschutzschalter) {
      previousMillis_sturmschutzschalter = millis(); 
      // Windstärke prüfen
     sturmschutzschalter();
    }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Panele senkrecht
  if (millis() - previousMillis_panelsenkrecht > interval_panelsenkrecht) {
      previousMillis_panelsenkrecht = millis(); 
      // Panel senkrecht schalten
      panel_senkrecht();
    }

  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Auf mqtt Bewegung prüfen
  if (millis() - previousMillis_mqttbewegung_pruefen > interval_mqttbewegung_pruefen) {
      previousMillis_mqttbewegung_pruefen = millis(); 
      // mqtt - Bewgungsabfragen
      mqtt_sturmschutz();
      
        // Weitere mqtt Anweisungen fahren wenn Sturmschutz inaktiv.      
        if (mqtt_sturmschutz_status == 0) {
          mqtt_panele_senkrecht();

          // Fahrt rechts und links gegenseitig verriegeln
          if (mqtt_panel_links == 0) {
              mqtt_panele_rechts();
          }

          if (mqtt_panel_rechts == 0) {
              mqtt_panele_links();
          }


        }
    }    

}