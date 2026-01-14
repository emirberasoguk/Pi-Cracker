#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

// --- Constants & Config ---
#define MAX_PI_DROPS 80
#define MAX_FILES 50
#define MAX_LOG_LINES 18
#define TMP_DIR "/tmp/pi_temp"

// --- Structures ---
typedef struct { float x; float y; float speed; char value[2]; Color color; } PiDrop;
typedef struct { 
    char essid[64]; char bssid[64]; char name[64]; 
    char surname[64]; char year[16]; char city[64]; 
    char plate[8]; int teamIndex; 
    int passLen;
} TargetProfile;

// --- Globals ---
char filesCap[MAX_FILES][256]; char pathsCap[MAX_FILES][512]; int countCap = 0;
char filesHc[MAX_FILES][256]; char pathsHc[MAX_FILES][512]; int countHc = 0;
int selectedFileIndex = -1; int selectedFileType = 0; // 1=cap, 2=hc22000
bool hasHashcat = false; bool hasAircrack = false; bool hasGPU = false;
int attackMode = 0; 
char logHistory[MAX_LOG_LINES][128];
bool monitorMode = false;
bool passwordFound = false;
char foundPassword[128] = "";

// Status Globals
char currentPhase[64] = "IDLE";
char currentSpeed[64] = "0 H/s";
char currentCycle[64] = "0";
char currentTriedPass[128] = "Initializing...";
float cpuTemp = 0.0f;
float gpuTemp = 0.0f;

// --- Helper Functions ---
void AddLog(const char* msg) {
    for(int i=0; i<MAX_LOG_LINES-1; i++) strcpy(logHistory[i], logHistory[i+1]);
    strncpy(logHistory[MAX_LOG_LINES-1], msg, 127);
    logHistory[MAX_LOG_LINES-1][127] = '\0';
}

void DetectTools() {
    if (system("command -v hashcat > /dev/null 2>&1") == 0) { 
        hasHashcat = true; 
        if (system("hashcat -I | grep -Ei 'CUDA|OpenCL|NVIDIA|AMD|Intel' > /dev/null 2>&1") == 0) hasGPU = true; 
    }
    if (system("command -v aircrack-ng > /dev/null 2>&1") == 0) hasAircrack = true;
    AddLog("> System Tools Detected.");
}

void ScanCaptureDirectory() {
    countCap = 0; countHc = 0;
    const char *basePath = "../Handshake/captures"; struct stat st; if (stat(basePath, &st) != 0) basePath = "Handshake/captures";
    
    char path[512]; snprintf(path, sizeof(path), "%s/cap", basePath); DIR *dr = opendir(path);
    if (dr) { struct dirent *de; while ((de = readdir(dr)) && countCap < MAX_FILES) { if (strstr(de->d_name, ".cap")) { snprintf(filesCap[countCap], 255, "%s", de->d_name); snprintf(pathsCap[countCap++], 511, "%s/%s", path, de->d_name); } } closedir(dr); }
    
    snprintf(path, sizeof(path), "%s/hc22000", basePath); dr = opendir(path);
    if (dr) { struct dirent *de; while ((de = readdir(dr)) && countHc < MAX_FILES) { if (strstr(de->d_name, ".hc22000")) { snprintf(filesHc[countHc], 255, "%s", de->d_name); snprintf(pathsHc[countHc++], 511, "%s/%s", path, de->d_name); } } closedir(dr); }
}

// Extract hex string to ASCII
void HexToAscii(char* hex, char* ascii) {
    int len = strlen(hex);
    int i = 0, j = 0;
    while(i < len) {
        char tmp[3] = { hex[i], hex[i+1], '\0' };
        ascii[j++] = (char)strtol(tmp, NULL, 16);
        i += 2;
    }
    ascii[j] = '\0';
}

