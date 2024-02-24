#define AP_SSID "13-masterskaya"  //Имя Wi-Fi
#define AP_PASS "12856008"        //Пароль от Wi-Fi
#define NTP_OFFSET 10800          // В секундах
#define NTP_INTERVAL 60 * 1000    // В милисекундах
#define NTP_ADDRESS "europe.pool.ntp.org"
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 10800;
const int daylightOffset_sec = 10800;
//Подключение библиотек
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include "time.h"
//Переменные
TFT_eSPI tft = TFT_eSPI();
String group;
String predmet;
String predmet2;
WiFiClientSecure client;
const int sendInterval = 130000;
TaskHandle_t Task1;
TaskHandle_t Task2;
//Переменные часов
uint32_t targetTime = 0;               // для следующего 1-секундного тайм-аута
static uint8_t conv2d(const char* p);
//uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time
uint8_t hh, mm, ss;
byte omm = 99, oss = 99;
byte xcolon = 0, xsecs = 0;
unsigned int colour = 0;
//Код
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;} 
  int time_hr = timeinfo.tm_hour;
  int time_min = timeinfo.tm_min;
  int time_sec = timeinfo.tm_sec;
  hh = time_hr;
  mm = time_min;
  ss = time_sec;
}
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  //Начало подключения к Wi-Fi
  WiFi.begin(AP_SSID, AP_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());  //Конец подключения к Wi-Fi и вывод локального IP-адреса
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  tft.init();                   // инициализация дисплея
  tft.setRotation(1);           // вращение на 180 градусов
  tft.fillScreen(TFT_BLACK);    // заливка фона чёрным цветом
  tft.setCursor(60, 15);        // x,y координаты текста
  tft.setTextColor(TFT_WHITE);  // цвет текста - белый
  tft.setTextSize(3);           // размер текста - №2 (7*2 = 14 точек)
  tft.print("13 мастерская");
  xTaskCreatePinnedToCore(
    Task2code, /* Функция задачи. */
    "Task2",   /* Имя задачи. */
    10000,     /* Размер стека */
    NULL,      /* Параметры задачи */
    1,         /* Приоритет */
    &Task2,    /* Дескриптор задачи для отслеживания */
    1);        /* Указываем пин для этой задачи */
  delay(500);
  xTaskCreatePinnedToCore(
    Task1code, 
    "Task1",  
    10000,     
    NULL,      
    1,         
    &Task1,    
    0);        
  delay(500);  
  //Создаем задачу, которая будет выполняться на ядре 1 с наивысшим приоритетом (1)
  targetTime = millis() + 1000;
  // вывод текста
  tft.setCursor(100, 50);       // x,y координаты текста
  tft.setTextColor(TFT_WHITE);  // цвет текста - белый
  tft.setTextSize(2);           // размер текста - №2 (7*2 = 14 точек)
  tft.print("Занятие:");        // вывод текста
  tft.setCursor(10, 165);       
  tft.setTextColor(TFT_WHITE);  
  tft.setTextSize(2);
  tft.print("Группа:");
  tft.setCursor(100, 165);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("КСК-20");
  tft.setCursor(10, 185);                 
  tft.setTextColor(TFT_WHITE);             
  tft.setTextSize(2);                      
  tft.print("Время: ");                   
  tft.setCursor(10, 205);                  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);                    
  tft.print("Преподаватель: Леонов В.А");
}
void ntpcortime() {
  tft.setTextSize(2);
  int xpos = 80;
  int ypos = 185;
  int ysecs = ypos;
  if (omm != mm) {  // Перерисовывайте часы и минуты каждую минуту
    omm = mm;
    // Рисуем
    if (hh < 10) xpos += tft.drawChar('0', xpos, ypos, 1);  // Добавьте часы, ведущие к нулю, для 24-часовых часов
    xpos += tft.drawNumber(hh, xpos, ypos, 1);                                // Рисуем часы
    xcolon = xpos;                                                            // Сохраните код двоеточия на потом, чтобы включить / выключить вспышку позже
    xpos += tft.drawChar(':', xpos, ypos - 1, 1);
    if (mm < 10) xpos += tft.drawChar('0', xpos, ypos, 1);  // Добавим минуты, начинающиеся с нуля
    xpos += tft.drawNumber(mm, xpos, ypos, 1);              // Ричуем минуты
    xsecs = xpos;                                             
  }
  if (oss != ss) {  // Перерисовывайте время в секундах каждую секунду
    oss = ss;
    xpos = xsecs;
    if (ss % 2) {                                  // Мигаем двоеточиями вкл /выкл
      tft.setTextColor(0x39C4, TFT_BLACK);         
      tft.drawChar(':', xcolon, ypos - 1, 1);      // Час: минутное двоеточие
      xpos += tft.drawChar(':', xsecs, ysecs, 1);  // Секунды двоеточие
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    } else {
      tft.drawChar(':', xcolon, ypos - 1, 1);      
      xpos += tft.drawChar(':', xsecs, ysecs, 1);  
    }
    //Draw seconds
    if (ss < 10) xpos += tft.drawChar('0', xpos, ysecs, 1);  // Добавим начальный ноль
    tft.drawNumber(ss, xpos, ysecs, 1);                      // Ресуем секунды
  }
  if (targetTime < millis()) {
    // Установите следующее обновление на 1 секунду позже
    targetTime = millis() + 1000;
    // Отрегулируйте значения времени, добавив 1 секунду
    ss++;             
    if (ss == 60) {   
      ss = 0;        
      omm = mm;     
      mm++;         
      if (mm > 59) { 
        mm = 0;
        hh++;         
        if (hh > 23) {  
          hh = 0;
        }
      }
    }
  }
}
void Task1code(void* pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    spreadsheet_commB1B2();
    tft.setCursor(10, 90);                  
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setTextSize(2);                     
    tft.print(predmet2);                 
    tft.setCursor(10, 90);              
    tft.setTextColor(TFT_WHITE);          
    tft.setTextSize(2);            
    tft.print(predmet);                 
    delay(10000);
    tft.setCursor(10, 90);                 
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setTextSize(2);                   
    tft.print(predmet);                  
    tft.setCursor(10, 90);              
    tft.setTextColor(TFT_WHITE);       
    tft.setTextSize(2);              
    tft.print(predmet2);
    delay(10000);
  }
}
void Task2code(void* pvParameters) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    // Обновление цифрового времени
    ntpcortime();
  }
}
// Функции запроса к Google Таблицам
void spreadsheet_comm(void) {
  HTTPClient http3;
  String urlB2 = "https://script.google.com/macros/s/AKfycbx-OoeOdYPOAi0tKi-MSf2ZdPw8tsFfbCcGFuAnZ-ZH1mjhOFaRpT-gU2XxmXt31DQE/exec?cell=B2";
  Serial.print("Making a request");
  http3.begin(urlB2.c_str());  // Укажим URL-адрес и сертификат
  http3.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCodeB2 = http3.GET();
  String payloadB2;
  if (httpCodeB2 > 0) {  // Проверим наличие возвращаемого кода
    payloadB2 = http3.getString();
    group = payloadB2;
    Serial.println("Готов " + payloadB2);
  } else {
    Serial.println("Error on HTTP request");
  }	
}

