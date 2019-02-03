#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>

#define NTP_OFFSET   60 * 60    
#define NTP_INTERVAL 60 * 1000
#define NTP_ADDRESS  "europe.pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

MDNSResponder mdns;

/*
 *  WLAN-Zugangsdaten ändern 
 *  ssid    -> Netzwerkname
 *  password    -> Passwort furs Netzwerk
 *  machine_name  -> Name des ESPs im Netzwerk; http://kaffeemaschine.local/      
 */

const char* ssid = "WLAN_SSID";
const char* password = "WLAN_PASSWORT";
const char* machine_name = "Kaffeemaschine";



ESP8266WebServer server(80);

/*
 *  Pin-Belegung der beiden Relais.
 */

int toggle_power = 15;    //  Pin D8 auf dem ESP
int toggle_coffee = 5;  //    Pin D1 auf dem ESP

/*
 *  Standardwerte der einzelnen Zustände.
 */

int machine_is_on = 0;
int machine_is_hot = 0;
int flow_active = 0;
int flow_proceeded = 0;
int flow_pressed = 0;

int start_heating_at = 0;
int machine_turned_on_at = 0;
int flow_pressed_at = 0;


/*
 *  Folgende Variablen müssen angepasst werden, sie bestimmen die Aufheizzeit sowie die Zeit wie
 *  lange die Maschine braucht, um eine Tasse Kaffee rauszulassen.
 *  time_to_heat_up   -> Zeit in Sekunden, welche die Maschine braucht zum Aufheizen.
 *  time_flow_active  -> Zeit in Sekunden bis der Kaffee fertig rausgelaufen ist.
 *  auto_off_bypass -> Zeit in Sekunden, bis die Maschine kurz aus- und wieder eingeschaltet wird 
 *        um Auto-Off zu verhindern.
 */


int time_to_heat_up = 20; //90
int time_flow_active = 10; //50
int auto_off_bypass = 50; //300


//Dieser Code wird ausgeführt, wenn man den ESP mal resettet, also z.B. mal vom Strom nimmt.

void setup(void){  
  timeClient.begin();
  
  //Beide Pins der Relais als Output definieren, und anschließend gleich auf LOW schalten.

  pinMode(toggle_power, OUTPUT);
  digitalWrite(toggle_power, LOW);
  pinMode(toggle_coffee, OUTPUT);
  digitalWrite(toggle_coffee, LOW);
 
  Serial.begin(115200); 
  delay(5000);

  //Einstellungen für die WLAN-Verbindung

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  Serial.println("Verbindet mit Netzwerk…");
  Serial.print("Verbunden mit: ");
  Serial.println(ssid);
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // DNS-Service für http://kaffeemaschine.local/ ;zusätzlich zur IP-Adresse 192.168.x.x
  if (mdns.begin(machine_name, WiFi.localIP())) {
    Serial.println("MDNS responder gestartet.");
  }

  // Was passiert beim Aufrufen von http://kaffeemaschine.local/
  // Einfache Textausgabe „Senseo Steuerung“.
  server.on("/", [](){
    server.send(200, "text/html", "Senseo Steuerung");
  });

  // Was passiert beim Aufrufen von http://kaffeemaschine.local/turn_machine_on
  // Methode turn_machine_on(); wird ausgeführt
  server.on("/turn_machine_on", [](){
    server.send(200, "text/html", "Maschine wurde eingeschaltet!");
    turn_machine_on();
  });

  // Was passiert beim Aufrufen von http://kaffeemaschine.local/turn_machine_off
  // Methode turn_machine_off(); wird ausgeführt
  server.on("/turn_machine_off", [](){
    server.send(200, "text/html", "Maschine wurde ausgeschaltet!");
    turn_machine_off();
  });

  // Was passiert beim Aufrufen von http://kaffeemaschine.local/start_flow
  // Variable flow_pressed wird auf 1 gesetzt, somit kann unser loop() damit arbeiten
  server.on("/start_flow", [](){
    server.send(200, "text/html", "Kaffee wird rausgelassen");
    flow_pressed = 1;
  });

  // Was passiert beim Aufrufen von http://kaffeemaschine.local/machine_status
  // Ausgabe von 0 oder 1 durch die Variable machine_is_on
  server.on("/machine_status", [](){
    server.send(200, "text/html", String(machine_is_on));
  });

  
  server.begin();
  Serial.println(String(machine_name) + " ist online und bereit.");
}


