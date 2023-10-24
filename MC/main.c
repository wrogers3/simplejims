#include <WiFi.h>

void setup()
{
    // .begin(speed)
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    Serial.println("setup done");
}

void loop() 
{
    Serial.println("start scanning");
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
	// no wifi 
	Serial.println("no wifi networks found");
    } else {
	Serial.print(n);
	for (int i = 0; i < n; ++i) {
	    // SSID RSSI for each network
	    Serial.print(i + 1);
	    Serial.print(": ");
	    Serial.print(WiFi.SSID(i));
	    Serial.print(" (");
	    Serial.print(WiFi.RSSI(i));
	    Serial.print(")");
	    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
	}
    }
    Serial.println("");

     
}
