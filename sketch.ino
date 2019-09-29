/*
*/
#include "EEPROM.h"
#include <avr/wdt.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x25, 20, 4);
#include <OneWire.h>
#include <DallasTemperature.h>
OneWire oneWire(7);
DallasTemperature sensors(& oneWire);
DeviceAddress sondeRouge =
{
  0x28, 0x66, 0x1F, 0x79, 0x97, 0x18, 0x03, 0x6D
};

DeviceAddress sondeVerte =
{
  0x28, 0xEE, 0xBA, 0x79, 0x97, 0x18, 0x03, 0xC7
};

#include <Encoder.h>
Encoder myEnc(3, 2);
#define bp 5
byte etatBp;
float position;
#define resistance 8
#define compresseur 9
byte choix;
boolean etatDuChoix = false;
boolean choixFlag = false;
boolean FlagErreur = false;
unsigned long previousMillis = 0;
const long intervalAffichageTemperature = 5000;
float tempChauffe, tempFroid;
float consigneresistance = 22;
float consigneFroid = 2;
float hysteresyFroid = 0.50;
float hysteresyresistance = 0.50; 
#define Led 6
int brightness = 0;
int fadeAmount = 10;
int caseFroid = 80;
int caseChaud = 90;
void setup()
{
  wdt_enable(WDTO_8S);
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  sensors.setResolution(sondeRouge, 12);
  sensors.setResolution(sondeVerte, 12);
  pinMode(bp, INPUT_PULLUP);
  pinMode(resistance, OUTPUT);
  pinMode(compresseur, OUTPUT);
  pinMode(Led, OUTPUT);
  delay(5000);
  digitalWrite(resistance, LOW);
  digitalWrite(compresseur, LOW);
  digitalWrite(Led, HIGH);
  sensors.requestTemperatures();
  tempChauffe = sensors.getTempC(sondeRouge);
  tempFroid = sensors.getTempC(sondeVerte);
  Serial.println(tempChauffe);
  Serial.println(tempFroid);
  // EEPROM
  consigneresistance = EEPROM.read(caseChaud);
  consigneFroid = EEPROM.read(caseFroid);
}

void loop()
{
  wdt_reset();
  if (tempChauffe > 25 || tempFroid < 0 || tempChauffe < -50 && FlagErreur == 0)
  {
    digitalWrite(Led, LOW);
    digitalWrite(resistance, LOW);
    digitalWrite(compresseur, LOW);
    erreur();
  }
  else
  {
    FlagErreur = 1;
    fade();
    affichagelcd();
    consigneElement();
    choixEtEEprom();
  }
  delay(10);
}

void erreur()
{
  digitalWrite(Led, 1);
  lcd.setCursor(6, 1);
  lcd.print("        ");
  lcd.clear();
  delay(200);
  lcd.setCursor(4, 1);
  lcd.print("!!!ERREUR!!! ");
  digitalWrite(Led, 0);
  delay(500);
}

void fade()
{
  analogWrite(Led, brightness);
  brightness = brightness + fadeAmount;
  if (brightness <= 0 || brightness >= 200)
  {
    fadeAmount = -fadeAmount;
  }
}

void choixEtEEprom()
{
  etatBp = digitalRead(bp);
  if (etatBp == LOW)
  {
    etatDuChoix = !etatDuChoix;
    delay(500);
    choixFlag = 0;
  }
  if (etatDuChoix == 0)
  {
    if (choixFlag == 0)
    {
      myEnc.write(consigneFroid * 4);
      EEPROM.write(caseChaud, consigneresistance);
      choixFlag = 1;
    }
    lcd.setCursor(6, 1);
    lcd.print(" ");
    lcd.setCursor(6, 3);
    lcd.print((char) 62);
    consigneFroid = myEnc.read() / 4;
  }
  else
  {
    // chauffe
    if (choixFlag == 0)
    {
      myEnc.write(consigneresistance * 4);
      EEPROM.write(caseFroid, consigneFroid);
      choixFlag = 1;
    }
    lcd.setCursor(6, 3);
    lcd.print(" ");
    lcd.setCursor(6, 1);
    lcd.print((char) 62);
    consigneresistance = myEnc.read() / 4;
  }
}

void affichagelcd()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalAffichageTemperature)
  {
    sensors.requestTemperatures();
    tempChauffe = sensors.getTempC(sondeRouge);
    tempFroid = sensors.getTempC(sondeVerte);
    lcd.setCursor(0, 0);
    lcd.print("Chaud: ");
    lcd.print(tempChauffe);
    lcd.print((char) 223);
    lcd.print("C");
    lcd.print("  ");
    lcd.setCursor(0, 2);
    lcd.print("Froid: ");
    lcd.print(tempFroid);
    lcd.print((char) 223);
    lcd.print("C");
    lcd.print("  ");
    previousMillis = currentMillis;
  }
  lcd.setCursor(0, 1);
  lcd.print("Consi:");
  lcd.setCursor(7, 1);
  if (consigneresistance < 10)
  {
    lcd.print('0');
  }
  lcd.print(consigneresistance);
  lcd.print((char) 223);
  lcd.print("C");
  lcd.print("  ");
  lcd.setCursor(0, 3);
  lcd.print("Consi:");
  lcd.setCursor(7, 3);
  if (consigneFroid < 10)
  {
    lcd.print('0');
  }
  lcd.print(consigneFroid);
  lcd.print((char) 223);
  lcd.print("C");
  lcd.print("  ");
}

void consigneElement()
{
  if (tempFroid <= consigneFroid - hysteresyFroid)
  {
    digitalWrite(compresseur, 0);
    lcd.setCursor(17, 3);
    lcd.print("off");
  }
  if (tempFroid >= consigneFroid + hysteresyFroid)
  {
    digitalWrite(compresseur, 1);
    lcd.setCursor(17, 3);
    lcd.print("on ");
  }
  if (tempChauffe >= consigneresistance + hysteresyresistance)
  {
    digitalWrite(resistance, 0);
    lcd.setCursor(17, 1);
    lcd.print("off");
  }
  // 18             20.       .25
  if (tempChauffe <= consigneresistance - hysteresyresistance)
  {
    digitalWrite(resistance, 1);
    lcd.setCursor(17, 1);
    lcd.print("on ");
  }
}

void reboot()
{
  while (1);
}
