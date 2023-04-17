//definiamo i servomotori orizzontale e verticale
Servo servohori;
int servoh = 0;
int servohLimitHigh = 160;
int servohLimitLow = 60;

Servo servoverti; 
int servov = 0; 
int servovLimitHigh = 160;
int servovLimitLow = 60;
//Pin fotoresistenze
int ldrtopl = 2; //top left 
int ldrtopr = 1; //top right 
int ldrbotl = 3; // bottom left 
int ldrbotr = 0; // bottom right 

 void setup () 
 {
  servohori.attach(10);
  servohori.write(60);
  servoverti.attach(9);
  servoverti.write(60);
  Serial.begin(9600);
  delay(500);
  
 }

void loop()
{
  servoh = servohori.read();
  servov = servoverti.read();

  //Valore Analogico delle fotoresistenza
  int oben_links  = analogRead(ldrtopl);
  int oben_rechts = analogRead(ldrtopr);
  int unten_links = analogRead(ldrbotl);
  int unten_rechts = analogRead(ldrbotr);
  // Calcoliamo una Media
  int avgtop = (topl + topr) ; //Durchschnitt von rauf 
  int avgbot = (botl + botr) ; //Durchschnitt von runter 
  int avgleft = (topl + botl) ; //Durchschnitt von links 
  int avgright = (topr + botr) ; //Durchschnitt von rechts 

  
  // Oben Unten ausrichten
  if (avgtop < avgbot)
  {
        // Nach oben ausrichten

  }
  else if (avgbot < avgtop)
  {
        // Nach unten ausrichten

  }
  else 
  {
        // ?
  }
  




  // Rechts / Links ausrichten
  if (avgleft > avgright)
  {
        // rechts

  }
  else if (avgright > avgleft)
  {
        // links


  }
  else 
  {



  }



}