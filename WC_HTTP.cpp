/**
* Контроллер управления вытяжным вентилятором. Версия 2.0 WiFi
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/

#include "WC_HTTP.h"
#include "WC_EEPROM.h"
#include "WC_NTP.h"
#include "FS.h"

ESP8266WebServer server(80);
bool isAP = false;
String authPass = "";

String bootstrap = "/bootstrap.min.css";
String fileData;

/**
 * Старт WiFi
 */
void WiFi_begin(void) {
  // Подключаемся к WiFi
  isAP = false;
  if ( ! ConnectWiFi()  ) {
    Serial.printf("Start AP %s\n", EC_Config.ESP_NAME);
    WiFi.mode(WIFI_STA);
    WiFi.softAP(EC_Config.ESP_NAME);
    WiFi.hostname(EC_Config.ESP_NAME);
    isAP = true;
    Serial.printf("Open http://192.168.4.1 in your browser\n");
  }
  else {
    // Получаем статический IP если нужно
    if ( EC_Config.IP != 0 ) {
      WiFi.config(EC_Config.IP, EC_Config.GW, EC_Config.MASK);
      Serial.print("Open http://");
      Serial.print(WiFi.localIP());
      Serial.println(" in your browser");
    }
  }
  // Запускаем MDNS
  MDNS.begin(EC_Config.ESP_NAME);
  Serial.printf("Or by name: http://%s.local\n", EC_Config.ESP_NAME);

  SPIFFS.begin();


}

/**
 * Соединение с WiFi
 */
bool ConnectWiFi(void) {

  // Если WiFi не сконфигурирован
  if ( strcmp(EC_Config.AP_SSID, "none") == 0 ) {
    Serial.printf("WiFi is not config ...\n");
    return false;
  }

  WiFi.mode(WIFI_STA);

  // Пытаемся соединиться с точкой доступа
  Serial.printf("\nConnecting to: %s/%s\n", EC_Config.AP_SSID, EC_Config.AP_PASS);
  WiFi.begin(EC_Config.AP_SSID, EC_Config.AP_PASS);
  delay(1000);

  // Максиммум N раз проверка соединения (12 секунд)
  for ( int j = 0; j < 15; j++ ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nWiFi connect: ");
      Serial.print(WiFi.localIP());
      Serial.print("/");
      Serial.print(WiFi.subnetMask());
      Serial.print("/");
      Serial.println(WiFi.gatewayIP());
      return true;
    }
    delay(1000);
    Serial.print(WiFi.status());
  }
  Serial.printf("\nConnect WiFi failed ...\n");
  return false;
}

/**
 * Старт WEB сервера
 */
void HTTP_begin(void) {
  // Поднимаем WEB-сервер
  server.on ( "/", HTTP_handleRoot );
  server.on ( "/config", HTTP_handleConfig );
  server.on ( "/config2", HTTP_handleConfig2 );
  server.on ( "/save", HTTP_handleSave );
  server.on ( "/reboot", HTTP_handleReboot );
  server.on ( "/default", HTTP_handleDefault );
  server.on ( "/termostat-on-off", HTTP_handleTermostatOnOff );
  server.on ( "/bootstrap.min.css", HTTP_handleBootstrap );
  server.onNotFound ( HTTP_handleRoot );
  server.begin();
  Serial.printf( "HTTP server started ...\n" );


}

/**
 * Обработчик событий WEB-сервера
 */
void HTTP_loop(void) {
  server.handleClient();
}

void HTTP_handleBootstrap(void) {

  if (SPIFFS.exists(bootstrap)){

      File f = SPIFFS.open(bootstrap, "r");
      if (f && f.size()) {
          Serial.println("Dumping log file");
          server.streamFile(f, "text/css");
      }
  }
}

/*
 * Оработчик главной страницы сервера
 */