//Definitionen unserer einzelnen Funktionen

// Funktion, um alle Variablen wieder auf ihre Standardwerte zu setzen
void reset_variables(){
  machine_is_on = 0;
  machine_is_hot = 0;
  flow_active = 0;
  flow_proceeded = 0;
  flow_pressed = 0;

  start_heating_at = 0;
  machine_turned_on_at = 0;
  flow_pressed_at = 0;

}

void turn_machine_on(){
  // Nur wenn die Maschine aus ist, dann soll sie eingeschaltet werden.
  if(machine_is_on == 0){
   
    // Zeitstempel setzen, an dem die Maschine eingeschaltet wurde
    // und somit auch mit dem Heizvorgang beginnt.
    machine_turned_on_at = timeClient.getEpochTime();
    start_heating_at = timeClient.getEpochTime();

    // Relais wird geschalten, Maschine geht an, Variable machine_is_on = 1.
    digitalWrite(toggle_power, HIGH);
    delay(200);
    digitalWrite(toggle_power, LOW);
    machine_is_on = 1;
    Serial.println("Maschine wurde eingeschaltet!");
  }else{  
    // Ansonsten soll er die Maschine nicht
    // nochmal anschalten, um ein Aus- statt Einschalten zu verhindern.
    Serial.println("Maschine ist bereits an!");
  } 
}

void turn_machine_off(){
  // Nur wenn die Maschine an ist, dann soll sie ausgeschaltet werden.
  if(machine_is_on == 1 && flow_active == 0){
  // Wenn Maschine ausgeschaltet wird, bekommen alle Variablen, die im Betrieb andere 
  // Werte bekommen, wieder ihre Standardwerte.
    reset_variables();
    digitalWrite(toggle_power, HIGH);
    delay(200);
    digitalWrite(toggle_power, LOW);
    machine_is_on = 0;
    Serial.println("Maschine wurde ausgeschaltet!");
  }else{
    if(machine_is_on == 0){
      // Ansonsten soll er die Maschine nicht
      // nochmal ausschalten, um ein Ein- statt Ausschalten zu verhindern.
      Serial.println("Maschine ist bereits aus!");
    }else{
      Serial.println("Es wird gerade ein Kaffee rausgelassen! Maschine wird nicht ausgeschaltet."); 
    }
  }
}

void start_flow(){
  // 2-Tassen-Taster betätigen durch Relais.
  digitalWrite(toggle_coffee, HIGH);
  delay(200);
  digitalWrite(toggle_coffee, LOW);
  // Zeitstempel, an dem der 2-Tassen-Taster betätigt wurde.
  flow_pressed_at = timeClient.getEpochTime();
  // Kaffee wird rausgelassen, flow_active auf = 1 setzen.
  flow_active = 1;
  Serial.println("Kaffee wird rausgelassen!");
  // Sobald wir einen Kafffee rauslassen, ist die Maschine „nicht mehr heiß“. 
  machine_is_hot = 0; 
}


// Dieser Code wird in Dauerschleife ausgeführt, sobald unsere setup()-Methode ausgeführt wurde.

