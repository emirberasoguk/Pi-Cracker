#include <ctype.h>
#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// --- CONFIGURATION ---
#define STATE_FILE "/tmp/pi_temp/pi_generator.state"
#define PASS_STATUS_FILE "/tmp/pi_temp/pi_current_pass.txt"
#define MAX_BUFFER 256

// User Info Structure
typedef struct {
  char name[MAX_BUFFER];
  char surname[MAX_BUFFER];
  char year[MAX_BUFFER];
  char city[MAX_BUFFER];
  char plate[MAX_BUFFER];
  char team[MAX_BUFFER]; // gs, fb, bjk, ts
  char bssid[MAX_BUFFER];
  char essid[MAX_BUFFER];
} UserInfo;

// Globals
int PASS_LEN = 8;
long NUM_PASS = 0;
char *OUTPUT_FILE = NULL;
UserInfo USER_INFO = {0};
long long gen_counter = 0; // Global counter for status updates

// Error handling
void die(const char *message) {
  perror(message);
  exit(EXIT_FAILURE);
}

// State management
long get_last_offset() {
  FILE *file = fopen(STATE_FILE, "r");
  if (file == NULL)
    return 0;
  long offset = 0;
  fscanf(file, "%ld", &offset);
  fclose(file);
  return offset;
}

void save_next_offset(long offset) {
  FILE *file = fopen(STATE_FILE, "w");
  if (file == NULL)
    die("Error: Cannot write state file");
  fprintf(file, "%ld", offset);
  fclose(file);
}

// --- HELPER FUNCTIONS ---

// Update the UI status file with the current password
void update_ui_status(const char *pass) {
    gen_counter++;
    // Write every 3000th password to file to save I/O
    if (gen_counter % 3000 == 0) {
        FILE *fp = fopen(PASS_STATUS_FILE, "w");
        if (fp) {
            fprintf(fp, "%s", pass);
            fclose(fp);
        }
    }
}

void write_pass(FILE *f, const char *pass) {
  if (strlen(pass) < 8) return; 
  
  fprintf(f, "%s\n", pass);
  update_ui_status(pass);
}

// Turkish Character Support (UTF-8 to Lowercase)
void turkish_tolower(char *str) {
  if (!str) return;
  char *p = str;
  char buffer[MAX_BUFFER];
  int j = 0;

  while (*p) {
    unsigned char c = (unsigned char)*p;

    if (c == 0xC3 || c == 0xC4 || c == 0xC5) {
      unsigned char next = (unsigned char)*(p + 1);
      if (!next) break;

      if (c == 0xC4 && next == 0xB0) { buffer[j++] = 'i'; p += 2; } // İ -> i
      else if (c == 0x49) { buffer[j++] = 0xC4; buffer[j++] = 0xB1; p++; } // I -> ı
      else if (c == 0xC5 && next == 0x9E) { buffer[j++] = 0xC5; buffer[j++] = 0x9F; p += 2; } // Ş -> ş
      else if (c == 0xC4 && next == 0x9E) { buffer[j++] = 0xC4; buffer[j++] = 0x9F; p += 2; } // Ğ -> ğ
      else if (c == 0xC3 && next == 0x9C) { buffer[j++] = 0xC3; buffer[j++] = 0xBC; p += 2; } // Ü -> ü
      else if (c == 0xC3 && next == 0x96) { buffer[j++] = 0xC3; buffer[j++] = 0xB6; p += 2; } // Ö -> ö
      else if (c == 0xC3 && next == 0x87) { buffer[j++] = 0xC3; buffer[j++] = 0xA7; p += 2; } // Ç -> ç
      else { buffer[j++] = c; buffer[j++] = next; p += 2; }
    } else {
      if (c == 'I') { buffer[j++] = 0xC4; buffer[j++] = 0xB1; p++; } 
      else { buffer[j++] = tolower(c); p++; }
    }
  }
  buffer[j] = '\0';
  strcpy(str, buffer);
}

