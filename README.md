# 🥧 Pi-Cracker: Advanced Hybrid WiFi Penetration Tool

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
*   **Sosyal Kaynak (OSINT):** Hedef kişinin adı, soyadı, yaşadığı şehir, plaka kodu ve tuttuğu takım gibi verileri alarak, Türk kullanıcılarının yaygın şifre oluşturma alışkanlıklarına (Örn: `Ahmet1905`, `istanbul34`) uygun kombinasyonlar üretir.

### 2. ⚡ Akıllı Donanım Optimizasyonu (Unified Script)
Tek bir script (`pi_cracker.sh`) sisteminizi analiz eder ve en uygun saldırı vektörünü seçer:
*   **GPU Modu (Hashcat):** Eğer sisteminizde uyumlu bir ekran kartı ve Hashcat varsa, şifreleri C motorundan doğrudan Hashcat'e "pipe" (boru hattı) ile aktarır. Disk G/Ç darboğazına takılmadan saniyede binlerce/milyonlarca deneme yapabilir.
*   **CPU Modu (Aircrack-ng):** GPU yoksa veya eski bir sistemse, otomatik olarak Aircrack-ng moduna geçer.

### 3. 🔄 Otomatik Dosya Yönetimi
*   **.cap -> .hc22000 Dönüşümü:** Hashcat modu için gerekli olan dosya formatı dönüşümünü (`hcxpcapngtool` varsa) otomatik yapar.
*   **Klasör Yapısı:** Yakalanan ağ dosyalarını (`Handshake/captures/`) ve wordlistleri (`Wordlist/`) otomatik tanır.

---

## 📂 Proje Mimarisi

```text
/
├── Docs/                   # Detaylı analiz raporları ve geliştirme notları
│   ├── Turk_Wifi_Parola_Analizi.md  # Türkiye'ye özgü şifre kalıpları analizi
│   └── Genel_Siber_Guvenlik_Raporu.md
├── Handshake/
│   └── captures/           # Yakalanan ağ paketleri (.cap, .hc22000)
├── Wordlist/               # Yardımcı ve "En Çok Kullanılan" wordlistler
├── pi_script_bruteforce/   # Kaynak kodlar
│   ├── pi_cracker.sh       # (ANA ÇALIŞTIRILABİLİR DOSYA)
│   ├── pi_generator_dynamic.c # C tabanlı yüksek performanslı wordlist motoru
│   └── pi_generator.state  # Kaldığı yeri hatırlayan durum dosyası
└── README.md
```

---

## 🛠️ Kurulum

Bu araç **Linux** ortamında (Kali Linux, Parrot OS, Ubuntu vb.) çalışmak üzere tasarlanmıştır.

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
Bu proje MIT Lisansı ile sunulmuştur. Açık kaynaklıdır ve geliştirilmeye açıktır.

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
*   **Social Source (OSINT):** Gathers information like the target's first name, last name, city, license plate code, and favorite team to generate combinations tailored to common Turkish password patterns (e.g., `Ahmet1905`, `istanbul34`).

### 2. ⚡ Smart Hardware Optimization (Unified Script)
A single script (`pi_cracker.sh`) analyzes your system and selects the most appropriate attack vector:
*   **GPU Mode (Hashcat):** If your system has a compatible graphics card and Hashcat, it pipes passwords directly from the C engine to Hashcat. This allows for thousands/millions of attempts per second without disk I/O bottlenecks.
*   **CPU Mode (Aircrack-ng):** If no GPU is detected or on older systems, it automatically switches to Aircrack-ng mode.

### 3. 🔄 Automatic File Management
*   **.cap -> .hc22000 Conversion:** Automatically performs the necessary file format conversion for Hashcat mode (if `hcxpcapngtool` is available).
*   **Folder Structure:** Automatically recognizes capture files (`Handshake/captures/`) and wordlists (`Wordlist/`).

---

## 📂 Project Architecture

```text
/
├── Docs/                   # Detailed analysis reports and development notes
│   ├── Turk_Wifi_Parola_Analizi.md  # Analysis of Turkish-specific password patterns
│   └── Genel_Siber_Guvenlik_Raporu.md
├── Handshake/
│   └── captures/           # Captured network packets (.cap, .hc22000)
├── Wordlist/               # Auxiliary and "most common" wordlists
├── pi_script_bruteforce/   # Source code
│   ├── pi_cracker.sh       # (MAIN EXECUTABLE FILE)
│   ├── pi_generator_dynamic.c # C-based high-performance wordlist engine
│   └── pi_generator.state  # State file to remember progress
└── README.md
```

---

## 🛠️ Installation

This tool is designed to run in a **Linux environment** (e.g., Kali Linux, Parrot OS, Ubuntu).

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
This project is licensed under the MIT License. It is open-source and open for further development.
