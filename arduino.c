/*
  Implementarea unei retele de senzori wireless pentru monitorizarea unui teren agricol

  Agregarea valorilor primite de la diversi senzori de mediu ce sunt apoi transmise unui server central unde sunt stocate, analizate si dupa care afisate.
  Transmisia datelor este realizata prin intermediul modulului wireless ESP8266 ce se conecteaza la o retea wireless definita in prealabil.

  Circuitul contine:

  * O clona Arduino Pro Mini 5V
  * Modulul ESP8266
  * Senzorii de mediu

  10/03/2015
  Holicov Alexandru Gabriel

  http://proiect.holicov.com

*/

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DHT.h>
#include <stdlib.h>
#include <JsonParser.h>
#include <Narcoleptic.h>

using namespace ArduinoJson::Parser;
JsonParser<32> parser;

#define PIN_senzor 12 // Senzorul de umiditate / temperatura - Pin 12
#define TIP_senzor DHT22 // Tipul de senzor - DHT 22  (AM2302)
#define WIFI // Asigura comunicarea cu ESP8266
#define RESET_WIFI 10 // Asigura Hardware Reset pentru ESP8266 - Pin 10
#define DEBUG true

// Instructiuni de conectare la aplicatia
#define SSID "xxx" // Numele retelei wireless
#define PASS "xxx" // Parola retelei wireless
#define SITE "xxx" // Adresa IP a aplicatiei: 178.62.85.182 -> proiect.holicov.com

OneWire  ds(5);  // senzorrul de temperatura DS18B20 - Pin 5
SoftwareSerial wifi (3,2); // Software serial - RX - Pin 3, TX - Pin 2
DHT dht(PIN_senzor, TIP_senzor);


// Declarat variabile

char buffer[5];
bool conectat=false;

//

void setup()
{
  //initializam pinii de reset pentru modulul ESP8266 si il setam pe HIGH
  pinMode(RESET_WIFI, OUTPUT);
  digitalWrite(RESET_WIFI, HIGH);

  // Initializam portul serial la o viteza de 9600bps
  Serial.begin( 9600 );
  while (!Serial)
   {
    ; // Asteptam sa se conecteze portul serial
   }

  // Initializam comunicarea software serial - asigura comunicarea cu ESP8266
  wifi.begin( 9600 );

  //EEPROM
  // scrierePERM();
  //EEPROM
}


void sendWifi(String cmd)
{
  // Trimite comenzi catre interfata serial
  Serial.print(F("Comanda: "));
  Serial.println(cmd);
  delay(200);
  // Trimite comenzi catre interfata ESP8266
  wifi.println(cmd);
}

void curataSerial()
{
  while(wifi.available() > 0) {
    wifi.read();
  }
}

boolean wifiOK()
{
  // Asteapta raspuns de la modulul ESP8266
  int n = 0;
  boolean gasit = false;
  curataSerial(); // Curata portul serial
  do
  {
    if(wifi.find("OK"))
     {
       gasit = true; // Am primit raspuns de la ESP8266
     }
     n+=1;
  }
  while ((gasit == false) && (n<8));
}

void restartWIFI()
{
 // Restartam modulul Wifi
  digitalWrite(RESET_WIFI, LOW);
  delay(200);
  digitalWrite(RESET_WIFI, HIGH);
  delay(1000);
  sendWifi("AT+CWMODE=1");
  sendWifi("AT+CIPMUX=0");
  if(wifiOK())
  Serial.println("Trimis comenzi");
  delay(2000);
}

boolean connectWiFi()
{
  String cmd="AT+CWJAP=\"";
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  sendWifi(cmd);
  delay(2000);
  sendWifi("AT+CIFSR");  // Verificam daca modulul a primit IP prin DHCP

  if ( wifiOK() )  {
      Serial.println(F("ESP8266 a primit IP"));
      //wifiIP();
      conectat=true;
      return 1;
        }
    else {
    Serial.println(F("Nu am gasit ip"));
    conectat=false;
    return 0;
    }


}

