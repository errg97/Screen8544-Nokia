#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "bitmap.h"
#include "esp_pm.h"
//===========================================
// Variables NEO pixel
//===========================================
#define PIN        48//Integrated LED*/3
//===========================================
// Variables Deep Sleep
//===========================================
#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define uS_TO_S_FACTOR 1000000UL//1000000ULL//1000000LL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  3        /* Time ESP32 will go to sleep (in seconds) Tiempo para dormir la mayor cantidad posible*/
#define TIME_TO_SLEEP2 .25        /* Time ESP32 will go to sleep (in seconds) Tiempo para parpadeo LED*/
//===========================================
// Variables Entradas
//===========================================
#define stat             0     //Pin para saber si la unidad está en estación de carga
#define adc0            10     //Lectura de nivel bateria
#define lowBatery       38     //Indicador bateria baja vbat > vin 1/0 en bajo cuando completamente cargada, prender led en verde cuando este al 100
#define chargeStatus    37     //Indicador de carga 1/0 vbat < vin activo cuando se este cargando, cuando se esta cargando en color anaranjado
#define chargeEnable    35     //Habilitar carga 1/0 dependiendo el nivel activar
#define timerEnable     36     //Habilitar timer 1/0 activar junto con chargeEnable
//===========================================
// Variables resguardadas en el RTC
//===========================================
RTC_DATA_ATTR unsigned long segundos = 0;
RTC_DATA_ATTR unsigned int minutos = 0;
RTC_DATA_ATTR unsigned int dias = 0;
RTC_DATA_ATTR unsigned int horas = 0;
RTC_DATA_ATTR String SerialNumber = "SN:FGN22170036";
RTC_DATA_ATTR char *model = "M:";
RTC_DATA_ATTR bool estado = false;
RTC_DATA_ATTR bool encendido = true;
RTC_DATA_ATTR bool primeraVez= true;
//Variables globales
bool cambiarPantalla=false;


// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);
//Dimensiones
//84 Width = X 
//48 Height = Y


