#include <Arduino.h>

#if ARDUINO_WAVESHARE_ESP32S3_TOUCH_LCD_128
#include <CST816S.h>
// #include <TouchDrvCSTXXX.hpp>
#include <waveshare_esp32s3_touch_lcd_128/touch_config.h>

#define HAS_TOUCH 1
#elif ARDUINO_RASPBERRY_PI_PICO_2W
#include <TouchDrvGT911.hpp>
#include <rpipico2w/touch_config.h>

#define HAS_TOUCH 1
#else
#define HAS_TOUCH 0
#endif

#include <lvgl.h>
#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif

#if HAS_TOUCH

#if ARDUINO_WAVESHARE_ESP32S3_TOUCH_LCD_128
CST816S touch(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_IRQ); // sda, scl, rst, irq
#elif HAS_TOUCH && ARDUINO_RASPBERRY_PI_PICO_2W
#endif

TouchDrvInterface *touch;
int16_t x[5], y[5];
#endif

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];


#if HAS_TOUCH
void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (!touch->isPressed())
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    else
    {
        data->state = LV_INDEV_STATE_PRESSED;

        uint8_t touched = touch->getPoint(x, y, 1);

        Serial.println("touch " + String(x[0]) + ", " + String(y[0]));

        data->point.x = x[0];
        data->point.y = y[0];
    }

}
#endif

/*use Arduino millis() as tick source*/
static uint32_t tick_source(void)
{
    return millis();
}

void setup()
{
    String LVGL_Arduino = "Hello 1: ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() + " " + __TIME__;

    Serial.begin(SERIAL_BAUDRATE);
    Serial.println(LVGL_Arduino);

    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(tick_source);


    lv_display_t *disp;
#if LV_USE_TFT_ESPI
    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
    disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);
#else
    /*Else create a display yourself*/
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

#if HAS_TOUCH
    bool result = false;

    touch = new TouchDrvGT911();
    touch->setPins(TOUCH_RST, TOUCH_IRQ);
    result = touch->begin(Wire, GT911_SLAVE_ADDRESS_L, TOUCH_SDA, TOUCH_SCL);
    if (result)
    {
        const char *model = touch->getModelName();
        Serial.printf("Successfully initialized %s, using %s Driver!\n", model, model);
    }

    /*Initialize the (dummy) input device driver*/
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, touchpad_read);
#endif

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, LVGL_Arduino.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    Serial.println("Setup done");
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */

    delay(5);           /* let this time pass */
}