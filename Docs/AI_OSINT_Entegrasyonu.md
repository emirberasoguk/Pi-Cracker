# Pi-Cracker AI Destekli OSINT Entegrasyonu (Fikir ve Mimari)

## Temel Konsept
Pi-Cracker projesine, hedefin (birey veya kurum) profilini analiz ederek tamamen o hedefe özel, akıllı wordlist üretim kuralları hazırlayan **Lokal bir Büyük Dil Modeli (LLM)** entegre edilmesi planlanmaktadır.

LLM'in amacı doğrudan şifreleri üretmek **değildir** (bu çok yavaş olur ve RAM'i doldurur). Bunun yerine LLM'in görevi; profil bilgilerini okuyarak **Hashcat Kuralları (Rules)**, **Hashcat Maskeleri** ve özel durumlarda **Crunch komutları** üreten bir "saldırı orkestratörü" olarak çalışmasıdır. LLM işini bitirip kapandıktan sonra (RAM'i serbest bırakır), ürettiği komutlar ve kurallar sistemin donanım gücüyle (C motoru, Aircrack/Hashcat) çalıştırılır.

## Araç Kıyaslaması: Crunch vs Hashcat (Mask & Rule)

Yapay zekanın hangi aracın komutlarını üreteceğine karar vermek için yapılan kıyaslama:

| Özellik | Crunch | Hashcat (Maskeler ve Kurallar) |
| :--- | :--- | :--- |
| **Donanım Kullanımı** | Sadece işlemci (CPU) kullanır. Şifreleri RAM'de üretip pipe (`\|`) ile gönderir. | Doğrudan işlemi yapan birimde (GPU/CPU) üretilir. Veri transfer darboğazı yoktur. |
| **Hız** | Daha yavaştır (özellikle harici GPU'ya aktarımda). | Çok daha hızlıdır (100 kata kadar fark edebilir). |
| **Çalışma Mantığı** | Matematiksel permütasyon (kaba kuvvet). Her kombinasyonu yazar (`ahmet@@@` -> tüm sembolleri dener). | Akıllı manipülasyon. Kök kelimeleri alır (örn. `ahmet`), sonuna yıl ekleme, baş harfi büyütme, leetspeak gibi özel kuralları doğrudan uygular. |
| **LLM Eğitimi (Prompting)** | LLM için bu komutları (`crunch 8 8 -t ...`) üretmek oldukça basittir. | Kural sözdizimi karmaşıktır (`c $1 $9`, `l` vs.). LLM'i bunu yazacak şekilde eğitmek daha zor ama sonuçları çok daha etkilidir. |

## Önerilen Yapı: Hibrit Model

LLM, C motorunun tek başına düşünemeyeceği kadar karmaşık "insan davranışlarını" ve bağlamı yakalamalıdır.

**Örnek Senaryo:**
Kullanıcı girdisi: *"Hedef bir eczane. Adı Şifa Eczanesi. Kuruluş tarihi 2015. Sahibi Ayşe. Konumu Ankara/Çankaya."*

**LLM Çıktısı (Beklenen):**
1.  **Temel Sözlük (Base Words):** `sifa`, `eczane`, `ayse`, `ankara`, `cankaya`, `2015`
2.  **Hashcat Maskeleri:** `sifa?d?d?d?d` (örn: sifa1234), `ayse?d?d?d?d`
3.  **Crunch Komutları:** `crunch 11 11 -t sifaecz@@@@` (gerekli görülen özel desenler için)

## Sonraki Adımlar
1.  Sisteme hafif ve hızlı çalışan lokal bir model yönetici olan **Ollama**'nın kurulması.
2.  Bellek (4GB RAM) sınırlarına uygun, ~0.5B ile ~1.5B parametreli modellerin (örn: Qwen2.5) test edilmesi.
3.  LLM'in Hashcat kural yapısını ve doğru Crunch komutlarını üretebilmesi için özel "System Prompt" (sistem komutu) yazılması ve ince ayarların yapılması.
4.  Bu yeni katmanın (Aşama 0: AI Profil Analizi) `pi_cracker.sh` ana betiğine entegre edilmesi.
