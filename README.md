# esp8266-senseo-control
Einfache Senseo-Sterung mit Berücksichtigung von Heizzeit und Auto-Off bypass.


Ein Sketch, um eine Senseo-Padmaschine mit einem ESP8266 übers Handy via Webbrowser zu steuern.

Was kann man einstellen?
- Name der Maschine (kaffeemaschine: http://kaffeemaschine.local/)
- Aufheizzeit in Sekunden
- Dauer, wie lange der Kaffee rausläuft
- Auto-Off-Bypass in Sekunden, dass die Maschine nicht automatisch ausgeht

# Wie steuert man die Kaffeemaschine?
  - http://kaffeemaschine.local/turn_machine_on - Schaltet die Maschine an.

  - http://kaffeemaschine.local/turn_machine_off - Schaltet die Maschine aus, wenn aktuell kein Kaffee durchläuft.

  - http://kaffeemaschine.local/start_flow - Lässt Kaffee raus; Wenn Maschine aus ist, wird sie erst angeschaltet und es wird gewartet, bis sie heiß ist.


# Was brauche in an Hardware?

  - ESP8266 (z.B Wemos D1 Mini)
  - 2 Relais 

# Folgende Bibliotheken werden benötigt:
- <ESP8266WiFi.h>
- <WiFiClient.h>
- <ESP8266WebServer.h>
- <ESP8266mDNS.h>
- <NTPClient.h>



# [Video folgt]
