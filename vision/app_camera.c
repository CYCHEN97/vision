/* ESPRESSIF MIT License
 * 
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 * 
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_camera.h"
#include "app_camera.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

static const char *TAG = "app_camera";

void app_camera_main () {
#if CONFIG_CAMERA_MODEL_ESP_EYE || CONFIG_CAMERA_MODEL_ESP32_CAM_BOARD
    /* IO13, IO14 is designed for JTAG by default,
     * to use it as generalized input,
     * firstly declair it as pullup input */
    gpio_config_t conf;
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_up_en = GPIO_PULLUP_ENABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pin_bit_mask = 1LL << 13;
    gpio_config(&conf);
    conf.pin_bit_mask = 1LL << 14;
    gpio_config(&conf);
#endif 
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_1;
    config.ledc_timer = LEDC_TIMER_0;
/*
    config.pin_d0 = 	Y2_GPIO_NUM;
    config.pin_d1 = 	Y3_GPIO_NUM;
    config.pin_d2 = 	Y4_GPIO_NUM;
    config.pin_d3 = 	Y5_GPIO_NUM;
    config.pin_d4 = 	Y6_GPIO_NUM;
    config.pin_d5 = 	Y7_GPIO_NUM;
    config.pin_d6 = 	Y8_GPIO_NUM;
    config.pin_d7 = 	Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
*/
	// new s3-camera board
    config.pin_d0 = 17;
    config.pin_d1 = 16;
    config.pin_d2 = 14;
    config.pin_d3 = 15;
    config.pin_d4 = 21;
    config.pin_d5 = 38;
    config.pin_d6 = 39;
    config.pin_d7 = 40;
    config.pin_xclk = 41;
    config.pin_pclk = 18;
    config.pin_vsync = 45;
    config.pin_href = 47;
    config.pin_sscb_sda = 48;
    config.pin_sscb_scl = 3;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
/*
	// old-s2 camera board
    config.pin_d0 = 36;
    config.pin_d1 = 37;
    config.pin_d2 = 41;
    config.pin_d3 = 42;
    config.pin_d4 = 39;
    config.pin_d5 = 40;
    config.pin_d6 = 35;
    config.pin_d7 = 38;
    config.pin_xclk = 12;
    config.pin_pclk = 6;
    config.pin_vsync = 2;
    config.pin_href = 3;
    config.pin_sscb_sda = 8;
    config.pin_sscb_scl = 7;
    config.pin_pwdn = -1;
    config.pin_reset = 26;
*/
/*
	// kauga
    config.pin_d0 = 36;
    config.pin_d1 = 37;
    config.pin_d2 = 41;
    config.pin_d3 = 42;
    config.pin_d4 = 39;
    config.pin_d5 = 40;
    config.pin_d6 = 21;
    config.pin_d7 = 38;
    config.pin_xclk = 1;
    config.pin_pclk = 33;
    config.pin_vsync = 2;
    config.pin_href = 3;
    config.pin_sscb_sda = 8;
    config.pin_sscb_scl = 7;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
*/

    config.xclk_freq_hz = 20000000;// 2 20000000
    //init with high specs to pre-allocate larger buffers
    //config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size =  FRAMESIZE_UXGA;//FRAMESIZE_SVGA;//FRAMESIZE_CIF;//FRAMESIZE_VGA;FRAMESIZE_SVGA; //
    // config.frame_size = FRAMESIZE_96X96;
    config.jpeg_quality = 36;//12;
    config.fb_count = 2;// 2 2

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    ESP_LOGI(TAG, "Camera init successful.!!!!!");

    sensor_t * s = esp_camera_sensor_get();
    //drop down frame size for higher initial frame rate
    s->set_brightness(s, 2);//up the blightness just a bit
    s->set_saturation(s, -1);//lower the saturation
    s->set_special_effect(s, 2);
    s->set_res_raw(s,0/* OV2640_MODE_UXGA*/, 0, 0, 0, 400, 200, 800, 800, 800, 800, 0, 0); // 参数必须为16的整数倍
    // s->set_res_raw(s,0/* OV2640_MODE_UXGA*/, 0, 0, 0, 208, 0, 1184, 1184, 1184, 1184, 0, 0); // 参数必须为16的整数倍
    // s->set_res_raw(s,0/* OV2640_MODE_UXGA*/, 0, 0, 0, 2, 202, 1196, 1196, 1196, 1196, 0, 0);
    //s->set_framesize(s, FRAMESIZE_96X96);
    //s->set_res_raw(s, 1, 0, 0, 0, 0, 0, 1632, 1220, 1632, 1220, 0, 0);


    // s->set_brightness(s, 2);     // -2 to 2
    // s->set_contrast(s, 0);       // -2 to 2
    // s->set_saturation(s, 0);     // -2 to 2
    // s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    // s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    // s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    // s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    // s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    // s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    // s->set_ae_level(s, 0);       // -2 to 2
    // s->set_aec_value(s, 300);    // 0 to 1200
    // s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    // s->set_agc_gain(s, 0);       // 0 to 30
    // s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    // s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    // s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    // s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    // s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    // s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    // s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    // s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    // s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

}

void Enable_camera_power(void)
{
    gpio_pad_select_gpio(GPIO_NUM_46);
    gpio_set_direction(GPIO_NUM_46, GPIO_MODE_OUTPUT); 
    gpio_set_level(GPIO_NUM_46,1);
}

void Disable_camera_power(void)
{
    gpio_pad_select_gpio(GPIO_NUM_46);
    gpio_set_direction(GPIO_NUM_46, GPIO_MODE_OUTPUT); 
    gpio_set_level(GPIO_NUM_46,0);
}

