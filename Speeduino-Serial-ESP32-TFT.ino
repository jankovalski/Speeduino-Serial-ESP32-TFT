#include "FS.h"
#include "BluetoothSerial.h"
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "Comms.h"
#include "settings.h"
#include "reset.h"
#include "back.h"

#define CALIBRATION_FILE "/TouchCalData3"
#define REPEAT_CAL false
#define TFT_GREY 0x5AEB

/*
This goes to User_Setup.h of TFT_eSPI library
Some pins can be shared between screen and touch panel
#define TFT_DC    2
#define TFT_RST   4
#define TFT_CS   15
#define TFT_SCLK 18
#define TFT_MOSI 23
#define TOUCH_CS  5
#define TOUCH_DO 19
*/

Preferences Preferences;
TFT_eSPI tft = TFT_eSPI();
BluetoothSerial SerialBT;

// Speeduino bluetooth device address
uint8_t address[6] = {0x00, 0x11, 0x35, 0x21, 0x86, 0x02};
//uint8_t address[6] = {0x00, 0x11, 0x35, 0x82, 0x44, 0x36};
//uint8_t address[6] = {0x00, 0x11, 0x35, 0x82, 0x44, 0x14};
// Speeduino bluetooth device pin
char pin[] = "1337";

bool connected = 0, mainFilled = 0, settingsFilled = 0;
uint16_t x, y;
int screen, refreshRate, clt, iat, rpm, tps, lps, kpa, ge, ve, adv, iac, vss, sl;
float afr, pw, bat;

void setup()
{
	tft.init();
	tft.fillScreen(TFT_BLACK);
	tft.setRotation(1);
	tft.setSwapBytes(true);
	tft.drawString("Connecting...",20,20,4);

	SerialBT.begin("ESP32", true);
	SerialBT.setPin(pin);
	SerialBT.register_callback(callback);
	connected = SerialBT.connect(address);

	Preferences.begin("ESP32", false);
	refreshRate = Preferences.getInt("refreshRate", 30);
	Preferences.end();
	
	touchCalibrate();
}

void loop()
{
	if (screen == 0)
	{
		if (connected)
		{
			mainScreen();
		}
		else
		{
			tft.drawString("Connection failed",20,20,4);
			tft.drawString("Reconnecting...",20,60,4);
			delay(2000);
			ESP.restart();
		}
		if (tft.getTouch(&x, &y))
		{
			if ((x > 10) && (x < (10 + 32)))
			{
				if ((y > 198) && (y <= (198 + 32)))
				{
					ESP.restart();
				}
			}
			else if ((x > 278) && (x < (278 + 32)))
			{
				if ((y > 198) && (y <= (198 + 32)))
				{
					mainFilled = false;
					screen = 1;
					return;
				}
			}
		}
	}
	else if (screen == 1)
	{
		settingsScreen();
	}
}

void mainScreen()
{
	if (!mainFilled)
	{
		fillMainScreen();
	}
	
	static uint32_t lastUpdate = millis();
	if (millis() - lastUpdate > 10)
	{
		requestData(50);
		lastUpdate = millis();
	}

	rpm = getWord(14);
	kpa = getWord(4);
	tps = getByte(24)/2;
	afr = getByte(10)/10.0;
	clt = getByte(7) - 40;
	iat = getByte(6) - 40;
	ge = (int16_t)getByte(17);
	ve = getByte(18);
	adv = (int8_t)getByte(23);
	pw = getWord(20)/1000.0;
	bat = getByte(9)/10.0;
	iac = getByte(37);
	lps = getWord(25);
	vss = getWord(100);
	sl = getByte(122);
	
	if (rpm > 10000) ESP.restart();

	tft.setTextPadding(tft.textWidth("8888", 4));
	
	// FIRST LINE

	tft.drawNumber(rpm,110,18,4);
	tft.drawNumber(kpa,215,18,4);
	tft.drawNumber(tps,317,18,4);
	
	// SECOND LINE	

	tft.drawFloat(afr,1,110,56,4);
	tft.drawNumber(clt,215,56,4);
	tft.drawNumber(iat,317,56,4);

	// THIRD LINE

	tft.drawNumber(ge,110,93,4);
	tft.drawNumber(ve,215,93,4);
	tft.drawNumber(adv,317,93,4);

	// FOURTH LINE

	tft.drawFloat(pw,1,110,130,4);
	tft.drawFloat(bat,1,215,130,4);
	tft.drawNumber(iac,317,130,4);

	// FIFTH LINE

	tft.drawNumber(vss,215,167,4);
	tft.drawNumber(sl,317,167,4);
	tft.setTextPadding(tft.textWidth("88888", 4));
	tft.drawNumber(lps,110,167,4);

	if (refreshRate == 1) delay(1000);
	else if (refreshRate == 5) delay(200);
	else if (refreshRate == 10) delay(100);
	else if (refreshRate == 15) delay(66);
	else if (refreshRate == 30) delay(33);
	else if (refreshRate == 100) delay(10);
}

