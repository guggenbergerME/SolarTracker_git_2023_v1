void tracking(){
 // Serial.println("FUNCTION ################################### Tracking");
durchschnitt_oben = (oben_links + oben_rechts); //Durchschnitt von rauf 
durchschnitt_unten = (unten_links + unten_rechts) ; //Durchschnitt von runter 
durchschnitt_links = (oben_links + unten_links); //Durchschnitt von links 
durchschnitt_rechts = (oben_rechts + unten_rechts); //Durchschnitt von rechts 

durchschnitt_bewoelkt = (oben_links + oben_rechts + unten_links + unten_rechts) / 4;

// Messen des Schwellwertes für Bewölkung
if (durchschnitt_bewoelkt < schwellwert_bewoelkt) {

          // Oben Unten ausrichten
          if (durchschnitt_oben < durchschnitt_unten)
          {
                // Nach unten ausrichten
              Serial.println("BEWEGEN ---- unten");
                m1(1); // Unten
                
          }
          else if (durchschnitt_unten < durchschnitt_oben)
          {
                // Nach oben ausrichten
              Serial.println("BEWEGEN ---- oben");
                m1(2); // Oben

          }
          else 
          {
                //Serial.println("BEWEGEN OBEN/UNTEN ---- NICHTS");
                //m1(3);

          }


        // Rechts / Links ausrichten
        if (durchschnitt_links > durchschnitt_rechts)
        {
              // Links
             Serial.println("BEWEGEN ---- links");
              m2(1); // Links

        }
        else if (durchschnitt_rechts > durchschnitt_links)
        {
              // Rechts
             Serial.println("BEWEGEN ---- rechts");
              m2(2); //rechts


        }
        else 
        {
              //Serial.println("BEWEGEN RECHTS/UNTEN ---- NICHTS");
            // Serial.println("BEWEGEN ---- AUSGERICHTET");
            
              m2(3);

        }          


} else { // schwellwert prüfen

           Serial.println("##########################   -- > Keine Aktion - zuviel Wolken");
           m1(3);
           m2(3);

}


}