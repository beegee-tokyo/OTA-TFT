#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <TFT_eSPI.h>

/** Build time */
const char compileDate[] = __DATE__ " " __TIME__;
/** OTA progress */
int otaStatus = 0;
/** TFT_eSPI class for display */
TFT_eSPI tft = TFT_eSPI();

/** Function definition */
void activateOTA();

void setup() {
	// Start serial connection
	Serial.begin(115200);

	// Initialize TFT screen
	tft.init();
	// Clear screen
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 40);
	tft.setTextColor(TFT_WHITE);

	// Put some information on the screen
	tft.println("Build: ");
	tft.setTextSize(1);
	tft.println(compileDate);

	// Connect to WiFi
	WiFi.mode(WIFI_STA);
	WiFi.begin("MHC2", "teresa1963");
	uint32_t startTime = millis();
	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		if (millis()-startTime > 30000) { // wait maximum 30 seconds for a connection
			tft.println("Failed to connect to WiFI");
			tft.println("Rebooting in 30 seconds");
			delay(30000);
			esp_restart();
		}
	}
	// WiFi connection successfull
	tft.println("Connected to ");
	tft.println(WiFi.SSID());
	tft.println("with IP address ");
	tft.println(WiFi.localIP());

	// Activate OTA
	activateOTA();
	tft.println("\n\nWaiting for OTA to start");
}

void loop() {
	ArduinoOTA.handle();
}

/**
 * Activate OTA
 */
void activateOTA() {
	uint8_t baseMac[6];
	// Get MAC address for WiFi station
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	char baseMacChr[19] = {0}; 
	sprintf(baseMacChr, "ESP32-%02X%02X%02X%02X%02X%02X"
		, baseMac[0], baseMac[1], baseMac[2]
		, baseMac[3], baseMac[4], baseMac[5]);
	baseMacChr[18] = 0;

	ArduinoOTA
		.setHostname(baseMacChr)
		.onStart([]() {
			/**********************************************************/
			// Close app tasks and sto p timers here
			/**********************************************************/
			// Prepare LED for visual signal during OTA
			pinMode(16, OUTPUT);
			// Clear the screen and print OTA text
			tft.fillScreen(TFT_BLUE);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(TFT_WHITE);
			tft.setTextSize(2);
			tft.drawString("OTA",64,50);
			tft.drawString("Progress:",64,75);
		})
		.onEnd([]() {
			// Clear the screen and print OTA finished text
			tft.fillScreen(TFT_GREEN);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(TFT_BLACK);
			tft.setTextSize(2);
			tft.drawString("OTA",64,50);
			tft.drawString("FINISHED!",64,80);
			delay(10);
		})
		.onProgress([](unsigned int progress, unsigned int total) {
			// Calculate progress
			unsigned int achieved = progress / (total / 100);
			// Update progress every 1 %
			if (otaStatus == 0 || achieved == otaStatus + 1) {
				// Toggle the LED
				digitalWrite(16, !digitalRead(16));
				otaStatus = achieved;
				// Print the progress
				tft.setTextDatum(MC_DATUM);
				tft.setTextSize(2);
				tft.fillRect(32,91,64,28,TFT_BLUE);
				// String progVal = String(achieved) + "%";
				String progVal = String(progress) + "%";
				tft.drawString(progVal,64,105);
			}
		})
		.onError([](ota_error_t error) {
			// Clear the screen for error message
			tft.fillScreen(TFT_RED);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(TFT_WHITE);
			// Print error message
			tft.setTextSize(2);
			tft.drawString("OTA",64,60);
			tft.drawString("ERROR:",64,90);
			// Get detailed error and print error reason
			if (error == OTA_AUTH_ERROR) {
				tft.drawString("Auth Failed",64,120);
			}
			else if (error == OTA_BEGIN_ERROR) {
				tft.drawString("Begin Failed",64,120);
			}
			else if (error == OTA_CONNECT_ERROR) {
				tft.drawString("Connect Failed",64,120);
			}
			else if (error == OTA_RECEIVE_ERROR) {
				tft.drawString("Receive Failed",64,120);
			}
			else if (error == OTA_END_ERROR) {
				tft.drawString("End Failed",64,120);
			}
            otaStatus = 0;
		});

	// Initialize OTA
	ArduinoOTA.begin();

	// Add some extra service text to the mDNS service
	MDNS.addServiceTxt("_arduino", "_tcp", "service", "OTA-TEST");
	MDNS.addServiceTxt("_arduino", "_tcp", "type", "ESP32");
	MDNS.addServiceTxt("_arduino", "_tcp", "id", "BeeGee");
}
