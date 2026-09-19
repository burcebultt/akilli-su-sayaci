# Akıllı Su Sayacı - ESP32 Fatura Otomasyonu

Mekanik su sayacına dışarıdan eklenen bir IR sensör ile tüketimi takip eden,
belirli aralıklarla gerçek tarifeye göre hesaplanmış bir "fatura" e-postası
gönderen düşük maliyetli bir gömülü sistem projesi.

## Proje Hakkında

Bu proje, evlerdeki standart mekanik su sayaçlarını (herhangi bir dijital
çıkışı olmayan modeller dahil) düşük maliyetli bir donanım eklentisiyle
"akıllı" hale getirmeyi amaçlar. Sayacın düşük debi göstergesindeki dönen
ibre optik olarak okunur, tüketim hesaplanır ve kullanıcıya düzenli
aralıklarla otomatik bir e-posta raporu gönderilir.

## Özellikler

-  Sayaca temassız, optik (IR reflektif) okuma
-  WiFi üzerinden internete bağlanma
-  Kalıcı hafıza (NVS) — cihaz yeniden başlasa bile veri kaybolmaz
-  Gmail SMTP üzerinden otomatik, HTML formatlı fatura e-postası
-  Gerçek tarife yapısına göre TL tutarı hesaplama (su bedeli, atık su,
  KDV, çevre temizlik vergisi, ücretsiz kullanım hakkı indirimi)
-  Ayarlanabilir raporlama aralığı


## Gerekli Donanım

| Bileşen | Açıklama |
|---|---|
| ESP32 DevKit V1 | WiFi + Bluetooth destekli mikrodenetleyici |
| IR Reflektif Sensör (KY-033 / TCRT5000) | Sayaç ibresini optik olarak okur |
| Breadboard | Prototipleme için |
| Jumper Kablolar | Bağlantılar için |
| USB Kablosu | Güç ve programlama için |

##  Bağlantı Şeması

| Sensör Pini | ESP32 Pini |
|---|---|
| VCC | 3.3V |
| GND | GND |
| OUT | GPIO 4 |

##  Kurulum

### 1. Gerekli Kütüphaneler

Arduino IDE Library Manager üzerinden kur:
- `ESP Mail Client` by Mobizt

### 2. Config Dosyasını Hazırla

```bash
cp config.h.example config.h
```

`config.h` dosyasını açıp kendi bilgilerinle doldur:

```cpp
const char* WIFI_SSID = "wifi-adin";
const char* WIFI_PASSWORD = "wifi-sifren";

#define SMTP_AUTHOR_EMAIL "eposta-adresin@gmail.com"
#define SMTP_AUTHOR_PASSWORD "16-haneli-uygulama-sifresi"
#define SMTP_RECIPIENT_EMAIL "bildirim-gidecek-eposta@gmail.com"

const char* SOZLESME_NO = "sozlesme-numaran";
const char* SAYAC_NO = "sayac-seri-numaran";
const float VARSAYILAN_ILK_OKUMA_M3 = 0.0;  // sayacinin baslangic indeksi
```

> **Gmail Uygulama Şifresi nasıl alınır?**
> [myaccount.google.com/apppasswords](https://myaccount.google.com/apppasswords)
> adresinden, 2 Adımlı Doğrulama açıkken oluşturulabilir. Normal Gmail
> şifreniz **değildir**.

### 3. Kalibrasyon

`LITRE_PER_PULSE` değeri, kullandığınız sayaç modeline özgüdür ve
sayacın kadranındaki ölçekten ya da fiziksel test ile bulunmalıdır:

1. Sensörü sayaca yerleştirin.
2. Bilinen bir miktar su akıtın (örn. 1 litre).
3. Serial Monitor'de kaç "pals" sayıldığını gözlemleyin.
4. `litre / pals_sayısı` formülüyle kendi değerinizi hesaplayın.

### 4. Yükleme

`su_sayaci_fatura.ino` dosyasını Arduino IDE ile ESP32'ye yükleyin.

## Yapılandırma Seçenekleri

`su_sayaci_fatura.ino` içinde:

```cpp
// Gerçek kullanım için (yaklaşık 30 günde bir rapor)
const unsigned long EMAIL_INTERVAL = 30UL * 24UL * 60UL * 60UL * 1000UL;

// Test için (1 dakikada bir rapor) - geliştirme sırasında kullanın
// const unsigned long EMAIL_INTERVAL = 60UL * 1000UL;
```

## Tarife Bilgileri

Kod, örnek olarak İstanbul (İSKİ) su tarifesi yapısını kullanır. Kendi
bölgenizin tarifesine göre şu sabitleri güncelleyebilirsiniz:

```cpp
const float SU_BIRIM_FIYATI = 41.30;       // TL/m³
const float ATIK_SU_BIRIM_FIYATI = 20.63;  // TL/m³
const float CTV_BIRIM_FIYATI = 4.00;       // TL/m³
const float INSANI_SU_HAKKI_M3 = 2.50;     // ücretsiz kullanım hakkı (m³)
const float SU_KDV_ORANI = 0.01;
const float ATIK_SU_KDV_ORANI = 0.10;
```

> Tarifeler periyodik olarak değişir. Güncel oranları yerel su
> idarenizin resmi kaynaklarından kontrol edin.

##  Önemli Notlar

- `config.h` dosyası `.gitignore` ile hariç tutulmuştur, hiçbir zaman
  versiyon kontrolüne eklenmemelidir — WiFi şifresi ve Gmail Uygulama
  Şifresi gibi hassas bilgiler içerir.
- Bu proje bir hobi/kişisel takip aracıdır, resmi bir su idaresi faturası
  yerine geçmez; hesaplanan tutarlar yalnızca bilgilendirme amaçlıdır.

##  Lisans

Bu proje kişisel kullanım ve öğrenme amaçlı paylaşılmıştır.

---

**Katkıda bulunmak ister misiniz?** Issue açabilir veya pull request
gönderebilirsiniz.
