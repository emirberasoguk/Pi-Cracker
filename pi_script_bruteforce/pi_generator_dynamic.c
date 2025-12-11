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
    if (strlen(pass) < 8) return; // WPA min uzunluk kontrolü (opsiyonel) 
    fprintf(f, "%s\n", pass);
}

// --- FAZ 1: STATİK DESENLER (CUPP Mantığı) ---
// Sadece ilk çalıştırıldığında (offset == 0) çalışır.
void generate_static_patterns(FILE *out) {
    char buffer[MAX_BUFFER];
    
    printf("[*] Faz 1: Kişiselleştirilmiş desenler üretiliyor...\n");

    // 1. İsim + Yıl Kombinasyonları
    if (USER_INFO.name[0] && USER_INFO.year[0]) {
        snprintf(buffer, sizeof(buffer), "%s%s", USER_INFO.name, USER_INFO.year);
        write_pass(out, buffer);
        snprintf(buffer, sizeof(buffer), "%s%s", USER_INFO.year, USER_INFO.name); // Ters
        write_pass(out, buffer);
    }
    
    // 2. Şehir + Plaka
    if (USER_INFO.city[0] && USER_INFO.plate[0]) {
        snprintf(buffer, sizeof(buffer), "%s%s", USER_INFO.city, USER_INFO.plate);
        write_pass(out, buffer);
        snprintf(buffer, sizeof(buffer), "%s%s%s", USER_INFO.city, USER_INFO.plate, USER_INFO.plate); // istanbul3434
        write_pass(out, buffer);
         snprintf(buffer, sizeof(buffer), "%s%s", USER_INFO.plate, USER_INFO.city);
        write_pass(out, buffer);
    }

    // 3. Takım Desenleri
    if (USER_INFO.team[0]) {
        if (strcmp(USER_INFO.team, "gs") == 0) {
            write_pass(out, "galatasaray1905");
            write_pass(out, "cimbom1905");
            write_pass(out, "gs1905");
            write_pass(out, "1905gs");
        } else if (strcmp(USER_INFO.team, "fb") == 0) {
            write_pass(out, "fenerbahce1907");
            write_pass(out, "fener1907");
            write_pass(out, "fb1907");
        } else if (strcmp(USER_INFO.team, "bjk") == 0) {
            write_pass(out, "besiktas1903");
            write_pass(out, "bjk1903");
            write_pass(out, "karakartal1903");
        } else if (strcmp(USER_INFO.team, "ts") == 0) {
            write_pass(out, "trabzonspor1967");
            write_pass(out, "ts1967");
            write_pass(out, "61ts1967");
        }
    }

    // 4. Genel Türk Şifreleri
    write_pass(out, "12345678");
    write_pass(out, "123456789");
    write_pass(out, "1234567890");
    write_pass(out, "asdfghjk");
    write_pass(out, "superonline");
    write_pass(out, "ttnet123");
}

// --- FAZ 2: Pİ HİBRİT MODU ---
void generate_pi_hybrid(long start_offset, long count, FILE *out) {
    long digits_needed = count + PASS_LEN + 20; // Biraz pay bırakalım
    
    // Hassasiyet hesabı
    mpfr_prec_t precision = (start_offset + digits_needed) * 3.322 + 64;
    
    // printf("[*] Faz 2: Pi Hibrid Modu (%ld basamak hassasiyet)...\n", start_offset + digits_needed);

    mpfr_t pi;
    mpfr_init2(pi, precision);
    mpfr_const_pi(pi, MPFR_RNDN);

    char *pi_str = NULL;
    mpfr_asprintf(&pi_str, "%.*Rf", (int)(start_offset + digits_needed), pi);
    mpfr_clear(pi);

    if (!pi_str) die("Hata: Pi string'e çevrilemedi.");

    // "3." kısmını atla ve offset'e git
    char *p = pi_str + 2 + start_offset;
    char buffer[MAX_BUFFER];

    for (long i = 0; i < count; i++) {
        if (*(p + PASS_LEN) == '\0') break;

        // 1. Saf Pi (Sadece rakamlar)
        // Yöntem: "%.*s" kullanarak PASS_LEN kadar karakter bas
        fprintf(out, "%.*s\n", PASS_LEN, p);

        // 2. İsim + Pi (Hibrit)
        // Eğer isim varsa, ismin sonuna Pi rakamlarını ekle (toplam PASS_LEN olana kadar veya sabit ekle)
        // Strateji: İsim + 4 hane Pi (Örn: emir3141)
        if (USER_INFO.name[0]) {
            // İsim + Pi'den gelen (PASS_LEN - isim_uzunlugu) kadar rakam
            int name_len = strlen(USER_INFO.name);
            int remaining = PASS_LEN - name_len;
            if (remaining > 0) {
                fprintf(out, "%s%.*s\n", USER_INFO.name, remaining, p);
            }
            // Ayrıca İsim + Sabit 4 Rakam (Pi'den)
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
    
    // Eğer en baştan başlıyorsak, statik (sözlük) desenleri bas
    if (start_offset == 0) {
        generate_static_patterns(out);
    }

    // Ardından sonsuz/büyük döngü (Pi)
    generate_pi_hybrid(start_offset, NUM_PASS, out);

    // Yeni konumu kaydet (State)
    // Not: Hibrit modda aslında 1 Pi basamağı ilerlemesi için birden fazla şifre ürettik.
    // Ancak offset hala Pi üzerindeki konumdur.
    save_next_offset(start_offset + NUM_PASS);

    if (out != stdout) {
        printf("[+] İşlem tamamlandı. Sonraki offset: %ld\n", start_offset + NUM_PASS);
        fclose(out);
    }

    mpfr_free_cache();
    return 0;
}