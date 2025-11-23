#include <wifi_manager_lib/WiFiManager.h>
#include <kodedot/pin_config.h>
#include <Arduino.h>
#include <SD_MMC.h>

// Global instance
WiFiManager wifiManager;

// Optional demo API constants
const char* COINGECKO_API_URL = "https://api.coingecko.com/api/v3/simple/price";
const unsigned long API_UPDATE_INTERVAL = 30000; // 30s

// Global API key storage
String OPENAI_API_KEY_STR;

bool WiFiManager::ensureSDMounted() {
    if (sdMounted) return true;
    SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0);
    if (!SD_MMC.begin(SD_MOUNT_POINT, true)) {
        Serial.println("[WiFiManager] ERROR: SD card init failed (SD_MMC)");
        Serial.printf("[WiFiManager] Hint: Check SDMMC pins CMD=%d CLK=%d D0=%d\n", SD_PIN_CMD, SD_PIN_CLK, SD_PIN_D0);
        sdMounted = false;
        return false;
    }
    sdMounted = true;
    return true;
}

// Load WiFi credentials from /wifi.txt on SD
bool WiFiManager::loadCredentialsFromSD() {
    Serial.println("[WiFiManager] Loading WiFi credentials from SD...");
    if (!ensureSDMounted()) return false;

    if (!SD_MMC.exists("/wifi.txt")) {
        Serial.println("[WiFiManager] ERROR: /wifi.txt not found on SD");
        return false;
    }

    File wifiFile = SD_MMC.open("/wifi.txt", FILE_READ);
    if (!wifiFile) {
        Serial.println("[WiFiManager] ERROR: Cannot open /wifi.txt");
        return false;
    }

    networkCount = 0;
    String line;
    WiFiCredentials currentNetwork;

    Serial.println("[WiFiManager] Reading /wifi.txt...");

    while (wifiFile.available() && networkCount < 3) {
        line = wifiFile.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) { continue; }
        char c0 = line.charAt(0);
        if (c0 == '#' || (line.length() >= 2 && line.charAt(0) == '/' && line.charAt(1) == '/')) {
            continue;
        }
        if (line.startsWith("NAME=")) {
            currentNetwork.name = line.substring(5);
            currentNetwork.name.trim();
        } else if (line.startsWith("SSID=")) {
            currentNetwork.ssid = line.substring(5);
            currentNetwork.ssid.trim();
        } else if (line.startsWith("PASSWORD=")) {
            currentNetwork.password = line.substring(9);
            currentNetwork.password.trim();
        } else if (line.startsWith("---") || line.startsWith("END")) {
            if (currentNetwork.ssid.length() > 0) {
                currentNetwork.isValid = true;
                networks[networkCount] = currentNetwork;
                networkCount++;
                Serial.printf("[WiFiManager] Loaded WiFi #%d: %s (%s)\n",
                              networkCount,
                              currentNetwork.name.c_str(),
                              currentNetwork.ssid.c_str());
                currentNetwork = WiFiCredentials();
            }
        }
    }

    if (currentNetwork.ssid.length() > 0 && networkCount < 3) {
        currentNetwork.isValid = true;
        networks[networkCount] = currentNetwork;
        networkCount++;
        Serial.printf("[WiFiManager] Loaded WiFi #%d: %s (%s)\n",
                      networkCount,
                      currentNetwork.name.c_str(),
                      currentNetwork.ssid.c_str());
    }

    wifiFile.close();
    if (networkCount == 0) {
        Serial.println("[WiFiManager] ERROR: No valid WiFi networks found in /wifi.txt");
        return false;
    }
    Serial.printf("[WiFiManager] Total WiFi networks loaded: %d\n", networkCount);
    return true; // Keep SD mounted for future reads
}

