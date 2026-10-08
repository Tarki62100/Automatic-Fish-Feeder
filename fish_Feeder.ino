#include <FS.h>           
#include <WiFi.h>
#include <WebServer.h>    
#include <TFT_eSPI.h>     
#include <SPI.h>

using namespace fs;       

TFT_eSPI tft = TFT_eSPI();
WebServer server(80);

//User Settings
const char* ssid = "WIFI_NAME";          
const char* password = "WIFI_PASSWORD";    
const char* web_username = "USER_NAME";       
const char* web_password = "USER_PASSWORD";       

const int BUTTON_SELECT = 0;   
const int BUTTON_CONFIRM = 35; 
const int SERVO_PIN = 27;      

const int PWM_FREQ = 50;       
const int PWM_RES = 16;        

// 180 degree boundaries for a MSG90 SERVO
const int SERVO_0_DERECE = 1400;   // ~0.5ms 0
const int SERVO_180_DERECE = 7864; // ~2.4ms 180

//Menu variables
enum CihazDurumu { WELCOME_SCREEN, WIFI_ERROR_SCREEN, INTERVAL_SCREEN, RUNNING_SCREEN };
CihazDurumu mevcutDurum = WELCOME_SCREEN;

int secilenMod = 0;     
int secilenSureIdx = 0; 

const char* sureYazilari[] = {"15 Saniye", "30 Saniye", "60 Saniye", "12 Saat", "24 Saat", "48 Saat"};
unsigned long sureDegerleri[] = {15, 30, 60, 12 * 3600, 24 * 3600, 48 * 3600}; 
const int TOPLAM_SURE_SECENEGI = 6;

unsigned long sonBeslemeZamani = 0;
unsigned long beslemeAraligiSaniye = 0;
unsigned long sonButonZamani = 0;
const int debounceGecikmesi = 250;
unsigned long sonEkranYenileme = 0;

unsigned long sonTiklamaZamani = 0;
const int ciftTiklamaAraligi = 500; 

unsigned long sonWifiKontrol = 0;

//Feeding Function
void balikBesle() {
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(3);
  tft.drawString("BESLENIYOR!", 25, 50);
  
  //Activate the motor for the PWM function
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RES);
  delay(50); 

  //Rotating the Container
  for (int sinyal = SERVO_0_DERECE; sinyal <= SERVO_180_DERECE; sinyal += 50) {
    ledcWrite(SERVO_PIN, sinyal);
    delay(10); 
  }

  //Waiting for the feed to fall out
  delay(500); 
  
  //Traversing back to the start degree
  for (int sinyal = SERVO_180_DERECE; sinyal >= SERVO_0_DERECE; sinyal -= 75) {
    ledcWrite(SERVO_PIN, sinyal);
    delay(5); 
  }
  
  delay(200);
  
  //Shake container so that feed does not stick to the sides
  for (int i = 0; i < 2; i++) {
    ledcWrite(SERVO_PIN, SERVO_0_DERECE + 400); 
    delay(80);
    ledcWrite(SERVO_PIN, SERVO_0_DERECE);       
    delay(80);
  }
  delay(200);

  //Close down the motor
  ledcDetach(SERVO_PIN);
  pinMode(SERVO_PIN, INPUT_PULLDOWN); 
  
  sonBeslemeZamani = millis(); 
  sonEkranYenileme = 0;        
  tft.fillScreen(TFT_BLACK);
}

//WEB INTERFACE (HTML) 
void handleRoot() {
  if (!server.authenticate(web_username, web_password)) {
    return server.requestAuthentication();
  }

  unsigned long gecenSaniye = (millis() - sonBeslemeZamani) / 1000;
  unsigned long saat = gecenSaniye / 3600;
  unsigned long dakika = (gecenSaniye % 3600) / 60;
  unsigned long saniye = gecenSaniye % 60;

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial; text-align:center; background:#f4f4f9; padding:50px;}";
  html += ".btn{background:#4CAF50; color:white; padding:20px 40px; font-size:24px; border:none; border-radius:10px; cursor:pointer;}";
  html += ".btn:hover{background:#45a049;} .timer{font-size:28px; color:#333; margin:20px;}</style></head><body>";
  html += "<h1>\xF0\x9F\x90\x9F Akilli Balik Besleyici \xF0\x9F\x90\x9F</h1>";
  html += "<div class='timer'>Son Beslemeden Beri Gecen Sure:<br><b>" + String(saat) + " Saat " + String(dakika) + " Dakika " + String(saniye) + " Saniye</b></div>";
  html += "<form action='/besle' method='POST'><input class='btn' type='submit' value='Simdi Besle'></form>";
  html += "<script>setInterval(function(){ location.reload(); }, 5000);</script>"; 
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleBesle() {
  if (!server.authenticate(web_username, web_password)) {
    return server.requestAuthentication();
  }
  
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "Yonlendiriliyor...");
  balikBesle();
}

void anaMenuyeDon() {
  if (secilenMod == 0) {
    WiFi.disconnect(); 
  }
  mevcutDurum = WELCOME_SCREEN;
  sonEkranYenileme = 0;
  ekranGuncelle();
}