void citireSenzori()
{
  //
  // Se cauta senzorul de temperatura DS18B20
  // Libraria 1Wire ne permite sa folosim zeci de senzori
  // conectati in retea, folosind ca si magistrala de date un singur fir.
  //
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  if ( !ds.search(addr)) {
  ds.reset_search();
  delay(250);
  return;
  }

 if (OneWire::crc8(addr, 7) != addr[7]) {
     Serial.println("CRC is not valid!");
     return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44);
  delay(1000);
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  for ( i = 0; i < 9; i++)
   {
  data[i] = ds.read();
  }


  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3;
    if (data[7] == 0x10) {
        raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;
    else if (cfg == 0x20) raw = raw & ~3;
    else if (cfg == 0x40) raw = raw & ~1;
  }

    // @@@@ Citim valorile senzorilor @@@@

    // Citim valoarea senzorlui DS18B20 - Temperatura sol
    celsius = (float)raw / 16.0;
    // Citim valoarea senzorului DHT22  - Temperatura & Umiditate Relativa
    dht.begin();
    // Citim valoarea senzorului LM-393 - Pin A0 - Umiditatea Solului
    int sensorValue = analogRead(A0);
    sensorValue = constrain(sensorValue, 485, 1023);
    //Transformam valoarea intr-un procent
    int soil=0;
    soil = map(sensorValue, 485, 1023, 100, 0);

     // @@@@ Citim valorile senzorilor @@@@


    #ifdef DEBUG

   Serial.print("Umiditatea aerului = ");
   Serial.print(dht.readHumidity());
   Serial.print("%");
   Serial.print("Temperatura aerului = ");
   Serial.print(dht.readTemperature());
   Serial.println("C");

   Serial.print("Umiditatea solului = ");
   Serial.print(soil);
   Serial.print("%");
   Serial.print("Temperatura solului = ");
   Serial.print(celsius);
   Serial.print("C ");
   Serial.println("\n\n");

    #endif

  // Pregatim valorile senzorilor pentru a le trimite catre aplicatia web
  // Valorile trebuie sa fie de tipul String

  String aumiditate;
  String atemperatura;
  String sumiditate;
  String stemperatura;

  sumiditate = String(soil);

  float AU = dht.readHumidity();
  float AT = dht.readTemperature();

  dtostrf(AU, 4, 2, buffer);
    for(int i=0;i<sizeof(buffer);i++)
   {
     aumiditate+=buffer[i];
   }

   dtostrf(AT, 4, 2, buffer);
    for(int i=0;i<sizeof(buffer);i++)
   {
     atemperatura+=buffer[i];
   }

  dtostrf(celsius, 4, 2, buffer);
   for(int i=0;i<sizeof(buffer);i++)
  {
    stemperatura+=buffer[i];
  }

 // dtostrf(soil, 3, 2, buffer);
 //  for(int i=0;i<sizeof(buffer);i++)
 // {
  //  sumiditate+=buffer[i];
//  }



  trimiteVal(aumiditate, atemperatura, sumiditate, stemperatura);

  }

