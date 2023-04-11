#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
int sensor = 11;
unsigned long start_time = 0;
unsigned long end_time = 0;
int steps=0;
float steps_old=0;
float temp=0;
float rps=0;

void setup() 
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(sensor,INPUT_PULLUP);
  lcd.setCursor(0,0);
  lcd.print(" STEPS - 0");
  lcd.setCursor(0,1);
  lcd.print(" RPS   - 0.00");
}
 
void loop()
{
 start_time=millis();
 end_time=start_time+1000;
 while(millis()<end_time)
 {
   if(digitalRead(sensor))
   {
    steps=steps+1; 
    while(digitalRead(sensor));
   }
   lcd.setCursor(9,0);
   lcd.print(steps);
   lcd.print("   ");
 }
    temp=steps-steps_old;
    steps_old=steps;
    rps=(temp/20);
    lcd.setCursor(9,1);
    lcd.print(rps);
    lcd.print("   ");
}