void str_reverse(char *str) {
  if (!str) return;
  int len = strlen(str);
  for (int i = 0; i < len / 2; i++) {
    char temp = str[i];
    str[i] = str[len - 1 - i];
    str[len - 1 - i] = temp;
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

// MAC Address Variations
void generate_mac_variations(FILE *out, const char *mac) {
  if (!mac || strlen(mac) < 4) return;
  char clean_mac[MAX_BUFFER];
  int j = 0;
  for (int i = 0; mac[i]; i++) {
    if (isalnum(mac[i])) clean_mac[j++] = tolower(mac[i]);
  }
  clean_mac[j] = '\0';
  int len = strlen(clean_mac);
  if (len < 6) return;

  write_pass(out, clean_mac + len - 4);
  write_pass(out, clean_mac + len - 6);
  write_pass(out, clean_mac);
  
  char buffer[MAX_BUFFER];
  snprintf(buffer, MAX_BUFFER, "superonline%s", clean_mac + len - 4);
  write_pass(out, buffer);
}

void generate_variations(FILE *out, const char *keyword) {
  if (!keyword || strlen(keyword) < 3) return;

  char bases[5][MAX_BUFFER];
  int base_count = 0;

  strncpy(bases[base_count], keyword, MAX_BUFFER); base_count++; // original
  
  strncpy(bases[base_count], bases[0], MAX_BUFFER);
  capitalize(bases[base_count]); base_count++; // Capitalized

  strncpy(bases[base_count], bases[0], MAX_BUFFER);
  str_reverse(bases[base_count]); base_count++; // Reversed

  to_leetspeak(bases[0], bases[base_count]); base_count++; // Leet

  char *numbers[10];
  int num_count = 0;
  if (USER_INFO.year[0]) numbers[num_count++] = USER_INFO.year;
  if (USER_INFO.plate[0]) numbers[num_count++] = USER_INFO.plate;
  numbers[num_count++] = "123";
  numbers[num_count++] = "1905";
  numbers[num_count++] = "1234";
  
  char mac_last4[5] = {0};
  if (USER_INFO.bssid[0]) {
    char clean_mac[MAX_BUFFER];
    int j = 0;
    for (int i = 0; USER_INFO.bssid[i]; i++)
      if (isalnum(USER_INFO.bssid[i])) clean_mac[j++] = USER_INFO.bssid[i];
    clean_mac[j] = '\0';
    int clen = strlen(clean_mac);
    if (clen >= 4) {
      strncpy(mac_last4, clean_mac + clen - 4, 4);
      numbers[num_count++] = mac_last4;
    }
  }

  const char *symbols[] = {".", "_", "!", "@", "*"};
  int sym_count = 5;
  char buffer[MAX_BUFFER];

  for (int i = 0; i < base_count; i++) {
    char *b = bases[i];
    write_pass(out, b);

    for (int n = 0; n < num_count; n++) {
      snprintf(buffer, MAX_BUFFER, "%s%s", b, numbers[n]); write_pass(out, buffer);
      snprintf(buffer, MAX_BUFFER, "%s%s", numbers[n], b); write_pass(out, buffer);
    }

    for (int s = 0; s < sym_count; s++) {
      for (int n = 0; n < num_count; n++) {
        snprintf(buffer, MAX_BUFFER, "%s%s%s", b, symbols[s], numbers[n]); write_pass(out, buffer);
        snprintf(buffer, MAX_BUFFER, "%s%s%s", numbers[n], symbols[s], b); write_pass(out, buffer);
        snprintf(buffer, MAX_BUFFER, "%s%s%s", b, numbers[n], symbols[s]); write_pass(out, buffer);
      }
    }
  }
}

// --- PHASE 1: STATIC PATTERNS ---
void generate_static_patterns(FILE *out) {
  char buffer[MAX_BUFFER];
  if (USER_INFO.essid[0]) generate_variations(out, USER_INFO.essid);
  if (USER_INFO.bssid[0]) generate_mac_variations(out, USER_INFO.bssid);
  if (USER_INFO.name[0]) generate_variations(out, USER_INFO.name);
  if (USER_INFO.surname[0]) generate_variations(out, USER_INFO.surname);
  if (USER_INFO.city[0]) generate_variations(out, USER_INFO.city);

  if (USER_INFO.name[0] && USER_INFO.surname[0]) {
    snprintf(buffer, sizeof(buffer), "%s%s", USER_INFO.name, USER_INFO.surname);
    generate_variations(out, buffer);
  }

  if (USER_INFO.team[0]) {
    if (strstr(USER_INFO.team, "gs")) { write_pass(out, "galatasaray1905"); write_pass(out, "cimbom1905"); }
    else if (strstr(USER_INFO.team, "fb")) { write_pass(out, "fenerbahce1907"); }
    else if (strstr(USER_INFO.team, "bjk")) { write_pass(out, "besiktas1903"); }
    else if (strstr(USER_INFO.team, "ts")) { write_pass(out, "trabzonspor1967"); }
  }

  write_pass(out, "12345678");
  write_pass(out, "123456789");
  write_pass(out, "ttnet123");
  write_pass(out, "superonline");
}

// --- PHASE 2: PI HYBRID MODE ---
void generate_pi_hybrid(long start_offset, long count, FILE *out) {
  long digits_needed = count + PASS_LEN + 20;
  mpfr_prec_t precision = (start_offset + digits_needed) * 3.322 + 64;

  mpfr_t pi;
  mpfr_init2(pi, precision);
  mpfr_const_pi(pi, MPFR_RNDN);

  char *pi_str = NULL;
  mpfr_asprintf(&pi_str, "%.*Rf", (int)(start_offset + digits_needed), pi);
  mpfr_clear(pi);

  if (!pi_str) die("Error: PI string generation failed.");

  char *p = pi_str + 2 + start_offset; // Skip "3."

  char name_leet[MAX_BUFFER] = {0};
  if (USER_INFO.name[0]) to_leetspeak(USER_INFO.name, name_leet);

  char essid_clean[MAX_BUFFER] = {0};
  if (USER_INFO.essid[0]) {
    for (int i = 0; USER_INFO.essid[i]; i++) essid_clean[i] = tolower(USER_INFO.essid[i]);
  }

  for (long i = 0; i < count; i++) {
    if (*(p + PASS_LEN) == '\0') break;

    // 1. Pure Pi
    fprintf(out, "%.*s\n", PASS_LEN, p);
    update_ui_status(p); // Update UI roughly here (p is just a ptr, but update_ui_status takes char*)
                         // Note: We need a temp buffer to send exact pass to UI, but 
                         // sending 'p' blindly prints whole string. Let's fix.
    
    char temp_pass[64];
    snprintf(temp_pass, PASS_LEN + 1, "%s", p);
    update_ui_status(temp_pass);

    // 2. Name + Pi
    if (USER_INFO.name[0]) {
      int name_len = strlen(USER_INFO.name);
      int remaining = PASS_LEN - name_len;
      if (remaining > 0) {
        fprintf(out, "%s%.*s\n", USER_INFO.name, remaining, p);
        
        snprintf(temp_pass, 64, "%s%.*s", USER_INFO.name, remaining, p);
        update_ui_status(temp_pass);
      }
      
      // Name + 4 digits
      fprintf(out, "%s%.*s\n", USER_INFO.name, 4, p);
    }

    // 3. ESSID + Pi
    if (essid_clean[0]) {
      int elen = strlen(essid_clean);
      int rem = PASS_LEN - elen;
      if (rem > 0) fprintf(out, "%s%.*s\n", essid_clean, rem, p);
    }

    p++;
  }

  mpfr_free_str(pi_str);
}

void parse_args(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <len> <count> <out> [options]\n", argv[0]);
    exit(1);
  }

  PASS_LEN = atoi(argv[1]);
  NUM_PASS = atol(argv[2]);
  OUTPUT_FILE = argv[3];

  for (int i = 4; i < argc; i++) {
    if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) strcpy(USER_INFO.name, argv[++i]);
    else if (strcmp(argv[i], "--surname") == 0 && i + 1 < argc) strcpy(USER_INFO.surname, argv[++i]);
    else if (strcmp(argv[i], "--year") == 0 && i + 1 < argc) strcpy(USER_INFO.year, argv[++i]);
    else if (strcmp(argv[i], "--city") == 0 && i + 1 < argc) strcpy(USER_INFO.city, argv[++i]);
    else if (strcmp(argv[i], "--plate") == 0 && i + 1 < argc) strcpy(USER_INFO.plate, argv[++i]);
    else if (strcmp(argv[i], "--team") == 0 && i + 1 < argc) strcpy(USER_INFO.team, argv[++i]);
    else if (strcmp(argv[i], "--bssid") == 0 && i + 1 < argc) strcpy(USER_INFO.bssid, argv[++i]);
    else if (strcmp(argv[i], "--essid") == 0 && i + 1 < argc) strcpy(USER_INFO.essid, argv[++i]);
  }
}

int main(int argc, char *argv[]) {
  parse_args(argc, argv);
  turkish_tolower(USER_INFO.name);
  turkish_tolower(USER_INFO.surname);
  turkish_tolower(USER_INFO.city);
  turkish_tolower(USER_INFO.team);
  turkish_tolower(USER_INFO.essid);

  FILE *out;
  if (strcmp(OUTPUT_FILE, "/dev/stdout") == 0) out = stdout;
  else {
    out = fopen(OUTPUT_FILE, "w");
    if (!out) die("Error: Cannot open output file");
  }

  long start_offset = get_last_offset();

  if (start_offset == 0) generate_static_patterns(out);
  generate_pi_hybrid(start_offset, NUM_PASS, out);

  save_next_offset(start_offset + NUM_PASS);

  if (out != stdout) {
    printf("[+] Done. Next offset: %ld\n", start_offset + NUM_PASS);
    fclose(out);
  }
  mpfr_free_cache();
  return 0;
}