void trimiteVal(String AU, String AT , String SU, String ST)
{
  curataSerial();
  //Aceasta functie este responsabila de conectarea si transmiterea valorilor senzorilor catre aplicatie
  // ESP8266 se conecteaza la IP-ul aplicatiei, se poate folosi numele domeniului - dar conexiunea pare mai stabila daca folosim adresa IP
  String cmd;
  cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += SITE;
  cmd += "\",80";
  sendWifi(cmd);

  delay(500);

  if( wifi.find( "Linked" ) )
  {
    Serial.println(F( "S-a realizat conexiunea" ));
  }

  /* Dimensiunile sirurilor

  lungpost -  Numarul total de caractere pe care ESP8266 il trimite catre aplicatie;
              valoarea este salvata in EEPROM cu ajutorul functiei scrierePERM();
  lungcerere - Lungimea cererii HTTP - Valoarea pentru campul Content Length - necesar pentru a face cererea POST;

  */
   int lungpost = EEPROM.read(257);
   int lungcerere = EEPROM.read(256);


  lungcerere+= AU.length() + AT.length() + SU.length() + ST.length(); // Dimensiunea cererii (content-length)

  // Adunam dimensiunile sirurilor
  String lungcereredim =String(lungcerere);
  lungpost+= lungcerere;
  lungpost+= lungcereredim.length();

  // AT+CIPSEND - trimite date catre ESP8266
  wifi.print( "AT+CIPSEND=" );
  wifi.println( lungpost ); // marimea cererii catre server

  // Asteptam raspuns de la ESP8266
  delay(500);
    if(wifi.find( ">" ) )
    {
    /*
     Construim cererea HTTP

     ESP8266 se conecteaza la IP-ul aplicatiei. Verificam daca am primit confirmare
     cum ca s-a realizat conexiunea (" > "). Dupa ce gasim ">" incepem sa transmitem cererea.
     IP + /api/senzor - adresa aplicatiei web;
     Secret - reprezinta parola aplicatiei;

     Specificam hostul, lungimea cererii si informam aplicatia de tipul de continut pe care urmeaza sa il trimitem.

     \r\n - reprezinta o linie noua

     Corpul cererii cuprinde valorile senzorilor si ID-ul nodului sub forma de titlu.


    */

    wifi.print(F("POST /api/senzor?secret=xxx HTTP/1.0\r\n"));
    wifi.print(F("Host: proiect.holicov.com\r\n"));
    wifi.print(F("Content-Length: "));
    wifi.print(lungcerere);
    wifi.print(F("\r\n"));
    wifi.print(F("Content-Type: application/x-www-form-urlencoded\r\n"));
    wifi.print(F("\r\n"));

    // Corpul cererii

    wifi.print(F("title=NODE1&type=senzori"));
    wifi.print(F("&field_stemp[und][0][value]="));
    wifi.print(ST);
    wifi.print(F("&field_sumiditate[und][0][value]="));
    wifi.print(SU);
    wifi.print(F("&field_atemp[und][0][value]="));
    wifi.print(AT);
    wifi.print(F("&field_aumiditate[und][0][value]="));
    wifi.print(AU);
    wifi.print("\r\n\r\n");

    #ifdef DEBUG

    Serial.print(F("POST /api/senzor?secret=xxx HTTP/1.0\r\n"));
    Serial.print(F("Host: proiect.holicov.com\r\n"));
    Serial.print(F("Content-Length: "));
    Serial.print(lungcerere);
    Serial.print(F("\r\n"));
    Serial.print(F("Content-Type: application/x-www-form-urlencoded\r\n"));
    Serial.print(F("\r\n"));

    Serial.print(F("title=NODE1&type=senzori"));
    Serial.print(F("&field_stemp[und][0][value]="));
    Serial.print(ST);
    Serial.print(F("&field_sumiditate[und][0][value]="));
    Serial.print(SU);
    Serial.print(F("&field_atemp[und][0][value]="));
    Serial.print(AT);
    Serial.print(F("&field_aumiditate[und][0][value]="));
    Serial.print(AU);
    Serial.print("\r\n\r\n");

    #endif

    }

    // Asteptam un raspuns din partea modului
    if( wifiOK() )
     {
    Serial.println( "Primit: OK" );
      }
    else
      {
    Serial.println( "Primit: Eroare\nNu a trimis datele" );
      }

    // Inchidem conexiunea
    sendWifi( "AT+CIPCLOSE" );

    #ifdef DEBUG
    Serial.println("Asteptam...");
    #endif


}


void wifiIP()
{
  // Verificam IP-ul modulului ESP8266
  wifi.println(F("AT+CIFSR"));
  wifiRaspuns();
}

void wifiRaspuns() //
{
  int n = 0;
   while ((n<3000) && (!wifi.available())) {
    n +=1; delay(1);
   }
   // Asteptam pana primim raspuns de la modulul Wifi
    while (wifi.available()) {
    Serial.write(wifi.read());
      }
}



