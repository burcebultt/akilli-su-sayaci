// SU SAYACI - TAM ENTEGRE KOD
// Sensor -> Pals sayma -> Litre hesaplama -> Belirli aralikla e-posta gonderme
//
// GEREKLI KUTUPHANE: "ESP Mail Client" by Mobizt (Library Manager'dan kur)

#include <WiFi.h>
#include <ESP_Mail_Client.h>

// ================== AYARLAR - BURALARI DUZENLE ==================
// WiFi bilgileri

const char *ssid = "SUPERONLINE_WiFi_AFB0";

const char *password = "BULUT1553";

// Gmail bilgileri

#define SMTP_HOST "smtp.gmail.com"

#define SMTP_PORT 465

#define AUTHOR_EMAIL "burcebulut41@gmail.com"

#define AUTHOR_PASSWORD "oyyxqnrgtcceaogy" // normal sifre DEGIL

#define RECIPIENT_EMAIL "burcebulut41@gmail.com"

// Sensor pini
const int SENSOR_PIN = 4;

// KALIBRASYON: Bunu kendi olcumune gore degistir!
// Ornek: 47 atim = 10 litre olculduyse -> 10.0 / 47.0
const float LITRE_PER_PULSE = 0.213;

// E-posta gonderme araligi (test icin kisa, gercekte aylik yapilacak)
// Simdilik 10 dakika = 10 * 60 * 1000 milisaniye
const unsigned long EMAIL_INTERVAL = 10UL * 60UL * 1000UL;

// ================== DEGISKENLER ==================

volatile int pulseCount = 0;
int lastSensorState = HIGH;
unsigned long lastEmailTime = 0;

SMTPSession smtp;

// ================== KURULUM ==================

void setup()
{
    Serial.begin(115200);
    pinMode(SENSOR_PIN, INPUT);

    // WiFi baglantisi
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

    // ONEMLI: Gmail SSL sertifikasini dogrulayabilmesi icin ESP32'nin
    // dogru tarih/saati bilmesi gerekiyor. NTP sunucusundan zamani cekiyoruz.
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

    lastEmailTime = millis(); // sayaci simdi baslat
}

// ================== ANA DONGU ==================

void loop()
{
    // 1. Sensorden pals oku
    int currentState = digitalRead(SENSOR_PIN);
    if (currentState != lastSensorState)
    {
        if (currentState == LOW)
        {
            pulseCount++;
        }
        lastSensorState = currentState;
        delay(50); // basit debounce
    }

    // 2. Belirlenen aralik doldu mu kontrol et
    if (millis() - lastEmailTime >= EMAIL_INTERVAL)
    {
        float totalLitre = pulseCount * LITRE_PER_PULSE;

        Serial.print("Rapor zamani! Toplam atim: ");
        Serial.print(pulseCount);
        Serial.print(" | Litre: ");
        Serial.println(totalLitre);

        sendReportEmail(totalLitre, pulseCount);

        // Sayaci sifirla, yeni donem basla
        pulseCount = 0;
        lastEmailTime = millis();
    }
}

// ================== E-POSTA GONDERME FONKSIYONU ==================

void sendReportEmail(float litre, int pulses)
{
    Serial.println("E-posta gonderiliyor...");

    smtp.debug(1);

    Session_Config config;
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = "Su Sayaci Sistemi";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Su Tuketim Raporun Hazir";
    message.addRecipient("Kullanici", RECIPIENT_EMAIL);

    String body = "Merhaba,\n\n";
    body += "Bu donemki su tuketim raporun:\n\n";
    body += "Toplam tuketim: " + String(litre, 2) + " litre\n";
    body += "Algilanan atim sayisi: " + String(pulses) + "\n\n";
    body += "Iyi gunler dileriz.\n";
    body += "Su Sayaci Takip Sistemi";

    message.text.content = body.c_str();

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
        Serial.println("E-posta basariyla gonderildi!");
    }
}