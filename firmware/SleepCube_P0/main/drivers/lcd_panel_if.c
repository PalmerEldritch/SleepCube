#include "lcd_panel_if.h"

#include "board_pins.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_jd9853.h"
#include "esp_lcd_touch_axs5106.h"
#include "esp_log.h"

static const char *TAG = "sc_lcd_panel_if";

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6 && CONFIG_SC_UI_BACKEND_LCD_TOUCH

#define SC_LCD_PIXEL_CLOCK_HZ              (80 * 1000 * 1000)
#define SC_LCD_BACKLIGHT_TIMER             LEDC_TIMER_0
#define SC_LCD_BACKLIGHT_MODE              LEDC_LOW_SPEED_MODE
#define SC_LCD_BACKLIGHT_CHANNEL           LEDC_CHANNEL_0
#define SC_LCD_BACKLIGHT_DUTY_RES          LEDC_TIMER_10_BIT
#define SC_LCD_BACKLIGHT_MAX_DUTY          1024
#define SC_LCD_BACKLIGHT_FREQUENCY_HZ      5000
#define SC_LCD_TOUCH_ROTATION              0

static esp_lcd_panel_io_handle_t s_io_handle;
static esp_lcd_panel_handle_t s_panel_handle;
static esp_lcd_touch_handle_t s_touch_handle;
static i2c_master_bus_handle_t s_i2c_bus_handle;
static bool s_started;
static uint16_t s_backlight_level = SC_LCD_BACKLIGHT_MAX_DUTY - 1U;

static esp_err_t sc_lcd_backlight_init(void)
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode = SC_LCD_BACKLIGHT_MODE,
        .timer_num = SC_LCD_BACKLIGHT_TIMER,
        .duty_resolution = SC_LCD_BACKLIGHT_DUTY_RES,
        .freq_hz = SC_LCD_BACKLIGHT_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    const ledc_channel_config_t channel_cfg = {
        .speed_mode = SC_LCD_BACKLIGHT_MODE,
        .channel = SC_LCD_BACKLIGHT_CHANNEL,
        .timer_sel = SC_LCD_BACKLIGHT_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SC_LCD_BL_GPIO,
        .duty = 0,
        .hpoint = 0,
    };

    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "backlight timer config failed");
    return ledc_channel_config(&channel_cfg);
}

static esp_err_t sc_lcd_backlight_set_percent(uint8_t brightness)
{
    if (brightness > 100) {
        brightness = 100;
    }

    const uint32_t duty = (brightness * (SC_LCD_BACKLIGHT_MAX_DUTY - 1U)) / 100U;
    s_backlight_level = (uint16_t)duty;
    ESP_RETURN_ON_ERROR(ledc_set_duty(SC_LCD_BACKLIGHT_MODE, SC_LCD_BACKLIGHT_CHANNEL, duty),
                        TAG,
                        "backlight duty set failed");
    return ledc_update_duty(SC_LCD_BACKLIGHT_MODE, SC_LCD_BACKLIGHT_CHANNEL);
}

esp_err_t sc_lcd_panel_if_set_backlight_level(uint16_t level)
{
    if (level >= SC_LCD_BACKLIGHT_MAX_DUTY) {
        level = SC_LCD_BACKLIGHT_MAX_DUTY - 1U;
    }

    s_backlight_level = level;
    ESP_RETURN_ON_ERROR(ledc_set_duty(SC_LCD_BACKLIGHT_MODE, SC_LCD_BACKLIGHT_CHANNEL, level),
                        TAG,
                        "backlight duty set failed");
    return ledc_update_duty(SC_LCD_BACKLIGHT_MODE, SC_LCD_BACKLIGHT_CHANNEL);
}

uint16_t sc_lcd_panel_if_get_backlight_level(void)
{
    return s_backlight_level;
}

static esp_err_t sc_i2c_bus_init(void)
{
    const i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = (i2c_port_num_t)SC_TOUCH_I2C_PORT,
        .sda_io_num = SC_TOUCH_I2C_SDA_GPIO,
        .scl_io_num = SC_TOUCH_I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };

    return i2c_new_master_bus(&bus_cfg, &s_i2c_bus_handle);
}

