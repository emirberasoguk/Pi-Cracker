#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gmp.h>
#include <mpfr.h>

// --- YAPILANDIRMA ---
#define STATE_FILE "pi_generator.state"
#define MAX_BUFFER 256

// Kullanıcı Bilgileri Yapısı
typedef struct {
    char name[MAX_BUFFER];
    char surname[MAX_BUFFER];
    char year[MAX_BUFFER];
    char city[MAX_BUFFER];
    char plate[MAX_BUFFER];
    char team[MAX_BUFFER]; // gs, fb, bjk, ts
} UserInfo;

// Global değişkenler
int PASS_LEN = 8;
long NUM_PASS = 0;
char *OUTPUT_FILE = NULL;
UserInfo USER_INFO = {0};

// Hata yönetimi
void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// State yönetimi
long get_last_offset() {
    FILE *file = fopen(STATE_FILE, "r");
    if (file == NULL) return 0;
    long offset = 0;
    fscanf(file, "%ld", &offset);
    fclose(file);
    return offset;
}

void save_next_offset(long offset) {
    FILE *file = fopen(STATE_FILE, "w");
    if (file == NULL) die("Hata: State dosyası yazılamadı");
    fprintf(file, "%ld", offset);
    fclose(file);
}

// Yardımcı: Dosyaya yaz (stdout veya dosya)
void write_pass(FILE *f, const char *pass) {
    if (strlen(pass) < 8) return; // WPA min uzunluk kontrolü
    fprintf(f, "%s\n", pass);
}

// --- YENİ EKLENEN FONKSİYONLAR (Python 'ilk.py' referans alınarak) ---

void str_reverse(char *str) {
    if (!str) return;
    int len = strlen(str);
    for(int i=0; i<len/2; i++) {
        char temp = str[i];
        str[i] = str[len-1-i];
        str[len-1-i] = temp;
    }
}

void to_leetspeak(const char *input, char *output) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        char c = tolower(input[i]);
        switch (c) {
            case 'a': output[j++] = '@'; break;
            case 'e': output[j++] = '3'; break;
            case 'i': output[j++] = '1'; break;
            case 's': output[j++] = '$'; break;
            case 'o': output[j++] = '0'; break;
            case 'b': output[j++] = '8'; break;
            default: output[j++] = input[i]; break;
        }
    }
    output[j] = '\0';
}

void capitalize(char *str) {
    if (str && str[0]) str[0] = toupper(str[0]);
}

// Belirli bir anahtar kelime için akıllı varyasyonlar üretir
void generate_variations(FILE *out, const char *keyword) {
    if (!keyword || strlen(keyword) < 3) return; // Çok kısa kelimeleri atla

    char bases[5][MAX_BUFFER]; // Temel varyasyonlar
    int base_count = 0;

    // 1. Orijinal (küçük harf)
    strncpy(bases[base_count], keyword, MAX_BUFFER);
    // Zaten main'de tolower yapılıyor ama garanti olsun
    for(int i=0; bases[base_count][i]; i++) bases[base_count][i] = tolower(bases[base_count][i]);
    base_count++;

    // 2. Baş harfi büyük
    strncpy(bases[base_count], bases[0], MAX_BUFFER);
    capitalize(bases[base_count]);
    base_count++;

    // 3. Tümü büyük (OPSİYONEL - genellikle WPA için çok denenmez ama ekleyelim)
    // strncpy(bases[base_count], bases[0], MAX_BUFFER);
    // for(int i=0; bases[base_count][i]; i++) bases[base_count][i] = toupper(bases[base_count][i]);
    // base_count++;

    // 3. Ters (Reverse)
    strncpy(bases[base_count], bases[0], MAX_BUFFER);
    str_reverse(bases[base_count]);
    base_count++;

    // 4. Leetspeak
    to_leetspeak(bases[0], bases[base_count]);
    base_count++;

    // Kullanılacak sayılar
    char *numbers[10];
    int num_count = 0;
    if (USER_INFO.year[0]) numbers[num_count++] = USER_INFO.year;
    if (USER_INFO.plate[0]) numbers[num_count++] = USER_INFO.plate;
    numbers[num_count++] = "123";
    numbers[num_count++] = "1905"; // Takım yılları eklenebilir ama statik kısımda var
    numbers[num_count++] = "1234";

    // Kullanılacak semboller
    const char *symbols[] = {".", "_", "!", "@", "*"};
    int sym_count = 5;

    char buffer[MAX_BUFFER];

    for (int i = 0; i < base_count; i++) {
        char *b = bases[i];
        
        // Sadece kelime (eğer 8 haneyse)
        write_pass(out, b);

        // Kelime + Sayı ve Sayı + Kelime
        for (int n = 0; n < num_count; n++) {
            snprintf(buffer, MAX_BUFFER, "%s%s", b, numbers[n]);
            write_pass(out, buffer);
            snprintf(buffer, MAX_BUFFER, "%s%s", numbers[n], b);
            write_pass(out, buffer);
        }

        // Kelime + Sembol + Sayı
        for (int s = 0; s < sym_count; s++) {
            for (int n = 0; n < num_count; n++) {
                // ahmet.1990
                snprintf(buffer, MAX_BUFFER, "%s%s%s", b, symbols[s], numbers[n]);
                write_pass(out, buffer);
                // 1990.ahmet
                snprintf(buffer, MAX_BUFFER, "%s%s%s", numbers[n], symbols[s], b);
                write_pass(out, buffer);
                // ahmet1990!
                snprintf(buffer, MAX_BUFFER, "%s%s%s", b, numbers[n], symbols[s]);
                write_pass(out, buffer);
            }
        }
    }
}