void spreadsheet_commB1B2(void) {
  HTTPClient http;
  HTTPClient http2;
  String 
    urlB1 = "https://script.google.com/macros/s/AKfycbx-OoeOdYPOAi0tKi-MSf2ZdPw8tsFfbCcGFuAnZ-ZH1mjhOFaRpT-gU2XxmXt31DQE/exec?cell=A2";
  String urlC23 =
    "https://script.google.com/macros/s/AKfycbx-OoeOdYPOAi0tKi-MSf2ZdPw8tsFfbCcGFuAnZ-ZH1mjhOFaRpT-gU2XxmXt31DQE/exec?cell=A4";
  // Serial.print(url);
  Serial.print("Making a request");
  http.begin(urlB1.c_str());    
  http2.begin(urlC23.c_str()); 
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCodeB1 = http.GET();
  String payloadB1;
  if (httpCodeB1 > 0) {  
    payloadB1 = http.getString();
    predmet = payloadB1;
    Serial.println("Готов " + payloadB1);
  } else {
    Serial.println("Error on HTTP request");
  }
  int httpCode23 = http2.GET();
  String payload23;
  if (httpCode23 > 0) {  
    payload23 = http2.getString();
    predmet2 = payload23;
    Serial.println("Готов " + payload23);
  } else {
    Serial.println("Error on HTTP request");
  }
}
void loop() {
}
static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';	
}
