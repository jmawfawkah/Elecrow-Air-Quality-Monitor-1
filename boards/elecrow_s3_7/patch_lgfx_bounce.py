"""
patch_lgfx_bounce.py — PlatformIO pre-build extra_script.

Rewrites LovyanGFX's Bus_RGB::init() in libdeps to use ESP-IDF's
esp_lcd_new_rgb_panel with bounce_buffer_size_px instead of the raw
i80/GDMA hack shipped by LovyanGFX. This is the standard fix for RGB
panel tearing on ESP32-S3 with PSRAM framebuffers.

Also removes ARM-specific assembly folders (helium, neon) from the LVGL
library if present, as they cause assembler errors on Xtensa (ESP32) builds.
"""

import os
import sys
import shutil

Import("env")  # noqa: F821  (provided by PlatformIO SCons)

MARKER = "BRUCE_BOUNCE_PATCH_APPLIED_V3"
MARKER_PREFIX = "BRUCE_BOUNCE_PATCH_APPLIED"

PATCHED_INIT = r"""bool Bus_RGB::init(void)
  {
    // BRUCE_BOUNCE_PATCH: replaced upstream raw-i80/GDMA scanout with
    // esp_lcd_new_rgb_panel + bounce_buffer_size_px so DMA reads from
    // internal SRAM, not the PSRAM framebuffer. CPU writes to the FB no
    // longer race DMA scanout, eliminating tearing.
    //
    // Bounce buffer sizing tradeoff:
    //   Larger = more tearing headroom during Wi-Fi/BT DMA bursts, but
    //   eats more contiguous internal DMA-capable SRAM (which BLE
    //   controller init and radioHasMemForBle() require).
    //   Smaller = frees SRAM for BLE, but small bursts of Wi-Fi/BT DMA
    //   can starve the refill and cause visible tearing again.
    // Default 6 lines (~9.6 KB per buffer, ~19.2 KB total). Override with
    // -DLGFX_BOUNCE_LINES=N in the board .ini if needed:
    //   - 4-6 for last-resort BLE headroom (accept more Wi-Fi tearing)
    //   - 6-8 for practical BLE/Wi-Fi balance on V1.3+ panels
    //   - 20+ for maximum smoothness (BLE may refuse to init)
#ifndef LGFX_BOUNCE_LINES
#define LGFX_BOUNCE_LINES 6
#endif
    esp_lcd_rgb_panel_config_t rgb_cfg = {};
    rgb_cfg.clk_src = LCD_CLK_SRC_DEFAULT;
    rgb_cfg.timings.pclk_hz = _cfg.freq_write;
    rgb_cfg.timings.h_res = _cfg.panel->width();
    rgb_cfg.timings.v_res = _cfg.panel->height();
    rgb_cfg.timings.hsync_pulse_width = _cfg.hsync_pulse_width;
    rgb_cfg.timings.hsync_back_porch = _cfg.hsync_back_porch;
    rgb_cfg.timings.hsync_front_porch = _cfg.hsync_front_porch;
    rgb_cfg.timings.vsync_pulse_width = _cfg.vsync_pulse_width;
    rgb_cfg.timings.vsync_back_porch = _cfg.vsync_back_porch;
    rgb_cfg.timings.vsync_front_porch = _cfg.vsync_front_porch;
    rgb_cfg.timings.flags.hsync_idle_low  = !_cfg.hsync_polarity;
    rgb_cfg.timings.flags.vsync_idle_low  = !_cfg.vsync_polarity;
    rgb_cfg.timings.flags.de_idle_high    = _cfg.de_idle_high;
    rgb_cfg.timings.flags.pclk_active_neg = _cfg.pclk_active_neg;
    rgb_cfg.timings.flags.pclk_idle_high  = _cfg.pclk_idle_high;
    rgb_cfg.data_width = 16;
    rgb_cfg.bits_per_pixel = 16;
    rgb_cfg.num_fbs = 1;
    rgb_cfg.bounce_buffer_size_px = _cfg.panel->width() * LGFX_BOUNCE_LINES;
    rgb_cfg.dma_burst_size = 64;
    rgb_cfg.hsync_gpio_num = _cfg.pin_hsync;
    rgb_cfg.vsync_gpio_num = _cfg.pin_vsync;
    rgb_cfg.de_gpio_num    = _cfg.pin_henable;
    rgb_cfg.pclk_gpio_num  = _cfg.pin_pclk;
    rgb_cfg.disp_gpio_num  = -1;
    // Swap the two 8-bit halves of the data bus. RGB565 is stored in memory
    // as big-endian (R:G high byte, G:B low byte) but the S3 LCD_CAM
    // peripheral clocks the low byte out first. Upstream LovyanGFX's raw
    // path did the same XOR-8 swap; without it the RGB channels are
    // scrambled (purple <-> green shift, etc.).
    for (int i = 0; i < 16; ++i) rgb_cfg.data_gpio_nums[i ^ 8] = _cfg.pin_data[i];
    rgb_cfg.flags.fb_in_psram = 1;

    if (esp_lcd_new_rgb_panel(&rgb_cfg, &_panel_handle) != ESP_OK) return false;
    if (esp_lcd_panel_reset(_panel_handle) != ESP_OK) return false;
    if (esp_lcd_panel_init(_panel_handle)  != ESP_OK) return false;

    void* fb0 = nullptr;
    if (esp_lcd_rgb_panel_get_frame_buffer(_panel_handle, 1, &fb0) != ESP_OK) return false;
    _frame_buffer = (uint8_t*)fb0;

    _i80_bus     = nullptr;
    _dmadesc     = nullptr;
    _dma_ch      = -1;
    _intr_handle = nullptr;
    return true;
  }
"""