//Function to update screen
void ekranGuncelle() {
  unsigned long simdikiZaman = millis();
  
  if (mevcutDurum == WELCOME_SCREEN) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("HOS GELDINIZ", 45, 10);
    
    if (secilenMod == 0) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("> 1. WiFi Modu      ", 20, 50);
      tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
      tft.drawString("  2. Fiziksel Kurulum", 20, 80);
    } else {
      tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
      tft.drawString("  1. WiFi Modu      ", 20, 50);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("> 2. Fiziksel Kurulum", 20, 80);
    }
  } 
  
  else if (mevcutDurum == WIFI_ERROR_SCREEN) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("WIFI BAGLANAMADI!", 20, 30);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Ana menuye donmek icin", 40, 70);
    tft.drawString("herhangi bir butona basin.", 35, 85);
  }
  
  else if (mevcutDurum == INTERVAL_SCREEN) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Besleme Araligi?", 20, 10);
    
    tft.setTextSize(3);
    tft.drawString(sureYazilari[secilenSureIdx], 30, 55);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Degistir: Sol | Onayla: Sag", 10, 115);
  } 
  
  else if (mevcutDurum == RUNNING_SCREEN) {
    if (simdikiZaman - sonEkranYenileme >= 1000) {
      sonEkranYenileme = simdikiZaman;
      
      tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
      tft.setTextSize(2);
      tft.drawString("SISTEM AKTIF", 50, 10);

      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      
      if (secilenMod == 0) {
        tft.setTextSize(2);
        tft.drawString("Siteye Baglanin", 20, 45);
        tft.setTextSize(1);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("http://" + WiFi.localIP().toString() + "   ", 20, 80);
      } else {
        unsigned long gecenSaniye = (simdikiZaman - sonBeslemeZamani) / 1000;
        long kalanSaniye = beslemeAraligiSaniye - gecenSaniye;
        if (kalanSaniye < 0) kalanSaniye = 0;

        tft.setTextSize(2);
        tft.drawString("Kalan: " + String(kalanSaniye) + " sn   ", 20, 50);
        tft.setTextSize(1);
        tft.drawString("Mod: Cevrimdisi (Fiziksel)", 20, 90);
      }
      tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
      tft.drawString("Sifirlamak icin 2 kez basin", 20, 110);
    }
  }
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  Serial.begin(115200);
  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); // TFT Backlight

  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_CONFIRM, INPUT_PULLUP); 

  pinMode(SERVO_PIN, INPUT_PULLDOWN);

  // Web Server routing
  server.on("/", HTTP_GET, handleRoot);
  server.on("/besle", HTTP_POST, handleBesle);

  ekranGuncelle();
}

void loop() {
  unsigned long simdikiZaman = millis();

  // Non-blocking connection control every 10 seconds to ensure we are connected
  if (secilenMod == 0 && mevcutDurum == RUNNING_SCREEN) {
    if (WiFi.status() != WL_CONNECTED && (simdikiZaman - sonWifiKontrol > 10000)) {
      sonWifiKontrol = simdikiZaman;
      WiFi.reconnect();
    }
    if (WiFi.status() == WL_CONNECTED) {
      server.handleClient();
    }
  }

  bool solBasildi = (digitalRead(BUTTON_SELECT) == LOW);
  bool sagBasildi = (digitalRead(BUTTON_CONFIRM) == LOW);

  //Resetting screen (Press buttons twice consecutively)
  if (mevcutDurum == RUNNING_SCREEN) {
    if (solBasildi || sagBasildi) {
      if (simdikiZaman - sonButonZamani > debounceGecikmesi) {
        if (simdikiZaman - sonTiklamaZamani < ciftTiklamaAraligi) {
          anaMenuyeDon();
          sonButonZamani = simdikiZaman;
          delay(300); 
          return;
        }
        sonTiklamaZamani = simdikiZaman;
        sonButonZamani = simdikiZaman;
      }
    }
  }

  //Menu Controls
  if (simdikiZaman - sonButonZamani > debounceGecikmesi) {
    if (solBasildi) {
      if (mevcutDurum == WELCOME_SCREEN) {
        secilenMod = (secilenMod == 0) ? 1 : 0; 
        ekranGuncelle();
      } else if (mevcutDurum == INTERVAL_SCREEN) {
        secilenSureIdx = (secilenSureIdx + 1) % TOPLAM_SURE_SECENEGI; 
        ekranGuncelle();
      } else if (mevcutDurum == WIFI_ERROR_SCREEN) {
        anaMenuyeDon();
      }
      sonButonZamani = simdikiZaman;
    }

    if (sagBasildi) {
      if (mevcutDurum == WELCOME_SCREEN) {
        if (secilenMod == 0) {
          tft.fillScreen(TFT_BLACK);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextSize(2);
          tft.drawString("WiFi Baglaniyor...", 10, 50);
          
          WiFi.mode(WIFI_STA);
          WiFi.begin(ssid, password);
          unsigned long startAttempt = millis();
          
          while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
            delay(500);
          }
          
          if (WiFi.status() != WL_CONNECTED) {
            mevcutDurum = WIFI_ERROR_SCREEN;
            ekranGuncelle();
            sonButonZamani = simdikiZaman;
            return;
          }
          
          server.begin();
          sonBeslemeZamani = millis();
          mevcutDurum = RUNNING_SCREEN;
          tft.fillScreen(TFT_BLACK);
          ekranGuncelle();
        } else {
          mevcutDurum = INTERVAL_SCREEN;
          ekranGuncelle();
        }
      } 
      else if (mevcutDurum == INTERVAL_SCREEN) {
        beslemeAraligiSaniye = sureDegerleri[secilenSureIdx];
        sonBeslemeZamani = millis(); 
        mevcutDurum = RUNNING_SCREEN;
        tft.fillScreen(TFT_BLACK);
        ekranGuncelle();
      } else if (mevcutDurum == WIFI_ERROR_SCREEN) {
        anaMenuyeDon();
      }
      sonButonZamani = simdikiZaman;
    }
  }

  //Working Screen loop
  if (mevcutDurum == RUNNING_SCREEN) {
    ekranGuncelle(); 
    
    if (secilenMod == 1) {
      if ((simdikiZaman - sonBeslemeZamani) / 1000 >= beslemeAraligiSaniye) {
        balikBesle();
      }
    }
  }
}