

/**
 * Needed to add 6 time zones.
 * Each time zone have start time and set temperature.
 * Time in hours (integer)
 * setPoint in float.
 *
 * Class timeZone{
 *  startTime: integer;
 *
 * }
 */

#include <DallasTemperature.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


#include <EEPROM.h>
//#include <arduino.h>


#include "WC_HTTP.h"
#include "WC_NTP.h"
#include "WC_EEPROM.h"

// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5
#define ONE_WIRE_BUS 5

#define SHEDULE_PRESET_COUNT 6 // D2

#define ON 1
#define OFF 0;

//class TimeZone {
//  public:
////    int position;
//    int startTime;
//    int endTime;
//    float setPoint;
//    bool isActive = false;
//  public:
//    TimeZone (int startT, int endT, float setP) {
//      startTime = startT;
//      endTime = endT;
//      setPoint = setP;
//    }
//};


class TermostatSheduler {
    // TimeZone timeZones[SHEDULE_PRESET_COUNT] = {TimeZone(5, 12, 21), TimeZone(12, 18, 19), TimeZone(18, 5, 25)};
  public:
    T_ScheduleTimePreset ScheduleTimePreset[6];

    int hour = 0;
    int minute = 0;
    int i;

    int getHour() {
      return hour;
    }

    void updateMinite() {
      minute = minute + 1;
      if (minute >= 60) {
        hour = hour + 1;
        minute = 0;
      }
      if (hour >= 24) {
        hour = 0;
      }
//      Serial.println(hour);
//      Serial.println(minute);
    }

    float getCurrentSetPoint() {
      for (i = 0; i < SHEDULE_PRESET_COUNT; i = i + 1 )
      {

        if (ScheduleTimePreset[i].isActive == true) {
//          Serial.println(i);
          if (ScheduleTimePreset[i].startTime > ScheduleTimePreset[i].endTime) {
            if ((ScheduleTimePreset[i].startTime <= hour)
                || (ScheduleTimePreset[i].endTime > hour)) {
//              Serial.println(i);
              return ScheduleTimePreset[i].setPoint;
            };
          } else {
            if ((ScheduleTimePreset[i].startTime <= hour)
                && (ScheduleTimePreset[i].endTime > hour)) {
//              Serial.println(i);
              return ScheduleTimePreset[i].setPoint;
            };
          };
        }

      };
    };
};

class Termostat {
    float temperature;
    float hysteresis = 1;
    float setpoint = 24;

    int outputPort;

  public:
    bool heatingOn = false;
    bool state;

    void setPort(int port) {
      outputPort = port;
      pinMode(outputPort, OUTPUT);
    }
    void setTemperature(float temp) {
      temperature = temp;
//      Serial.println(temperature);
    }
    void setSetPoint(float setPoint) {
      setpoint = setPoint;
//      Serial.println(setpoint);
    }

    bool getState() {
      return state && heatingOn;
    }

    void run() {
      if (temperature > (setpoint + hysteresis)) state = 0;
      if (temperature < (setpoint - hysteresis)) state = 1;
      //      Serial.println(state);

      if (state && heatingOn) digitalWrite(outputPort, HIGH); else digitalWrite(outputPort, LOW);
    };
};

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature DS18B20(&oneWire);
//float setSetmperature = 26;
//float hysteris = 1;
float temperature;
char temperatureCString[6];
//char temperatureFString[6];
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long previousOneHour = 0;
bool termostatOut;
//unsigned long interval = 0;

char thingSpeakAddress[] = "api.thingspeak.com";
String apiKey = "3P1HKRVQR2VGJ2MB";
const int updateThingSpeakInterval = 16 * 1000; // Time interval in milliseconds to update ThingSpeak (number of seconds * 1000 = interval)
// Variable Setup
long lastConnectionTime = 0;
boolean lastConnected = false;
int failedCounter = 0;

int hour = 0;
int minute = 0;
float setPoint = 0;
char setPointString[6];

WiFiClient client;

#define PIN_RELE 4

Termostat termostat;
TermostatSheduler termostatShedule;

void initTermostat() {
  termostatShedule.ScheduleTimePreset[0] = EC_Config.ScheduleTimePreset[0];
  termostatShedule.ScheduleTimePreset[1] = EC_Config.ScheduleTimePreset[1];
  termostatShedule.ScheduleTimePreset[2] = EC_Config.ScheduleTimePreset[2];
  termostatShedule.ScheduleTimePreset[3] = EC_Config.ScheduleTimePreset[3];
  termostatShedule.ScheduleTimePreset[4] = EC_Config.ScheduleTimePreset[4];
  termostatShedule.ScheduleTimePreset[5] = EC_Config.ScheduleTimePreset[5];

  termostat.setPort(PIN_RELE);
  termostat.heatingOn = EC_Config.TERMOSTAT_STATUS;
}

void setup() {
  // put your setup code here, to run once:
  // Последовательный порт для отладки
  Serial.begin(115200);
  Serial.printf("\n\nFree memory %d\n", ESP.getFreeHeap());

  // Инициализация EEPROM
  EC_begin();
  EC_read();
  Serial.printf("\n\nFree memory %i\n", EC_Config.ScheduleTimePreset[0].startTime);

  initTermostat();


  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

  // Подключаемся к WiFi
  WiFi_begin();
  delay(2000);
  // Старт внутреннего WEB-сервера
  HTTP_begin();

  // Инициализация UDP клиента для NTP
  NTP_begin();

  syncTime();

  termostatLoop();
}

void getTemperature() {
  float tempC;
  float tempF;
  do {
    DS18B20.requestTemperatures();
    tempC = DS18B20.getTempCByIndex(0);
    dtostrf(tempC, 2, 2, temperatureCString);
    //tempF = DS18B20.getTempFByIndex(0);
    //dtostrf(tempF, 3, 2, temperatureFString);
    delay(100);
  } while (tempC == 85.0 || tempC == (-127.0));
  temperature = tempC;
}

void sendTemperature() {
  if (client.connect(thingSpeakAddress, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += temperatureCString;
    postStr +="&field2=";
    postStr += String(setPoint);
    postStr +="&field3=";
    postStr += String(termostatOut);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);

    Serial.println("% send to Thingspeak");
  }
  client.stop();
}

void syncTime() {
 
  uint32_t t = GetNTP();
  Serial.println("NTP:");
  Serial.println(t);
  Serial.println((( t / 60 ) % 60));
  Serial.println((( t / 3600 ) % 24));
  termostatShedule.minute = ( t / 60 ) % 60;
  termostatShedule.hour = ( t / 3600 ) % 24;
}

void termostatLoop() {
  termostatShedule.updateMinite();
    getTemperature();
//    Serial.printf("\nTemperature %s\n", temperatureCString);

    termostat.setTemperature(temperature);
    setPoint = termostatShedule.getCurrentSetPoint();
    termostat.setSetPoint(setPoint);
    termostat.run();
    termostatOut = termostat.getState();
    sendTemperature();
}

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();
  // Checking temperature and run termostat every minute.
  if (currentMillis - previousMillis >= 60000) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    termostatLoop();
  }

  // Update time every 1 hour.
  if (currentMillis - previousOneHour >= EC_Config.TIMEOUT_NTP) {
    previousOneHour = currentMillis;
    syncTime();
  }

  hour = termostatShedule.hour;
  minute = termostatShedule.minute;


  HTTP_loop();
  delay(100);

}