static esp_err_t sc_lcd_touch_init(void)
{
    static i2c_master_dev_handle_t dev_handle;
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ESP_LCD_TOUCH_IO_I2C_AXS5106_ADDRESS,
        .scl_speed_hz = 400000,
    };

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = SC_LCD_H_RES,
        .y_max = SC_LCD_V_RES,
        .rst_gpio_num = SC_TOUCH_RST_GPIO,
        .int_gpio_num = SC_TOUCH_INT_GPIO,
    };

    if (SC_LCD_TOUCH_ROTATION == 90) {
        touch_cfg.flags.swap_xy = 1;
    } else if (SC_LCD_TOUCH_ROTATION == 180) {
        touch_cfg.flags.mirror_y = 1;
    } else if (SC_LCD_TOUCH_ROTATION == 270) {
        touch_cfg.flags.swap_xy = 1;
        touch_cfg.flags.mirror_x = 1;
        touch_cfg.flags.mirror_y = 1;
    } else {
        touch_cfg.flags.mirror_x = 1;
    }

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, &dev_handle),
                        TAG,
                        "touch i2c add device failed");
    return esp_lcd_touch_new_i2c_axs5106(dev_handle, &touch_cfg, &s_touch_handle);
}

esp_err_t sc_lcd_panel_if_init(void)
{
    if (s_started) {
        return ESP_OK;
    }

    ESP_LOGI(TAG,
             "lcd init start: host=%d din=%d miso=%d clk=%d cs=%d dc=%d rst=%d bl=%d",
             SC_LCD_SPI_HOST,
             SC_LCD_DIN_GPIO,
             SC_LCD_MISO_GPIO,
             SC_LCD_CLK_GPIO,
             SC_LCD_CS_GPIO,
             SC_LCD_DC_GPIO,
             SC_LCD_RST_GPIO,
             SC_LCD_BL_GPIO);

    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = SC_LCD_CLK_GPIO,
        .mosi_io_num = SC_LCD_DIN_GPIO,
        .miso_io_num = SC_LCD_MISO_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    esp_lcd_panel_io_spi_config_t io_cfg = JD9853_PANEL_IO_SPI_CONFIG(SC_LCD_CS_GPIO, SC_LCD_DC_GPIO, NULL, NULL);
    io_cfg.pclk_hz = SC_LCD_PIXEL_CLOCK_HZ;

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = SC_LCD_RST_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    esp_err_t err = spi_bus_initialize(SC_LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    ESP_RETURN_ON_ERROR(sc_i2c_bus_init(), TAG, "touch i2c init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SC_LCD_SPI_HOST, &io_cfg, &s_io_handle),
                        TAG,
                        "panel io create failed");
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_jd9853(s_io_handle, &panel_cfg, &s_panel_handle),
                        TAG,
                        "panel create failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel_handle), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel_handle), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel_handle, true), TAG, "panel invert failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(s_panel_handle, false, false), TAG, "panel mirror failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel_handle, true), TAG, "panel display on failed");
    ESP_RETURN_ON_ERROR(sc_lcd_touch_init(), TAG, "touch init failed");
    ESP_RETURN_ON_ERROR(sc_lcd_backlight_init(), TAG, "backlight init failed");
    ESP_RETURN_ON_ERROR(sc_lcd_backlight_set_percent(100), TAG, "backlight set failed");

    ESP_LOGI(TAG,
             "lcd panel init complete: host=%d din=%d miso=%d clk=%d cs=%d dc=%d rst=%d bl=%d res=%dx%d offset=%d,%d",
             SC_LCD_SPI_HOST,
             SC_LCD_DIN_GPIO,
             SC_LCD_MISO_GPIO,
             SC_LCD_CLK_GPIO,
             SC_LCD_CS_GPIO,
             SC_LCD_DC_GPIO,
             SC_LCD_RST_GPIO,
             SC_LCD_BL_GPIO,
             SC_LCD_H_RES,
             SC_LCD_V_RES,
             34,
             0);

    s_started = true;
    return ESP_OK;
}

esp_lcd_panel_io_handle_t sc_lcd_panel_if_get_io_handle(void)
{
    return s_io_handle;
}

esp_lcd_panel_handle_t sc_lcd_panel_if_get_panel_handle(void)
{
    return s_panel_handle;
}

esp_lcd_touch_handle_t sc_lcd_panel_if_get_touch_handle(void)
{
    return s_touch_handle;
}

#else

esp_err_t sc_lcd_panel_if_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_lcd_panel_io_handle_t sc_lcd_panel_if_get_io_handle(void)
{
    return NULL;
}

esp_lcd_panel_handle_t sc_lcd_panel_if_get_panel_handle(void)
{
    return NULL;
}

esp_lcd_touch_handle_t sc_lcd_panel_if_get_touch_handle(void)
{
    return NULL;
}

esp_err_t sc_lcd_panel_if_set_backlight_level(uint16_t level)
{
    (void)level;
    return ESP_ERR_NOT_SUPPORTED;
}

uint16_t sc_lcd_panel_if_get_backlight_level(void)
{
    return 0;
}

#endif
