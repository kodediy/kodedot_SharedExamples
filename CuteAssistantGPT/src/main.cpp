/*
 * CuteAssistant - AI Voice Assistant with ESP32-S3 Kode Dot
 * ---------------------------------------------------------
 * - Touch screen to talk to the assistant
 * - Animated cute eyes respond to interaction
 * - AI responses shown on screen with typewriter effect
 * - Eyes disappear when showing text responses
 * - GPIO control via voice commands
 */
#include <Arduino.h>
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <vector>
#include <ESP32Servo.h>
#include <Wire.h>
#include <PMIC_BQ25896.h>
#include <Wire.h>
#include <PMIC_BQ25896.h>

// ==================== API SELECTION ====================
// Uncomment ONE of these to choose API implementation:
// #define USE_REALTIME_API      // WebSocket streaming (ultra-low latency)
#define USE_CHAT_API          // HTTP POST (stable, proven)

// Project libraries
#include <audio_manager/AudioManager.h>
// Nuevo main KS Ticker con interfaz EXACTA solicitada

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>
#include <lvgl.h>
#include <kodedot/display_manager.h>
#include <TCA9555.h>
#include <kodedot/pin_config.h>
#include <audio_manager/AudioManager.h>
#include <wifi_manager_lib/WiFiManager.h>
#include "images/PWL.h"

// Fuentes e imagen
LV_FONT_DECLARE(Inter_70);
LV_FONT_DECLARE(Inter_40);
LV_FONT_DECLARE(Inter_30);
LV_FONT_DECLARE(Inter_20);

// Instancia del gestor de display
DisplayManager display;

// Audio: usar AudioManager de la librería del proyecto
static AudioManager audioManager;

// IO Expander (necesario para audio)
static TCA9555 ioexp(IOEXP_I2C_ADDR);

// Colores
#define COLOR_BLACK   0x000000
#define COLOR_WHITE   0xFFFFFF
#define COLOR_GREEN_KS  0x00D26E
#define COLOR_GREEN   0x00FF00
#define COLOR_RED     0xFF0000

// Dimensiones
#define HEADER_HEIGHT        280
#define PRICE_SECTION_HEIGHT 120
#define CHANGE_PADDING       30

// Fuentes
#define FONT_TITLE       &Inter_40
#define FONT_PRICE       &Inter_70
#define FONT_PAIR        &Inter_20
#define FONT_CHANGE      &Inter_30
#define FONT_CONNECTING  &Inter_40

// Datos de Kickstarter
struct KickstarterData {
    float pledged;
    int backers;
    float goal;
    float percentFunded;
    unsigned long lastUpdate;
};

static KickstarterData ksData = {0, 0, 5000.0, 0, 0};

// Variables para animación de contador
static float targetPledged = 0;
static float currentDisplayPledged = 0;
static int targetBackers = 0;
static int currentDisplayBackers = 0;
static bool isAnimating = false;

// UI
static lv_obj_t* price_label = nullptr;
static lv_obj_t* change_label = nullptr;
static lv_obj_t* pair_label = nullptr;
// Guardamos el contenedor del precio para poder forzar su repintado y evitar artefactos
static lv_obj_t* price_container = nullptr;

// Timers
static unsigned long lastAPICall = 0;

// Declaraciones
static void createKSTickerUI();
static void fetchKickstarterData();
static void updateUI();
static void animateCounterUp();
static String formatPrice(float price);
static String formatBackers(int backers);
static void freeMemory();
static void useSimulatedData();
static uint32_t freeInternal();
static uint32_t freePsram();
 