//==================    Method definitions   ==========================
void print_wakeup_reason();
void writeDateTime(int _days);
void writeRisk(int _op);
void writeSerial(String _serial);
int readBateryLevel(void);
void writeBateryLevel(int _op);
void writeImage(int _op, int _x, int _y, int _sizeX, int _sizeY);
//=====================================================================
//                           SETUP
//=====================================================================
void setup() {
  setCpuFrequencyMhz(10);
  unsigned long inicio = millis();
  Serial.begin(115200);
  //delay(5000);
  Serial.println("PCD test");
  Serial.println(String(segundos));
  Serial.println("minutos"+String(minutos));
  Serial.println("horas"+String(horas));
  Serial.println("dias"+String(dias));
  print_wakeup_reason();//Se necesita para inicializar la variable primeraVez

  //Set pin I/O modes
  pinMode(lowBatery, INPUT);
  pinMode(chargeStatus, INPUT);
  pinMode(stat, INPUT);
  pinMode(chargeEnable, OUTPUT);
  pinMode(timerEnable, OUTPUT);

  //==== Init Screen 
  display.begin();
  display.setContrast(60);
  display.clearDisplay();
  display.setRotation(2);//Rotate 90 degrees
  writeImage(1, 0, 0, 6, 30);//Logo Cisco
  //
  writeSerial("FLM27370774");
  display.display();
  
  //Siempre va a estar en bajo
  digitalWrite(timerEnable, LOW);
  //
  digitalWrite(chargeEnable, LOW);
  // init done

  //Validar si nos encontramos en la estacion
  if(!digitalRead(stat))
  {
    Serial.println("En estacion");  
    segundos = 0; // Reset de 
    horas = 0;    // variables globales
    dias = 0;     // guardadas en
    minutos = 0;  // RTC
    
    //Siempre va a estar en bajo
    digitalWrite(timerEnable, LOW);
    //Habilitamos carga mietras estemos en la estacion
    digitalWrite(chargeEnable, HIGH);
    //actualizarCargar(0);// Validar si cambiamos full pantalla a imagen cargando
    display.display();
    delay(2000);
  }
  else
  {
    //Validar tiempo
    if(primeraVez)
    {
      segundos = 0;
      horas = 0;
      dias = 0;
      minutos = 0;
      //actualizarHora(dias, horas, minutos);
      //updated to
      writeDateTime(dias);
      // and 
      writeBateryLevel(readBateryLevel());
      // and
      writeRisk(determinarRiesgo(dias));
      display.display();
      //delay(2000);
    }
    if(segundos >=60000 || segundos < 100)
    {
      if(segundos >=60000)
      {
        //Serial.print("Entre");
        while(segundos>=60000)
        {
          minutos =  minutos + 1;
          segundos = segundos - 60000;
          //Serial.println("Restante:"+String(segundos));
          
        }
        //actualizarHora(dias, horas, minutos);
      }
      if(minutos >=60)
      {
        while(minutos>=60)
        {
          horas = horas + 1;
          minutos = minutos-60;
        }
        //actualizarHora(dias, horas, minutos);
        //  updated to 
        writeDateTime(dias);
        // and 
        writeBateryLevel(readBateryLevel());
        // and
        writeRisk(determinarRiesgo(dias));
        display.display();
        //delay(2000);
      }
      if(horas >=24)
      {
        dias = dias + 1;
        horas = horas-24;
        //actualizarHora(dias, horas, minutos);
        writeDateTime(dias);
        // and 
        writeBateryLevel(readBateryLevel());
        // and
        writeRisk(determinarRiesgo(dias)); 
        //display.display();
        //delay(2000);
      }
    
    }
    //==============Blink LED============================
    if(!estado && encendido)
    {
      if(horas < 24 && dias == 0)
      {
        //color_leds(0,20,0);  //GREEN
        //neopixelWrite(PIN,0,200,0);  //GREEN
      }
      else if (dias >= 1 && dias <= 2)
      {
        //color_leds(20,9,1); // AMBAR
        //neopixelWrite(PIN,200,200,0);
      }
      else 
      {
        //color_leds(20,0,0);  //RED
        //neopixelWrite(PIN,200,0,0);  //RED
      }
      delay(2000);
    }
    else
    {
      //neopixelWrite(PIN,0,0,0);
      display.display();
      delay(2000);
    }
    //==============Ir a dormir===========================
    //variable estado anterior
    estado = !estado;
    unsigned long fin = millis()-inicio;
    segundos += !encendido ?( 3600000 + fin) : estado ? (250+fin):(3000+fin);
    unsigned long tiempoDormirS = !encendido ?(3600000) : estado ? (TIME_TO_SLEEP2 ):(TIME_TO_SLEEP );
    Serial.println("A mimir");
    //esp_sleep_enable_timer_wakeup(3000000);
    esp_sleep_enable_timer_wakeup((tiempoDormirS) * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }

  //en caso de entrar a estacion dar 10 s para conectarse

 /*  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  
  // Wait for connection
  unsigned long iniEsperar = millis();
  unsigned long actualEsperar = millis();
  unsigned long finEsperar = actualEsperar - iniEsperar ;
  while (WiFi.status() != WL_CONNECTED && finEsperar < 10000) {
    delay(500);
    Serial.print(".");
    actualEsperar = millis();
    finEsperar = actualEsperar - iniEsperar ;
  }

  if(finEsperar < 10000)
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");
  }
  else
  {
    Serial.print("NO Connected");
  }*/

  
}
//=====================================================================
//                             LOOP
//=====================================================================
void loop() {
  // put your main code here, to run repeatedly:
  setCpuFrequencyMhz(240);
}
//=====================================================================
void writeDateTime(int _days)
{
  display.setCursor(0,19);//place cursor in the middle of the screen
  display.setTextSize(2);
  display.setTextColor(BLACK);
  String daysTwoDigits = _days < 10 ? "0" + String(_days) : String(_days); // get days in two digit format
  // hoursTwoDigits = _hours < 10 ? "0" + String(_hours) : String(_hours); // get days in two digit format 
  display.println("DAY: " + daysTwoDigits);// + " H" + hoursTwoDigits);
  //display.setCursor(0,40);//place cursor in the middle of the screen
  //display.println("H:" + hoursTwoDigits);
  display.display();
}
//=====================================================================
int determinarRiesgo(int _dias)
{
  return _dias >= 29 ? 3 : _dias < 29 && _dias >=20 ? 2 : 1;  
}
//=====================================================================
void writeRisk(int _op)
{
  display.setCursor(20, 8);//place cursor in the middle of the screen
  display.setTextSize(1);
  display.setTextColor(BLACK);
  String riesgo = _op == 3 ? "HIGH" : _op==2 ? "MID" : "LOW";
  display.println("RISK " + riesgo);
  //display.setCursor(58,9);//place cursor in the middle of the screen
  //display.println("LOW");
  display.display();
}
//=====================================================================
void writeSerial(String _serial)
{
  display.setCursor(0, 38);//place cursor in the middle of the screen
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.println(_serial);
  //display.setCursor(58,9);//place cursor in the middle of the screen
  //display.println("LOW");
  display.display();
}
//=====================================================================
//   2: Full Batery - 3: 75 % batery   - 4: 50% Batery
//   5: 25 % Batery - 6: Empty batery  
void writeBateryLevel(int _op)
{
  writeImage(_op, 0, 65, 6, 15);//change batery image
}
//=====================================================================
void writeImage(int _op, int _x, int _y, int _sizeX, int _sizeY)
{
  display.setCursor(_x, _y);//place cursor in the given position
  for(int i=0; i< _sizeX; i++)
  {
    for(int j=0; j< _sizeY; j++)
    {
      switch(_op)
      {
        case 0:
          display.drawPixel(_y+j, _x+i, logoFlex[i][j]!=1 ? WHITE : BLACK);
          break;
        case 1:
          display.drawPixel(_y+j, _x+i, logoCisco2[i][j]!=1 ? WHITE : BLACK);
          break;
        case 2:
          display.drawPixel(_y+j, _x+i, bateriaFull[i][j]!=1 ? BLACK : WHITE);
          break;
        case 3:
          display.drawPixel(_y+j, _x+i, bateria75[i][j]!=1 ? BLACK : WHITE);
          break;
        case 4:
          display.drawPixel(_y+j, _x+i, bateria50[i][j]!=1 ? BLACK : WHITE);
          break;
        case 5:
          display.drawPixel(_y+j, _x+i, bateria25[i][j]!=1 ? BLACK : WHITE);
          break;
        case 6:
          display.drawPixel(_y+j, _x+i, bateriaEmpty[i][j]!=1 ? BLACK : WHITE);
          break;
         
      }
      //display.setCursor(_x, ++_y);//place cursor in the given position 
    }
    //display.setCursor(++_x,_y);//place cursor in the given position
  }
  display.display(); 
}
int readBateryLevel(void)
{
  //=================================
  //          ADC
  int sensorValue = analogRead(adc0);
  Serial.println("sensorValue: "+String(sensorValue));
  //=================================
  if(sensorValue <= 4095 && sensorValue >=3072)
  {
    return 2;//Full Batery
  }
  else if(sensorValue <= 3071 && sensorValue >=2048)
  {
    return 3;//75% Batery
  }
  else if(sensorValue <= 2047 && sensorValue >=1024)
  {
    return 4;//50% Batery
  }
  else if(sensorValue <= 1023 && sensorValue >= 300)
  {
    return 5;//25% Batery
  } 
  else if(sensorValue <= 300 && sensorValue >= 0)
  {
    return 6;//Empty Batery
  } 
}
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :/* Serial.println("Wakeup caused by external signal using RTC_IO");*/primeraVez= false; break;
    case ESP_SLEEP_WAKEUP_EXT1 :/* Serial.println("Wakeup caused by external signal using RTC_CNTL");*/primeraVez= false; break;
    case ESP_SLEEP_WAKEUP_TIMER : /*Serial.println("Wakeup caused by timer");*/primeraVez= false; break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD :/* Serial.println("Wakeup caused by touchpad");*/primeraVez= false; break;
    case ESP_SLEEP_WAKEUP_ULP : /*Serial.println("Wakeup caused by ULP program");*/ primeraVez= false;break;
    default : /*Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);*/primeraVez= true; break;
  }
}
