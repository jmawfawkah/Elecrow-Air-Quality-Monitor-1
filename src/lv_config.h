#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH          16      /* 16-bit RGB565 */
#define LV_COLOR_CHROMA_KEY     lv_color_hex(0x00ff00)

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_MEM_CUSTOM           1
#define LV_MEM_SIZE             (32U * 1024U)
#define LV_MEM_ADR              0
#define LV_MEM_BUF_MAX_NUM      16
#define LV_MEMCPY_MEMSET_INCLUDE <string.h>

/* Use PSRAM for LVGL */
#define LV_MEM_CUSTOM_INCLUDE   <esp32-hal-psram.h>

/*====================
   HAL SETTINGS
 *====================*/
#define LV_HOR_RES_MAX          800
#define LV_VER_RES_MAX          480
#define LV_DPI_DEF              130
#define LV_DRAW_BUF_DUAL        1
#define LV_VDB_SIZE             (LV_HOR_RES_MAX * LV_VER_RES_MAX / 10)

/*====================
   FEATURE CONFIGURATION
 *====================*/
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF           1

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         1
#define LV_USE_ASSERT_OBJ_IN_CREATE 1
#define LV_USE_ASSERT_MEM_INTEGRITY 1

#define LV_USE_REFR_DEBUG      0
#define LV_USE_DRAW_MASK       1
#define LV_USE_PERF_MONITOR    0
#define LV_USE_MEM_MONITOR     0
#define LV_USE_THEME_DEFAULT   1
#define LV_USE_THEME_BASIC     1
#define LV_USE_THEME_MONO      0

/*====================
   FONT USAGE
 *====================*/
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   0
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   0
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   1

#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0
#define LV_FONT_UNSCII_8                 0
#define LV_FONT_UNSCII_16                0

#define LV_FONT_DEFAULT         &lv_font_montserrat_16

/*====================
   WIDGET USAGE
 *====================*/
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       0
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1
#define LV_USE_TABLE        1

/* Extra widgets */
#define LV_USE_ANIMIMG      0
#define LV_USE_CALENDAR     0
#define LV_USE_CHART        1
#define LV_USE_COLORWHEEL   0
#define LV_USE_IMGBTN       0
#define LV_USE_KEYBOARD     0
#define LV_USE_LED          0
#define LV_USE_LIST         1
#define LV_USE_MENU         0
#define LV_USE_METER        1
#define LV_USE_MSGBOX       1
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      1
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0

/*====================
   STYLE SETTINGS
 *====================*/
#define LV_USE_LARGE_COORD      0
#define LV_THEME_DEFAULT_DARK   0

/*====================
   ANIMATION SETTINGS
 *====================*/
#define LV_USE_ANIMATION        1

/*====================
   FILE SYSTEM
 *====================*/
#define LV_USE_FS_STDIO         0
#define LV_USE_FS_POSIX         0
#define LV_USE_FS_WIN32         0
#define LV_USE_FS_FATFS         0
#define LV_USE_FS_ROMFS         0

/*====================
   PNG/JPEG DECODING
 *====================*/
#define LV_USE_PNG              0
#define LV_USE_BMP              0
#define LV_USE_SJPG             0
#define LV_USE_GIF              0
#define LV_USE_QRCODE           0
#define LV_USE_FREETYPE         0

/*====================
   OTHERS
 *====================*/
#define LV_USE_SNAPSHOT         0
#define LV_USE_MONKEY           0
#define LV_USE_GRIDNAV          0
#define LV_USE_FRAGMENT         0
#define LV_USE_IMGFONT          0
#define LV_USE_IME_PINYIN       0
#define LV_USE_OBJ_ID           0
#define LV_USE_OBJ_ID_BUILTIN   0
#define LV_USE_SYSMON           0

#endif /*LV_CONF_H*/