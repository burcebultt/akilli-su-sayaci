// SU SAYACI - 1 DAKİKALIK TEST FATURA KODU
// Sensör -> Pals Sayma -> m³ & TL Fatura Hesaplama -> 1 Dk Aralıkla HTML Fatura Gönderimi
// KÜTÜPHANE: "ESP Mail Client" by Mobizt

#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "time.h"

// ================== WİFİ VE SMTP AYARLARI ==================
const char *ssid = "SUPERONLINE_WiFi_AFB0";
const char *password = "BULUT1553";

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "burcebulut41@gmail.com"    // gonderıcı kısının maili
#define AUTHOR_PASSWORD "oyyxqnrgtcceaogy"       // Gmail Uygulama Şifresi
#define RECIPIENT_EMAIL "burcebulut41@gmail.com" // alıcı kısının maılı

// ================== SENSÖR VE FATURA HESAP AYARLARI ==================
const int SENSOR_PIN = 4;

// Kalibrasyon: 1 pals kaç litre?
const float LITRE_PER_PULSE = 0.213;

// Fatura Birim Fiyatları
const float BIRIM_FIYAT_M3 = 28.50; // TL / m³
const float KDV_ORANI = 0.10;       // %10 KDV

// E-posta Gönderme Aralığı: 1 dakika = 60 * 1000 milisaniye
const unsigned long EMAIL_INTERVAL = 60000UL;

// ================== DEĞİŞKENLER ==================
volatile unsigned long pulseCount = 0;
int lastSensorState = HIGH;
unsigned long lastEmailTime = 0;

SMTPSession smtp;

// Fonksiyon Bildirimi
void sendHtmlInvoiceEmail(float totalLitre, unsigned long pulses);

