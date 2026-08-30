// AKILLI SU SAYACI - TAM ENTEGRE FATURA SISTEMI (NVS KALICI HAFIZA DESTEKLI)
// Sensor -> Pals sayma -> m3 hesaplama -> ISKI tarifesine gore TL hesaplama -> E-posta
//
// GEREKLI KUTUPHANELER:
// 1. "ESP Mail Client" by Mobizt
// 2. Preferences (ESP32 Dahili)

#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <Preferences.h> // Kalıcı hafıza için

// ================== WİFİ VE SMTP AYARLARI ==================
const char *ssid = "SUPERONLINE_WiFi_AFB0";
const char *password = "BULUT1553";

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "burcebulut41@gmail.com"
#define AUTHOR_PASSWORD "oyyxqnrgtcceaogy"
#define RECIPIENT_EMAIL "burcebulut41@gmail.com"

// ================== ABONE VE SAYAC BILGILERI ==================
const char *SOZLESME_NO = "AN 16965002-5";
const char *SAYAC_NO = "2401159816";
const float VARSAYILAN_ILK_OKUMA_M3 = 276.000; // Hafıza boşsa kullanılacak başlangıç değeri

// KALIBRASYON: Sayacın 0,0001 m3 ibresi = 1 tam turda 0.1 Litre su demektir.
const float LITRE_PER_PULSE = 0.1;

// ================== ISKI GUNCEL BIRIM FIYATLARI VE VERGILER ==================
const float SU_BIRIM_FIYATI = 41.30;
const float ATIK_SU_BIRIM_FIYATI = 20.63;
const float CTV_BIRIM_FIYATI = 4.00;
const float INSANI_SU_HAKKI_M3 = 2.50;

const float SU_KDV_ORANI = 0.01;
const float ATIK_SU_KDV_ORANI = 0.10;

// ================== SENSOR, HAFIZA VE ZAMANLAYICI ==================
#define SENSOR_PIN 4

Preferences preferences;

volatile unsigned long periodPulseCount = 0;
int lastSensorState = HIGH;

unsigned long lastEmailTime = 0;
unsigned long lastPulseTime = 0; // Debounce için

// E-posta gonderme araligi
// const unsigned long EMAIL_INTERVAL = 30UL * 24UL * 60UL * 60UL * 1000UL;  // 30 gun (GERCEK KULLANIM)
const unsigned long EMAIL_INTERVAL = 60UL * 1000UL; // TEST ICIN: 1 dakika

float periodStartIndexM3 = 276.000;
float currentTotalIndexM3 = 276.000;

SMTPSession smtp;

void sendHtmlInvoiceEmail(float consumedLitre, unsigned long pulses);

// ================== KURULUM ==================
void setup()
{
    Serial.begin(115200);
    pinMode(SENSOR_PIN, INPUT_PULLUP);

    // --- NVS KALICI HAFIZA OKUMA ---
    preferences.begin("water_meter", false);
    currentTotalIndexM3 = preferences.getFloat("total_index", VARSAYILAN_ILK_OKUMA_M3);
    periodStartIndexM3 = currentTotalIndexM3;

    Serial.println("\n==================================================");
    Serial.print("HAFIZADAN OKUNAN GUNCEL SAYAC INDEKSI: ");
    Serial.print(currentTotalIndexM3, 3);
    Serial.println(" m3");
    Serial.println("==================================================");

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
        Serial.println("\nWiFi baglanamadi, kontrol et.");
    }

    Serial.println("Zaman senkronize ediliyor...");
    configTime(10800, 0, "pool.ntp.org", "time.nist.gov"); // GMT+3 (Turkiye)

    time_t now = time(nullptr);
    int timeAttempt = 0;
    while (now < 100000 && timeAttempt < 20)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        timeAttempt++;
    }
    Serial.println("\nZaman senkronize edildi: " + String(now));

    lastEmailTime = millis();
}