void loop(void){

  server.handleClient();
  timeClient.update();
  
  // actual_timestamp im loop() immer wieder aktualisieren
  int actual_timestamp = timeClient.getEpochTime();
  
  // Jetzt wird geprüft ob, und wann die Maschine heiß ist
  // Wenn Maschine an ist UND kein Kaffee rausläuft UND Maschine kalt ist,...
  if(machine_is_on == 1 && flow_active == 0 && machine_is_hot == 0){
    // ...,dann warten bis die Vorheizzeit erreicht ist.
    // Differenz aktueller Zeitstempel und Zeitstempel Start des Aufheizen muss >= 
    // Vorheizzeit sein
    if(actual_timestamp - start_heating_at >= time_to_heat_up){
      // Wenn das zutrifft, ist unsere Maschine heiß.
      machine_is_hot = 1;
      Serial.println("Maschine ist heiß!");
    } 
  }
  
   
  // Hier wird die Auto-Off-Funktion der Kaffeemaschine umgangen
  // Wenn Maschine an ist UND Maschine heiß ist UND kein Kaffee durchläuft     
  if(machine_is_on == 1 && machine_is_hot == 1 && flow_active == 0){
    // und die Maschine länger als auto_off_bypass an ist,
    if(actual_timestamp - machine_turned_on_at >= auto_off_bypass){
      // Dann die Maschine kurz aus- und wieder einschalten, um Auto-Off zu 
      // verhindern.
      digitalWrite(toggle_power, HIGH);
      delay(200);
      digitalWrite(toggle_power, LOW);
      delay(200);
      digitalWrite(toggle_power, HIGH);
      delay(200);
      digitalWrite(toggle_power, LOW);
      // Maschine wurde erneut eingeschaltet, neuen Zeitstempel setzen
      machine_turned_on_at = timeClient.getEpochTime();
      Serial.println("Maschine wurde kurz aus- und wieder angeschaltet!");
    }     
  }

    



  // Hier wird das Rauslassen des Kaffees initialisiert
  // Wurde der 2-Tassen_Taster durch server.on("/start_flow", []()) auf = 1 gesetzt, ...
  if(flow_pressed == 1){
    // ... dann wird erst gecheckt, ob die Maschine an ist.
    if(machine_is_on == 0){
      // Wenn nein, dann wird die Maschine eingeschaltet und dadurch ebenfalls 
      // das Vorheizen etc. eingeleitet; durch turn_machine_on();
      Serial.println("Taste für zwei Tassen wurde angefordert!");
      turn_machine_on();  
    }
    
    if(machine_is_hot == 1){
      // Wenn die Machine heiß ist, wird start_flow(); ausgeführt
      start_flow();
              
    } 
  } 



  // Wenn der 2-Tassen-Taster schon mal betätigt wurde, ...
  if(flow_pressed_at != 0){
    // dann heizt die Maschine noch nicht, da der Kaffee ja erstmal rauslaufen muss.
    // Wenn der aktueller Zeitstempel größer ist, als der Zeitstempel an dem gedrückt 
    // wurde, + der Zeit in der der Kaffee rausläuft, beginnt die eigtl. Aufheizzeit.
    if(actual_timestamp >= (flow_pressed_at + time_flow_active)){
      // Nachdem der Kaffee fertig ist, die Aufheizzeit einleiten
      start_heating_at = timeClient.getEpochTime();
      // Taster-Zeitstempel wieder zurücksetzen
      flow_pressed_at = 0;
      flow_pressed = 0;    
    }
  }



  // Wann wissen wir, dass der Kaffee fertig ist und somit flow_active = 0 gesetzt werden kann?
  // Wenn gerade ein Kaffee rausläuft ...
  if(flow_active == 1){
    // ... und Differenz zwischen aktuellem Zeitstempel und Betätigung des Tasters >=
    // Dauer während der Kaffee rausläuft ist,
    if((actual_timestamp - flow_pressed_at) >= time_flow_active){
      // wissen wir, dass der Kaffee fertig ist.
      flow_active = 0;
      Serial.println("Kaffee ist fertig!");    
    }  
  }    
}
