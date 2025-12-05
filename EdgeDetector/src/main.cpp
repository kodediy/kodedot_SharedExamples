/*
 * (Rewritten clean version in separate patch attempt - placeholder header)
 */
// Core & libs
#include <Arduino.h>
#include <lvgl.h>
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
#include <esp_camera.h>
#include <esp_heap_caps.h>
#include <esp_pm.h> // Mejora 14: para CPU freq lock
// #include <TCA9555.h>  // Commented out - causing I2C errors if device not present
#include <Adafruit_NeoPixel.h>
// SD + JPEG support
#include <FS.h>
#include <SD_MMC.h>
#include "img_converters.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern const lv_font_t Inter_30;

static DisplayManager display; 
// static TCA9555 ioexp(IOEXP_I2C_ADDR);  // Commented out - causing I2C errors if device not present
static Adafruit_NeoPixel pixel(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, LED_STRIP_COLOR_ORDER + LED_STRIP_TIMING);
static const uint32_t FPS_INTERVAL_MS=1000; 
static const int SQUARE_SIDE=LCD_HEIGHT, BUF_COUNT=6; // séxtuple buffering extremo
static const uint8_t PHOTO_LED_BRIGHTNESS = (uint8_t)(255 * 0.30f); // ~30%
static const uint16_t PHOTO_HOLD_MS = 300; // tiempo de pulsación larga requerido para foto (ms)
// Parámetros para snapshot de alta resolución
static const int PHOTO_JPEG_QUALITY = 10; // menor => mejor calidad (original 15)
static framesize_t stream_framesize = FRAMESIZE_QVGA; // se actualiza tras init
// Mejora 1: eliminar escalado dinámico & swap por defecto.
static bool camera_initialized=false, swap_bytes_rgb565=false, ioexp_available=false; // Pixel Art: swap false (B&W no necesita swap)
static int cam_w=0, cam_h=0; static lv_color_t* square_bufs[BUF_COUNT]={nullptr};
static bool sd_ok=false; static volatile bool snapshot_request=false; static volatile bool snapshot_in_progress=false; static uint32_t photo_counter=0; static const char * PHOTO_DIR="/GENERAL/Camera/Photos";
static volatile bool snapshot_ready=false; static uint8_t * snapshot_raw=nullptr; static size_t snapshot_raw_size=0; static uint16_t snapshot_w=0, snapshot_h=0;
static volatile int ready_index=-1, consume_index=-1, produce_index=0; 

// Pixel Art Settings
static const int PIXEL_BLOCK_SIZE = 6; // Tamaño del "pixel" (6x6 real pixels) - Aumentado para menos detalle/ruido
static const int EDGE_THRESHOLD = 40;   // Umbral de detección de bordes (0-255) - Subido drásticamente para filtrar ruido 
// Mapas retenidos por si se reactiva escalado (no usados en ruta rápida de recorte)
static uint16_t * square_map_x=nullptr, * square_map_y=nullptr; static int map_cam_w=-1, map_cam_h=-1; 
static uint32_t frame_counter=0, last_fps_tick=0;
// Mejora DEBUG: Instrumentación de rendimiento para identificar cuellos de botella
static uint32_t camera_frame_count=0, display_frame_count=0, lvgl_frame_count=0;
static uint32_t camera_total_time_us=0, display_total_time_us=0, lvgl_total_time_us=0;
static uint32_t camera_max_time_us=0, display_max_time_us=0, lvgl_max_time_us=0;
static uint32_t last_perf_report=0;
static const uint32_t PERF_REPORT_INTERVAL_MS = 5000; // cada 5 segundos
static lv_obj_t * camera_img=nullptr; static lv_style_t style_bg;
// Flash overlay
	// Frame (marco) animation overlay
	static lv_obj_t * frame_layer=nullptr; static volatile bool frame_anim_request=false; static bool frame_anim_active=false; static uint32_t frame_anim_start_ms=0;
	static const uint32_t FRAME_ANIM_TOTAL_MS=220; // duración animación
	static const uint16_t FRAME_BORDER_START= (LCD_WIDTH < LCD_HEIGHT ? LCD_WIDTH : LCD_HEIGHT)/15; // grosor inicial reducido a 1/3 (~6.7% del menor lado)

