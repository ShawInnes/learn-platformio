#if ARDUINO
#include <Arduino.h>
#endif

#if ARDUINO_WAVESHARE_ESP32S3_TOUCH_LCD_128
#include <TouchDrvInterface.hpp>
#include <TouchDrvCSTXXX.hpp>
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
#include <time.h>
#include <display/lv_display_private.h>
#include <indev/lv_indev_private.h>

#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif

#if HAS_TOUCH
TouchDrvInterface *touch;
int16_t x[5], y[5];
#endif

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))

uint32_t draw_buf[DRAW_BUF_SIZE / 4];

static lv_disp_t *disp;
static lv_indev_t *indev;

void draw_circle(lv_coord_t x, lv_coord_t y) {
    lv_obj_t *circle = lv_obj_create(lv_scr_act());
    lv_obj_set_size(circle, 10, 10);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFF0000), 0);

    lv_obj_align(circle, LV_ALIGN_CENTER, x - (TFT_WIDTH / 2), y - (TFT_HEIGHT / 2));
}

static lv_obj_t *clock_label;
static lv_style_t clock_style;

void update_clock(lv_timer_t *timer) {
    time_t now;
    struct tm timeinfo;
    char buffer[10];

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    lv_label_set_text(clock_label, buffer);
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);
}

void setup_clock() {
    lv_style_init(&clock_style);
    lv_style_set_text_font(&clock_style, &lv_font_montserrat_28); // Set font size

    clock_label = lv_label_create(lv_scr_act());
    lv_label_set_text(clock_label, "");
    lv_obj_add_style(clock_label, &clock_style, LV_PART_MAIN);
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);

    lv_timer_create(update_clock, 1000, NULL); // Update clock every second
}


#if HAS_TOUCH
void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    uint8_t touched = touch->getPoint(x, y, 1); // Directly get touch points
    if (touched > 0) {
        lv_coord_t adjusted_x = x[0];
        lv_coord_t adjusted_y = y[0];

        Serial.printf("Touch coordinates: x = %d, y = %d\n", adjusted_x, adjusted_y);
        Serial.printf("Data coordinates: x = %d, y = %d\n", data->point.x, data->point.y);

        last_x = adjusted_x;
        last_y = adjusted_y;

        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_PRESSED;

        draw_circle(last_x, last_y);
    } else {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
#endif

void setup() {
#if ARDUINO
    Serial.begin(SERIAL_BAUDRATE);
#endif
    lv_init();

    // lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
#if LV_USE_TFT_ESPI
    disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, sizeof(draw_buf));
#elif LV_USE_SDL
    disp = lv_sdl_window_create(TFT_WIDTH, TFT_HEIGHT);
#else
#pragma error "no display driver configured"
#endif
    // lv_display_set_rotation(disp, TFT_ROTATION);

#if HAS_TOUCH
    bool result = false;

#if ARDUINO_WAVESHARE_ESP32S3_TOUCH_LCD_128
    touch = new TouchDrvCSTXXX();
    touch->setPins(TOUCH_RST, TOUCH_IRQ);
    result = touch->begin(Wire, CST816_SLAVE_ADDRESS, TOUCH_SDA, TOUCH_SCL);
    if (result)
    {
        const char *model = touch->getModelName();
        Serial.printf("Successfully initialized %s, using %s Driver!\n", model, model);
    }
#elif ARDUINO_RASPBERRY_PI_PICO_2W
    touch = new TouchDrvGT911();
    touch->setPins(TOUCH_RST, TOUCH_IRQ);
    result = touch->begin(Wire, GT911_SLAVE_ADDRESS_L, TOUCH_SDA, TOUCH_SCL);
    if (result) {
        const char *model = touch->getModelName();
        Serial.printf("Successfully initialized %s, using %s Driver!\n", model, model);
    }
#endif

    /*Initialize the (dummy) input device driver*/
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_mode(indev, LV_INDEV_MODE_TIMER);
    lv_indev_set_display(indev, disp);
    lv_indev_set_read_cb(indev, touchpad_read);
    lv_indev_enable(indev, true);
#endif

    setup_clock();
}

void loop() {
    lv_tick_inc(5); // Fixed tick increment
    lv_timer_handler();

#if ARDUINO    
    delay(5);
#endif
}

#if LV_USE_SDL
    int main(int argc, char *argv[])
    {
        setup();  // Call the existing setup()
        
        while (true) {
            loop();  // Run loop continuously
        }

        return 0;
    }
#endif