// ================== ANA DONGU ==================
void loop()
{
    int currentState = digitalRead(SENSOR_PIN);

    // Debounce Koruması (En az 80ms arayla pals kabul et)
    if (currentState != lastSensorState && (millis() - lastPulseTime > 80))
    {
        if (currentState == LOW)
        {
            periodPulseCount++;
            lastPulseTime = millis();

            float currentConsumedM3 = (periodPulseCount * LITRE_PER_PULSE) / 1000.0;
            currentTotalIndexM3 = periodStartIndexM3 + currentConsumedM3;

            Serial.print("[PALS] Toplam: ");
            Serial.print(periodPulseCount);
            Serial.print(" | Donem Tuketimi: ");
            Serial.print(currentConsumedM3, 3);
            Serial.print(" m3 | Guncel Indeks: ");
            Serial.println(currentTotalIndexM3, 3);
        }
        lastSensorState = currentState;
    }

    // Periyot Sonu (Fatura Gönderimi)
    if (millis() - lastEmailTime >= EMAIL_INTERVAL)
    {
        lastEmailTime = millis();

        float consumedLitre = periodPulseCount * LITRE_PER_PULSE;

        Serial.println("\n--------------------------------------------------");
        Serial.println(">>> DONEM DOLDU! Fatura e-postasi gonderiliyor & Hafiza guncelleniyor...");
        Serial.println("--------------------------------------------------");

        sendHtmlInvoiceEmail(consumedLitre, periodPulseCount);

        // Hafızayı Güncelle
        periodStartIndexM3 = currentTotalIndexM3;
        preferences.putFloat("total_index", currentTotalIndexM3);

        Serial.print("--> Yeni Toplam Indeks Kalici Hafizaya Yazildi: ");
        Serial.println(currentTotalIndexM3, 3);

        periodPulseCount = 0;
    }
}

