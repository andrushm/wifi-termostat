/**
* Контроллер управления вытяжным вентилятором. Версия 2.0 WiFi
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/

#include "WC_EEPROM.h"

struct WC_Config EC_Config;

//struct T_ScheduleTimePreset ScheduleTimePreset[6];

/**
 * Инициализация EEPROM
 */
void EC_begin(void){
   size_t sz1 = sizeof(EC_Config);
   EEPROM.begin(sz1);   
   Serial.printf("EEPROM init. Size = %d\n",(int)sz1);

}

/**
 * Читаем конфигурацию из EEPROM в память
 */
void EC_read(void){
   size_t sz1 = sizeof(EC_Config);
   for( int i=0; i<sz1; i++ ){
       uint8_t c = EEPROM.read(i);
       *((uint8_t*)&EC_Config + i) = c; 
    }  
    uint16_t src = EC_SRC();
    if( EC_Config.SRC == src ){
       Serial.printf("EEPROM Config is correct\n");
    }
    else {
       Serial.printf("EEPROM SRC is not valid: %d %d\n",src,EC_Config.SRC);
       EC_default();
       EC_save();
    }        
}

/**
 * Устанавливаем значения конфигурации по-умолчанию
 */
void EC_default(void){
   size_t sz1 = sizeof(EC_Config);
   memset( &EC_Config, '\0',sz1);
//   for( int i=0, byte *p = (byte *)&EC_Config; i<sz1; i++, p++) 
//       *p = 0;   
     
   strcpy(EC_Config.ESP_NAME,"termostat");
   strcpy(EC_Config.ESP_PASS,"1234");
   strcpy(EC_Config.AP_SSID, "andrush");
   strcpy(EC_Config.AP_PASS, "A21111984");
   EC_Config.IP[0]      = 192;   
   EC_Config.IP[1]      = 168;   
   EC_Config.IP[2]      = 0;     
   EC_Config.IP[3]      = 4;
   EC_Config.MASK[0]    = 255; 
   EC_Config.MASK[1]    = 255; 
   EC_Config.MASK[2]    = 255; 
   EC_Config.MASK[3]    = 0;
   EC_Config.GW[0]      = 192;   
   EC_Config.GW[1]      = 168;   
   EC_Config.GW[2]      = 0;     
   EC_Config.GW[3]      = 1;
   strcpy(EC_Config.NTP_SERVER1, "0.ru.pool.ntp.org");
   strcpy(EC_Config.NTP_SERVER2, "1.ru.pool.ntp.org");
   strcpy(EC_Config.NTP_SERVER3, "2.ru.pool.ntp.org");
   EC_Config.TZ         = 2;
//   EC_Config.LIGHT_LIMIT          = 900;
//   EC_Config.TIMEOUT_CHANGE_HUM   = 60000;
//   EC_Config.TIMEOUT_DISPLAY      = 5000;
   EC_Config.TIMEOUT_NTP          = 600000;
//   EC_Config.TIMER_PERIOD         = 1200;
//   EC_Config.HUM_DELTA            = 3;
//   EC_Config.HUM_MAXIMUM          = 85;  
//   EC_Config.TIMEOUT_SEND1        = 900000; 
//   EC_Config.TIMEOUT_SEND2        = 60000; 
//   strcpy(EC_Config.HTTP_SERVER,"service.samopal.pro");
   EC_Config.TERMOSTAT_STATUS = true;
   EC_Config.ScheduleTimePreset[0].startTime = 9;
   EC_Config.ScheduleTimePreset[0].endTime = 19;
   EC_Config.ScheduleTimePreset[0].setPoint = 19;
   EC_Config.ScheduleTimePreset[0].isActive = true;
   EC_Config.ScheduleTimePreset[1].startTime = 19;
   EC_Config.ScheduleTimePreset[1].endTime = 23;
   EC_Config.ScheduleTimePreset[1].setPoint = 21;
   EC_Config.ScheduleTimePreset[1].isActive = true;
   EC_Config.ScheduleTimePreset[2].startTime = 23;
   EC_Config.ScheduleTimePreset[2].endTime = 7;
   EC_Config.ScheduleTimePreset[2].setPoint = 18;
   EC_Config.ScheduleTimePreset[2].isActive = true;
   EC_Config.ScheduleTimePreset[3].startTime = 7;
   EC_Config.ScheduleTimePreset[3].endTime = 9;
   EC_Config.ScheduleTimePreset[3].setPoint = 21;
   EC_Config.ScheduleTimePreset[3].isActive = true;
   EC_Config.ScheduleTimePreset[4].startTime = 0;
   EC_Config.ScheduleTimePreset[4].endTime = 0;
   EC_Config.ScheduleTimePreset[4].setPoint = 0;
   EC_Config.ScheduleTimePreset[4].isActive = false;
   EC_Config.ScheduleTimePreset[5].startTime = 0;
   EC_Config.ScheduleTimePreset[5].endTime = 0;
   EC_Config.ScheduleTimePreset[5].setPoint = 0;
   EC_Config.ScheduleTimePreset[5].isActive = false;
   
   //{9, 19, 19, true}; //, {19, 23, 21, true}, {23, 7, 18, true}, {7, 9, 21, true}, {0, 0, 0, false}, {0, 0, 0, false}};
//   strcpy(EC_Config.HTTP_REQUEST,"/save3?id=%s&h=%d&t=%d&a=%d&tm1=%d&tm2=%d&uptime=%ld");  
}

/**
 * Сохраняем значение конфигурации в EEPROM
 */
void EC_save(void){
   EC_Config.SRC = EC_SRC();
   size_t sz1 = sizeof(EC_Config);
   for( int i=0; i<sz1; i++)
      EEPROM.write(i, *((uint8_t*)&EC_Config + i));
   EEPROM.commit();     
   Serial.printf("Save Config to EEPROM. SCR=%d\n",EC_Config.SRC);   
}

/**
 * Сохраняем значение конфигурации в EEPROM
 */
uint16_t EC_SRC(void){
   uint16_t src = 0;
   size_t sz1 = sizeof(EC_Config);
   uint16_t src_save = EC_Config.SRC;
   EC_Config.SRC = 0;
   for( int i=0; i<sz1; i++)src +=*((uint8_t*)&EC_Config + i);
   Serial.printf("SCR=%d\n",src); 
   EC_Config.SRC = src_save;
   return src;  
}