// --- FAZ 1: STATİK DESENLER (GÜNCELLENMİŞ) ---
// Sadece ilk çalıştırıldığında (offset == 0) çalışır.
void generate_static_patterns(FILE *out) {
    char buffer[MAX_BUFFER];
    
    // printf("[*] Faz 1: Kişiselleştirilmiş desenler ve varyasyonlar üretiliyor...\n");

    // 1. İsim ve Soyad için Akıllı Varyasyonlar (YENİ ÖZELLİK)
    if (USER_INFO.name[0]) generate_variations(out, USER_INFO.name);
    if (USER_INFO.surname[0]) generate_variations(out, USER_INFO.surname);
    if (USER_INFO.city[0]) generate_variations(out, USER_INFO.city);
    
    // İsim + Soyad kombinasyonları
    if (USER_INFO.name[0] && USER_INFO.surname[0]) {
        snprintf(buffer, sizeof(buffer), "%s%s", USER_INFO.name, USER_INFO.surname);
        generate_variations(out, buffer); // ahmet yilmaz -> varyasyonları
    }

    // 2. Takım Desenleri (Mevcut mantık korunuyor)
    if (USER_INFO.team[0]) {
        if (strcmp(USER_INFO.team, "gs") == 0) {
            write_pass(out, "galatasaray1905");
            write_pass(out, "cimbom1905");
            write_pass(out, "ultraslan");
        } else if (strcmp(USER_INFO.team, "fb") == 0) {
            write_pass(out, "fenerbahce1907");
            write_pass(out, "gencfenerbahceliler");
        } else if (strcmp(USER_INFO.team, "bjk") == 0) {
            write_pass(out, "besiktas1903");
            write_pass(out, "carsi1903");
        } else if (strcmp(USER_INFO.team, "ts") == 0) {
            write_pass(out, "trabzonspor1967");
            write_pass(out, "bizeheryertrabzon");
        }
    }

    // 3. Genel Türk Şifreleri ve Yaygın Desenler
    write_pass(out, "12345678");
    write_pass(out, "123456789");
    write_pass(out, "1234567890");
    write_pass(out, "asdfghjk");
    write_pass(out, "qwertyuı");
    write_pass(out, "00000000");
    write_pass(out, "11111111");

    // Milli ve Tarihi
    write_pass(out, "fetih1453");
    write_pass(out, "istanbul1453");
    write_pass(out, "turkiye1923");
    write_pass(out, "cumhuriyet1923");
    write_pass(out, "ataturk1881");
    write_pass(out, "mustafakemal");

    // Modem / ISP
    write_pass(out, "superonline");
    write_pass(out, "ttnet123");
    write_pass(out, "turktelekom");
    write_pass(out, "vodafone");
    
    // Misafir
    write_pass(out, "misafir123");
    write_pass(out, "musteri123");
    write_pass(out, "internetsifresi");
}