// ================== HTML FATURA E-POSTASI GONDERIMI ==================
void sendHtmlInvoiceEmail(float consumedLitre, unsigned long pulses)
{
    float consumedM3 = consumedLitre / 1000.0;
    float sonOkumaM3 = periodStartIndexM3 + consumedM3;

    float suBedeli = consumedM3 * SU_BIRIM_FIYATI;
    float atikSuBedeli = consumedM3 * ATIK_SU_BIRIM_FIYATI;
    float suKdv = suBedeli * SU_KDV_ORANI;
    float atikSuKdv = atikSuBedeli * ATIK_SU_KDV_ORANI;
    float ctvBedeli = consumedM3 * CTV_BIRIM_FIYATI;

    float insaniIndirimM3 = (consumedM3 >= INSANI_SU_HAKKI_M3) ? INSANI_SU_HAKKI_M3 : consumedM3;
    float insaniIndirimTL = insaniIndirimM3 * SU_BIRIM_FIYATI;

    float toplamTutar = suBedeli + atikSuBedeli + suKdv + atikSuKdv + ctvBedeli - insaniIndirimTL;
    if (toplamTutar < 0)
        toplamTutar = 0;

    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char dateStr[30];
    strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", timeinfo);

    Session_Config config;
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    SMTP_Message message;
    message.sender.name = "Akilli Su Sayaci Otomasyonu";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Su Tuketim Faturaniz Hazir";
    message.addRecipient("Abone", RECIPIENT_EMAIL);

    String htmlMsg = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    htmlMsg += "<style>";
    htmlMsg += "body { font-family: Arial, sans-serif; background-color: #f4f6f9; margin: 0; padding: 20px; }";
    htmlMsg += ".invoice-card { max-width: 550px; background: #ffffff; margin: auto; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); border-top: 6px solid #0056b3; }";
    htmlMsg += ".header { text-align: center; border-bottom: 2px solid #eef2f5; padding-bottom: 15px; margin-bottom: 15px; }";
    htmlMsg += ".header h2 { color: #0056b3; margin: 0; font-size: 20px; }";
    htmlMsg += ".header p { color: #6c757d; font-weight: bold; margin: 4px 0 0 0; font-size: 13px; }";
    htmlMsg += ".info-box { background-color: #f8f9fa; padding: 10px 15px; border-radius: 6px; font-size: 13px; margin-bottom: 15px; }";
    htmlMsg += "table { width: 100%; border-collapse: collapse; }";
    htmlMsg += "th, td { padding: 9px 10px; text-align: left; font-size: 13px; border-bottom: 1px solid #eef2f5; }";
    htmlMsg += "th { color: #495057; background-color: #f1f3f5; }";
    htmlMsg += ".discount { color: #28a745; font-weight: bold; }";
    htmlMsg += ".total-row { font-weight: bold; background-color: #e8f4ff; color: #0056b3; font-size: 15px; }";
    htmlMsg += ".footer { text-align: center; margin-top: 20px; font-size: 11px; color: #adb5bd; }";
    htmlMsg += "</style></head><body>";

    htmlMsg += "<div class='invoice-card'>";
    htmlMsg += "<div class='header'>";
    htmlMsg += "<h2>SU TÜKETIM FATURASI</h2>";
    htmlMsg += "<p>Rapor Tarihi: " + String(dateStr) + "</p>";
    htmlMsg += "</div>";

    htmlMsg += "<div class='info-box'>";
    htmlMsg += "<b>Sözleşme No:</b> " + String(SOZLESME_NO) + "<br>";
    htmlMsg += "<b>Sayaç Seri No:</b> " + String(SAYAC_NO) + "<br>";
    htmlMsg += "<b>ilk Okuma / Son Okuma:</b> " + String(periodStartIndexM3, 3) + " m&sup3; / " + String(sonOkumaM3, 3) + " m&sup3;";
    htmlMsg += "</div>";

    htmlMsg += "<table>";
    htmlMsg += "<tr><th>Fatura Açıklaması</th><th style='text-align:right;'>Tutar</th></tr>";
    htmlMsg += "<tr><td>Dönemlik Tüketim (m&sup3;)</td><td style='text-align:right;'><b>" + String(consumedM3, 3) + " m&sup3; (" + String(pulses) + " Pals)</b></td></tr>";
    htmlMsg += "<tr><td>Su Bedeli (" + String(SU_BIRIM_FIYATI, 2) + " TL/m&sup3;)</td><td style='text-align:right;'>" + String(suBedeli, 2) + " TL</td></tr>";
    htmlMsg += "<tr><td>Atık Su Bedeli (" + String(ATIK_SU_BIRIM_FIYATI, 2) + " TL/m&sup3;)</td><td style='text-align:right;'>" + String(atikSuBedeli, 2) + " TL</td></tr>";
    htmlMsg += "<tr><td>Su KDV (%1)</td><td style='text-align:right;'>" + String(suKdv, 2) + " TL</td></tr>";
    htmlMsg += "<tr><td>Atık Su KDV (%10)</td><td style='text-align:right;'>" + String(atikSuKdv, 2) + " TL</td></tr>";
    htmlMsg += "<tr><td>Çevre Temizlik Vergisi (CTV)</td><td style='text-align:right;'>" + String(ctvBedeli, 2) + " TL</td></tr>";
    htmlMsg += "<tr class='discount'><td>İnsani Su Hakkı İndirimi</td><td style='text-align:right;'>-" + String(insaniIndirimTL, 2) + " TL</td></tr>";
    htmlMsg += "<tr class='total-row'><td>ÖDENECEK NET TUTAR</td><td style='text-align:right;'>" + String(toplamTutar, 2) + " TL</td></tr>";
    htmlMsg += "</table>";

    htmlMsg += "<div class='footer'>";
    htmlMsg += "<p>ESP32 Akıllı Su Sayacı Otomasyon Sistemi</p>";
    htmlMsg += "</div></div></body></html>";

    message.html.content = htmlMsg.c_str();
    message.html.charSet = "utf-8";
    message.text.content = ("Tuketim: " + String(consumedM3, 3) + " m3. Toplam Tutar: " + String(toplamTutar, 2) + " TL").c_str();

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
        Serial.println("Fatura e-postasi basariyla gonderildi!");
    }
}