void afisareRAM()
{
  #ifdef DEBUG
  Serial.print(F("Ram disponibil = "));
  Serial.println(freeRam());
  #endif
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void scrierePERM(){

// String lungimetemp =  "title=NODE1&type=senzori&field_stemp[und][0][value]=&field_sumiditate[und][0][value]=&field_atemp[und][0][value]=&field_aumiditate[und][0][value]=";
// int lungcerere = lungimetemp.length();

// String post= "POST /api/senzor?secret=xxx HTTP/1.0\r\nHost: proiect.holicov.com\r\nContent-Length: \r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n\r\n\r\n";
// int lungpost = post.length();

// Serial.println(lungcerere);
// Serial.println(lungpost);

// EEPROM.write(256, lungcerere);
// EEPROM.write(257, lungpost);

// delay(1000);

// int value = EEPROM.read(256);
// int value1 = EEPROM.read(257);
// Serial.print(value);
// Serial.print(value1);

}



void luatValori()
{
 /*

 Prin intermediul acestei functii modulul se conecteaza la aplicatia noastra si trimite o cerere HTTP pentru a primi inapoi o serie de setari.
 In felul acesta realizam o conversatie in ambele sensuri - un operator poate modifica de la distanta setarile modului.
 Aplicatia afiseaza un raspuns in format JSON.

 */
  curataSerial();

  String content = "";
  char character;

  String cmd;
  cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += SITE;
  cmd += "\",80";
  sendWifi(cmd);

  delay(500);

  if( wifi.find( "Linked" ) )
  {
    Serial.println(F( "S-a realizat conexiunea" ));
  }
  String post = "GET /citire/setari/control?title=NODE1\r\nAccept: application/xml\r\nContent-Type: application/xml\r\n\r\n";
  int lungpost = post.length();
  wifi.print( "AT+CIPSEND=" );
  wifi.println( lungpost ); // marimea cererii catre server

  delay(500);
    if(wifi.find( ">" ) )
    {
      //////
   wifi.print("GET /citire/setari/control?title=NODE1\r\nAccept: application/xml\r\nContent-Type: application/xml\r\n\r\n");

  unsigned int i = 0; //timer
  int n = 1; // numarul de caractere
  int bucla = 1;
  char json[100]="{";

  while (!wifi.find("NODE1\",")){

  bucla++;
   if(bucla > 10000)
    {return;}

  } // Cautam id-ul Nodului si citim restul sirului
  while (i<100) {
    if(wifi.available()) {
      char c = wifi.read();
      json[n]=c;
      if(c=='}') break;
      n++;
      i=0;
    }
    i++;
  }

  // Intarziere - intervalul la care se vor citi valorile senzorilor
  // Releu - starea releu

  String intarziere;
  String releu;
  JsonObject root = parser.parse(json);

  intarziere = root["intarziere"];
  releu = root["releu"];

  int sleepdelay = intarziere.toInt();
  int releustare = releu.toInt();

  Serial.println(sleepdelay);
  Serial.println(releustare);


  sendWifi( "AT+CIPCLOSE" );
  curataSerial();

  /*
   Inainte de a pasa valoarea in functia de sleep, verificam daca intervalul nu este corupt.
   Intarzierea trebuie sa fie cuprinsa intre 1 minut si 1440 de minute.
   In caz ca citirea a fost corupta pe parcurs, folosim 5 minute de sleep ca valoare de siguranta.
  */

  if (sleepdelay >=1 && sleepdelay <=1440){
  sleep(sleepdelay);
  }
  else {
  sleepdelay=5;
  sleep(sleepdelay);
       }

  }

}

void sleep(int intarziere){

  // Modulul va astepta pentru 5 secunds dupa care va intra intr-o bucla de somn.

  delay(5000);
  int n= intarziere * 8;

  for (byte i = 0; i <= n; ++i)
  {

   Narcoleptic.delay( 7500 );
  }


}

void loop() {

  afisareRAM();
  restartWIFI();
  if(connectWiFi())
  {
  citireSenzori();
   luatValori();
  }
  else {
    sleep(1);
  }

}
