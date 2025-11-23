/*
 * DOOM for Kode Dot (ESP32-S3)
 * ------------------------------------------------------------
 * Port of the Arduboy Doom game to Kode Dot hardware
 * Uses SSD1306 display and IO expander buttons
 */
#include <Arduino.h>
#include <Wire.h>
#include <kodedot/display_manager.h>
#include <TCA9555.h>
#include <kodedot/pin_config.h>
// LED
#include <led_manager/LEDManager.h>
// Audio
#include <audio_manager/AudioManager.h>

// Game includes
#include "constants.h"
#include "level.h"
#include "sprites.h"
#include "types.h"
#include "entities.h"

// LVGL image asset (Logo) used as an overlay during gameplay
extern "C" {
  extern const lv_image_dsc_t Logo; // declared in src/images/Logo.c
  extern const lv_image_dsc_t Character; // declared in src/images/Character.c
  // LVGL font (Inter 30 px) for intro hint text
  extern const lv_font_t Inter_30; // declared in src/fonts/Inter_30.c
}

// --- Speaker I2S pin fallbacks ---
#ifndef SPK_I2S_SCK
#define SPK_I2S_SCK MIC_I2S_SCK
#endif
#ifndef SPK_I2S_WS
#define SPK_I2S_WS MIC_I2S_WS
#endif
#ifndef SPK_I2S_DOUT
#define SPK_I2S_DOUT -1
#endif

// Useful macros
#define swap(a, b)            do { typeof(a) temp = a; a = b; b = temp; } while (0)
#define sign(a, b)            (double) (a > b ? 1 : (b > a ? -1 : 0))

// Override min/max to handle type mismatches
#undef min
#undef max
template<typename T, typename U>
auto min(T a, U b) -> decltype(a < b ? a : b) {
  return (a < b) ? a : b;
}
template<typename T, typename U>
auto max(T a, U b) -> decltype(a > b ? a : b) {
  return (a > b) ? a : b;
}

// Display (Kode Dot panel via DisplayManager) + 1bpp game framebuffer
DisplayManager displayMgr;
Arduino_CO5300* gfx = nullptr;
static uint8_t display_buf[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];
static uint8_t sprite_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];    // generic sprites (items, fireballs)
static uint8_t enemy_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];     // enemies only
static uint8_t enemy_center_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)]; // enemy original (center) pixels
static uint8_t fire_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];      // muzzle flash (yellow outer)
static uint8_t fire_center_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)]; // muzzle flash center (red)
static uint8_t gun_hand_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];  // player's hand part of gun (outer - darker orange)
static uint8_t gun_hand_center_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)]; // hand center (lighter orange)
static uint8_t gun_metal_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)]; // metal part of gun (outer - darker gray)
static uint8_t gun_metal_center_mask[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)]; // metal center (lighter gray)
static const uint16_t COLOR_BLACK = 0x0000;
static const uint16_t COLOR_WHITE = 0xFFFF;

// IO Expander for buttons
static TCA9555 ioexp(IOEXP_I2C_ADDR);

// Gradient pattern for shading (uses GRADIENT_* constants from sprites.h)
const static uint8_t PROGMEM bit_mask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
#define read_bit(b, n)      b & pgm_read_byte(bit_mask + n) ? 1 : 0

// Z-buffer for depth
uint8_t zbuffer[ZBUFFER_SIZE];

// FPS control
double delta = 1;
uint32_t lastFrameTime = 0;

// General game state
uint8_t scene = INTRO;
bool exit_scene = false;
bool invert_screen = false;
uint8_t flash_screen = 0;
uint8_t muzzle_timer = 0; // frames to show muzzle flash after firing

// Player and entities
Player player;
Entity entity[MAX_ENTITIES];
StaticEntity static_entity[MAX_STATIC_ENTITIES];
uint8_t num_entities = 0;
uint8_t num_static_entities = 0;

// LED manager
static LEDManager g_led;
static uint8_t g_led_flash_timer = 0; // frames for firing flash

// Audio manager (match sample naming)
static AudioManager audioManager;
static bool audio_ready = false;

// LVGL overlay pointer for gameplay logo
static lv_obj_t* g_game_logo = nullptr;
static lv_obj_t* g_game_character = nullptr;
// LVGL label for intro hint
static lv_obj_t* g_intro_label = nullptr;
// Rate-limit LVGL updates to reduce overhead when using direct panel flush
static uint32_t g_lvgl_last_update_ms = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void updateHud();
uint8_t getBlockAt(const uint8_t level[], uint8_t x, uint8_t y);
Coords translateIntoView(Coords *pos);
void sortEntities();
bool input_fire();
bool input_fire();

// Local framebuffer helpers (1bpp like SSD1306 memory layout)
static inline void bufferClear() { memset(display_buf, 0x00, sizeof(display_buf)); }
static inline bool bufferGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (display_buf[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline bool maskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (sprite_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void bufferSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) display_buf[idx] |= bit; else display_buf[idx] &= ~bit;
}
static inline void maskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) sprite_mask[idx] |= bit; else sprite_mask[idx] &= ~bit;
}
static inline bool fireMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (fire_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void fireMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) fire_mask[idx] |= bit; else fire_mask[idx] &= ~bit;
}
static inline bool fireCenterMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (fire_center_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void fireCenterMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) fire_center_mask[idx] |= bit; else fire_center_mask[idx] &= ~bit;
}
static inline bool gunHandMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (gun_hand_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void gunHandMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) gun_hand_mask[idx] |= bit; else gun_hand_mask[idx] &= ~bit;
}
static inline bool gunMetalMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (gun_metal_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void gunMetalMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) gun_metal_mask[idx] |= bit; else gun_metal_mask[idx] &= ~bit;
}
static inline bool gunHandCenterMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (gun_hand_center_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void gunHandCenterMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) gun_hand_center_mask[idx] |= bit; else gun_hand_center_mask[idx] &= ~bit;
}
static inline bool gunMetalCenterMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (gun_metal_center_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void gunMetalCenterMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) gun_metal_center_mask[idx] |= bit; else gun_metal_center_mask[idx] &= ~bit;
}
static inline bool enemyMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (enemy_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void enemyMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) enemy_mask[idx] |= bit; else enemy_mask[idx] &= ~bit;
}
static inline bool enemyCenterMaskGetPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;
  return (enemy_center_mask[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1;
}
static inline void enemyCenterMaskSetPixel(int16_t x, int16_t y, bool on) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  uint16_t idx = (y >> 3) * SCREEN_WIDTH + x;
  uint8_t  bit = 1 << (y & 7);
  if (on) enemy_center_mask[idx] |= bit; else enemy_center_mask[idx] &= ~bit;
}
static void fillRectBuffer(int16_t x, int16_t y, int16_t w, int16_t h, bool on) {
  if (w <= 0 || h <= 0) return;
  int16_t x2 = x + w - 1, y2 = y + h - 1;
  if (x2 < 0 || y2 < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
  int16_t sx = max<int16_t>(0, x);
  int16_t sy = max<int16_t>(0, y);
  int16_t ex = min<int16_t>(SCREEN_WIDTH - 1, x2);
  int16_t ey = min<int16_t>(SCREEN_HEIGHT - 1, y2);
  for (int16_t yy = sy; yy <= ey; ++yy) {
    for (int16_t xx = sx; xx <= ex; ++xx) {
      bufferSetPixel(xx, yy, on);
    }
  }
}

// Draw a row-major 1bpp bitmap (Adafruit-style) into our mono buffer
static void drawBitmap1BPP(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h) {
  int16_t byteWidth = (w + 7) / 8;
  for (int16_t j = 0; j < h; ++j) {
    for (int16_t i = 0; i < w; ++i) {
      uint8_t b = pgm_read_byte(bmp + j * byteWidth + (i >> 3));
      if (b & (0x80 >> (i & 7))) bufferSetPixel(x + i, y + j, true);
    }
  }
}

// Draw 1bpp bitmap and also mark sprite_mask for colorization (no separate mask)
static void drawBitmapMark1BPP(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h) {
  int16_t byteWidth = (w + 7) / 8;
  for (int16_t j = 0; j < h; ++j) {
    for (int16_t i = 0; i < w; ++i) {
      uint8_t b = pgm_read_byte(bmp + j * byteWidth + (i >> 3));
      if (b & (0x80 >> (i & 7))) {
        int16_t px = x + i;
        int16_t py = y + j;
        bufferSetPixel(px, py, true);
        maskSetPixel(px, py, true);
      }
    }
  }
}

// Draw muzzle flash into buffer and fire_mask with dilation for better fill
// Center pixels (original sprite) go to fire_center_mask (red), dilated go to fire_mask (yellow)
static void drawBitmapFire1BPP(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h) {
  int16_t byteWidth = (w + 7) / 8;
  
  // First pass: draw original pixels and mark center
  for (int16_t j = 0; j < h; ++j) {
    for (int16_t i = 0; i < w; ++i) {
      uint8_t b = pgm_read_byte(bmp + j * byteWidth + (i >> 3));
      if (b & (0x80 >> (i & 7))) {
        int16_t px = x + i;
        int16_t py = y + j;
        bufferSetPixel(px, py, true);
        fireMaskSetPixel(px, py, true);
        fireCenterMaskSetPixel(px, py, true); // mark center as red
      }
    }
  }
  
  // Second pass: dilate fire pixels (4-neighbors) for yellow border
  static uint8_t tempFire[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];
  memcpy(tempFire, fire_mask, sizeof(tempFire));
  
  for (int16_t py = 0; py < SCREEN_HEIGHT; ++py) {
    for (int16_t px = 0; px < SCREEN_WIDTH; ++px) {
      if (((tempFire[(py >> 3) * SCREEN_WIDTH + px] >> (py & 7)) & 0x1) != 0) {
        const int dxs[4] = {1, -1, 0, 0};
        const int dys[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
          int16_t nx = px + dxs[k];
          int16_t ny = py + dys[k];
          if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT) {
            bufferSetPixel(nx, ny, true);
            fireMaskSetPixel(nx, ny, true);
            // Don't mark dilated pixels as center - they stay yellow
          }
        }
      }
    }
  }
}