PATCHED_RELEASE = r"""void Bus_RGB::release(void)
  {
    // BRUCE_BOUNCE_PATCH: also delete the ESP-IDF panel we created.
    if (_panel_handle) {
      esp_lcd_panel_del(_panel_handle);
      _panel_handle = nullptr;
    }
    if (_intr_handle) {
      esp_intr_free(_intr_handle);
      _intr_handle = nullptr;
    }
    if (_i80_bus)
    {
      esp_lcd_del_i80_bus(_i80_bus);
      _i80_bus = nullptr;
    }
    if (_dmadesc)
    {
      heap_caps_free(_dmadesc);
      _dmadesc = nullptr;
    }
  }
"""


def find_bus_rgb_cpp(env):
    project_dir = env["PROJECT_DIR"]
    libdeps_dir = env.get("PROJECT_LIBDEPS_DIR") or os.path.join(project_dir, ".pio", "libdeps")
    envname = env["PIOENV"]
    candidate = os.path.join(
        libdeps_dir, envname, "LovyanGFX", "src", "lgfx", "v1", "platforms", "esp32s3", "Bus_RGB.cpp"
    )
    if os.path.isfile(candidate):
        return candidate
    for root, _dirs, files in os.walk(libdeps_dir):
        if "Bus_RGB.cpp" in files and "esp32s3" in root:
            return os.path.join(root, "Bus_RGB.cpp")
    return None


def replace_function_body(src, header_line, replacement_full):
    idx = src.find(header_line)
    if idx == -1:
        return None
    brace = src.find("{", idx)
    if brace == -1:
        return None
    depth = 1
    i = brace + 1
    n = len(src)
    while i < n and depth > 0:
        c = src[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        i += 1
    if depth != 0:
        return None
    return src[:idx] + replacement_full + src[i:]


def apply_patch(path):
    with open(path, "r", encoding="utf-8", newline="") as f:
        original = f.read()

    if MARKER in original:
        print("[patch_lgfx_bounce] Bus_RGB.cpp already patched at current version — skipping.")
        return True

    # Strip any previous BRUCE_BOUNCE_PATCH marker line so re-patches don't
    # accumulate stamps at the top of the file.
    lines = original.splitlines(keepends=True)
    while lines and MARKER_PREFIX in lines[0]:
        lines.pop(0)
    original = "".join(lines)

    # replace_function_body only anchors on the header signature. It works
    # whether the current body is the upstream original OR a previously-
    # patched body from an earlier marker version — brace-matching finds
    # the current body's boundary either way.
    new = replace_function_body(original, "bool Bus_RGB::init(void)", PATCHED_INIT)
    if new is None:
        print("[patch_lgfx_bounce] ERROR: could not locate Bus_RGB::init() body", file=sys.stderr)
        return False

    new2 = replace_function_body(new, "void Bus_RGB::release(void)", PATCHED_RELEASE)
    if new2 is None:
        print("[patch_lgfx_bounce] ERROR: could not locate Bus_RGB::release() body", file=sys.stderr)
        return False

    stamp = f"// {MARKER}: LovyanGFX Bus_RGB patched to use esp_lcd_new_rgb_panel + bounce buffer.\n"
    new2 = stamp + new2

    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(new2)
    print(f"[patch_lgfx_bounce] Patched: {path}")
    return True


def remove_arm_asm_folders(env):
    """Removes ARM-specific assembly folders from LVGL that break Xtensa builds."""
    project_dir = env["PROJECT_DIR"]
    libdeps_dir = env.get("PROJECT_LIBDEPS_DIR") or os.path.join(project_dir, ".pio", "libdeps")
    envname = env["PIOENV"]
    blend_dir = os.path.join(libdeps_dir, envname, "lvgl", "src", "draw", "sw", "blend")
    
    for folder in ["helium", "neon"]:
        folder_path = os.path.join(blend_dir, folder)
        if os.path.isdir(folder_path):
            print(f"[patch_lgfx_bounce] Removing incompatible ARM ASM folder: {folder_path}")
            shutil.rmtree(folder_path)


def patch_action(source, target, env):
    path = find_bus_rgb_cpp(env)
    if not path:
        print(
            "[patch_lgfx_bounce] Bus_RGB.cpp not found yet (libdeps not populated). "
            "The patch will apply on the next build after LovyanGFX is fetched.",
            file=sys.stderr,
        )
        return
    if not apply_patch(path):
        env.Exit(1)


# Run the ARM ASM folder removal immediately. If LVGL is already downloaded,
# this will delete the folders before compilation starts.
remove_arm_asm_folders(env)

path_now = find_bus_rgb_cpp(env)  # noqa: F821
if path_now:
    if not apply_patch(path_now):
        env.Exit(1)  # noqa: F821
else:
    # LovyanGFX not fetched yet — hook into the lib install step.
    env.AddPreAction("checkprogsize", patch_action)  # noqa: F821