void settingsScreen()
{
	if (!settingsFilled)
	{
		fillSettingsScreen();
	}
	if (tft.getTouch(&x, &y))
	{
		if ((x > 5) && (x < (5 + 38)))
		{
			if ((y > 5) && (y <= (5 + 32)))
			{
				settingsFilled = false;
				screen = 0;
				return;
			}
		}
		else if ((x > 170) && (x < (170 + 40)))
		{
			if ((y > 54) && (y <= (54 + 40)))
			{
				if (refreshRate == 100) refreshRate = 30;
				else if (refreshRate == 30) refreshRate = 15;
				else if (refreshRate == 15) refreshRate = 10;
				else if (refreshRate == 10) refreshRate = 5;
				else if (refreshRate == 5) refreshRate = 1;
				redrawRefreshRate(true);
			}
		}
		else if ((x > 270) && (x < (270 + 40)))
		{
			if ((y > 54) && (y <= (54 + 40)))
			{
				if (refreshRate == 1) refreshRate = 5;
				else if (refreshRate == 5) refreshRate = 10;
				else if (refreshRate == 10) refreshRate = 15;
				else if (refreshRate == 15) refreshRate = 30;
				else if (refreshRate == 30) refreshRate = 100;
				redrawRefreshRate(true);
			}
		}
		if ((x > 240) && (x < (240 + 70)))
		{
			if ((y > 4) && (y <= (4 + 35)))
			{
				tft.setTextColor(TFT_GREEN, TFT_BLACK);
				tft.drawString("SAVE",259,22,2);
				Preferences.begin("ESP32", false);
				Preferences.putInt("refreshRate", refreshRate);
				Preferences.end();
				delay(500);
				tft.setTextColor(TFT_CYAN, TFT_BLACK);
				tft.drawString("SAVE",259,22,2);
			}
		}
	}
}

void fillMainScreen()
{
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_CYAN, TFT_BLACK);
	tft.setTextDatum(MR_DATUM);
	tft.drawLine(115,0,115,182,TFT_GREY);
	tft.drawLine(220,0,220,182,TFT_GREY);
	tft.drawLine(0,34,320,34,TFT_GREY);
	tft.drawLine(0,71,320,71,TFT_GREY);
	tft.drawLine(0,108,320,108,TFT_GREY);
	tft.drawLine(0,145,320,145,TFT_GREY);
	tft.drawLine(0,182,320,182,TFT_GREY);
	tft.pushImage(10,198,32,32,reset);
	tft.pushImage(278,198,32,32,settings);
	
	tft.setTextPadding(0);
	tft.drawString("RPM",35,16,2);
	tft.drawString("MAP",155,16,2);
	tft.drawString("TPS",260,16,2);
	tft.drawString("AFR",33,54,2);
	tft.drawString("CLT",150,54,2);
	tft.drawString("IAT",255,54,2);
	tft.drawString("GE",25,91,2);
	tft.drawString("VE",142,91,2);
	tft.drawString("ADV",258,91,2);
	tft.drawString("PW",27,128,2);
	tft.drawString("BAT",148,128,2);
	tft.drawString("IAC",255,128,2);
	tft.drawString("LPS",33,165,2);
	tft.drawString("VSS",148,165,2);
	tft.drawString("SL",250,165,2);

	mainFilled = true;
}

void fillSettingsScreen()
{
	tft.fillScreen(TFT_BLACK);
	tft.setTextColor(TFT_CYAN, TFT_BLACK);
	tft.setTextDatum(ML_DATUM);
	tft.pushImage(5,5,38,32,back);
	tft.drawString("SETTINGS",64,25,4);
	tft.drawLine(0,44,320,44,TFT_GREY);

	tft.setTextPadding(0);
	tft.drawString("Refresh (Hz)",10,75,4);
	tft.drawRoundRect(170,54,40,40,10,TFT_WHITE);
	tft.drawString("-",186,75,4);
	redrawRefreshRate(false);
	tft.drawRoundRect(270,54,40,40,10,TFT_WHITE);
	tft.drawString("+",284,75,4);

	tft.drawRoundRect(240,4,70,35,10,TFT_WHITE);
	tft.drawString("SAVE",259,22,2);

	settingsFilled = true;
}

void redrawRefreshRate(bool doDelay)
{
	tft.setTextDatum(MR_DATUM);
	tft.setTextPadding(tft.textWidth("888", 4));
	tft.drawNumber(refreshRate,258,75,4);
	tft.setTextDatum(ML_DATUM);
	tft.setTextPadding(0);
	if (doDelay) delay(50);
}

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
	if (event == ESP_SPP_CLOSE_EVT)
	{
		screen = -1;
		tft.fillScreen(TFT_BLACK);
		tft.drawString("Connection lost",20,20,4);
		tft.drawString("Reconnecting...",20,60,4);
		delay(1000);
		ESP.restart();
	}
}

void touchCalibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}