// Smart Parse for HC22000
void ParseHC22000(const char *filepath, TargetProfile *t) {
    FILE *f = fopen(filepath, "r");
    if (f) {
        char line[1024];
        if (fgets(line, sizeof(line), f)) {
            char *token = strtok(line, "*");
            int count = 0;
            while (token) {
                if (count == 3) strncpy(t->bssid, token, 63); 
                else if (count == 5) HexToAscii(token, t->essid);
                token = strtok(NULL, "*");
                count++;
            }
        }
        fclose(f);
    }
}

void CompileEngine() { system("gcc pi_generator_dynamic.c -o pi_generator_dynamic -lmpfr -lgmp"); }

void ExportConfig(TargetProfile t, const char *captureFile) {
    char path[256];
    snprintf(path, sizeof(path), "%s/pi_config.env", TMP_DIR);
    FILE *f = fopen(path, "w");
    if (!f) return;
    char teamCode[5] = "";
    if (t.teamIndex == 1) strcpy(teamCode, "gs"); else if (t.teamIndex == 2) strcpy(teamCode, "fb");
    else if (t.teamIndex == 3) strcpy(teamCode, "bjk"); else if (t.teamIndex == 4) strcpy(teamCode, "ts");

    fprintf(f, "TARGET_FILE=\"%s\"\n", captureFile);
    fprintf(f, "UI_ATTACK_MODE=\"%d\"\n", attackMode);
    fprintf(f, "TARGET_ESSID=\"%s\"\n", t.essid);
    fprintf(f, "TARGET_BSSID=\"%s\"\n", t.bssid);
    fprintf(f, "TARGET_NAME=\"%s\"\n", t.name);
    fprintf(f, "TARGET_SURNAME=\"%s\"\n", t.surname);
    fprintf(f, "TARGET_YEAR=\"%s\"\n", t.year);
    fprintf(f, "TARGET_CITY=\"%s\"\n", t.city);
    fprintf(f, "TARGET_PLATE=\"%s\"\n", t.plate);
    fprintf(f, "TARGET_TEAM=\"%s\"\n", teamCode);
    fprintf(f, "TARGET_PASS_LEN=\"%d\"\n", t.passLen > 0 ? t.passLen : 8);
    fclose(f);
    
    snprintf(path, sizeof(path), "%s/pi_start.signal", TMP_DIR);
    FILE *sig = fopen(path, "w"); if(sig) fclose(sig);
}

float GetCPUTemp() {
    float temp = 0.0f;
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (f) { fscanf(f, "%f", &temp); temp /= 1000.0f; fclose(f); }
    return temp;
}

float GetGPUTemp() {
    if (!hasGPU) return 0.0f;
    float temp = 0.0f;
    FILE *fp = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader", "r");
    if (fp) { if(fscanf(fp, "%f", &temp) != 1) temp = 0.0f; pclose(fp); }
    return temp;
}

void ReadStatusFiles() {
    char path[256];
    
    // 1. Phase & Cycle
    snprintf(path, sizeof(path), "%s/pi_status.txt", TMP_DIR);
    FILE *f = fopen(path, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "PHASE=")) {
                char *v = strchr(line, '='); if(v) { strncpy(currentPhase, v+1, 63); currentPhase[strcspn(currentPhase, "\n")] = 0; }
            } else if (strstr(line, "CYCLE=")) {
                char *v = strchr(line, '='); if(v) { strncpy(currentCycle, v+1, 63); currentCycle[strcspn(currentCycle, "\n")] = 0; }
            }
        }
        fclose(f);
    }

    // 2. Real Password
    snprintf(path, sizeof(path), "%s/pi_current_pass.txt", TMP_DIR);
    FILE *fp = fopen(path, "r");
    if (fp) {
        fscanf(fp, "%127s", currentTriedPass);
        fclose(fp);
    }

    // 3. Speed (Log is in TMP_DIR)
    snprintf(path, sizeof(path), "tail -n 2 %s/attack.log 2>/dev/null", TMP_DIR);
    FILE *fl = popen(path, "r");
    if (fl) {
        char line[512];
        while(fgets(line, sizeof(line), fl)) {
             if (strstr(line, "H/s") || strstr(line, "kH/s")) {
                 char *s = strstr(line, "Speed");
                 if (s) { strncpy(currentSpeed, s, 63); } 
                 else {
                     if(strlen(line) < 60) strncpy(currentSpeed, line, 63);
                     else strncpy(currentSpeed, "Running...", 63);
                 }
             }
        }
        currentSpeed[strcspn(currentSpeed, "\n")] = 0;
        pclose(fl);
    }

    // 4. Cracked
    snprintf(path, sizeof(path), "%s/cracked.key", TMP_DIR);
    FILE *fk = fopen(path, "r");
    if (fk) {
        if(fgets(foundPassword, 127, fk)) {
            passwordFound = true;
        }
        fclose(fk);
    }
}

