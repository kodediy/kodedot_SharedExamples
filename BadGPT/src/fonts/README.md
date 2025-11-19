## Generating fonts for LVGL (Inter)

This project uses LVGL-compatible C fonts generated from a TTF (Inter). Follow these steps to regenerate or add new sizes.

### Web tool (recommended)

Use the LVGL Font Converter: [LVGL Font Converter](https://lvgl.io/tools/fontconverter)

1) Upload your TTF:
   - Click "Choose File" and select the Inter TTF you want to convert.

2) Configure settings:
   - Bpp / Color depth: 1 (Monochrome, 1 bit per pixel)
   - Size (px): set the pixel height you need (repeat these steps for each size, e.g. 20, 30, 40, 50, 70, 100, 130, 150, 200)
   - Character ranges:
     - Ranges: `0x20-0x7A,0x30-0x39`  
       (covers basic Latin and digits)
   - Extra symbols (add as needed): `@#$%&*!:°`  
     - Degree symbol: `°` (U+00B0)
     - You may also add other common symbols if required, e.g.: `+ - / ( ) [ ] { } , . ? ; ' " _ = < > ^ ~ | \\`
   - Output format: LVGL v8, C array (.c)
   - Font name: use `Inter_<SIZE>` (e.g. `Inter_20`, `Inter_30`, ...)

3) Convert and download:
   - Click "Convert" and save the resulting `.c` file into `src/fonts/`.
   - Example filenames: `Inter_20.c`, `Inter_30.c`, `Inter_40.c`, etc.

4) Commit the new/updated font files.

Notes:
- Keep Bpp at 1 to minimize flash usage. If you ever change Bpp, ensure all target devices have sufficient memory.
- Ensure the chosen sizes match your UI usage to avoid unnecessary binary size growth.

### Optional: CLI (lv_font_conv)

If you prefer a reproducible command-line workflow, you can use the LVGL font converter CLI: [lv_font_conv](https://github.com/lvgl/lv_font_conv)

Example command (adjust paths and sizes as needed):

```bash
npx lv_font_conv \
  --font ./Inter.ttf \
  --size 20 \
  --bpp 1 \
  --range 0x20-0x7A,0x30-0x39 \
  --symbols "@#$%&*!:°" \
  --format lvgl \
  --font-name "Inter_20" \
  --output "src/fonts/Inter_20.c"
```

Repeat for each size you need (30, 40, 50, 70, 100, 130, 150, 200, ...), updating `--size`, `--font-name`, and `--output` accordingly.