// --- FAZ 2: Pİ HİBRİT MODU ---
void generate_pi_hybrid(long start_offset, long count, FILE *out) {
    long digits_needed = count + PASS_LEN + 20; 
    
    mpfr_prec_t precision = (start_offset + digits_needed) * 3.322 + 64;
    
    mpfr_t pi;
    mpfr_init2(pi, precision);
    mpfr_const_pi(pi, MPFR_RNDN);

    char *pi_str = NULL;
    mpfr_asprintf(&pi_str, "%.*Rf", (int)(start_offset + digits_needed), pi);
    mpfr_clear(pi);

    if (!pi_str) die("Hata: Pi string'e çevrilemedi.");

    // "3." kısmını atla ve offset'e git
    char *p = pi_str + 2 + start_offset;
    
    // Leetspeak ismi önceden hazırla
    char name_leet[MAX_BUFFER] = {0};
    if (USER_INFO.name[0]) to_leetspeak(USER_INFO.name, name_leet);

    for (long i = 0; i < count; i++) {
        if (*(p + PASS_LEN) == '\0') break;

        // 1. Saf Pi 
        fprintf(out, "%.*s\n", PASS_LEN, p);

        // 2. İsim + Pi (Hibrit)
        if (USER_INFO.name[0]) {
            // Normal İsim + Pi
            int name_len = strlen(USER_INFO.name);
            int remaining = PASS_LEN - name_len;
            if (remaining > 0) {
                fprintf(out, "%s%.*s\n", USER_INFO.name, remaining, p);
            }
            
            // Leetspeak İsim + Pi (YENİ)
            if (remaining > 0) {
                fprintf(out, "%s%.*s\n", name_leet, remaining, p);
            }

            // İsim + Sabit 4 Rakam
            fprintf(out, "%s%.*s\n", USER_INFO.name, 4, p);
        }

        p++; // Bir basamak kaydır
    }

    mpfr_free_str(pi_str);
}

// Argüman Ayrıştırıcı
void parse_args(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Kullanım: %s <len> <count> <out> [options]\n", argv[0]);
        exit(1);
    }
    
    PASS_LEN = atoi(argv[1]);
    NUM_PASS = atol(argv[2]);
    OUTPUT_FILE = argv[3];

    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--name") == 0 && i+1 < argc) strcpy(USER_INFO.name, argv[++i]);
        else if (strcmp(argv[i], "--surname") == 0 && i+1 < argc) strcpy(USER_INFO.surname, argv[++i]);
        else if (strcmp(argv[i], "--year") == 0 && i+1 < argc) strcpy(USER_INFO.year, argv[++i]);
        else if (strcmp(argv[i], "--city") == 0 && i+1 < argc) strcpy(USER_INFO.city, argv[++i]);
        else if (strcmp(argv[i], "--plate") == 0 && i+1 < argc) strcpy(USER_INFO.plate, argv[++i]);
        else if (strcmp(argv[i], "--team") == 0 && i+1 < argc) strcpy(USER_INFO.team, argv[++i]);
    }
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    // Küçük harfe çevir (normalize et)
    for(int i=0; USER_INFO.name[i]; i++) USER_INFO.name[i] = tolower(USER_INFO.name[i]);
    for(int i=0; USER_INFO.surname[i]; i++) USER_INFO.surname[i] = tolower(USER_INFO.surname[i]);
    for(int i=0; USER_INFO.city[i]; i++) USER_INFO.city[i] = tolower(USER_INFO.city[i]);
    for(int i=0; USER_INFO.team[i]; i++) USER_INFO.team[i] = tolower(USER_INFO.team[i]);

    FILE *out;
    if (strcmp(OUTPUT_FILE, "/dev/stdout") == 0) {
        out = stdout;
    } else {
        out = fopen(OUTPUT_FILE, "w");
        if (!out) die("Hata: Çıktı dosyası açılamadı");
    }

    long start_offset = get_last_offset();
    
    // Eğer en baştan başlıyorsak, statik desenleri bas
    if (start_offset == 0) {
        generate_static_patterns(out);
    }

    // Ardından sonsuz/büyük döngü (Pi)
    generate_pi_hybrid(start_offset, NUM_PASS, out);

    save_next_offset(start_offset + NUM_PASS);

    if (out != stdout) {
        printf("[+] İşlem tamamlandı. Sonraki offset: %ld\n", start_offset + NUM_PASS);
        fclose(out);
    }

    mpfr_free_cache();
    return 0;
}