// Draw gun bitmap and split into hand vs metal masks by local Y threshold
// Also apply dilation to fill gaps, with center vs outer distinction
static void drawGunBitmap1BPP(int16_t x, int16_t y, const uint8_t* bmp, const uint8_t* msk, int16_t w, int16_t h) {
  const int handThresholdFromBottom = 12; // bottom rows considered hand
  int16_t byteWidth = (w + 7) / 8;
  
  // First pass: draw original pixels and mark centers
  for (int16_t j = 0; j < h; ++j) {
    for (int16_t i = 0; i < w; ++i) {
      uint8_t mb = pgm_read_byte(msk + j * byteWidth + (i >> 3));
      if (!(mb & (0x80 >> (i & 7)))) continue;
      uint8_t pb = pgm_read_byte(bmp + j * byteWidth + (i >> 3));
      if (pb & (0x80 >> (i & 7))) {
        int16_t px = x + i;
        int16_t py = y + j;
        bufferSetPixel(px, py, true);
        if (j >= (h - handThresholdFromBottom)) {
          gunHandMaskSetPixel(px, py, true);
          gunHandCenterMaskSetPixel(px, py, true); // mark original as center
        } else {
          gunMetalMaskSetPixel(px, py, true);
          gunMetalCenterMaskSetPixel(px, py, true); // mark original as center
        }
      }
    }
  }
  
  // Second pass: dilate gun pixels (4-neighbors) to create outer border
  static uint8_t tempHand[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];
  static uint8_t tempMetal[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];
  memcpy(tempHand, gun_hand_mask, sizeof(tempHand));
  memcpy(tempMetal, gun_metal_mask, sizeof(tempMetal));
  
  for (int16_t py = 0; py < SCREEN_HEIGHT; ++py) {
    for (int16_t px = 0; px < SCREEN_WIDTH; ++px) {
      bool isHand = ((tempHand[(py >> 3) * SCREEN_WIDTH + px] >> (py & 7)) & 0x1) != 0;
      bool isMetal = ((tempMetal[(py >> 3) * SCREEN_WIDTH + px] >> (py & 7)) & 0x1) != 0;
      
      if (isHand || isMetal) {
        // Dilate 4 neighbors
        const int dxs[4] = {1, -1, 0, 0};
        const int dys[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
          int16_t nx = px + dxs[k];
          int16_t ny = py + dys[k];
          if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT) {
            bufferSetPixel(nx, ny, true);
            if (isHand) gunHandMaskSetPixel(nx, ny, true);
            else gunMetalMaskSetPixel(nx, ny, true);
            // Don't mark dilated pixels as center - they stay as outer border
          }
        }
      }
    }
  }
}

// Draw a 1bpp bitmap with 1bpp mask into mono buffer, also marks sprite_mask for colorization
static void drawMaskedBitmap1BPP(int16_t x, int16_t y, const uint8_t* bmp, const uint8_t* msk, int16_t w, int16_t h) {
  int16_t byteWidth = (w + 7) / 8;
  for (int16_t j = 0; j < h; ++j) {
    for (int16_t i = 0; i < w; ++i) {
      uint8_t mb = pgm_read_byte(msk + j * byteWidth + (i >> 3));
      if (!(mb & (0x80 >> (i & 7)))) continue; // masked out
      uint8_t pb = pgm_read_byte(bmp + j * byteWidth + (i >> 3));
      if (pb & (0x80 >> (i & 7))) {
        bufferSetPixel(x + i, y + j, true);
        maskSetPixel(x + i, y + j, true);
      }
    }
  }
}