void HTTP_handleRoot(void) {
  String out = "";
  //server.send ( 200, "text/html", out );
  //return;
  out = "<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <meta http-equiv='refresh' content='5'/>\
    <title>WiFi Термостат</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
    <link rel=\"stylesheet\" href=\"/bootstrap.min.css\">\
  </head>\
  <body>\
  <div class=\"container\">\
   <h1>WiFi Термостат</h1>\n";
  out += "<h2>";
  out += EC_Config.ESP_NAME;
  out += "</h2>\n";

  if ( isAP ) {
    out += "<p>Режим точки доступа: ";
    out += EC_Config.ESP_NAME;
    out += " </p>";
  }
  else {
    out += "<p>Підєднано до ";
    out += EC_Config.AP_SSID;
    out += " </p>";
  }
  char str[100];

  out += "<h2>";
  sprintf(str, "Час: %02d:%02d</br>", hour, minute);
  out += str;
  sprintf(str, "Температура кімнати: %s C'<br>", temperatureCString);
  out += str;
  dtostrf(setPoint, 3, 2, setPointString);
  sprintf(str, "Встановленна температура: %s C'</br>", setPointString);
  out += str;
  out += "Термостат: ";
  if ( EC_Config.TERMOSTAT_STATUS )out += "включений";
  else out += "виключений";
  out += "</h2>\n";
  out += "<h1><a class=\"btn btn-success\" href='/termostat-on-off'>";
  if ( EC_Config.TERMOSTAT_STATUS )out += "виключити";
  else out += "включити";
  out += "</a></h1>\n Статус: ";

  if (termostatOut)out += "Нагрів";
  else out += "Готов";




  out += "\
     <p><a href=\"/config\">Налаштування мережі</a>\
     <p><a href=\"/config2\">Налаштування Термостата</a>\
     </div>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );
}



/*
 * Оработчик страницы настройки сервера
 */
void HTTP_handleConfig(void) {
  String out = "";
  char str[10];
  out =
    "<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <title>ESP8266 SmartHome Controller Config</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Налаштування мережі</h1>\
     <ul>\
     <li><a href=\"/\">Головна</a>\
     <li><a href=\"/config2\">Налаштування Термостата</a>\
     <li><a href=\"/reboot\">Перезагрузка</a>\
     <li><a href=\"/default\">Заводські налаштування</a>\
     </ul>\n";

  out +=
    "     <form method='get' action='/save'>\
         <p><b>Параметры ESP модуля</b></p>\
         <table border=0><tr><td>Наименование:</td><td><input name='esp_name' length=32 value='";
  out += EC_Config.ESP_NAME;
  out += "'></td></tr>\
         <tr><td>Пароль admin:</td><td><input name='esp_pass' length=32 value='";
  out += EC_Config.ESP_PASS;
  out += "'></td></tr></table><br>\
         <p><b>Параметры WiFi подключения</b></p>\
         <table border=0><tr><td>Имя сети: </td><td><input name='ap_ssid' length=32 value='";
  out += EC_Config.AP_SSID;
  out += "'></td></tr>\
         <tr><td>Пароль:</td><td><input name='ap_pass' length=32 value='";
  out += EC_Config.AP_PASS;
  out += "'></td></tr>\
        <tr><td>IP-адрес:</td><td><input name='ip1' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[0]);
  out += str;
  out += "'>.<input name='ip2' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[1]);
  out += str;
  out += "'>.<input name='ip3' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[2]);
  out += str;
  out += "'>.<input name='ip4' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.IP[3]);
  out += str;
  out += "'></td></tr>\
        <tr><td>IP-маска:</td><td><input name='mask1' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[0]);
  out += str;
  out += "'>.<input name='mask2' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[1]);
  out += str;
  out += "'>.<input name='mask3' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[2]);
  out += str;
  out += "'>.<input name='mask4' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.MASK[3]);
  out += str;
  out += "'></td></tr>\
        <tr><td>IP-шлюз:</td><td><input name='gw1' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[0]);
  out += str;
  out += "'>.<input name='gw2' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[1]);
  out += str;
  out += "'>.<input name='gw3' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[2]);
  out += str;
  out += "'>.<input name='gw4' length=4 size=4 value='";
  sprintf(str, "%d", EC_Config.GW[3]);
  out += str;
  out += "'></td></tr></table>\n\
         <p><b>Параметры сервера времени</b></p>\
         <table border=0><tr><td>NTP сервер 1:</td><td><input name='ntp_server1' length=32 value='";
  out += EC_Config.NTP_SERVER1;
  out += "'></td></tr>\
         <tr><td>NTP сервер 2:</td><td><input name='ntp_server2' length=32 value='";
  out += EC_Config.NTP_SERVER2;
  out += "'></td></tr>\
         <tr><td>NTP сервер 3:</td><td><input name='ntp_server3' length=32 value='";
  out += EC_Config.NTP_SERVER3;
  out += "'></td></tr>\
         <tr><td>Таймзона:</td><td><input name='tz' length=4 size 4 value='";
  sprintf(str, "%d", EC_Config.TZ);
  out += str;
  out += "'></td></tr>\
         <tr><td>Интервал опроса NTP, сек:</td><td><input name='tm_ntp' length=32 value='";
  sprintf(str, "%d", EC_Config.TIMEOUT_NTP / 1000);
  out += str;
  out += "'></td></tr></table>\
     <input type='submit' value='Сохранить'>\
     </form>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );
}

