### Convert images to `.c` for LVGL (v9, RGB565)

Follow these steps to generate `.c` files from images and use them in this project.

- **Tool**: [LVGL Image Converter](https://lvgl.io/tools/imageconverter)
- **Version**: LVGL v9
- **Color format**: RGB565
- **Output format**: C array

#### Steps
1. Open the converter: [LVGL Image Converter](https://lvgl.io/tools/imageconverter).
2. At the top, select **LVGL v9**.
3. Upload your image (PNG/JPG/SVG/BMP).
4. In **Color format**, choose **RGB565**.
5. In **Output format**, choose **C array**.
6. (Optional) Enable **Dither images** if it improves quality for your asset.
7. Click **Convert** and download the generated `.c` file.
8. Copy the `.c` file into this directory: `src/images/`.

Note: RGB565 does not include an alpha channel. If your image requires transparency, adapt it (e.g., solid background) or use an alternative format that supports alpha if your configuration allows it.

#### Code usage (LVGL v9 example)
```c
#include "lvgl.h"

LV_IMG_DECLARE(logotipo); // Replace with your generated image symbol name

void ui_show_logo(void) {
    lv_obj_t *img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, &logotipo);
    lv_obj_center(img);
}
```

For more details, see: [LVGL Image Converter](https://lvgl.io/tools/imageconverter).