int main(void) {
    InitWindow(1200, 800, "Pi-Cracker v6.2 - HYBRID OPS"); SetTargetFPS(60);
    DetectTools(); CompileEngine(); ScanCaptureDirectory();

    PiDrop drops[MAX_PI_DROPS];
    for(int i=0; i<MAX_PI_DROPS; i++) { 
        drops[i].x = GetRandomValue(0, 1200); 
        drops[i].y = GetRandomValue(-800, 0); 
        drops[i].speed = GetRandomValue(5, 12); 
        sprintf(drops[i].value, "%d", GetRandomValue(0, 9)); 
        drops[i].color = Fade(LIME, GetRandomValue(30,90)/100.0f);
    }

    TargetProfile target = {0}; target.passLen = 8; int activeTeam = 0; bool editMode[6] = {0}; bool editModePlate = false; bool editModeLen = false;
    bool showBrowser = false;

    // Cleanup legacy files in current dir just in case
    remove("pi_start.signal"); remove("pi_status.txt"); remove("attack.log"); remove("cracked.key");

    while (!WindowShouldClose()) {
        for(int i=0; i<MAX_PI_DROPS; i++) { 
            drops[i].y += drops[i].speed; 
            if(drops[i].y > 800) { drops[i].y = -20; drops[i].x = GetRandomValue(0, 1200); sprintf(drops[i].value, "%d", GetRandomValue(0, 9)); } 
        }

        if (monitorMode) {
            static int frameCounter = 0;
            if (frameCounter++ > 30) { 
                cpuTemp = GetCPUTemp();
                gpuTemp = GetGPUTemp();
                ReadStatusFiles();
                frameCounter = 0;
            }
        }

        BeginDrawing(); 
        ClearBackground(GetColor(0x050505ff));

        for(int i=0; i<MAX_PI_DROPS; i++) DrawText(drops[i].value, drops[i].x, drops[i].y, 20, drops[i].color);

        DrawRectangle(0,0,1200,60,Fade(BLACK,0.95f)); 
        DrawText("PI-CRACKER", 20, 15, 30, GREEN); 
        DrawText(monitorMode ? "ACTIVE MONITOR" : "CONFIGURATION", 220, 25, 10, monitorMode ? RED : GOLD); 
        DrawLine(0, 60, 1200, 60, DARKGREEN);

        if (!monitorMode) {
            Rectangle lP = {20,80,320,700}; DrawRectangleRec(lP,Fade(BLACK,0.8f)); DrawRectangleLinesEx(lP,1,DARKGREEN);
            DrawText("TARGET PROFILE",40,95,20,GREEN);
            int y=140; 
            GuiLabel((Rectangle){40,y,200,20},"WiFi Name (ESSID)"); if(GuiTextBox((Rectangle){40,y+20,280,30},target.essid,64,editMode[0])) editMode[0]=!editMode[0];
            y+=65; GuiLabel((Rectangle){40,y,200,20},"MAC (BSSID)"); if(GuiTextBox((Rectangle){40,y+20,280,30},target.bssid,64,editMode[1])) editMode[1]=!editMode[1];
            y+=65; GuiLabel((Rectangle){40,y,130,20},"Name"); if(GuiTextBox((Rectangle){40,y+20,130,30},target.name,64,editMode[2])) editMode[2]=!editMode[2];
            GuiLabel((Rectangle){190,y,130,20},"Surname"); if(GuiTextBox((Rectangle){190,y+20,130,30},target.surname,64,editMode[3])) editMode[3]=!editMode[3];
            y+=65; GuiLabel((Rectangle){40,y,280,20},"Birth Year"); if(GuiTextBox((Rectangle){40,y+20,280,30},target.year,16,editMode[4])) editMode[4]=!editMode[4];
            y+=65; GuiLabel((Rectangle){40,y,130,20},"City"); if(GuiTextBox((Rectangle){40,y+20,130,30},target.city,64,editMode[5])) editMode[5]=!editMode[5];
            GuiLabel((Rectangle){190,y,130,20},"Plate"); if(GuiTextBox((Rectangle){190,y+20,130,30},target.plate,8,editModePlate)) editModePlate=!editModePlate;
            y+=65; GuiLabel((Rectangle){40,y,280,20},"Team"); GuiToggleGroup((Rectangle){40,y+20,55,30},"NONE;GS;FB;BJK;TS",&activeTeam); target.teamIndex=activeTeam;
            
            y+=65; GuiLabel((Rectangle){40,y,150,20},"Min Password Len"); 
            if (GuiSpinner((Rectangle){40, y+20, 100, 30}, NULL, &target.passLen, 8, 63, editModeLen)) editModeLen = !editModeLen;

            Rectangle rP = {880,80,300,700}; DrawRectangleRec(rP,Fade(BLACK,0.8f)); DrawRectangleLinesEx(rP,1,DARKGREEN);
            DrawText("SETTINGS",900,95,20,GREEN);
            DrawText("ATTACK MODE:", 900, 140, 10, GRAY); GuiToggleGroup((Rectangle){900, 155, 80, 30}, "AUTO;HASHCAT;AIRCRACK", &attackMode);
            
            DrawText("DETECTED TOOLS:", 900, 220, 10, GRAY);
            DrawText(hasHashcat ? " [X] Hashcat" : " [ ] Hashcat", 900, 240, 20, hasHashcat ? LIME : RED);
            DrawText(hasAircrack ? " [X] Aircrack-ng" : " [ ] Aircrack-ng", 900, 265, 20, hasAircrack ? LIME : RED);
            DrawText(hasGPU ? " [X] GPU (NVIDIA)" : " [ ] GPU (NVIDIA)", 900, 290, 20, hasGPU ? GOLD : DARKGRAY);

            if(showBrowser) {
                Rectangle cP = {360,80,500,700}; DrawRectangleRec(cP,Fade(BLACK,0.95f)); DrawRectangleLinesEx(cP,2,LIME);
                DrawText("SELECT HANDSHAKE FILE", 380, 100, 20, LIME);
                int listY = 140;
                for(int i=0; i<countHc; i++) { 
                    if(GuiButton((Rectangle){380, listY, 460, 30}, filesHc[i])) { 
                        selectedFileIndex=i; selectedFileType=2; showBrowser=false; 
                        ParseHC22000(pathsHc[i], &target); 
                    } listY+=35; 
                }
                listY+=20;
                for(int i=0; i<countCap; i++) { 
                    if(GuiButton((Rectangle){380, listY, 460, 30}, filesCap[i])) { 
                        selectedFileIndex=i; selectedFileType=1; showBrowser=false; 
                        char *f=strrchr(filesCap[i],'/'); if(f) f++; else f=filesCap[i];
                        char t[64]; strncpy(t,f,63); char *d=strrchr(t,'.'); if(d) *d=0; strncpy(target.essid,t,63);
                    } listY+=35; 
                } 
                if(GuiButton((Rectangle){380, 700, 460, 30}, "CANCEL")) showBrowser=false;
            } else {
                Rectangle cP = {360,80,500,200}; DrawRectangleRec(cP,Fade(BLACK,0.5f)); DrawRectangleLinesEx(cP,1,DARKGREEN);
                DrawText("TARGET FILE:", 380, 100, 10, GRAY);
                const char* fName = (selectedFileType==0) ? "NONE SELECTED" : ((selectedFileType==1) ? filesCap[selectedFileIndex] : filesHc[selectedFileIndex]);
                DrawText(fName, 380, 115, 20, WHITE);
                if(GuiButton((Rectangle){750, 100, 90, 40}, "BROWSE")) { showBrowser=true; ScanCaptureDirectory(); }

                if(GuiButton((Rectangle){460,500,300,80},"START ATTACK")) { 
                    if(selectedFileType != 0) {
                        const char* fPath = (selectedFileType==1) ? pathsCap[selectedFileIndex] : pathsHc[selectedFileIndex]; 
                        ExportConfig(target, fPath);
                        monitorMode = true; 
                    }
                }
            }
        } 
        else { 
            // === MONITOR PAGE ===
            DrawRectangle(0, 60, 1200, 50, Fade(BLACK, 0.8f)); DrawLine(0, 110, 1200, 110, DARKGREEN);
            
            char tempStr[32]; sprintf(tempStr, "CPU: %.0f C", cpuTemp);
            DrawText(tempStr, 30, 75, 20, (cpuTemp > 70) ? RED : LIME);

            if (hasGPU) {
                char gpuStr[32]; sprintf(gpuStr, "GPU: %.0f C", gpuTemp);
                DrawText(gpuStr, 150, 75, 20, (gpuTemp > 75) ? RED : GOLD);
            }
            
            char cycleStr[64]; sprintf(cycleStr, "CYCLE: %s", currentCycle);
            DrawText(cycleStr, 350, 75, 20, SKYBLUE);

            char phaseStr[64]; sprintf(phaseStr, "STATUS: %s", currentPhase);
            DrawText(phaseStr, 600, 75, 20, ORANGE);

            if (passwordFound) {
                DrawRectangle(200, 200, 800, 400, BLACK);
                DrawRectangleLines(200, 200, 800, 400, LIME);
                DrawText("PASSWORD FOUND!", 400, 300, 40, LIME);
                DrawText(foundPassword, 400, 380, 30, WHITE);
                DrawText("Process Completed.", 430, 450, 20, GRAY);
                if (GuiButton((Rectangle){450, 500, 300, 50}, "EXIT")) break;
            } else {
                DrawRectangle(100, 200, 1000, 400, Fade(BLACK, 0.8f));
                DrawRectangleLines(100, 200, 1000, 400, DARKGREEN);
                
                DrawText("CURRENT SPEED", 130, 230, 10, GRAY);
                DrawText(currentSpeed, 130, 250, 50, LIME);
                
                DrawText("TRYING PASSWORD:", 130, 450, 20, DARKGRAY);
                DrawText(currentTriedPass, 130, 480, 40, WHITE);

                DrawText("ATTACK LOG", 600, 230, 10, GRAY);
                DrawRectangleLines(600, 250, 450, 300, DARKGREEN);
                DrawText("> Attack running...", 610, 260, 10, LIME);
                DrawText(TextFormat("> Phase: %s", currentPhase), 610, 280, 10, GREEN);
                if (hasGPU) DrawText(TextFormat("> GPU Temp: %.1f", gpuTemp), 610, 300, 10, GREEN);
                DrawText("> Engine: Pi-Generator v2", 610, 320, 10, DARKGREEN);

                if (GuiButton((Rectangle){450, 700, 300, 60}, "STOP ATTACK")) break;
            }
        }
        EndDrawing();
    }
    
    char path[256];
    snprintf(path, sizeof(path), "%s/pi_stop.signal", TMP_DIR);
    FILE *f = fopen(path, "w"); if(f) fclose(f);
    CloseWindow(); 
    return 0;
}