void setup()
{
    Serial.begin(115200);
    pinMode(SENSOR_PIN, INPUT);

    // 1. WiFi Bağlantısı
    Serial.print("WiFi'ye baglaniliyor: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 40)
    {
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi baglandi! IP: " + WiFi.localIP().toString());
    }
    else
    {
        Serial.println("\nWiFi baglanamadi. Kontrol ediniz.");
    }

    // 2. NTP Zaman Senkronizasyonu (SSL sertifikası doğrulaması için)
    Serial.println("Zaman senkronize ediliyor...");
    configTime(10800, 0, "pool.ntp.org", "time.nist.gov");

    lastEmailTime = millis(); // Zaman sayacını başlat
}

void loop()
{
    // 1. Sensörden Pals Oku (Debounce Mekanizmalı)
    int currentState = digitalRead(SENSOR_PIN);
    if (currentState != lastSensorState)
    {
        if (currentState == LOW)
        {
            pulseCount++;
            Serial.print("Pals Algilandi! Toplam Pals: ");
            Serial.println(pulseCount);
        }
        lastSensorState = currentState;
        delay(50); // Titreşim önleyici debounce
    }

    // 2. 1 Dakika Doldu mu Kontrol Et
    if (millis() - lastEmailTime >= EMAIL_INTERVAL)
    {
        float totalLitre = pulseCount * LITRE_PER_PULSE;

        Serial.println("=== 1 DAKIKA DOLDU! TEST FATURASI GONDERILIYOR ===");
        sendHtmlInvoiceEmail(totalLitre, pulseCount);

        // Yeni periyot için zamanı güncelle
        lastEmailTime = millis();
    }
}

// ================== HTML FATURA VE GÖNDERİM FONKSİYONU ==================
void sendHtmlInvoiceEmail(float litre, unsigned long pulses)
{
    // Tüketim ve Tutar Hesaplamaları
    float m3 = litre / 1000.0;
    float araTutar = m3 * BIRIM_FIYAT_M3;
    float kdvTutari = araTutar * KDV_ORANI;
    float toplamTutar = araTutar + kdvTutari;

    Serial.println("HTML Fatura hazirlaniyor ve gonderiliyor...");

    smtp.debug(1);

    Session_Config config;
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = "Akilli Su Sayaci Otomasyonu";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Su Tuketim Test Faturasi (1 Dk Raporu)";
    message.addRecipient("Abone", RECIPIENT_EMAIL);

    // HTML Şablon Oluşturma
    String htmlMsg = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    htmlMsg += "<style>";
    htmlMsg += "body { font-family: Arial, sans-serif; background-color: #f4f6f9; margin: 0; padding: 20px; }";
    htmlMsg += ".invoice-card { max-width: 500px; background: #ffffff; margin: auto; padding: 25px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); border-top: 6px solid #007bff; }";
    htmlMsg += ".header { text-align: center; border-bottom: 2px solid #eef2f5; padding-bottom: 15px; margin-bottom: 20px; }";
    htmlMsg += ".header h2 { color: #007bff; margin: 0; font-size: 22px; }";
    htmlMsg += ".header p { color: #6c757d; margin: 5px 0 0 0; font-size: 13px; }";
    htmlMsg += "table { width: 100%; border-collapse: collapse; margin-top: 15px; }";
    htmlMsg += "th, td { padding: 10px; text-align: left; font-size: 14px; border-bottom: 1px solid #eef2f5; }";
    htmlMsg += "th { color: #495057; background-color: #f8f9fa; }";
    htmlMsg += ".total-row { font-weight: bold; background-color: #e8f4ff; color: #007bff; }";
    htmlMsg += ".footer { text-align: center; margin-top: 25px; font-size: 12px; color: #adb5bd; }";
    htmlMsg += "</style></head><body>";

    htmlMsg += "<div class='invoice-card'>";
    htmlMsg += "<div class='header'>";
    htmlMsg += "<h2>AKILLI SU SAYACI FATURASI</h2>";
    htmlMsg += "<p>Test Modu - 1 Dakikalık Tüketim Özet Raporu</p>";
    htmlMsg += "</div>";

    htmlMsg += "<table>";
    htmlMsg += "<tr><th>Aciklama</th><th style='text-align:right;'>Deger</th></tr>";
    htmlMsg += "<tr><td>Toplam Pals Sayisi</td><td style='text-align:right;'>" + String(pulses) + " ad.</td></tr>";
    htmlMsg += "<tr><td>Tuketim (Litre)</td><td style='text-align:right;'>" + String(litre, 2) + " L</td></tr>";
    htmlMsg += "<tr><td>Tuketim (m³)</td><td style='text-align:right;'><b>" + String(m3, 4) + " m³</b></td></tr>";
    htmlMsg += "<tr><td>m³ Birim Fiyati</td><td style='text-align:right;'>" + String(BIRIM_FIYAT_M3, 2) + " TL</td></tr>";
    htmlMsg += "<tr><td>Su Tuketim Bedeli</td><td style='text-align:right;'>" + String(araTutar, 2) + " TL</td></tr>";
    htmlMsg += "<tr><td>KDV (%10)</td><td style='text-align:right;'>" + String(kdvTutari, 2) + " TL</td></tr>";
    htmlMsg += "<tr class='total-row'><td>ODENECEK TOPLAM</td><td style='text-align:right;'>" + String(toplamTutar, 2) + " TL</td></tr>";
    htmlMsg += "</table>";

    htmlMsg += "<div class='footer'>";
    htmlMsg += "<p>Bu e-posta Akilli Su Sayaci Otomasyonu tarafindan otomatik uretilmistir.</p>";
    htmlMsg += "</div></div></body></html>";

    message.html.content = htmlMsg.c_str();
    message.text.content = "E-posta istemciniz HTML formatini desteklemiyor. Toplam tuketim: " + String(m3, 4) + " m3";

    if (!smtp.connect(&config))
    {
        Serial.println("SMTP baglanti hatasi: " + smtp.errorReason());
        return;
    }

    if (!MailClient.sendMail(&smtp, &message))
    {
        Serial.println("E-posta gonderilemedi: " + smtp.errorReason());
    }
    else
    {
        Serial.println("1 Dakikalık HTML Fatura basariyla gonderildi!");
    }
}