// Flush mono framebuffer to panel (centered), with optional invert mapping.
// Optionally skip clearing the top/bottom letterbox bars to avoid touching areas the game never uses.
static void flushToPanel(bool invert = false, bool clearBars = true) {
  if (!gfx) return;

  // Scale the 128x64 mono buffer to full panel WIDTH while preserving aspect
  // Resulting destination height: dstH = LCD_WIDTH * SCREEN_HEIGHT / SCREEN_WIDTH
  // Use even-aligned origin and write two rows per chunk to match panel requirements
  const int16_t dstW = LCD_WIDTH;                  // full width
  const int16_t dstH_raw = (LCD_WIDTH * SCREEN_HEIGHT) / SCREEN_WIDTH; // 410*64/128 = 205
  const int16_t dstH = (dstH_raw & ~1);            // make even -> 204
  const int16_t ox = 0;                            // full width, start at x=0 (already even)
  const int16_t oy = (((LCD_HEIGHT - dstH) / 2) & ~1); // centered and even-aligned

  // Precompute mapping from destination to source (nearest neighbor)
  static bool maps_built = false;
  static uint16_t mapX[LCD_WIDTH];
  static uint16_t mapY[512]; // max panel height safeguard
  if (!maps_built) {
    for (int16_t x = 0; x < dstW; ++x) {
      // srcX = round(x * SCREEN_WIDTH / dstW)
      mapX[x] = (uint16_t)((int32_t)x * SCREEN_WIDTH / dstW);
      if (mapX[x] >= SCREEN_WIDTH) mapX[x] = SCREEN_WIDTH - 1;
    }
    for (int16_t y = 0; y < dstH; ++y) {
      mapY[y] = (uint16_t)((int32_t)y * SCREEN_HEIGHT / dstH_raw); // use raw to keep ratio precise
      if (mapY[y] >= SCREEN_HEIGHT) mapY[y] = SCREEN_HEIGHT - 1;
    }
    maps_built = true;
  }

  static uint16_t twoLines[LCD_WIDTH * 2];

  // Fill top and bottom bars with black (only if requested)
  if (clearBars) {
    if (oy > 0) {
      gfx->fillRect(0, 0, LCD_WIDTH, oy, COLOR_BLACK);
    }
    int16_t bottomH = LCD_HEIGHT - (oy + dstH);
    if (bottomH > 0) {
      gfx->fillRect(0, oy + dstH, LCD_WIDTH, bottomH, COLOR_BLACK);
    }
  }

  auto rgb565 = [](uint8_t r, uint8_t g, uint8_t b) -> uint16_t {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  };
  auto wallBrownFromDist = [&](uint8_t dist, uint16_t sx, uint16_t sy) -> uint16_t {
    // Gray concrete palette with ordered-dither-like texture and subtle horizontal lines
    // Distance to 4 levels (near brighter)
    uint8_t level = 3 - (dist >> 6); // 0..3 -> far..near; invert to near high
    if (level > 3) level = 0;

    // Base gray ramp (concrete gray - equal RGB values with slight variation)
    int base = 70 + level * 35;  // 70,105,140,175 - darker to lighter
    int r = base;
    int g = base;
    int b = base + 5; // slight blue tint for concrete look

    // Ordered texture: 4x4 threshold pattern to add variation
    uint8_t pat = ((sx & 3) << 2) | (sy & 3); // 0..15
    int tweak = (pat < 4) ? 8 : (pat > 11 ? -8 : 0);
    r = max(0, min(255, r + tweak));
    g = max(0, min(255, g + tweak));
    b = max(0, min(255, b + tweak));

    // Horizontal mortar-ish line every 8 px of source Y (very subtle)
    if ((sy & 7) == 0 && (sx & 1) == 0) {
      r = (r * 4) / 5; // darken slightly
      g = (g * 4) / 5;
      b = (b * 4) / 5;
    }

    return rgb565((uint8_t)r, (uint8_t)g, (uint8_t)b);
  };
  const uint16_t HUD_COLOR = 0x07FF;      // cyan
  // Doom Imp-like browns
  const uint16_t ENEMY_OUTER_COLOR  = 0x8083; // dark brown border
  const uint16_t ENEMY_CENTER_COLOR = 0xD707; // lighter brown center
  const uint16_t SPRITE_COLOR = 0xFD20;   // orange (items/fireballs)
  const uint16_t FIRE_COLOR = 0xFFE0;     // yellow for muzzle flash outer
  const uint16_t FIRE_CENTER_COLOR = 0xF920; // light red/orange for muzzle flash center
  const uint16_t HAND_COLOR = 0xE300;     // darker orange for hand outer border
  const uint16_t HAND_CENTER_COLOR = 0xFD20; // lighter orange for hand center
  const uint16_t GUN_METAL_COLOR = 0x8410; // darker gray for metal outer border
  const uint16_t GUN_METAL_CENTER_COLOR = 0xC618; // lighter gray for metal center

  gfx->startWrite();
  for (int16_t dy = 0; dy < dstH; dy += 2) {
    uint16_t sy0 = mapY[dy];
    uint16_t sy1 = mapY[dy + 1];
    bool colorize = (scene == GAME_PLAY) || (scene == INTRO);
    for (int16_t dx = 0; dx < dstW; ++dx) {
      uint16_t sx = mapX[dx];
      bool on0 = bufferGetPixel(sx, sy0);
      bool on1 = bufferGetPixel(sx, sy1);
      if (invert) { on0 = !on0; on1 = !on1; }

      if (!colorize) {
        twoLines[dx] = on0 ? COLOR_WHITE : COLOR_BLACK;
        twoLines[dstW + dx] = on1 ? COLOR_WHITE : COLOR_BLACK;
        continue;
      }

      if (!on0) twoLines[dx] = COLOR_BLACK;
      else if (scene == INTRO) {
        // Intro coloring: masked pixels colored by band (logo vs. text), others white
        if (maskGetPixel(sx, sy0)) {
          twoLines[dx] = (sy0 >= (SCREEN_HEIGHT * 7) / 10) ? 0xFFE0 /*yellow*/ : 0xFD20 /*orange*/;
        } else {
          twoLines[dx] = COLOR_WHITE;
        }
  } else if (enemyCenterMaskGetPixel(sx, sy0)) twoLines[dx] = ENEMY_CENTER_COLOR; // enemy center first
  else if (enemyMaskGetPixel(sx, sy0)) twoLines[dx] = ENEMY_OUTER_COLOR; // enemy border
  else if (fireCenterMaskGetPixel(sx, sy0)) twoLines[dx] = FIRE_CENTER_COLOR; // muzzle flash center (red)
  else if (fireMaskGetPixel(sx, sy0)) twoLines[dx] = FIRE_COLOR;   // muzzle flash outer (yellow)
  else if (gunHandCenterMaskGetPixel(sx, sy0)) twoLines[dx] = HAND_CENTER_COLOR; // hand center (lighter)
  else if (gunHandMaskGetPixel(sx, sy0)) twoLines[dx] = HAND_COLOR;      // hand outer (darker)
  else if (gunMetalCenterMaskGetPixel(sx, sy0)) twoLines[dx] = GUN_METAL_CENTER_COLOR; // metal center (lighter)
  else if (gunMetalMaskGetPixel(sx, sy0)) twoLines[dx] = GUN_METAL_COLOR;// metal outer (darker)
  else if (maskGetPixel(sx, sy0)) twoLines[dx] = SPRITE_COLOR;     // other sprites (items/fireballs)
      else if (sy0 >= 58) twoLines[dx] = HUD_COLOR;
      else {
        uint8_t zi = (uint8_t)min<uint16_t>(sx / Z_RES_DIVIDER, ZBUFFER_SIZE - 1);
        twoLines[dx] = wallBrownFromDist(zbuffer[zi], sx, sy0);
      }

      if (!on1) twoLines[dstW + dx] = COLOR_BLACK;
      else if (scene == INTRO) {
        if (maskGetPixel(sx, sy1)) {
          twoLines[dstW + dx] = (sy1 >= (SCREEN_HEIGHT * 7) / 10) ? 0xFFE0 : 0xFD20;
        } else {
          twoLines[dstW + dx] = COLOR_WHITE;
        }
  } else if (enemyCenterMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = ENEMY_CENTER_COLOR;
  else if (enemyMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = ENEMY_OUTER_COLOR;
  else if (fireCenterMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = FIRE_CENTER_COLOR; // center red
  else if (fireMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = FIRE_COLOR; // outer yellow
  else if (gunHandCenterMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = HAND_CENTER_COLOR; // hand center
  else if (gunHandMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = HAND_COLOR; // hand outer
  else if (gunMetalCenterMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = GUN_METAL_CENTER_COLOR; // metal center
  else if (gunMetalMaskGetPixel(sx, sy1)) twoLines[dstW + dx] = GUN_METAL_COLOR; // metal outer
  else if (maskGetPixel(sx, sy1)) twoLines[dstW + dx] = SPRITE_COLOR;
      else if (sy1 >= 58) twoLines[dstW + dx] = HUD_COLOR;
      else {
        uint8_t zi = (uint8_t)min<uint16_t>(sx / Z_RES_DIVIDER, ZBUFFER_SIZE - 1);
        twoLines[dstW + dx] = wallBrownFromDist(zbuffer[zi], sx, sy1);
      }
    }
    gfx->writeAddrWindow(ox, oy + dy, dstW, 2);
    gfx->writePixels(twoLines, dstW * 2);
  }
  gfx->endWrite();
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

void fps() {
  while (millis() - lastFrameTime < FRAME_TIME);
  delta = (double)(millis() - lastFrameTime) / FRAME_TIME;
  lastFrameTime = millis();
}

double getActualFps() {
  return 1000 / (FRAME_TIME * delta);
}

void drawByte(uint8_t x, uint8_t y, uint8_t b) {
  display_buf[(y / 8)*SCREEN_WIDTH + x] = b;
}

// Gradient helper used by fade and vertical lines
static inline bool getGradientPixel(uint8_t x, uint8_t y, uint8_t i) {
  if (i == 0) return false;
  if (i >= GRADIENT_COUNT - 1) return true;
  uint8_t ii = i;
  if (ii > (GRADIENT_COUNT - 1)) ii = GRADIENT_COUNT - 1;
  // Compute lookup index into gradient table
  uint16_t base = (uint16_t)ii * (GRADIENT_WIDTH * GRADIENT_HEIGHT);
  uint16_t idx = base
               + (uint16_t)((y * GRADIENT_WIDTH) % (GRADIENT_WIDTH * GRADIENT_HEIGHT))
               + (uint16_t)((x / GRADIENT_HEIGHT) % GRADIENT_WIDTH);
  uint8_t byte = pgm_read_byte(gradient + idx);
  return (byte >> (x % 8)) & 0x1;
}

void fadeScreen(uint8_t intensity, bool color = 0) {
  for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
      if (getGradientPixel(x, y, intensity)) {
        if (color) bufferSetPixel(x, y, true);
        else bufferSetPixel(x, y, false);
      }
    }
  }
}

void drawPixel(int8_t x, int8_t y, bool color, bool raycasterViewport = false) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= (raycasterViewport ? RENDER_HEIGHT : SCREEN_HEIGHT)) {
    return;
  }
  
  bufferSetPixel(x, y, color);
}

void drawVLine(uint8_t x, int8_t start_y, int8_t end_y, uint8_t intensity) {
  int8_t y;
  int8_t lower_y = max(min(start_y, end_y), 0);
  int8_t higher_y = min(max(start_y, end_y), RENDER_HEIGHT - 1);
  uint8_t c;
  
  uint8_t bp;
  uint8_t b;
  for (c = 0; c < RES_DIVIDER; c++) {
    y = lower_y;
    b = 0;
    while (y <= higher_y) {
      bp = y % 8;
      b = b | getGradientPixel(x + c, y, intensity) << bp;
      
      if (bp == 7) {
        drawByte(x + c, y, b);
        b = 0;
      }
      y++;
    }
    
    if (bp != 7) {
      drawByte(x + c, y - 1, b);
    }
  }
}

void drawSprite(int8_t x, int8_t y, const uint8_t bitmap[], const uint8_t mask[],
                int16_t w, int16_t h, uint8_t sprite, double distance, bool isEnemy = false) {
  uint8_t tw = (double) w / distance;
  uint8_t th = (double) h / distance;
  uint8_t byte_width = w / 8;
  uint8_t pixel_size = max(1, 1.0 / distance);
  uint16_t sprite_offset = byte_width * h * sprite;
  
  bool pixel;
  bool maskPixel;
  
  if (zbuffer[min(max(x, 0), ZBUFFER_SIZE - 1) / Z_RES_DIVIDER] < distance * DISTANCE_MULTIPLIER) {
    return;
  }
  
  for (uint8_t ty = 0; ty < th; ty += pixel_size) {
    if (y + ty < 0 || y + ty >= RENDER_HEIGHT) {
      continue;
    }
    
    uint8_t sy = ty * distance;
    
    for (uint8_t tx = 0; tx < tw; tx += pixel_size) {
      uint8_t sx = tx * distance;
      
      maskPixel = read_bit(pgm_read_byte(mask + sprite_offset + sy * byte_width + sx / 8), sx % 8);
      
      if (maskPixel) {
        pixel = read_bit(pgm_read_byte(bitmap + sprite_offset + sy * byte_width + sx / 8), sx % 8);
        
        for (uint8_t iy = 0; iy < pixel_size; iy++) {
          for (uint8_t ix = 0; ix < pixel_size; ix++) {
            // Write sprite pixel and mark it in sprite_mask for colorization
            if (pixel) {
              int16_t px = x + tx + ix;
              int16_t py = y + ty + iy;
              bufferSetPixel(px, py, true);
              if (isEnemy) {
                // Mark original enemy pixel as center and also in overall enemy mask
                enemyCenterMaskSetPixel(px, py, true);
                enemyMaskSetPixel(px, py, true);
              }
              else maskSetPixel(px, py, true);
            }
          }
        }
      }
    }
  }
}

void drawChar(int8_t x, int8_t y, char ch) {
  const char* char_map = CHAR_MAP;
  uint8_t index = 0;
  
  while (char_map[index] != '\0' && char_map[index] != ch) {
    index++;
  }
  
  if (char_map[index] == '\0') return;
  
  for (uint8_t row = 0; row < CHAR_HEIGHT; row++) {
    uint8_t line = pgm_read_byte(bmp_font + row * bmp_font_width + index);
    for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
      if (line & (1 << (7 - col))) {
        drawPixel(x + col, y + row, 1, false);
      }
    }
  }
}

void drawText(int8_t x, int8_t y, const char *txt, uint8_t space = 1) {
  for (uint8_t i = 0; txt[i] != '\0'; i++) {
    drawChar(x + i * (CHAR_WIDTH + space), y, txt[i]);
  }
}

void drawText(int8_t x, int8_t y, int value) {
  char buf[10];
  itoa(value, buf, 10);
  drawText(x, y, buf);
}

// Variants that also mark sprite_mask for colorization (used in INTRO)
void drawCharMark(int8_t x, int8_t y, char ch) {
  const char* char_map = CHAR_MAP;
  uint8_t index = 0;
  while (char_map[index] != '\0' && char_map[index] != ch) index++;
  if (char_map[index] == '\0') return;
  for (uint8_t row = 0; row < CHAR_HEIGHT; row++) {
    uint8_t line = pgm_read_byte(bmp_font + row * bmp_font_width + index);
    for (uint8_t col = 0; col < CHAR_WIDTH; col++) {
      if (line & (1 << (7 - col))) {
        int16_t px = x + col;
        int16_t py = y + row;
        bufferSetPixel(px, py, true);
        maskSetPixel(px, py, true);
      }
    }
  }
}

void drawTextMark(int8_t x, int8_t y, const char *txt, uint8_t space = 1) {
  for (uint8_t i = 0; txt[i] != '\0'; i++) {
    drawCharMark(x + i * (CHAR_WIDTH + space), y, txt[i]);
  }
}

void renderHud() {
  drawText(2, 58, "HP:", 0);
  drawText(40, 58, "K:", 0);
  updateHud();
}

void updateHud() {
  fillRectBuffer(20, 58, 15, 6, false);
  fillRectBuffer(50, 58, 5, 6, false);
  
  drawText(20, 58, player.health);
  drawText(50, 58, player.keys);
}

void renderStats() {
  fillRectBuffer(58, 58, 70, 6, false);
  drawText(114, 58, int(getActualFps()));
  drawText(82, 58, num_entities);
  // Minimal input debug: show 'F' when fire input is detected
  if (input_fire()) {
    drawText(2, 58, "F", 0);
  }
}

// ============================================================================
// INPUT FUNCTIONS
// ============================================================================

bool isPressed(uint8_t pinIndex) {
  int v = ioexp.read1(pinIndex);
  return (v != TCA9555_INVALID_READ) && (v == LOW);
}

bool isGpioPressed(int gpio) {
  // Match D-pad semantics: external pull-ups, pressed == LOW
  return digitalRead(gpio) == LOW;
}

bool input_up() {
  return isPressed(EXPANDER_PAD_TOP);
}

bool input_down() {
  return isPressed(EXPANDER_PAD_BOTTOM);
}

bool input_left() {
  return isPressed(EXPANDER_PAD_LEFT);
}

bool input_right() {
  return isPressed(EXPANDER_PAD_RIGHT);
}

bool input_fire() {
  return isGpioPressed(BUTTON_TOP) || isPressed(EXPANDER_BUTTON_BOTTOM);
}

bool input_start() {
  return isPressed(EXPANDER_BUTTON_BOTTOM);
}

// ============================================================================
// GAME LOGIC FUNCTIONS
// ============================================================================

void jumpTo(uint8_t target_scene) {
  scene = target_scene;
  exit_scene = true;
}

void initializeLevel(const uint8_t level[]) {
  for (uint8_t y = LEVEL_HEIGHT - 1; y >= 0; y--) {
    for (uint8_t x = 0; x < LEVEL_WIDTH; x++) {
      uint8_t block = getBlockAt(level, x, y);
      
      if (block == E_PLAYER) {
        player = create_player(x, y);
        return;
      }
    }
  }
}

uint8_t getBlockAt(const uint8_t level[], uint8_t x, uint8_t y) {
  if (x < 0 || x >= LEVEL_WIDTH || y < 0 || y >= LEVEL_HEIGHT) {
    return E_FLOOR;
  }
  
  return pgm_read_byte(level + (((LEVEL_HEIGHT - 1 - y) * LEVEL_WIDTH + x) / 2))
         >> (!(x % 2) * 4)
         & 0b1111;
}

bool isSpawned(UID uid) {
  for (uint8_t i = 0; i < num_entities; i++) {
    if (entity[i].uid == uid) return true;
  }
  return false;
}

bool isStatic(UID uid) {
  for (uint8_t i = 0; i < num_static_entities; i++) {
    if (static_entity[i].uid == uid) return true;
  }
  return false;
}

void spawnEntity(uint8_t type, uint8_t x, uint8_t y) {
  if (num_entities >= MAX_ENTITIES) {
    return;
  }
  
  switch (type) {
    case E_ENEMY:
      entity[num_entities] = create_enemy(x, y);
      num_entities++;
      break;
      
    case E_KEY:
      entity[num_entities] = create_key(x, y);
      num_entities++;
      break;
      
    case E_MEDIKIT:
      entity[num_entities] = create_medikit(x, y);
      num_entities++;
      break;
  }
}

void spawnFireball(double x, double y) {
  if (num_entities >= MAX_ENTITIES) {
    return;
  }
  
  UID uid = create_uid(E_FIREBALL, x, y);
  if (isSpawned(uid)) return;
  
  int16_t dir = FIREBALL_ANGLES + atan2(y - player.pos.y, x - player.pos.x) / PI * FIREBALL_ANGLES;
  if (dir < 0) dir += FIREBALL_ANGLES * 2;
  entity[num_entities] = create_fireball(x, y, dir);
  num_entities++;
}

void removeEntity(UID uid, bool makeStatic = false) {
  uint8_t i = 0;
  bool found = false;
  
  while (i < num_entities) {
    if (!found && entity[i].uid == uid) {
      found = true;
      num_entities--;
    }
    
    if (found) {
      entity[i] = entity[i + 1];
    }
    
    i++;
  }
}

void removeStaticEntity(UID uid) {
  uint8_t i = 0;
  bool found = false;
  
  while (i < num_static_entities) {
    if (!found && static_entity[i].uid == uid) {
      found = true;
      num_static_entities--;
    }
    
    if (found) {
      static_entity[i] = static_entity[i + 1];
    }
    
    i++;
  }
}

UID detectCollision(const uint8_t level[], Coords *pos, double relative_x, double relative_y, bool only_walls = false) {
  uint8_t round_x = int(pos->x + relative_x);
  uint8_t round_y = int(pos->y + relative_y);
  uint8_t block = getBlockAt(level, round_x, round_y);
  
  if (block == E_WALL) {
    return create_uid(block, round_x, round_y);
  }
  
  if (only_walls) {
    return UID_null;
  }
  
  for (uint8_t i = 0; i < num_entities; i++) {
    if (&(entity[i].pos) == pos) {
      continue;
    }
    
    uint8_t type = uid_get_type(entity[i].uid);
    
    if (type != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
      continue;
    }
    
    Coords new_coords = { entity[i].pos.x - relative_x, entity[i].pos.y - relative_y };
    uint8_t distance = coords_distance(pos, &new_coords);
    
    if (distance < ENEMY_COLLIDER_DIST && distance < entity[i].distance) {
      return entity[i].uid;
    }
  }
  
  return UID_null;
}

void fire() {
  for (uint8_t i = 0; i < num_entities; i++) {
    if (uid_get_type(entity[i].uid) != E_ENEMY || entity[i].state == S_DEAD || entity[i].state == S_HIDDEN) {
      continue;
    }
    
    Coords transform = translateIntoView(&(entity[i].pos));
    if (abs(transform.x) < 20 && transform.y > 0) {
      // Convert stored scaled distance back to tile units to keep damage in a sensible range
      double distTiles = (double)entity[i].distance / (double)DISTANCE_MULTIPLIER;
      double aimSpread = max(1.0, abs(transform.x));
      double denom = max(0.75, distTiles) * aimSpread / 5.0; // original formula had /5 factor
      double raw = (double)GUN_MAX_DAMAGE / max(1.0, denom);
      uint8_t damage = (uint8_t)min<double>(GUN_MAX_DAMAGE, max(1.0, floor(raw + 0.5)));
      if (damage > 0) {
        entity[i].health = max(0, entity[i].health - damage);
        entity[i].state = S_HIT;
        entity[i].timer = 4;
      }
    }
  }
  // Trigger muzzle flash for a few frames
  muzzle_timer = 3;
  
  // Play gunshot sound effect (match sample style first for reliability)
  if (audio_ready) {
    audioManager.playBeep(2000, 200, 5000); // 2kHz, 200ms, medium volume (as in sample)
  }
}

UID updatePosition(const uint8_t level[], Coords *pos, double relative_x, double relative_y, bool only_walls = false) {
  UID collide_x = detectCollision(level, pos, relative_x, 0, only_walls);
  UID collide_y = detectCollision(level, pos, 0, relative_y, only_walls);
  
  if (!collide_x) pos->x += relative_x;
  if (!collide_y) pos->y += relative_y;
  
  return collide_x || collide_y || UID_null;
}

void updateEntities(const uint8_t level[]) {
  uint8_t i = 0;
  while (i < num_entities) {
    entity[i].distance = coords_distance(&(player.pos), &(entity[i].pos));
    
    if (entity[i].timer > 0) entity[i].timer--;
    
    if (entity[i].distance > MAX_ENTITY_DISTANCE) {
      removeEntity(entity[i].uid);
      continue;
    }
    
    if (entity[i].state == S_HIDDEN) {
      i++;
      continue;
    }
    
    uint8_t type = uid_get_type(entity[i].uid);
    
    switch (type) {
  case E_ENEMY: {
        if (entity[i].health == 0) {
          if (entity[i].state != S_DEAD) {
            entity[i].state = S_DEAD;
            entity[i].timer = 6;
          }
        } else if (entity[i].state == S_HIT) {
          if (entity[i].timer == 0) {
            entity[i].state = S_ALERT;
            entity[i].timer = 40;
          }
        } else if (entity[i].state == S_FIRING) {
          if (entity[i].timer == 0) {
            spawnFireball(entity[i].pos.x, entity[i].pos.y);
            entity[i].state = S_ALERT;
            entity[i].timer = 40;
          }
        } else {
          if (entity[i].distance > ENEMY_MELEE_DIST && entity[i].distance < MAX_ENEMY_VIEW) {
            if (entity[i].timer == 0) {
              entity[i].state = S_FIRING;
              entity[i].timer = 10;
            }
            
            Coords futurePos = {
              entity[i].pos.x + sign(player.pos.x, entity[i].pos.x) * ENEMY_SPEED * delta,
              entity[i].pos.y + sign(player.pos.y, entity[i].pos.y) * ENEMY_SPEED * delta
            };
            
            if (!detectCollision(level, &futurePos, 0, 0)) {
              entity[i].pos = futurePos;
            }
          } else if (entity[i].distance <= ENEMY_MELEE_DIST) {
            if (entity[i].timer == 0) {
              player.health = max(0, player.health - ENEMY_MELEE_DAMAGE);
              flash_screen = 1;
              updateHud();
              entity[i].timer = 30;
            }
          }
        }
        break;
      }
      
      case E_FIREBALL: {
        if (entity[i].distance < FIREBALL_COLLIDER_DIST) {
          player.health = max(0, player.health - ENEMY_FIREBALL_DAMAGE);
          flash_screen = 1;
          updateHud();
          removeEntity(entity[i].uid);
          continue;
        } else {
          UID collided = updatePosition(
            level,
            &(entity[i].pos),
            cos((double) entity[i].health / FIREBALL_ANGLES * PI) * FIREBALL_SPEED,
            sin((double) entity[i].health / FIREBALL_ANGLES * PI) * FIREBALL_SPEED,
            true
          );
          
          if (collided) {
            removeEntity(entity[i].uid);
            continue;
          }
        }
        break;
      }
      
      case E_MEDIKIT: {
        if (entity[i].distance < ITEM_COLLIDER_DIST) {
          entity[i].state = S_HIDDEN;
          player.health = min(100, player.health + 50);
          updateHud();
          flash_screen = 1;
        }
        break;
      }
      
      case E_KEY: {
        if (entity[i].distance < ITEM_COLLIDER_DIST) {
          entity[i].state = S_HIDDEN;
          player.keys++;
          updateHud();
          flash_screen = 1;
        }
        break;
      }
    }
    
    i++;
  }
}

Coords translateIntoView(Coords *pos) {
  double sprite_x = pos->x - player.pos.x;
  double sprite_y = pos->y - player.pos.y;
  
  double inv_det = 1.0 / (player.plane.x * player.dir.y - player.dir.x * player.plane.y);
  double transform_x = inv_det * (player.dir.y * sprite_x - player.dir.x * sprite_y);
  double transform_y = inv_det * (- player.plane.y * sprite_x + player.plane.x * sprite_y);
  
  return { transform_x, transform_y };
}

void sortEntities() {
  uint8_t gap = num_entities;
  bool swapped = false;
  while (gap > 1 || swapped) {
    gap = (gap * 10) / 13;
    if (gap == 9 || gap == 10) gap = 11;
    if (gap < 1) gap = 1;
    swapped = false;
    for (uint8_t i = 0; i < num_entities - gap; i++) {
      uint8_t j = i + gap;
      if (entity[i].distance < entity[j].distance) {
        swap(entity[i], entity[j]);
        swapped = true;
      }
    }
  }
}

void renderMap(const uint8_t level[], double view_height) {
  UID last_uid;
  
  for (uint8_t x = 0; x < SCREEN_WIDTH; x += RES_DIVIDER) {
    double camera_x = 2 * (double) x / SCREEN_WIDTH - 1;
    double ray_x = player.dir.x + player.plane.x * camera_x;
    double ray_y = player.dir.y + player.plane.y * camera_x;
    uint8_t map_x = uint8_t(player.pos.x);
    uint8_t map_y = uint8_t(player.pos.y);
    Coords map_coords = { player.pos.x, player.pos.y };
    double delta_x = abs(1 / ray_x);
    double delta_y = abs(1 / ray_y);
    
    int8_t step_x;
    int8_t step_y;
    double side_x;
    double side_y;
    
    if (ray_x < 0) {
      step_x = -1;
      side_x = (player.pos.x - map_x) * delta_x;
    } else {
      step_x = 1;
      side_x = (map_x + 1.0 - player.pos.x) * delta_x;
    }
    
    if (ray_y < 0) {
      step_y = -1;
      side_y = (player.pos.y - map_y) * delta_y;
    } else {
      step_y = 1;
      side_y = (map_y + 1.0 - player.pos.y) * delta_y;
    }
    
    uint8_t depth = 0;
    bool hit = 0;
    bool side;
    while (!hit && depth < MAX_RENDER_DEPTH) {
      if (side_x < side_y) {
        side_x += delta_x;
        map_x += step_x;
        side = 0;
      } else {
        side_y += delta_y;
        map_y += step_y;
        side = 1;
      }
      
      uint8_t block = getBlockAt(level, map_x, map_y);
      
      if (block == E_WALL) {
        hit = 1;
      } else {
        if (block == E_ENEMY || (block & 0b00001000)) {
          if (coords_distance(&(player.pos), &map_coords) < MAX_ENTITY_DISTANCE) {
            UID uid = create_uid(block, map_x, map_y);
            if (!isSpawned(uid) && !isStatic(uid)) {
              spawnEntity(block, map_x, map_y);
            }
          }
        }
      }
      
      depth++;
    }
    
    if (hit) {
      double distance;
      
      if (side == 0) {
        distance = max(1, (map_x - player.pos.x + (1 - step_x) / 2) / ray_x);
      } else {
        distance = max(1, (map_y - player.pos.y + (1 - step_y) / 2) / ray_y);
      }
      
      zbuffer[x / Z_RES_DIVIDER] = min(distance * DISTANCE_MULTIPLIER, 255);
      
      uint8_t line_height = RENDER_HEIGHT / distance;
      
      drawVLine(
        x,
        view_height / distance - line_height / 2 + RENDER_HEIGHT / 2,
        view_height / distance + line_height / 2 + RENDER_HEIGHT / 2,
        GRADIENT_COUNT - int(distance / MAX_RENDER_DEPTH * GRADIENT_COUNT) - side * 2
      );
    }
  }
}

void renderEntities(double view_height) {
  sortEntities();
  
  for (uint8_t i = 0; i < num_entities; i++) {
    if (entity[i].state == S_HIDDEN) continue;
    
    Coords transform = translateIntoView(&(entity[i].pos));
    
    if (transform.y <= 0.1 || transform.y > MAX_SPRITE_DEPTH) {
      continue;
    }
    
    int16_t sprite_screen_x = HALF_WIDTH * (1.0 + transform.x / transform.y);
    int8_t sprite_screen_y = RENDER_HEIGHT / 2 + view_height / transform.y;
    uint8_t type = uid_get_type(entity[i].uid);
    
    if (sprite_screen_x < - HALF_WIDTH || sprite_screen_x > SCREEN_WIDTH + HALF_WIDTH) {
      continue;
    }
    
    switch (type) {
      case E_ENEMY: {
        uint8_t sprite;
        if (entity[i].state == S_ALERT) {
          sprite = int(millis() / 500) % 2;
        } else if (entity[i].state == S_FIRING) {
          sprite = 2;
        } else if (entity[i].state == S_HIT) {
          sprite = 3;
        } else {
          sprite = 4;
        }
        
        drawSprite(
          sprite_screen_x - BMP_IMP_WIDTH * .5 / transform.y,
          sprite_screen_y - 8 / transform.y,
          bmp_imp_bits,
          bmp_imp_mask,
          BMP_IMP_WIDTH,
          BMP_IMP_HEIGHT,
          sprite,
          transform.y,
          true
        );
        break;
      }
      
      case E_FIREBALL: {
        drawSprite(
          sprite_screen_x - BMP_FIREBALL_WIDTH / 2 / transform.y,
          sprite_screen_y - BMP_FIREBALL_HEIGHT / 2 / transform.y,
          bmp_fireball_bits,
          bmp_fireball_mask,
          BMP_FIREBALL_WIDTH,
          BMP_FIREBALL_HEIGHT,
          0,
          transform.y,
          false
        );
        break;
      }
      
      case E_MEDIKIT: {
        drawSprite(
          sprite_screen_x - BMP_ITEMS_WIDTH / 2 / transform.y,
          sprite_screen_y + 5 / transform.y,
          bmp_items_bits,
          bmp_items_mask,
          BMP_ITEMS_WIDTH,
          BMP_ITEMS_HEIGHT,
          0,
          transform.y,
          false
        );
        break;
      }
      
      case E_KEY: {
        drawSprite(
          sprite_screen_x - BMP_ITEMS_WIDTH / 2 / transform.y,
          sprite_screen_y + 5 / transform.y,
          bmp_items_bits,
          bmp_items_mask,
          BMP_ITEMS_WIDTH,
          BMP_ITEMS_HEIGHT,
          1,
          transform.y,
          false
        );
        break;
      }
    }
  }
}

// ============================================================================
// GAME SCENES
// ============================================================================

void loopIntro() {
  bufferClear();
  memset(sprite_mask, 0, sizeof(sprite_mask));
  memset(enemy_mask, 0, sizeof(enemy_mask));
  
  // Create LVGL intro label once (PRESS FIRE, Inter 30) and place it under the logo
  if (!g_intro_label) {
    // Ensure LVGL screen/layers have a black background and transparent top layer
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_t* top_layer = lv_layer_top();
    lv_obj_set_style_bg_opa(top_layer, LV_OPA_TRANSP, 0);

    // Compute panel Y position that visually sits just below the mono logo
    const int16_t dstW = LCD_WIDTH;
    const int16_t dstH_raw = (LCD_WIDTH * SCREEN_HEIGHT) / SCREEN_WIDTH; // e.g., 205
    const int16_t dstH = (dstH_raw & ~1);                                 // even -> 204
    const int16_t oy = (((LCD_HEIGHT - dstH) / 2) & ~1);                  // letterbox top offset
    const int16_t logo_y = (SCREEN_HEIGHT - BMP_LOGO_HEIGHT) / 3;         // mono-space logo Y
    const int16_t text_y_src = logo_y + BMP_LOGO_HEIGHT + 2;              // just below logo
    const int16_t text_y_panel = oy + (int16_t)((int32_t)text_y_src * dstH_raw / SCREEN_HEIGHT);

    // Create the label on the top layer
    g_intro_label = lv_label_create(top_layer);
    lv_label_set_text(g_intro_label, "PRESS FIRE");
    // Style: Inter 30 font, orange text, transparent bg
    lv_obj_set_style_text_font(g_intro_label, &Inter_30, 0);
    lv_obj_set_style_text_color(g_intro_label, lv_color_hex(0xFF7A00), 0); // vivid orange
    lv_obj_set_style_bg_opa(g_intro_label, LV_OPA_TRANSP, 0);
    lv_obj_align(g_intro_label, LV_ALIGN_TOP_MID, 0, text_y_panel);
    lv_obj_move_foreground(g_intro_label);
  }
  
  // Draw logo
  drawBitmapMark1BPP(
    (SCREEN_WIDTH - BMP_LOGO_WIDTH) / 2,
    (SCREEN_HEIGHT - BMP_LOGO_HEIGHT) / 3,
    bmp_logo_bits,
    BMP_LOGO_WIDTH,
    BMP_LOGO_HEIGHT
  );
  flushToPanel(invert_screen);
  // Make sure the LVGL label is redrawn on top of our direct panel flush
  if (g_intro_label) lv_obj_invalidate(g_intro_label);
  // Service LVGL timers (won't draw anything unless LVGL UI is used)
  displayMgr.update();

  // LED breathing in orange while on intro (loading)
  {
    float t = (millis() % 1800) / 1800.0f;          // ~1.8s period
    float breath = 0.5f + 0.5f * sinf(t * 6.2831853f);
    uint8_t br = 5 + (uint8_t)(breath * 15);        // 5..20 (quarter of previous)
    g_led.setBrightness(br);
    g_led.setColor(255, 100, 0);                    // vivid orange
    g_led.show();
  }
  
  delay(100);
  
  if (input_fire()) {
    delay(200); // debounce
    // Clean up intro LVGL label before leaving the scene
    if (g_intro_label) { lv_obj_delete(g_intro_label); g_intro_label = nullptr; }
    jumpTo(GAME_PLAY);
  }
}

void loopGamePlay() {
  bool gun_fired = false;
  uint8_t gun_pos = 0;
  double rot_speed;
  double old_dir_x;
  double old_plane_x;
  double view_height = 0;
  double jogging = 0;
  uint8_t fade = GRADIENT_COUNT - 1;
  
  // Initialize level
  initializeLevel(sto_level_1);

  // Clear the entire screen first to remove any residue from previous scenes
  if (gfx) {
    gfx->fillScreen(COLOR_BLACK);
  }

  // Paint the top and bottom letterbox bars in black once on entry
  {
    const int16_t dstW = LCD_WIDTH;
    const int16_t dstH_raw = (LCD_WIDTH * SCREEN_HEIGHT) / SCREEN_WIDTH;
    const int16_t dstH = (dstH_raw & ~1);
    const int16_t oy = (((LCD_HEIGHT - dstH) / 2) & ~1);
    
    if (oy > 0) {
      gfx->fillRect(0, 0, LCD_WIDTH, oy, COLOR_BLACK);
    }
    int16_t bottomH = LCD_HEIGHT - (oy + dstH);
    if (bottomH > 0) {
      gfx->fillRect(0, oy + dstH, LCD_WIDTH, bottomH, COLOR_BLACK);
    }
  }

  // Create LVGL logo overlay at the top (only during gameplay)
  if (!g_game_logo) {
    // Set the LVGL screen background to black to avoid white residue
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    // Also set the top layer background to black
    lv_obj_t* top_layer = lv_layer_top();
    lv_obj_set_style_bg_color(top_layer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(top_layer, LV_OPA_TRANSP, 0); // transparent so it doesn't block the screen below
    
    // Put the logo on the top-most LVGL layer so it always draws above the game
    g_game_logo = lv_image_create(top_layer);
    lv_image_set_src(g_game_logo, &Logo);
    lv_obj_align(g_game_logo, LV_ALIGN_TOP_MID, 0, 30); // Bajado 30 píxeles
    lv_obj_move_foreground(g_game_logo);
    
    // Make the logo background transparent (no white box behind the image)
    lv_obj_set_style_bg_opa(g_game_logo, LV_OPA_TRANSP, 0);
    // Ensure it's drawn at least once
    lv_obj_invalidate(g_game_logo);
  }
  
  // Create Character image at the bottom with red border
  if (!g_game_character) {
    lv_obj_t* top_layer = lv_layer_top();
    
    // Create the character image (61x70 pixels)
    g_game_character = lv_image_create(top_layer);
    lv_image_set_src(g_game_character, &Character);
    lv_obj_align(g_game_character, LV_ALIGN_BOTTOM_MID, 0, -30); // Subido 30 píxeles
    lv_obj_move_foreground(g_game_character);
    
    // Add red border around the character
    lv_obj_set_style_border_color(g_game_character, lv_color_hex(0xFF0000), 0); // red
    lv_obj_set_style_border_width(g_game_character, 2, 0);
    lv_obj_set_style_border_opa(g_game_character, LV_OPA_COVER, 0);
    
    // Make the background transparent
    lv_obj_set_style_bg_opa(g_game_character, LV_OPA_TRANSP, 0);
    // Ensure it's drawn at least once
    lv_obj_invalidate(g_game_character);
  }
  
  do {
    fps();
    
    // Clear 3D view area
  memset(display_buf, 0, SCREEN_WIDTH * (RENDER_HEIGHT / 8));
  memset(sprite_mask, 0, sizeof(sprite_mask));
  memset(enemy_mask, 0, sizeof(enemy_mask));
  memset(enemy_center_mask, 0, sizeof(enemy_center_mask));
  memset(fire_mask, 0, sizeof(fire_mask));
  memset(fire_center_mask, 0, sizeof(fire_center_mask));
  memset(gun_hand_mask, 0, sizeof(gun_hand_mask));
  memset(gun_hand_center_mask, 0, sizeof(gun_hand_center_mask));
  memset(gun_metal_mask, 0, sizeof(gun_metal_mask));
  memset(gun_metal_center_mask, 0, sizeof(gun_metal_center_mask));
    
    // Process input if player is alive
    if (player.health > 0) {
      // Movement
      if (input_up()) {
        player.velocity += (MOV_SPEED - player.velocity) * .4;
        jogging = abs(player.velocity) * MOV_SPEED_INV;
      } else if (input_down()) {
        player.velocity += (- MOV_SPEED - player.velocity) * .4;
        jogging = abs(player.velocity) * MOV_SPEED_INV;
      } else {
        player.velocity *= .5;
        jogging = abs(player.velocity) * MOV_SPEED_INV;
      }
      
      // Rotation
      if (input_right()) {
        rot_speed = ROT_SPEED * delta;
        old_dir_x = player.dir.x;
        player.dir.x = player.dir.x * cos(-rot_speed) - player.dir.y * sin(-rot_speed);
        player.dir.y = old_dir_x * sin(-rot_speed) + player.dir.y * cos(-rot_speed);
        old_plane_x = player.plane.x;
        player.plane.x = player.plane.x * cos(-rot_speed) - player.plane.y * sin(-rot_speed);
        player.plane.y = old_plane_x * sin(-rot_speed) + player.plane.y * cos(-rot_speed);
      } else if (input_left()) {
        rot_speed = ROT_SPEED * delta;
        old_dir_x = player.dir.x;
        player.dir.x = player.dir.x * cos(rot_speed) - player.dir.y * sin(rot_speed);
        player.dir.y = old_dir_x * sin(rot_speed) + player.dir.y * cos(rot_speed);
        old_plane_x = player.plane.x;
        player.plane.x = player.plane.x * cos(rot_speed) - player.plane.y * sin(rot_speed);
        player.plane.y = old_plane_x * sin(rot_speed) + player.plane.y * cos(rot_speed);
      }
      
      view_height = abs(sin((double) millis() * JOGGING_SPEED)) * 6 * jogging;
      
      // Gun control: handle trigger first, then relax back to target
      if (!gun_fired && input_fire()) {
        gun_pos = GUN_SHOT_POS;   // kick the weapon
        gun_fired = true;
        fire();
  // LED flash red on fire
  g_led.setBrightness(25); // quarter brightness
        g_led.setColor(255, 0, 0);
        g_led_flash_timer = 6; // ~6 frames
      } else if (gun_fired && !input_fire()) {
        gun_fired = false;
      }

      if (gun_pos > GUN_TARGET_POS) {
        gun_pos -= 1;             // ease back towards target
      } else if (gun_pos < GUN_TARGET_POS) {
        gun_pos += 2;
      }
    } else {
      // Player is dead
      if (view_height > -10) view_height--;
      else if (input_fire()) jumpTo(INTRO);
      
      if (gun_pos > 1) gun_pos -= 2;
    }
    
    // Update player position
    if (abs(player.velocity) > 0.003) {
      updatePosition(
        sto_level_1,
        &(player.pos),
        player.dir.x * player.velocity * delta,
        player.dir.y * player.velocity * delta
      );
    } else {
      player.velocity = 0;
    }
    
    // Update entities
    updateEntities(sto_level_1);
    
    // Render
    renderMap(sto_level_1, view_height);
    renderEntities(view_height);

    // Thicken enemies slightly from their original center pixels (one-ring border)
    {
      static uint8_t temp[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];
      memcpy(temp, enemy_center_mask, sizeof(temp));
      for (int16_t y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int16_t x = 0; x < SCREEN_WIDTH; ++x) {
          if (((temp[(y >> 3) * SCREEN_WIDTH + x] >> (y & 7)) & 0x1) != 0) {
            const int dxs[4] = {1,-1,0,0};
            const int dys[4] = {0,0,1,-1};
            for (int k = 0; k < 4; ++k) {
              int16_t nx = x + dxs[k];
              int16_t ny = y + dys[k];
              if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT) {
                enemyMaskSetPixel(nx, ny, true); // border goes into outer mask
                bufferSetPixel(nx, ny, true);    // and is visible in mono buffer
              }
            }
          }
        }
      }
    }

    // Moved weapon draw below, after HUD render, so HUD clears don't cut the weapon
    
    // Fade in effect
    if (fade > 0) {
      fadeScreen(fade);
      fade--;
      
      if (fade == 0) {
        renderHud();
      }
    } else {
      renderStats();
    }
    
    // Flash screen
    if (flash_screen > 0) {
      invert_screen = !invert_screen;
      flash_screen--;
    } else if (invert_screen) {
      invert_screen = 0;
    }
    
    // Draw weapon (HUD layer) after HUD/text so it's not cleared underneath
  {
    int16_t gunX = (SCREEN_WIDTH - BMP_GUN_WIDTH) / 2;
    // Move weapon upward on shot: subtract offset relative to target
    int16_t gunY = (SCREEN_HEIGHT - BMP_GUN_HEIGHT) - (int16_t)(gun_pos - GUN_TARGET_POS);
    if (gunY < 0) gunY = 0;
    if (gunY > (int16_t)(SCREEN_HEIGHT - BMP_GUN_HEIGHT)) gunY = (SCREEN_HEIGHT - BMP_GUN_HEIGHT);
      drawGunBitmap1BPP(gunX, gunY, bmp_gun_bits, bmp_gun_mask, BMP_GUN_WIDTH, BMP_GUN_HEIGHT);

      // Muzzle flash just above the gun for a few frames
      if (muzzle_timer > 0) {
        int16_t fx = gunX + 6;              // small x offset towards muzzle
        int16_t fy = gunY - 11;             // above the gun
        drawBitmapFire1BPP(fx, fy, bmp_fire_bits, BMP_FIRE_WIDTH, BMP_FIRE_HEIGHT);
        muzzle_timer--;
      }
  }

    // LED per-frame maintenance for gameplay
    if (g_led_flash_timer > 0) {
      g_led_flash_timer--;
      if (g_led_flash_timer == 0) {
        g_led.turnOff();
      }
      g_led.show();
    }

  // Display frame: draw only the active viewport (skip bars) and then let LVGL draw overlays on top
  flushToPanel(invert_screen, false /* clearBars */);
    
    // Service LVGL less often (no need to redraw overlays every frame since bars are not touched)
    uint32_t now = millis();
    if (now - g_lvgl_last_update_ms >= 33) { // ~30 Hz
      displayMgr.update();
      g_lvgl_last_update_ms = now;
    }
    
    // Exit to menu
    if (input_left() && input_right()) {
      jumpTo(INTRO);
    }
  } while (!exit_scene);

  // Remove the LVGL overlay when leaving gameplay
  if (g_game_logo) {
    lv_obj_delete(g_game_logo);
    g_game_logo = nullptr;
  }
  
  if (g_game_character) {
    lv_obj_delete(g_game_character);
    g_game_character = nullptr;
  }
}

// ============================================================================
// ARDUINO SETUP AND LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("DOOM for Kode Dot starting...");
  
  // Initialize I2C
  Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);
  
  // Initialize Kode Dot panel
  if (!displayMgr.init()) {
    Serial.println("DisplayManager init failed");
  }
  gfx = displayMgr.getGfx();
  
  // Initialize LED strip
  {
    LEDConfig cfg;
    cfg.pin = NEO_PIXEL_PIN;
    cfg.count = NEO_PIXEL_COUNT;
  cfg.brightness = 10; // lower base brightness
#ifndef LED_USE_NEOPIXELBUS
    cfg.colorOrder = LED_STRIP_COLOR_ORDER + LED_STRIP_TIMING;
#endif
    if (!g_led.init(cfg)) {
      Serial.println("LEDManager init failed");
    } else {
      g_led.turnOff();
      g_led.show();
    }
  }

  // Initialize IO Expander FIRST (needed for audio amplifier control)
  if (!ioexp.begin(INPUT)) {
    Serial.println("Warning: IO Expander not connected");
  }

  // Initialize Audio Manager (speaker) - AFTER IO Expander (exactly like sample)
  {
    AudioConfig audioCfg;
    audioCfg.spkSckPin = SPK_I2S_SCK;
    audioCfg.spkWsPin  = SPK_I2S_WS;
    audioCfg.spkDoutPin = SPK_I2S_DOUT;
    bool spkPinsValid = (audioCfg.spkSckPin != -1 && audioCfg.spkWsPin != -1 && audioCfg.spkDoutPin != -1);
    if (spkPinsValid && audioManager.init(audioCfg)) {
      audio_ready = true;
      // Ensure expander controls the speaker amplifier/shutdown pin (exact match to sample)
      audioManager.attachExpander(&ioexp, EXPANDER_SPK_SHUTDOWN);
      Serial.println("[Audio] ready for beeps");
    } else {
      Serial.println("[Audio] speaker pins unavailable or init failed; audio disabled");
    }
  }
  
  // Configure TOP button with internal pull-up; pressed == LOW like the D-pad
  pinMode(BUTTON_TOP, INPUT_PULLUP);
  
  // Initialize zbuffer
  memset(zbuffer, 0xFF, ZBUFFER_SIZE);
  
  bufferClear();
  flushToPanel(false);
  
  Serial.println("System ready!");
}

void loop() {
  switch (scene) {
    case INTRO:
      loopIntro();
      break;
      
    case GAME_PLAY:
      loopGamePlay();
      break;
  }
  
  // Fade out effect between scenes
  for (uint8_t i = 0; i < GRADIENT_COUNT; i++) {
    fadeScreen(i, 0);
    flushToPanel(invert_screen);
    displayMgr.update();
    delay(40);
  }
  
  exit_scene = false;
}