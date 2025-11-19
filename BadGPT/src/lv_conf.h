#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_DEV_VERSION      1

/* Color settings */
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0

/* Memory settings */
#define LV_MEM_CUSTOM           0
#define LV_MEM_SIZE             (64U * 1024U)

/* HAL settings */
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_INDEV_DEF_READ_PERIOD 30

/* Feature usage */
#define LV_USE_ANIMATION        1
#define LV_USE_SHADOW           1
#define LV_USE_BLEND_MODES      1
#define LV_USE_OPA_SCALE        1
#define LV_USE_IMG_TRANSFORM    1

/* Widget usage */
#define LV_USE_ARC              1
#define LV_USE_ANIMIMG          1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_BTNMATRIX        1
#define LV_USE_CANVAS           1
#define LV_USE_CHECKBOX         1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_LABEL            1
#define LV_USE_LINE             1
#define LV_USE_LIST             1
#define LV_USE_METER            1
#define LV_USE_MSGBOX           1
#define LV_USE_ROLLER           1
#define LV_USE_SLIDER           1
#define LV_USE_SPAN             1
#define LV_USE_SPINBOX          1
#define LV_USE_SPINNER          1
#define LV_USE_SWITCH           1
#define LV_USE_TEXTAREA         1
#define LV_USE_TABLE            1
#define LV_USE_TABVIEW          1
#define LV_USE_TILEVIEW         1
#define LV_USE_WIN              1

/* Themes */
#define LV_USE_THEME_DEFAULT    1
#define LV_USE_THEME_BASIC      1

/* Font usage */
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_18    1
#define LV_FONT_MONTSERRAT_22    1
#define LV_FONT_MONTSERRAT_26    1
#define LV_FONT_MONTSERRAT_30    1
#define LV_FONT_MONTSERRAT_34    1
#define LV_FONT_MONTSERRAT_38    1
#define LV_FONT_MONTSERRAT_42    1
#define LV_FONT_MONTSERRAT_46    1
#define LV_FONT_MONTSERRAT_48    1

/* Others */
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_USE_REFR_DEBUG       0

/* Compiler settings */
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY

/* HAL settings */
#define LV_TICK_CUSTOM     0
#define LV_DPI_DEF         130

#endif /*LV_CONF_H*/ 