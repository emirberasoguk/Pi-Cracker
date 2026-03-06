# 🥧 Pi-Cracker: Advanced Hybrid WiFi Penetration Tool
![pi_cracker_fast_demo](https://github.com/user-attachments/assets/ec7b0e5e-a7ec-4b21-b907-fdeb7be47e8b)

**Türkçe | English**

---

## Türkçe

**Pi-Cracker**, WiFi ağ güvenliği testleri için geliştirilmiş, matematiksel karmaşıklık ile insan davranışlarını (OSINT) birleştiren yeni nesil bir sızma testi aracıdır.

Geleneksel "kaba kuvvet" (brute-force) saldırılarının hantallığını, **Pi sayısının sonsuz ve düzensiz basamakları** ile aşmayı hedeflerken, aynı zamanda hedefe özel kişisel bilgilerle (Doğum yılı, Şehir, Takım vb.) "akıllı" wordlistler oluşturur.

---

## 🚀 Temel Özellikler

### 1. 🧠 Hibrit Saldırı Motoru
Sıradan wordlistlerin aksine, Pi-Cracker iki farklı veri kaynağını harmanlar:
*   **Matematiksel Kaynak:** Pi sayısının basamaklarını dinamik olarak hesaplar. Bu, diskte terabaytlarca yer kaplayan "Rainbow Table"lara ihtiyaç duymadan sonsuz bir şifre uzayı sağlar.
*   **Sosyal Kaynak (OSINT & Akıllı Varyasyonlar):** Hedef kişinin adı, soyadı, yaşadığı şehir, plaka kodu ve tuttuğu takım gibi verileri alarak şu gelişmiş teknikleri kullanır:
    *   **Leetspeak:** Karakterleri benzer rakam ve sembollerle değiştirir (Örn: `ali` -> `@l1`).
    *   **Ters Çevirme (Reverse):** Kelimeleri tersten dener (Örn: `ahmet` -> `temha`).
    *   **Akıllı Kombinasyonlar:** Kelimeleri semboller ve sayılarla birleştirir (Örn: `ahmet.1990`, `ali_34`, `yilmaz!`).
    *   **Türk Kullanıcı Kalıpları:** Türk kullanıcılarının yaygın şifre oluşturma alışkanlıklarına uygun kombinasyonlar üretir.

### 2. ⚡ Akıllı Donanım Optimizasyonu (Unified Script)
Tek bir script (`pi_cracker.sh`) sisteminizi analiz eder ve en uygun saldırı vektörünü seçer:
*   **GPU Modu (Hashcat):** Eğer sisteminizde uyumlu bir ekran kartı ve Hashcat varsa, şifreleri C motorundan doğrudan Hashcat'e "pipe" (boru hattı) ile aktarır. Disk G/Ç darboğazına takılmadan saniyede binlerce/milyonlarca deneme yapabilir.
*   **CPU Modu (Aircrack-ng):** GPU yoksa veya eski bir sistemse, otomatik olarak Aircrack-ng moduna geçer.

### 3. 📡 Ağ Bilgisi Analizi ve Hedef Odaklı Saldırı
Pi-Cracker, sadece kişisel bilgileri değil, hedef ağın kendi kimliğini de saldırıya dahil eder:
*   **Otomatik Çıkarım:** Seçilen `.cap` veya `.hc22000` dosyasından ağın **BSSID** (MAC adresi) ve **ESSID** (WiFi adı) bilgilerini otomatik olarak çeker.
*   **Ağa Özel Varyasyonlar:**
    *   MAC adresinin son 4 ve 6 hanesini şifre olarak dener.
    *   WiFi adını (Örn: `Starbucks`) alarak `starbucks123`, `Starbucks2024!` gibi akıllı kombinasyonlar türetir.
    *   Modem marka/modeline yönelik (Örn: `superonlineXXXX`) varsayılan kalıpları test eder.

### 4. 🔄 Otomatik Dosya Yönetimi
*   **.cap -> .hc22000 Dönüşümü:** Hashcat modu için gerekli olan dosya formatı dönüşümünü (`hcxpcapngtool` varsa) otomatik yapar.
*   **Klasör Yapısı:** Yakalanan ağ dosyalarını (`Handshake/captures/`) ve wordlistleri (`Wordlist/`) otomatik tanır.

---

## 📂 Proje Mimarisi

```text
/
├── Docs/                       # Dokümantasyon
│   └── Turk_Wifi_Parola_Analizi.md  # Türkiye'ye özgü şifre kalıpları analizi
├── Handshake/
│   └── captures/               # Yakalanan ağ paketleri
│       ├── cap/                # .cap dosyaları (Aircrack-ng)
│       └── hc22000/            # .hc22000 dosyaları (Hashcat)
├── Wordlist/                   # Wordlistler
│   ├── capture.txt
│   └── probable-v2-wpa-top*.txt
├── pi_script_bruteforce/       # Kaynak Kodlar
│   ├── pi_cracker.sh           # (ANA ÇALIŞTIRILABİLİR DOSYA)
│   ├── pi_generator_dynamic.c  # C tabanlı wordlist motoru
│   └── pi_generator.state      # Durum dosyası
├── LICENSE                     # Lisans dosyası
└── README.md
```

---

## 🛠️ Kurulum

Bu araç **Linux** ortamında (Kali Linux, Parrot OS, Ubuntu vb.) çalışmak üzere tasarlanmıştır.

> **⚠️ Uyumluluk Notu (Windows/macOS):**
> Bu araç, WiFi kartı üzerinde düşük seviye kontrol (Monitor Mode) ve ham paket erişimi gerektirdiği için Windows veya macOS üzerinde doğrudan çalışmayabilir.
> Windows ve macOS kullanıcılarının **Sanal Makine (VirtualBox/VMware)** üzerine Kali Linux kurarak veya **Live USB** kullanarak çalıştırmaları önerilir.

### 1. Gereksinimlerin Yüklenmesi
Sisteminize gerekli kütüphaneleri ve araçları yükleyin:

```bash
# Debian/Ubuntu/Kali tabanlı sistemler için:
sudo apt-get update
sudo apt-get install build-essential libmpfr-dev libgmp-dev hashcat aircrack-ng hcxtools
```
*Not: `libmpfr` ve `libgmp`, C programının Pi sayısını yüksek hassasiyetle hesaplaması için zorunludur.*

### 2. Projenin İndirilmesi ve Derleme
C motorunu derlemek için:

```bash
cd pi_script_bruteforce
gcc pi_generator_dynamic.c -o pi_generator_dynamic -lmpfr -lgmp
```
*Hata alırsanız kütüphanelerin yüklü olduğundan emin olun.*

---

## 💻 Kullanım

Saldırıyı başlatmak için ana scripti çalıştırın. Script interaktif arayüzü ile sizi yönlendirecektir.

```bash
cd pi_script_bruteforce
./pi_cracker.sh
```

### Adım Adım Süreç:
1.  **Mod Seçimi:** Script sisteminizi tarar. GPU varsa Hashcat'i, yoksa Aircrack-ng'yi önerir.
2.  **Dosya Seçimi:** `../Handshake/captures/` klasöründeki dosyayı belirtmeniz istenir.
3.  **Hedef Bilgileri:** (Opsiyonel) Hedef kişi hakkında bildiğiniz detayları girin (Ad: `Mehmet`, Yıl: `1990`, Takım: `GS` vb.). Bu bilgiler başarı oranını ciddi ölçüde artırır.
4.  **Saldırı:** Araç önce en popüler şifreleri, ardından kişisel kombinasyonları ve son olarak Pi sayısının basamaklarını dener.

---

## ⚠️ Yasal Uyarı (Legal Disclaimer)

**Bu yazılım sadece eğitim amaçlı ve yasal güvenlik testleri (Penetration Testing) için geliştirilmiştir.**

*   Sadece **kendi ağınızda** veya **yazılı izniniz olan** ağlarda kullanınız.
*   İzinsiz ağlara erişmeye çalışmak, **5237 sayılı Türk Ceza Kanunu** (Bilişim Suçları) ve uluslararası yasalar kapsamında suçtur.
*   Geliştirici, bu aracın yasa dışı kullanımından doğabilecek hiçbir zarardan sorumlu tutulamaz.

---

## 📝 Lisans
Bu proje MIT Lisansı ile sunulmuştur. Detaylar için `LICENSE` dosyasına bakınız.

---

---

## English

# 🥧 Pi-Cracker: Advanced Hybrid WiFi Penetration Tool

**Pi-Cracker** is a next-generation penetration testing tool developed for WiFi network security assessments, combining mathematical complexity with human behavior (OSINT).

It aims to overcome the clumsiness of traditional "brute-force" attacks by leveraging the **infinite and irregular digits of Pi** while simultaneously creating "smart" wordlists using target-specific personal information (e.g., birth year, city, favorite team).

---

## 🚀 Key Features

### 1. 🧠 Hybrid Attack Engine
Unlike ordinary wordlists, Pi-Cracker blends two distinct data sources:
*   **Mathematical Source:** Dynamically calculates the digits of Pi. This provides an infinite password space without requiring terabytes of disk space for "Rainbow Tables."
*   **Social Source (OSINT & Smart Variations):** Gathers information like the target's name, city, and favorite team, utilizing advanced techniques:
    *   **Leetspeak:** Replaces characters with similar-looking numbers and symbols (e.g., `ali` -> `@l1`).
    *   **Reverse:** Tries passwords in reverse order (e.g., `ahmet` -> `temha`).
    *   **Smart Combinations:** Merges words with symbols and numbers (e.g., `ahmet.1990`, `ali_34`, `yilmaz!`).
    *   **Turkish Password Patterns:** Tailors combinations to match common Turkish password creation habits.

### 2. ⚡ Smart Hardware Optimization (Unified Script)
A single script (`pi_cracker.sh`) analyzes your system and selects the most appropriate attack vector:
*   **GPU Mode (Hashcat):** If your system has a compatible graphics card and Hashcat, it pipes passwords directly from the C engine to Hashcat. This allows for thousands/millions of attempts per second without disk I/O bottlenecks.
*   **CPU Mode (Aircrack-ng):** If no GPU is detected or on older systems, it automatically switches to Aircrack-ng mode.

### 3. 📡 Network Information Analysis & Targeted Attack
Pi-Cracker incorporates the target network's own identity into the attack:
*   **Automatic Extraction:** Automatically retrieves the network's **BSSID** (MAC address) and **ESSID** (WiFi name) from the selected `.cap` or `.hc22000` file.
*   **Network-Specific Variations:**
    *   Tests the last 4 and 6 digits of the MAC address as potential passwords.
    *   Uses the WiFi name (e.g., `Starbucks`) to derive smart combinations like `starbucks123` or `Starbucks2024!`.
    *   Tests default patterns related to specific ISP/Modem brands (e.g., `superonlineXXXX`).

### 4. 🔄 Automatic File Management
*   **.cap -> .hc22000 Conversion:** Automatically performs the necessary file format conversion for Hashcat mode (if `hcxpcapngtool` is available).
*   **Folder Structure:** Automatically recognizes capture files (`Handshake/captures/`) and wordlists (`Wordlist/`).

---

## 📂 Project Architecture

```text
/
├── Docs/                       # Documentation
│   └── Turk_Wifi_Parola_Analizi.md  # Analysis of Turkish-specific password patterns
├── Handshake/
│   └── captures/               # Captured network packets
│       ├── cap/                # .cap files (Aircrack-ng)
│       └── hc22000/            # .hc22000 files (Hashcat)
├── Wordlist/                   # Wordlists
│   ├── capture.txt
│   └── probable-v2-wpa-top*.txt
├── pi_script_bruteforce/       # Source Code
│   ├── pi_cracker.sh           # (MAIN EXECUTABLE FILE)
│   ├── pi_generator_dynamic.c  # C-based wordlist engine
│   └── pi_generator.state      # State file
├── LICENSE                     # License file
└── README.md
```

---

## 🛠️ Installation

This tool is designed to run in a **Linux environment** (e.g., Kali Linux, Parrot OS, Ubuntu).

> **⚠️ Compatibility Note (Windows/macOS):**
> Due to the requirement for low-level network card access (Monitor Mode) and raw packet manipulation, this tool may not function directly on Windows or macOS.
> Users on these platforms are strongly recommended to use a **Kali Linux Virtual Machine (VirtualBox/VMware)** or a **Live USB**.

### 1. Install Dependencies
Install the necessary libraries and tools on your system:

```bash
# For Debian/Ubuntu/Kali-based systems:
sudo apt-get update
sudo apt-get install build-essential libmpfr-dev libgmp-dev hashcat aircrack-ng hcxtools
```
*Note: `libmpfr` and `libgmp` are essential for the C program to calculate Pi digits with high precision.*

### 2. Download and Compile the Project
To compile the C engine:

```bash
cd pi_script_bruteforce
gcc pi_generator_dynamic.c -o pi_generator_dynamic -lmpfr -lgmp
```
*If you encounter errors, ensure the required libraries are installed.*

---

## 💻 Usage

To start the attack, run the main script. The script will guide you through an interactive interface.

```bash
cd pi_script_bruteforce
./pi_cracker.sh
```

### Step-by-Step Process:
1.  **Mode Selection:** The script scans your system. It will suggest Hashcat if a GPU is available, or Aircrack-ng otherwise.
2.  **File Selection:** You will be prompted to specify the target file from the `../Handshake/captures/` directory.
3.  **Target Information:** (Optional) Enter details you know about the target person (e.g., Name: `Mehmet`, Year: `1990`, Team: `GS`). This information significantly increases the success rate.
4.  **Attack:** The tool will first try the most popular passwords, then personal combinations, and finally the digits of Pi.

---

## ⚠️ Legal Disclaimer

**This software is developed for educational purposes and legitimate security testing (Penetration Testing) only.**

*   Use it only on **your own network** or on networks for which you have **explicit written permission**.
*   Attempting to access unauthorized networks is illegal under **Turkish Penal Code No. 5237** (Cybercrimes) and international laws.
*   The developer cannot be held responsible for any damage resulting from the misuse of this tool.

---

## 📝 License
This project is licensed under the MIT License. See `LICENSE` file for details.