// Forward declarations
static void ensure_square_maps(int w,int h); static void adopt_ready_frame(); static void update_fps();
// New forward declarations for SD / snapshot support
static void ensure_photo_dir();
static void init_sd();
static bool save_jpeg_from_rgb565(const uint8_t * rgb565, uint16_t w, uint16_t h);
static void request_snapshot();
// Fast processing task for parallel swap/conversion
static void processing_task(void * arg);
static TaskHandle_t processing_task_handle = nullptr;
static volatile bool processing_request = false;
static volatile int processing_buf_index = -1;
// Mejora 10: Direct panel access (bypass LVGL widget)
static DisplayManager * display_manager_ptr = nullptr;
static volatile bool direct_flush_request = false;
static volatile int direct_flush_buf_index = -1;

static void create_ui(){
	lv_obj_t * scr=lv_scr_act();
	// Quitar scroll y barras
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
	lv_style_init(&style_bg);
	lv_style_set_bg_color(&style_bg, lv_color_hex(0x000000));
	lv_obj_add_style(scr,&style_bg,0);
	camera_img=lv_image_create(scr);
	lv_obj_set_size(camera_img,SQUARE_SIDE,SQUARE_SIDE);
	int xo=(LCD_WIDTH-SQUARE_SIDE)/2;
	lv_obj_set_pos(camera_img,xo,0);
	// Crear capa para marco animado
	frame_layer = lv_obj_create(scr);
	lv_obj_set_size(frame_layer, LCD_WIDTH, LCD_HEIGHT);
	lv_obj_set_style_bg_opa(frame_layer, 0, 0); // sin fondo
	lv_obj_set_style_border_color(frame_layer, lv_color_hex(0x000000), 0);
	lv_obj_set_style_border_opa(frame_layer, 0, 0);
	lv_obj_set_style_border_width(frame_layer, 0, 0);
	uint16_t radius = (uint16_t)((LCD_WIDTH < LCD_HEIGHT ? LCD_WIDTH : LCD_HEIGHT) * 25 / 100); // 25% borde más redondeado
	lv_obj_set_style_radius(frame_layer, radius, 0);
	lv_obj_set_style_clip_corner(frame_layer, true, 0); // asegura redondeo interior/ exterior del borde grueso
	lv_obj_add_flag(frame_layer, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_move_foreground(frame_layer);
}

static void init_camera(){
	camera_config_t cfg={};
	cfg.ledc_channel=LEDC_CHANNEL_0; cfg.ledc_timer=LEDC_TIMER_0;
	cfg.pin_d0=CAMERA_PIN_D0; cfg.pin_d1=CAMERA_PIN_D1; cfg.pin_d2=CAMERA_PIN_D2; cfg.pin_d3=CAMERA_PIN_D3;
	cfg.pin_d4=CAMERA_PIN_D4; cfg.pin_d5=CAMERA_PIN_D5; cfg.pin_d6=CAMERA_PIN_D6; cfg.pin_d7=CAMERA_PIN_D7;
	cfg.pin_xclk=CAMERA_PIN_XCLK; cfg.pin_pclk=CAMERA_PIN_PCLK; cfg.pin_vsync=CAMERA_PIN_VSYNC; cfg.pin_href=CAMERA_PIN_HREF;
	cfg.pin_sccb_sda=CAMERA_PIN_SCCB_SDA; cfg.pin_sccb_scl=CAMERA_PIN_SCCB_SCL; cfg.pin_pwdn=CAMERA_PIN_PWDN; cfg.pin_reset=CAMERA_PIN_RESET;
	cfg.xclk_freq_hz=20000000; // Bajar un poco XCLK para mejorar calidad de señal en grayscale
	cfg.pixel_format=PIXFORMAT_GRAYSCALE; // Optimización: Solo luminancia (1 byte por pixel)
#ifdef FRAMESIZE_240X240
	cfg.frame_size=FRAMESIZE_240X240;
#else
	cfg.frame_size=FRAMESIZE_QVGA; // 320x240
#endif
	cfg.grab_mode=CAMERA_GRAB_LATEST;
	cfg.fb_location= psramFound()?CAMERA_FB_IN_PSRAM:CAMERA_FB_IN_DRAM;
	// Usar máximo framebuffers con PSRAM abundante para pipeline extremo
	cfg.jpeg_quality=12; cfg.fb_count= psramFound()?6:1; // Mejora 12: pipeline máximo
	if(esp_camera_init(&cfg)!=ESP_OK){ Serial.println("[CAM] init fail"); return; }
	if(sensor_t * s=esp_camera_sensor_get()){ s->set_vflip(s,1); s->set_hmirror(s,0); }
	// Dimensiones reales
	camera_fb_t * test=esp_camera_fb_get(); if(test){ cam_w=test->width; cam_h=test->height; esp_camera_fb_return(test);} else { cam_w=320; cam_h=240; }
	size_t need=(size_t)SQUARE_SIDE*SQUARE_SIDE*sizeof(lv_color_t);
	for(int i=0;i<BUF_COUNT;i++){
		// Preferir PSRAM para no consumir heap interno
		square_bufs[i]=(lv_color_t*)heap_caps_malloc(need, MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
		if(!square_bufs[i]) square_bufs[i]=(lv_color_t*)heap_caps_malloc(need, MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
		if(!square_bufs[i]) Serial.printf("[CAM] no mem buf %d\n",i);
	}
	stream_framesize = cfg.frame_size; // guardar framesize de streaming
	ready_index = -1; consume_index = -1; produce_index = 0; camera_initialized=true; Serial.printf("[CAM] OK (%dx%d)\n",cam_w,cam_h);
}

static void ensure_square_maps(int w,int h){ if(w==map_cam_w && h==map_cam_h && square_map_x && square_map_y) return; if(!square_map_x) square_map_x=(uint16_t*)heap_caps_malloc(SQUARE_SIDE*sizeof(uint16_t), MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT); if(!square_map_y) square_map_y=(uint16_t*)heap_caps_malloc(SQUARE_SIDE*sizeof(uint16_t), MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT); if(!square_map_x||!square_map_y) return; map_cam_w=w; map_cam_h=h; int64_t scaled_w=(int64_t)w*SQUARE_SIDE/h; int64_t offset=(scaled_w - SQUARE_SIDE)/2; for(int y=0;y<SQUARE_SIDE;y++) square_map_y[y]=(uint16_t)((uint32_t)y*h/SQUARE_SIDE); for(int x=0;x<SQUARE_SIDE;x++){ int64_t sx=(int64_t)(x+offset)*h/SQUARE_SIDE; if(sx<0)sx=0; if(sx>=w)sx=w-1; square_map_x[x]=(uint16_t)sx; }}

static void camera_task(void * arg){
	while(true){
		uint32_t task_start = micros(); // DEBUG: medir tiempo total de task
		
		if(!camera_initialized){ vTaskDelay(pdMS_TO_TICKS(10)); continue; }
		// Mejora 4: snapshot ligero (no cambia framesize)
		if(snapshot_request && !snapshot_in_progress){ snapshot_in_progress=true; snapshot_request=false; }
		
		uint32_t cam_start = micros(); // DEBUG: inicio captura cámara
		camera_fb_t * fb=esp_camera_fb_get();
		uint32_t cam_end = micros(); // DEBUG: fin captura cámara
		
		if(!fb){ taskYIELD(); continue; }
		if(fb->format!=PIXFORMAT_GRAYSCALE){ esp_camera_fb_return(fb); taskYIELD(); continue; }
		
		// DEBUG: medir tiempo de captura
		uint32_t cam_time = cam_end - cam_start;
		camera_total_time_us += cam_time;
		if(cam_time > camera_max_time_us) camera_max_time_us = cam_time;
		camera_frame_count++;
		
		cam_w=fb->width; cam_h=fb->height; // sin escalado
		int idx=produce_index; int loops=0;
		while((idx==ready_index || idx==consume_index) && loops<BUF_COUNT){ produce_index=(produce_index+1)%BUF_COUNT; idx=produce_index; loops++; }
		
		uint32_t copy_start = micros(); // DEBUG: inicio copia/procesamiento
		lv_color_t * dest=square_bufs[idx];
		if(dest){
			const uint8_t * src_gray=(const uint8_t*)fb->buf; // Ahora es 8-bit
			uint16_t * dst16=(uint16_t*)dest;
			
			// Helper para calcular promedio de bloque (Denoising)
			auto get_block_avg = [&](const uint8_t* base, int stride, int start_x, int start_y, int w, int h) -> int {
				int sum = 0;
				int count = 0;
				// Muestreo optimizado: saltar cada 2 pixeles si el bloque es grande para velocidad
				int step = (PIXEL_BLOCK_SIZE > 2) ? 2 : 1; 
				
				for(int y=0; y<PIXEL_BLOCK_SIZE; y+=step){
					int dy = start_y + y;
					if(dy >= h) break;
					const uint8_t* row = base + dy*stride;
					for(int x=0; x<PIXEL_BLOCK_SIZE; x+=step){
						int dx = start_x + x;
						if(dx >= w) break;
						sum += row[dx];
						count++;
					}
				}
				return count ? (sum / count) : 0;
			};

			if(cam_w>=SQUARE_SIDE && cam_h>=SQUARE_SIDE){
				// Ruta rápida: recorte centrado + Filtro Pixel Art (Edge Detection + Denoising)
				int cropX=0, cropY=0;
				if(cam_w>cam_h){ int extra=cam_w-SQUARE_SIDE; if(extra>0) cropX=extra/2; }
				else if(cam_h>cam_w){ int extra=cam_h-SQUARE_SIDE; if(extra>0) cropY=extra/2; }
				
				for(int y=0; y<SQUARE_SIDE; y+=PIXEL_BLOCK_SIZE){
					for(int x=0; x<SQUARE_SIDE; x+=PIXEL_BLOCK_SIZE){
						int sx = cropX + x;
						int sy = cropY + y;
						
						// Calcular promedio del bloque actual (suaviza ruido)
						int l_curr = get_block_avg(src_gray, cam_w, sx, sy, cam_w, cam_h);
						
						// Comparar con vecinos (derecha y abajo)
						int sx_right = sx + PIXEL_BLOCK_SIZE;
						int sy_down = sy + PIXEL_BLOCK_SIZE;
						
						// Si nos salimos, usamos el mismo valor (borde 0)
						int l_right = (sx_right < cam_w) ? get_block_avg(src_gray, cam_w, sx_right, sy, cam_w, cam_h) : l_curr;
						int l_down = (sy_down < cam_h) ? get_block_avg(src_gray, cam_w, sx, sy_down, cam_w, cam_h) : l_curr;

						int diff = abs(l_curr - l_right) + abs(l_curr - l_down);
						
						// Histéresis simple o umbral
						uint16_t color = (diff > EDGE_THRESHOLD) ? 0xFFFF : 0x0000;

						// Rellenar bloque de destino
						for(int by=0; by<PIXEL_BLOCK_SIZE; by++){
							int dy = y + by;
							if(dy >= SQUARE_SIDE) break;
							for(int bx=0; bx<PIXEL_BLOCK_SIZE; bx++){
								int dx = x + bx;
								if(dx >= SQUARE_SIDE) break;
								dst16[dy * SQUARE_SIDE + dx] = color;
							}
						}
					}
				}
			} else {
				// Fallback: escalado + Filtro Pixel Art (Unified)
				ensure_square_maps(cam_w, cam_h);
				if(square_map_x && square_map_y){
					// Helper para obtener pixel escalado (nearest neighbor para simplificar lógica de bloque en escalado)
					// Nota: En fallback no hacemos promedio de bloque complejo por simplicidad, 
					// pero usamos el pixel central del bloque mapeado para reducir ruido vs esquina.
					auto get_scaled_lum = [&](int x, int y) -> int {
						int sy = square_map_y[y + PIXEL_BLOCK_SIZE/2]; if(sy>=cam_h) sy=cam_h-1;
						int sx = square_map_x[x + PIXEL_BLOCK_SIZE/2]; if(sx>=cam_w) sx=cam_w-1;
						return src_gray[sy * cam_w + sx];
					};

					for(int y=0; y<SQUARE_SIDE; y+=PIXEL_BLOCK_SIZE){
						for(int x=0; x<SQUARE_SIDE; x+=PIXEL_BLOCK_SIZE){
							int l_curr = get_scaled_lum(x, y);
							
							int x_right = x + PIXEL_BLOCK_SIZE;
							if(x_right >= SQUARE_SIDE) x_right = SQUARE_SIDE - 1;
							int l_right = get_scaled_lum(x_right, y);

							int y_down = y + PIXEL_BLOCK_SIZE;
							if(y_down >= SQUARE_SIDE) y_down = SQUARE_SIDE - 1;
							int l_down = get_scaled_lum(x, y_down);

							int diff = abs(l_curr - l_right) + abs(l_curr - l_down);
							uint16_t color = (diff > EDGE_THRESHOLD) ? 0xFFFF : 0x0000;

							for(int by=0; by<PIXEL_BLOCK_SIZE; by++){
								int dy = y + by;
								if(dy >= SQUARE_SIDE) break;
								for(int bx=0; bx<PIXEL_BLOCK_SIZE; bx++){
									int dx = x + bx;
									if(dx >= SQUARE_SIDE) break;
									dst16[dy * SQUARE_SIDE + dx] = color;
								}
							}
						}
					}
				}
			}
			// Mejora 10: Direct flush bypass (sin LVGL widget overhead) - TEMPORALMENTE DESACTIVADO
			if(false && display_manager_ptr && !direct_flush_request){
				direct_flush_request = true;
				direct_flush_buf_index = idx;
			}
			
			// Mejora 6: Delegar swap a task paralelo si disponible
			if(swap_bytes_rgb565 && processing_task_handle && !processing_request){
				processing_request = true;
				processing_buf_index = idx;
				xTaskNotifyGive(processing_task_handle);
			} else if(swap_bytes_rgb565){ 
				// Fallback: swap inmediato optimizado
				const size_t total_pixels=(size_t)SQUARE_SIDE*SQUARE_SIDE;
				uint32_t * p32=(uint32_t*)dest; size_t pairs= total_pixels/2; 
				while(pairs--){ *p32=__builtin_bswap32(*p32); ++p32; }
				if(total_pixels & 1){ uint16_t * last=(uint16_t*)p32; uint16_t v=*last; *last=(uint16_t)((v>>8)|(v<<8)); }
			}
			ready_index=idx; produce_index=(idx+1)%BUF_COUNT;
		}
		
		uint32_t copy_end = micros(); // DEBUG: fin copia/procesamiento
		uint32_t copy_time = copy_end - copy_start;
		// Agregar a estadísticas de procesamiento (considerado como display time)
		display_total_time_us += copy_time;
		if(copy_time > display_max_time_us) display_max_time_us = copy_time;
		display_frame_count++;
		
		if(snapshot_in_progress){ 
			// Guardar el buffer procesado (Pixel Art) en lugar del raw
			size_t need = (size_t)SQUARE_SIDE * SQUARE_SIDE * 2; 
			if(!snapshot_raw || need>snapshot_raw_size){ 
				if(snapshot_raw) free(snapshot_raw); 
				snapshot_raw=(uint8_t*)heap_caps_malloc(need, MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT); 
				if(!snapshot_raw) snapshot_raw=(uint8_t*)heap_caps_malloc(need, MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT); 
				snapshot_raw_size=snapshot_raw?need:0; 
			}
			if(snapshot_raw){ 
				memcpy(snapshot_raw, dest, need); 
				snapshot_w=SQUARE_SIDE; 
				snapshot_h=SQUARE_SIDE; 
				snapshot_ready=true; 
				Serial.printf("[PHOTO] Pixel Art Snapshot %ux%u (%u bytes)\n", snapshot_w, snapshot_h, (unsigned)need);
			} else { Serial.println("[PHOTO] no mem snapshot"); }
			snapshot_in_progress=false; 
		}
		esp_camera_fb_return(fb);
		
		uint32_t task_end = micros(); // DEBUG: fin task completo
		uint32_t task_total = task_end - task_start;
		
		// Mejora 12: sin delays para máximo throughput
		// vTaskDelay(1); // eliminado para pipeline extremo
	}
}

static void start_camera_task(){ 
	// Mejora 12: máxima prioridad + core affinity extremo
	xTaskCreatePinnedToCore(camera_task,"cam_task",8192,nullptr,configMAX_PRIORITIES-1,nullptr,1); 
	// Mejora 6: Crear task de procesamiento paralelo en core 0
	xTaskCreatePinnedToCore(processing_task,"proc_task",4096,nullptr,configMAX_PRIORITIES-2,&processing_task_handle,0);
}

static void adopt_ready_frame(){ 
	uint32_t flush_start = micros(); // DEBUG: inicio flush display
	
	// Mejora 10: Direct flush hacia panel (bypass completo de LVGL widget)
	if(direct_flush_request && direct_flush_buf_index >= 0 && direct_flush_buf_index < BUF_COUNT && display_manager_ptr){
		lv_color_t * buf = square_bufs[direct_flush_buf_index];
		if(buf){
			// Flush directo al panel sin widgets
			Arduino_CO5300 * gfx = display_manager_ptr->getGfx();
			if(gfx){
				int xo = (LCD_WIDTH - SQUARE_SIDE) / 2;
				gfx->startWrite();
				gfx->writeAddrWindow(xo, 0, SQUARE_SIDE, SQUARE_SIDE);
				gfx->writePixels((uint16_t*)buf, SQUARE_SIDE * SQUARE_SIDE);
				gfx->endWrite();
				frame_counter++;
			}
		}
		direct_flush_request = false;
		direct_flush_buf_index = -1;
		
		uint32_t flush_end = micros(); // DEBUG: fin flush directo
		uint32_t flush_time = flush_end - flush_start;
		display_total_time_us += flush_time;
		if(flush_time > display_max_time_us) display_max_time_us = flush_time;
		display_frame_count++;
		return;
	}
	
	// Fallback: método tradicional con widget (mantener compatibilidad)
	if(ready_index==consume_index) return; 
	int idx=ready_index; if(idx<0||idx>=BUF_COUNT) return; 
	consume_index=idx; 
	if(!camera_img) return;
	
	uint32_t lvgl_start = micros(); // DEBUG: inicio LVGL
	static lv_image_dsc_t d; 
	d.header.w=SQUARE_SIDE; d.header.h=SQUARE_SIDE; d.header.cf=LV_COLOR_FORMAT_RGB565; 
	d.data_size=(uint32_t)SQUARE_SIDE*SQUARE_SIDE*sizeof(lv_color_t); d.data=(const uint8_t*)square_bufs[idx]; 
	lv_image_set_src(camera_img,&d); frame_counter++; 
	uint32_t lvgl_end = micros(); // DEBUG: fin LVGL
	
	uint32_t lvgl_time = lvgl_end - lvgl_start;
	lvgl_total_time_us += lvgl_time;
	if(lvgl_time > lvgl_max_time_us) lvgl_max_time_us = lvgl_time;
	lvgl_frame_count++;
}

static void update_fps(){ /* FPS UI removida */ uint32_t now=millis(); if(!last_fps_tick){ last_fps_tick=now; frame_counter=0; return;} if(now-last_fps_tick>=FPS_INTERVAL_MS){ frame_counter=0; last_fps_tick=now; } }

static void log_memory(){ size_t free_heap=heap_caps_get_free_size(MALLOC_CAP_DEFAULT); size_t free_psram=heap_caps_get_free_size(MALLOC_CAP_SPIRAM); Serial.printf("[MEM] Heap:%u PSRAM:%u\n", (unsigned)free_heap,(unsigned)free_psram); }

static void ensure_photo_dir(){ if(!sd_ok) return; if(!SD_MMC.exists(PHOTO_DIR)){
		String dir=""; String full=PHOTO_DIR; int start=0; while(start < (int)full.length()){
				int slash=full.indexOf('/', start); if(slash<0) slash=full.length(); String part=full.substring(start, slash); if(part.length()){
						dir += "/" + part; if(!SD_MMC.exists(dir)) SD_MMC.mkdir(dir);
				}
				start = slash+1; if(slash >= (int)full.length()) break;
		}
	}
}

static void init_sd(){ Serial.println("[SD] Mounting SD_MMC (1-bit)..."); if(!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)){ Serial.println("[SD] setPins failed"); return; } if(!SD_MMC.begin(SD_MOUNT_POINT,1)){ Serial.println("[SD] mount failed"); return; } if(SD_MMC.cardType()==CARD_NONE){ Serial.println("[SD] no card"); return; } sd_ok=true; ensure_photo_dir(); Serial.println("[SD] OK"); }

static bool save_jpeg_from_rgb565(const uint8_t * rgb565, uint16_t w, uint16_t h){ if(!sd_ok) return false; const uint8_t * src = rgb565; uint8_t * work_buf = nullptr; size_t pix_bytes = (size_t)w*h*2; // RGB565
	
	// BUGFIX: El snapshot RAW necesita formato específico para JPEG
	// Como el snapshot se toma ANTES del swap de display, está en formato nativo de cámara
	// fmt2jpg() espera RGB565 en formato específico - probamos sin swap primero
	bool need_swap_for_jpeg = false; // Si JPEG sale con colores incorrectos, cambiar a true
	
	if(need_swap_for_jpeg){
		// Crear buffer temporal en PSRAM si es posible para no modificar el original
		work_buf = (uint8_t*)heap_caps_malloc(pix_bytes, MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
		if(!work_buf) work_buf = (uint8_t*)heap_caps_malloc(pix_bytes, MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
		if(work_buf){
			const uint8_t * in = rgb565;
			for(size_t i=0;i<pix_bytes;i+=2){ work_buf[i] = in[i+1]; work_buf[i+1] = in[i]; }
			src = work_buf;
		} else {
			// Sin buffer: swap in-place (casting away const). Se revertiría si se reutilizara.
			uint8_t * mut = (uint8_t*)rgb565;
			for(size_t i=0;i<pix_bytes;i+=2){ uint8_t a=mut[i]; mut[i]=mut[i+1]; mut[i+1]=a; }
			src = mut;
		}
	}
	uint8_t * jpg_buf=nullptr; size_t jpg_len=0; if(!fmt2jpg((uint8_t*)src, (uint32_t)pix_bytes, w, h, PIXFORMAT_RGB565, PHOTO_JPEG_QUALITY, &jpg_buf, &jpg_len)){ Serial.println("[PHOTO] fmt2jpg failed"); if(work_buf) free(work_buf); return false; }
	if(work_buf) free(work_buf);
	ensure_photo_dir(); char fname[80]; snprintf(fname,sizeof(fname),"%s/PHOTO_%06lu.jpg", PHOTO_DIR, (unsigned long)photo_counter++); File f=SD_MMC.open(fname, FILE_WRITE); if(!f){ Serial.println("[PHOTO] open fail"); free(jpg_buf); return false; } size_t wr=f.write(jpg_buf, jpg_len); f.close(); free(jpg_buf); bool ok= wr==jpg_len; Serial.printf("[PHOTO] %s %u bytes %s (w=%u h=%u)\n", fname, (unsigned)wr, ok?"OK":"ERR", w, h); return ok; }

static void encode_and_store_snapshot(){ if(!snapshot_ready || snapshot_in_progress) return; snapshot_in_progress=true; bool ok=false; if(snapshot_raw && snapshot_w && snapshot_h){ ok = save_jpeg_from_rgb565(snapshot_raw, snapshot_w, snapshot_h); }
	// Disparar animación de marco (independiente de ok)
	frame_anim_request=true;
	// No apagar LED aquí: se apaga cuando la animación termina para sincronizar
	snapshot_ready=false; snapshot_in_progress=false; }

static void request_snapshot(){ if(!camera_initialized || snapshot_request || snapshot_in_progress || snapshot_ready) return; snapshot_request=true; Serial.println("[PHOTO] Solicitud snapshot alta resolución"); }

// Mejora 6: Task paralelo para procesamiento de swap/conversión
static void processing_task(void * arg){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // esperar notificación
		if(processing_request && processing_buf_index >= 0 && processing_buf_index < BUF_COUNT){
			lv_color_t * buf = square_bufs[processing_buf_index];
			if(buf && swap_bytes_rgb565){
				// Swap optimizado en core separado
				const size_t total_pixels = (size_t)SQUARE_SIDE*SQUARE_SIDE;
				uint32_t * p32 = (uint32_t*)buf;
				size_t pairs = total_pixels/2;
				while(pairs--){ *p32 = __builtin_bswap32(*p32); ++p32; }
				if(total_pixels & 1){ 
					uint16_t * last = (uint16_t*)p32; 
					uint16_t v = *last; 
					*last = (uint16_t)((v>>8)|(v<<8)); 
				}
			}
			processing_request = false;
			processing_buf_index = -1;
		}
	}
}

void setup(){ 
	Serial.begin(115200); delay(200); Serial.println("[APP] Camera viewer start"); pinMode(BUTTON_TOP,INPUT_PULLUP); 
	Serial.println("[IOEXP] TCA9555 initialization skipped");
	pixel.begin(); pixel.setBrightness(PHOTO_LED_BRIGHTNESS); pixel.clear(); pixel.show();
	init_sd();
	display.init(); 
	display_manager_ptr = &display; // Mejora 10: guardar referencia para direct flush
	create_ui(); init_camera(); start_camera_task(); log_memory(); 
}

void loop(){ 
	uint32_t loop_start = micros(); // DEBUG: inicio loop
	
	uint32_t display_upd_start = micros();
	display.update(); 
	uint32_t display_upd_end = micros();
	uint32_t display_upd_time = display_upd_end - display_upd_start;
	lvgl_total_time_us += display_upd_time;
	if(display_upd_time > lvgl_max_time_us) lvgl_max_time_us = display_upd_time;
	lvgl_frame_count++;
	
	adopt_ready_frame(); 
	update_fps(); 
	if(snapshot_ready && !snapshot_in_progress){ encode_and_store_snapshot(); }
	
	// DEBUG: Reporte de rendimiento cada 5 segundos
	uint32_t now = millis();
	if(now - last_perf_report >= PERF_REPORT_INTERVAL_MS){
		Serial.println("=== PERFORMANCE ANALYSIS ===");
		if(camera_frame_count > 0){
			Serial.printf("CAMERA: %u frames, avg=%uus, max=%uus, fps=%.1f\n", 
				camera_frame_count, camera_total_time_us/camera_frame_count, camera_max_time_us,
				camera_frame_count * 1000.0f / PERF_REPORT_INTERVAL_MS);
		}
		if(display_frame_count > 0){
			Serial.printf("DISPLAY: %u frames, avg=%uus, max=%uus, fps=%.1f\n", 
				display_frame_count, display_total_time_us/display_frame_count, display_max_time_us,
				display_frame_count * 1000.0f / PERF_REPORT_INTERVAL_MS);
		}
		if(lvgl_frame_count > 0){
			Serial.printf("LVGL: %u frames, avg=%uus, max=%uus, fps=%.1f\n", 
				lvgl_frame_count, lvgl_total_time_us/lvgl_frame_count, lvgl_max_time_us,
				lvgl_frame_count * 1000.0f / PERF_REPORT_INTERVAL_MS);
		}
		Serial.printf("TOTAL FPS: %.1f\n", frame_counter * 1000.0f / PERF_REPORT_INTERVAL_MS);
		log_memory();
		Serial.println("============================");
		
		// Reset contadores
		camera_frame_count = display_frame_count = lvgl_frame_count = 0;
		camera_total_time_us = display_total_time_us = lvgl_total_time_us = 0;
		camera_max_time_us = display_max_time_us = lvgl_max_time_us = 0;
		frame_counter = 0;
		last_perf_report = now;
	}
	
	// Gestion botón (pulsación larga)
	static bool last_level=true; static uint32_t press_start=0; bool level=digitalRead(BUTTON_TOP);
	if(last_level && !level){ press_start=millis(); }
	else if(!last_level && level){ uint32_t dur=millis()-press_start; if(dur>=PHOTO_HOLD_MS){ request_snapshot(); } }
	last_level=level;
	
	// Animación de marco
	if(frame_anim_request){ frame_anim_request=false; frame_anim_active=true; frame_anim_start_ms=millis(); if(frame_layer){
		// Encender LED blanco justo al inicio de la animación
		pixel.setBrightness(PHOTO_LED_BRIGHTNESS);
		pixel.setPixelColor(0, pixel.Color(255,255,255)); pixel.show();
		lv_obj_set_style_border_width(frame_layer, FRAME_BORDER_START, 0);
		lv_obj_set_style_border_opa(frame_layer, 255, 0);
		lv_obj_move_foreground(frame_layer);
	}}
	if(frame_anim_active){ uint32_t elapsed=millis()-frame_anim_start_ms; if(elapsed>=FRAME_ANIM_TOTAL_MS){
		// Apagar LED cuando la animación termina
		pixel.setPixelColor(0, pixel.Color(0,0,0)); pixel.show();
		if(frame_layer){ lv_obj_set_style_border_width(frame_layer, 0, 0); lv_obj_set_style_border_opa(frame_layer, 0, 0); }
		frame_anim_active=false;
	} else {
		uint32_t t = (elapsed * 1000) / FRAME_ANIM_TOTAL_MS; // escala pseudo-fina
		// Progreso 0..1
		float prog = (float)elapsed / (float)FRAME_ANIM_TOTAL_MS;
		uint16_t bw = (uint16_t)(FRAME_BORDER_START * (1.0f - prog));
		uint8_t opa = (uint8_t)(255 * (1.0f - prog));
		if(frame_layer){ lv_obj_set_style_border_width(frame_layer, bw, 0); lv_obj_set_style_border_opa(frame_layer, opa, 0); }
	}}
	// Mejora 12: loop sin delay para máximo throughput
	// vTaskDelay(1); // eliminado para performance extrema
}