// Connect to WiFi using loaded networks
bool WiFiManager::connectToWiFi() {
    if (networkCount == 0) {
        Serial.println("[WiFiManager] ERROR: No WiFi networks loaded");
        return false;
    }
    Serial.println("[WiFiManager] Attempting WiFi connection...");

    for (int i = 0; i < networkCount; i++) {
        if (!networks[i].isValid) continue;
        
        // Try each network up to 2 times with 4 second timeout each
        for (int attempt = 1; attempt <= 2; attempt++) {
            Serial.printf("[WiFiManager] Connecting to: %s (%s) - Attempt %d/2\n",
                          networks[i].name.c_str(), networks[i].ssid.c_str(), attempt);
            
            WiFi.begin(networks[i].ssid.c_str(), networks[i].password.c_str());
            int checks = 0;
            while (WiFi.status() != WL_CONNECTED && checks < 8) { // 8 checks * 500ms = 4 seconds
                delay(500);
                Serial.print('.');
                checks++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                currentNetworkIndex = i;
                Serial.println();
                Serial.printf("[WiFiManager] Connected to %s\n", networks[i].name.c_str());
                Serial.printf("[WiFiManager] IP: %s\n", WiFi.localIP().toString().c_str());
                Serial.printf("[WiFiManager] RSSI: %d dBm\n", WiFi.RSSI());
                return true;
            } else {
                Serial.println();
                Serial.printf("[WiFiManager] Connection failed: %s (attempt %d/2)\n", 
                             networks[i].name.c_str(), attempt);
                WiFi.disconnect();
                if (attempt < 2) {
                    delay(500); // Short delay between attempts on same network
                } else {
                    delay(1000); // Longer delay before trying next network
                }
            }
        }
    }
    Serial.println("[WiFiManager] ERROR: Could not connect to any WiFi network");
    return false;
}

// Set progress callback
void WiFiManager::setProgressCallback(ProgressCallback callback) {
    progressCallback = callback;
}

// Print loaded networks
void WiFiManager::printLoadedNetworks() {
    Serial.println("\n[WiFiManager] Loaded WiFi networks:");
    Serial.println("=================================");
    for (int i = 0; i < networkCount; i++) {
        if (!networks[i].isValid) continue;
        Serial.printf("%d. %s\n", i + 1, networks[i].name.c_str());
        Serial.printf("   SSID: %s\n", networks[i].ssid.c_str());
        String passHidden = "";
        passHidden.reserve(networks[i].password.length());
        for (size_t j = 0; j < (size_t)networks[i].password.length(); ++j) passHidden += '*';
        Serial.printf("   Password: %s\n", passHidden.c_str());
        if (i == currentNetworkIndex) {
            Serial.println("   Status: CONNECTED");
        }
        Serial.println();
    }
}

// Get current network name
String WiFiManager::getCurrentNetworkName() {
    if (currentNetworkIndex >= 0 && currentNetworkIndex < networkCount) {
        return networks[currentNetworkIndex].name;
    }
    return "Disconnected";
}

// Load API keys from /apis.txt on SD
bool WiFiManager::loadApiKeysFromSD() {
    Serial.println("[WiFiManager] Loading API keys from SD (/apis.txt)...");
    if (!ensureSDMounted()) return false;
    if (!SD_MMC.exists("/apis.txt")) {
        Serial.println("[WiFiManager] ERROR: /apis.txt not found on SD");
        return false;
    }
    File apis = SD_MMC.open("/apis.txt", FILE_READ);
    if (!apis) {
        Serial.println("[WiFiManager] ERROR: Cannot open /apis.txt");
        return false;
    }
    String line;
    while (apis.available()) {
        line = apis.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        char c0 = line.charAt(0);
        if (c0 == '#' || (line.length() >= 2 && line.charAt(0) == '/' && line.charAt(1) == '/')) continue;
        int eq = line.indexOf('=');
        if (eq <= 0) continue;
        String key = line.substring(0, eq); key.trim();
        String val = line.substring(eq + 1); val.trim();
        if (key.equalsIgnoreCase("OPENAI")) {
            OPENAI_API_KEY_STR = val;
            Serial.printf("[WiFiManager] OPENAI key loaded (%u chars)\n", (unsigned)OPENAI_API_KEY_STR.length());
        }
    }
    apis.close();
    if (OPENAI_API_KEY_STR.length() == 0) {
        Serial.println("[WiFiManager] WARNING: OPENAI key not found in /apis.txt");
        return false;
    }
    return true;
}