/*
 * Оработчик страницы настройки сервера
 */
void HTTP_handleConfig2(void) {
  String out = "";
  char str[10];
  int i = 0;
  String iStr = "";
  out =
    "<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <title>ESP8266 SmartHome Controller Adv Config</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
      .hidden { display:none; }\
    </style>\
  </head>\
  <body>\
    <h1>Настройка Термостата</h1>\
     <ul>\
     <li><a href=\"/\">Головна</a>\
     <li><a href=\"/config\">Налаштування мережі</a>\
     <li><a href=\"/reboot\">Перезагрузка</a>\
     <li><a href=\"/default\">Заводські налаштування</a>\
     </ul>";

  out +=
    "     <form method='get' action='/save'>\
          <p><b>Термостат</b></p>\
         <table border=0>\
         <tr><th></th><th>Preset #</th><th>Start год.</th><th>End год.</th><th>Temperatura C'</th>\
         </tr>";

  for (i = 0; i < 6; i = i + 1) {
    iStr = String(i + 1);
    String isActive = "isActive" + iStr;
    out += "<tr><td>" + iStr + "</td><td><input type='checkbox' name='" + isActive + "' ";
    if (EC_Config.ScheduleTimePreset[i].isActive) {
      out +=  "checked";
    }
    out += " value='1'><input type='hidden' name='" + isActive + "' value='0'></td>";
    out += "<td><input name='startTime" + iStr + "' length=32 value='";
    sprintf(str, "%d", EC_Config.ScheduleTimePreset[i].startTime);
    out += str;
    out += "'></td>\
         <td><input name='endTime" + iStr + "' length=32 value='";
    sprintf(str, "%d", EC_Config.ScheduleTimePreset[i].endTime);
    out += str;
    String ValueS = "";
    dtostrf(EC_Config.ScheduleTimePreset[i].setPoint, 2, 2, str);
    out += "'></td>\
         <td><input name='setPoint" + iStr + "' length=32 value='";
    out += str;
    out += "'></td></tr>";

  }
  out += "</table>\
         <input type='submit' value='Сохранить'>\
     </form>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );
}


/*
 * Оработчик сохранения в базу
 */