void setup() {
    Serial.begin(115200);
    Serial.println("🚀 KS Ticker iniciado");

    if (!display.init()) {
        Serial.println("❌ Error: No se pudo inicializar el display");
        while (true) { delay(1000); }
    }

    // Inicializar I2C para el IO Expander
    Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);

    // Inicializar IO Expander (necesario para audio)
    if (!ioexp.begin(INPUT)) {
        Serial.println("⚠️ Warning: IO Expander no conectado");
    }

    // Inicializar AudioManager con pines de altavoz y conectar el expander
    AudioConfig audioCfg;
    audioCfg.spkSckPin   = SPK_I2S_SCK;
    audioCfg.spkWsPin    = SPK_I2S_WS;
    audioCfg.spkDoutPin  = SPK_I2S_DOUT;
    audioCfg.ampExpanderPin = EXPANDER_SPK_SHUTDOWN; // control de amplificador por expander

    if (audioManager.init(audioCfg)) {
        audioManager.attachExpander(&ioexp, audioCfg.ampExpanderPin);
        // Pitido de bienvenida usando el motor interno de AudioManager
        audioManager.playBeep(1200, 200, 7000);
        Serial.println("🔊 Beep de inicio (AudioManager)");
    } else {
        Serial.println("⚠️ No se pudo inicializar AudioManager");
    }

    // Mensaje de conexión
    lv_obj_t* connecting = lv_label_create(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_text_font(connecting, FONT_CONNECTING, 0);
    lv_obj_set_style_text_color(connecting, lv_color_hex(COLOR_WHITE), 0);
    lv_label_set_text(connecting, "Connecting...");
    lv_obj_center(connecting);
    display.update();

    // WiFi desde SD
    if (wifiManager.loadCredentialsFromSD()) {
        wifiManager.printLoadedNetworks();
        wifiManager.connectToWiFi();
    } else {
        Serial.println("Error al cargar credenciales WiFi desde SD");
    }

    lv_obj_del(connecting);

    createKSTickerUI();

    if (WiFi.status() == WL_CONNECTED) {
        fetchKickstarterData();
        lastAPICall = millis();
    }
}

void loop() {
    display.update();

    static unsigned long lastWiFiCheck = 0;
    static unsigned long lastMemoryCleanup = 0;
    static int failedAPIAttempts = 0;
    
    // Animar contador si hay animación activa
    if (isAnimating) {
        animateCounterUp();
    }
    
    // Check WiFi every 30 seconds
    if (millis() - lastWiFiCheck >= 30000) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔄 Reconectando WiFi...");
            wifiManager.connectToWiFi();
        }
        lastWiFiCheck = millis();
    }

    // Memory cleanup every 5 minutes or after failed API attempts
    if (millis() - lastMemoryCleanup >= 300000 || failedAPIAttempts >= 3) {
        freeMemory();
        lastMemoryCleanup = millis();
        failedAPIAttempts = 0;
    }

    // API call with adaptive interval based on memory and failures
    unsigned long apiInterval = API_UPDATE_INTERVAL;
    if (ESP.getFreeHeap() < 40000 || failedAPIAttempts > 0) {
        apiInterval = API_UPDATE_INTERVAL * 2; // Duplicar intervalo si hay problemas
    }
    
    if (WiFi.status() == WL_CONNECTED &&
        (lastAPICall == 0 || millis() - lastAPICall >= apiInterval)) {
        
        // Solo intentar si hay suficiente memoria
        if (ESP.getFreeHeap() >= 30000) {
            uint32_t heapBefore = ESP.getFreeHeap();
            fetchKickstarterData();
            uint32_t heapAfter = ESP.getFreeHeap();
            
            // Detectar si hubo problema de memoria
            if (heapAfter < heapBefore * 0.6) { // Si se perdió más del 40% de memoria
                failedAPIAttempts++;
                Serial.printf("⚠️ Pérdida de memoria, intentos fallidos: %d\n", failedAPIAttempts);
            } else {
                failedAPIAttempts = 0; // Reset si todo fue bien
            }
        } else {
            Serial.printf("⚠️ Memoria insuficiente (%u), saltando API call\n", ESP.getFreeHeap());
            failedAPIAttempts++;
        }
        
        lastAPICall = millis();
    }

    updateUI();
    delay(50);
}

static void createKSTickerUI() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Header negro
    lv_obj_t* header = lv_obj_create(scr);
    lv_obj_set_size(header, LV_PCT(100), HEADER_HEIGHT);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, -30);
    lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);

    // Logo PWL (arriba-izquierda)
    lv_obj_t* pwl_img = lv_img_create(scr);
    lv_img_set_src(pwl_img, &PWL);
    lv_obj_align(pwl_img, LV_ALIGN_TOP_LEFT, 15, 15);
    lv_obj_move_foreground(pwl_img);

    // Título a la derecha
    lv_obj_t* title_container = lv_obj_create(header);
    lv_obj_set_size(title_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(title_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_container, 0, 0);
    lv_obj_set_style_pad_all(title_container, 0, 0);
    lv_obj_set_flex_flow(title_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(title_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(title_container, LV_ALIGN_RIGHT_MID, -40, 0);

    lv_obj_t* ks_label = lv_label_create(title_container);
    lv_obj_set_style_text_font(ks_label, FONT_TITLE, 0);
    lv_obj_set_style_text_color(ks_label, lv_color_hex(COLOR_WHITE), 0);
    lv_label_set_text(ks_label, "KS");

    lv_obj_t* ticker_label = lv_label_create(title_container);
    lv_obj_set_style_text_font(ticker_label, FONT_TITLE, 0);
    lv_obj_set_style_text_color(ticker_label, lv_color_hex(COLOR_WHITE), 0);
    lv_label_set_text(ticker_label, "Ticker");

    // Par Pedged
    pair_label = lv_label_create(header);
    lv_obj_set_style_text_font(pair_label, FONT_PAIR, 0);
    lv_obj_set_style_text_color(pair_label, lv_color_hex(COLOR_WHITE), 0);
    lv_label_set_text(pair_label, "Pedged");
    lv_obj_align(pair_label, LV_ALIGN_BOTTOM_LEFT, 20, 0);

    // Banner verde KS del precio
    // Contenedor del precio (guardado en variable global para poder invalidarlo al actualizar)
    price_container = lv_obj_create(scr);
    lv_obj_set_size(price_container, LV_PCT(100), PRICE_SECTION_HEIGHT);
    lv_obj_align_to(price_container, header, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(price_container, lv_color_hex(COLOR_GREEN_KS), 0);
    lv_obj_set_style_border_width(price_container, 0, 0);
    lv_obj_set_style_pad_all(price_container, 0, 0);
    lv_obj_set_flex_flow(price_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(price_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    price_label = lv_label_create(price_container);
    lv_obj_set_style_text_font(price_label, FONT_PRICE, 0);
    lv_obj_set_style_text_color(price_label, lv_color_hex(COLOR_BLACK), 0);
    // Hacemos el fondo del label opaco y verde KS para que al reducir dígitos no quede "fantasma".
    lv_obj_set_style_bg_color(price_label, lv_color_hex(COLOR_GREEN_KS), 0);
    lv_obj_set_style_bg_opa(price_label, LV_OPA_COVER, 0);
    lv_label_set_text(price_label, "$0.00");

    // Info de % y backers
    lv_obj_t* info_container = lv_obj_create(scr);
    lv_obj_set_size(info_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align_to(info_container, price_container, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(info_container, lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_border_width(info_container, 0, 0);
    lv_obj_set_style_pad_all(info_container, CHANGE_PADDING, 0);
    lv_obj_set_flex_flow(info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    change_label = lv_label_create(info_container);
    lv_obj_set_style_text_font(change_label, FONT_CHANGE, 0);
        lv_obj_set_style_text_color(change_label, lv_color_hex(COLOR_GREEN_KS), 0);
    lv_label_set_text(change_label, "0%   0 backers");
}

// Reutilizamos instancias globales para evitar fragmentación repetitiva
static WiFiClientSecure g_secureClient;
static HTTPClient g_httpsClient;
static bool g_httpsConfigured = false;

static void fetchKickstarterData() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("🌐 Preparando petición HTTPS...");
    uint32_t freeBefore = ESP.getFreeHeap();
    Serial.printf("🧠 Memoria disponible: %u bytes\n", freeBefore);
    
    // Si aún no hay suficiente memoria, abortar
    if (freeBefore < 50000) {
        Serial.printf("❌ Memoria insuficiente: %u bytes. Mínimo: 50KB\n", freeBefore);
        createKSTickerUI(); // Recrear UI
        useSimulatedData();
        return;
    }

    // Liberar cliente anterior completamente
    g_httpsClient.end();
    g_secureClient.stop();
    delay(300);

    if (!g_httpsConfigured) {
        g_secureClient.setInsecure();
        g_secureClient.setTimeout(15000);
        g_secureClient.setHandshakeTimeout(10000);
        g_httpsConfigured = true;
    }

    String url = "https://www.kickstarter.com/projects/kode/kode-dot-the-all-in-one-pocket-size-maker-device/stats.json?v=1";
    Serial.printf("🌐 Iniciando HTTPS GET (heap:%u)...\n", ESP.getFreeHeap());

    if (!g_httpsClient.begin(g_secureClient, url)) {
        Serial.println("❌ begin() falló");
        createKSTickerUI();
        useSimulatedData();
        return;
    }

    g_httpsClient.addHeader("Accept", "application/json");
    g_httpsClient.addHeader("User-Agent", "KSTicker/1.0");
    g_httpsClient.addHeader("Connection", "close");
    g_httpsClient.setTimeout(15000);
    g_httpsClient.setReuse(false);

    Serial.printf("🔐 Heap antes GET: %u\n", ESP.getFreeHeap());
    int code = g_httpsClient.GET();
    Serial.printf("📡 Heap después GET: %u\n", ESP.getFreeHeap());
    
    if (code == HTTP_CODE_OK) {
        String payload = g_httpsClient.getString();
        Serial.println("✅ Respuesta recibida");
        
        if (payload.length() > 0 && payload.indexOf("project") >= 0) {
            StaticJsonDocument<384> doc;
            auto err = deserializeJson(doc, payload);
            if (!err) {
                float pledged = doc["project"]["pledged"].as<String>().toFloat();
                int backers = doc["project"]["backers_count"] | 0;
                
                if (pledged > 0) {
                    ksData.pledged = pledged;
                    ksData.backers = backers;
                    ksData.percentFunded = (pledged / ksData.goal) * 100.0f;
                    ksData.lastUpdate = millis();
                    Serial.printf("✅ Pledged=$%.2f Backers=%d (%.1f%%)\n", 
                                  pledged, backers, ksData.percentFunded);
                }
            } else {
                Serial.printf("❌ JSON err: %s\n", err.c_str());
            }
        }
    } else {
        Serial.printf("❌ HTTPS code: %d\n", code);
        if (code == -1) {
            Serial.println("⚠️ Error de conexión SSL/TLS - memoria insuficiente");
        }
    }

    g_httpsClient.end();
    g_secureClient.stop();
    delay(50);
    
    uint32_t freeAfter = ESP.getFreeHeap();
    Serial.printf("🧠 Post fetch: %u bytes (delta:%d)\n", freeAfter, (int)freeAfter - (int)freeBefore);
}

static void updateUI() {
    if (ksData.pledged <= 0) return;

    // Detectar si hay incremento y activar animación + beep
    if (ksData.pledged > targetPledged || ksData.backers > targetBackers) {
        // Detectar incremento en backers para reproducir sonido
        bool backersIncreased = (ksData.backers > targetBackers);
        
        targetPledged = ksData.pledged;
        targetBackers = ksData.backers;
        
        // Si es la primera vez, establecer valores directamente sin animación
        if (currentDisplayPledged == 0) {
            currentDisplayPledged = targetPledged;
            currentDisplayBackers = targetBackers;
        } else {
            isAnimating = true;

            // Beep alegre si suben los backers
            if (backersIncreased) {
                audioManager.playBeep(1500, 120, 7000);
            }
        }
    }

    // Forzamos el repintado completo del contenedor antes de cambiar el texto para evitar restos de pixeles
    if (price_container) {
        lv_obj_invalidate(price_container);
    }
    lv_label_set_text(price_label, formatPrice(currentDisplayPledged).c_str());

    String infoText = formatBackers(currentDisplayBackers);
    lv_label_set_text(change_label, infoText.c_str());

    // Siempre verde para KS
        lv_obj_set_style_text_color(change_label, lv_color_hex(COLOR_GREEN_KS), 0);
}

static String formatPrice(float price) {
    char buffer[32];
    if (price >= 100000)      sprintf(buffer, "$%.0f", price);
    else if (price >= 10000)  sprintf(buffer, "$%.1f", price);
    else                      sprintf(buffer, "$%.2f", price);

    String s = String(buffer);
    int dot = s.indexOf('.');
    if (dot == -1) dot = s.length();
    for (int i = dot - 3; i > 1; i -= 3) s = s.substring(0, i) + "," + s.substring(i);
    return s;
}

static String formatBackers(int backers) {
    char buffer[64];
    sprintf(buffer, "%.0f%% - %d backers", ksData.percentFunded, backers);
    return String(buffer);
}

static void animateCounterUp() {
    static unsigned long lastAnimUpdate = 0;
    const unsigned long animInterval = 30; // Actualizar cada 30ms para animación suave
    
    if (millis() - lastAnimUpdate < animInterval) {
        return;
    }
    lastAnimUpdate = millis();
    
    bool pledgedDone = false;
    bool backersDone = false;
    
    // Animar pledged
    if (currentDisplayPledged < targetPledged) {
        float diff = targetPledged - currentDisplayPledged;
        float increment = diff * 0.15; // Incrementar 15% de la diferencia cada frame
        if (increment < 1.0) increment = 1.0; // Mínimo 1 para números pequeños
        
        currentDisplayPledged += increment;
        if (currentDisplayPledged >= targetPledged) {
            currentDisplayPledged = targetPledged;
            pledgedDone = true;
        }
    } else {
        pledgedDone = true;
    }
    
    // Animar backers
    if (currentDisplayBackers < targetBackers) {
        int diff = targetBackers - currentDisplayBackers;
        int increment = (int)(diff * 0.15); // Incrementar 15% de la diferencia cada frame
        if (increment < 1) increment = 1; // Mínimo 1
        
        currentDisplayBackers += increment;
        if (currentDisplayBackers >= targetBackers) {
            currentDisplayBackers = targetBackers;
            backersDone = true;
        }
    } else {
        backersDone = true;
    }
    
    // Si ambos terminaron, desactivar animación
    if (pledgedDone && backersDone) {
        isAnimating = false;
    }
}

static void freeMemory() {
    // Force garbage collection and memory cleanup
    Serial.printf("🧹 Limpieza de memoria - Heap libre: %d bytes\n", ESP.getFreeHeap());

    WiFi.disconnect(true);
    delay(100);

    // Flush DNS cache to help with resolution issues
    WiFi.dnsIP(0);
    WiFi.dnsIP(1);

    // Force heap defragmentation
    heap_caps_malloc_extmem_enable(1000);

    Serial.printf("🧹 Memoria después limpieza: %d bytes\n", ESP.getFreeHeap());
}

static void useSimulatedData() {
    // Datos base de Kickstarter con variación aleatoria
    static unsigned long lastSimUpdate = 0;
    
    // Actualizar datos simulados cada 2 minutos
    if (ksData.pledged == 0 || millis() - lastSimUpdate > 120000) {
        float basePledged = 174408.0;
        int baseBackers = 987;
        
        // Añadir variación aleatoria
        float pledgedVariation = (random(-1000, 1001));
        int backersVariation = random(-10, 11);
        
        ksData.pledged = basePledged + pledgedVariation;
        ksData.backers = baseBackers + backersVariation;
        ksData.percentFunded = (ksData.pledged / ksData.goal) * 100.0f;
        ksData.lastUpdate = millis();
        lastSimUpdate = millis();
        
        Serial.printf("⚠️ Datos simulados: $%.2f (%d backers, %.0f%%)\n", 
                      ksData.pledged, ksData.backers, ksData.percentFunded);
    }
}

// Helpers memoria
static uint32_t freeInternal() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL); }
static uint32_t freePsram() {
#ifdef BOARD_HAS_PSRAM
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
#else
    return 0;
#endif
}