void HTTP_handleSave(void) {
  int i = 0;
  String iStr;
  if ( server.hasArg("esp_name")     )strcpy(EC_Config.ESP_NAME, server.arg("esp_name").c_str());
  if ( server.hasArg("esp_pass")     )strcpy(EC_Config.ESP_PASS, server.arg("esp_pass").c_str());
  if ( server.hasArg("ap_ssid")      )strcpy(EC_Config.AP_SSID, server.arg("ap_ssid").c_str());
  if ( server.hasArg("ap_pass")      )strcpy(EC_Config.AP_PASS, server.arg("ap_pass").c_str());
  if ( server.hasArg("ip1")          )EC_Config.IP[0] = atoi(server.arg("ip1").c_str());
  if ( server.hasArg("ip2")          )EC_Config.IP[1] = atoi(server.arg("ip2").c_str());
  if ( server.hasArg("ip3")          )EC_Config.IP[2] = atoi(server.arg("ip3").c_str());
  if ( server.hasArg("ip4")          )EC_Config.IP[3] = atoi(server.arg("ip4").c_str());
  if ( server.hasArg("mask1")        )EC_Config.MASK[0] = atoi(server.arg("mask1").c_str());
  if ( server.hasArg("mask2")        )EC_Config.MASK[1] = atoi(server.arg("mask2").c_str());
  if ( server.hasArg("mask3")        )EC_Config.MASK[2] = atoi(server.arg("mask3").c_str());
  if ( server.hasArg("mask4")        )EC_Config.MASK[3] = atoi(server.arg("mask4").c_str());
  if ( server.hasArg("gw1")          )EC_Config.GW[0] = atoi(server.arg("gw1").c_str());
  if ( server.hasArg("gw2")          )EC_Config.GW[1] = atoi(server.arg("gw2").c_str());
  if ( server.hasArg("gw3")          )EC_Config.GW[2] = atoi(server.arg("gw3").c_str());
  if ( server.hasArg("gw4")          )EC_Config.GW[3] = atoi(server.arg("gw4").c_str());
  if ( server.hasArg("ntp_server1")  )strcpy(EC_Config.NTP_SERVER1, server.arg("ntp_server1").c_str());
  if ( server.hasArg("ntp_server2")  )strcpy(EC_Config.NTP_SERVER2, server.arg("ntp_server2").c_str());
  if ( server.hasArg("ntp_server3")  )strcpy(EC_Config.NTP_SERVER3, server.arg("ntp_server3").c_str());
  if ( server.hasArg("tz")           )EC_Config.TZ = atoi(server.arg("tz").c_str());
  if ( server.hasArg("tm_ntp")       )EC_Config.TIMEOUT_NTP  = atoi(server.arg("tm_ntp").c_str()) * 1000;
  for (i = 0; i < 6; i = i + 1) {
    iStr = String(i + 1);
    if ( server.hasArg("startTime" + iStr)  ) EC_Config.ScheduleTimePreset[i].startTime  = atoi(server.arg("startTime" + iStr).c_str());
    if ( server.hasArg("endTime" + iStr)  ) EC_Config.ScheduleTimePreset[i].endTime  = atoi(server.arg("endTime" + iStr).c_str());
    if ( server.hasArg("setPoint" + iStr)  ) EC_Config.ScheduleTimePreset[i].setPoint  = server.arg("setPoint" + iStr).toFloat();
    if ( server.hasArg("isActive" + iStr)  ) EC_Config.ScheduleTimePreset[i].isActive  = atoi(server.arg("isActive" + iStr).c_str());
    Serial.println(server.arg("isActive" + iStr).c_str());
  }

  EC_save();
  initTermostat();

  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>WiFi термостат</h1>\
     <h2>Конфігурація збережена</h2>\
     <ul>\
     <li><a href=\"/reboot\">Перезагрузка</a>\
     <li><a href=\"/config\">Налаштування мережі</a>\
     <li><a href=\"/config2\">Налаштування Термостата</a>\
     <li><a href=\"/\">Головна</a>\
     </ul>\
  </body>\
</html>";
  server.send ( 200, "text/html", out );

}

/*
 * Сброс настрое по умолчанию
 */
void HTTP_handleDefault(void) {
  EC_default();
  initTermostat();
  HTTP_handleConfig();
}

/*
 * Перезагрузка часов
 */
void HTTP_handleReboot(void) {

  String out = "";

  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='30;URL=/'>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Перезагрузка контроллера</h1>\
     <p><a href=\"/\">Через 30 сек будет переадресация на главную</a>\
   </body>\
</html>";
  server.send ( 200, "text/html", out );
  ESP.reset();

}

/*
 * Нажатие кнопки
 */
void HTTP_handleTermostatOnOff(void) {
  //  flag_btn = true;
  String out = "";
  EC_Config.TERMOSTAT_STATUS = !EC_Config.TERMOSTAT_STATUS;
  EC_save();
  initTermostat();
  
  out =
    "<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='3;URL=/'>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Нажата кнопка</h1>\
     <p><a href=\"/\">Через 3 сек будет переадресация на главную</a>\
   </body>\
</html>";
  server.send ( 200, "text/html", out );

}


/**
* Сохранить параметры на HTTP сервере
*/
bool SetParamHTTP() {

  WiFiClient client;
  IPAddress ip1;
  //   WiFi.hostByName(EC_Config.HTTP_SERVER, ip1);
  Serial.print("IP=");
  Serial.println(ip1);

  String out = "";
  char str[256];
  if (!client.connect(ip1, 80)) {
    //       Serial.printf("Connection %s failed",EC_Config.HTTP_SERVER);
    return false;
  }
  out += "GET ";
  out += "http://";
  //   out += EC_Config.HTTP_SERVER;
  // Формируем строку запроса

  //   sprintf(str,"/save3.php?id=%s&h=%d&t=%d&a=%d&tm1=%d&tm2=%d&uptime=%ld",EC_Config.ESP_NAME,Hum,Temp,Avalue,Count1/2,Count2/2,(tm-first_tm));
  out += str;
  out += " HTTP/1.0\r\n\r\n";
  Serial.print(out);
  client.print(out);
  delay(100);
  return true;


}

/**
 * Конвертирование time_t в строку
 */
/*
void Time2Str(char *s,time_t t){
  sprintf(s,"%02d.%02d.%02d %02d:%02d:%02d",
      day(t),
      month(t),
      year(t),
      hour(t),
      minute(t),
      second(t));
}
*/
