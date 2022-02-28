#include "esp32_cam_scp.h"

#ifdef ESP32_CAM
#include "esp_camera.h"
#include "esp_log.h"
#endif // ESP32_CAM


#ifdef ESP32_CAM
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _LOG
static const char* TAG = "SCP";
#endif

int esp32camscp_set_value(sensor_t* sensor, CAM_DATA_ID id, void* val_buf) {
#define CAST_TO(__type__) (*((__type__ * )(val_buf)))
#define CASE(__id__, __function__, __type__) \
    case __id__: ret = sensor->__function__(sensor, *((__type__ * )(val_buf))); break

    int ret;
    switch (id) {
        case ID__XCLK_FREQ_MHZ: ret = sensor->set_xclk(sensor, LEDC_TIMER_0, CAST_TO(int)); break;
        CASE(ID__PIXFORMAT, set_pixformat, pixformat_t);
        case ID_DSP_FRAMESIZE: ret = sensor->set_framesize(sensor, *((uint8_t*)(val_buf))); break;
        //CASE(ID_DSP_FRAMESIZE, set_framesize, framesize_t);
        CASE(ID_DSP_QUALITY, set_quality, uint8_t);
        CASE(ID_DSP_SDE, set_special_effect, uint8_t);
        CASE(ID_DSP_WB_MODE, set_wb_mode, uint8_t);
        CASE(ID_DSP_BRIGHTNESS, set_brightness, int8_t);
        CASE(ID_DSP_CONTRAST, set_contrast, int8_t);
        CASE(ID_DSP_SATURATION, set_saturation, int8_t);
        CASE(ID_DSP_EN_AWB, set_whitebal, uint8_t);
        CASE(ID_DSP_EN_AWB_GAIN, set_awb_gain, uint8_t);
        CASE(ID_DSP_EN_AEC, set_aec2, uint8_t);
        CASE(ID_DSP_EN_BPC, set_bpc, uint8_t);
        CASE(ID_DSP_EN_WPC, set_wpc, uint8_t);
        CASE(ID_DSP_EN_RAW_GMA, set_raw_gma, uint8_t);
        CASE(ID_DSP_EN_LENC, set_lenc, uint8_t);
        CASE(ID_DSP_EN_DCW, set_dcw, uint8_t);
        CASE(ID_SENSOR_AEC_VALUE, set_aec_value, uint16_t);
        CASE(ID_SENSOR_AE_LEVEL, set_ae_level, int8_t);
        CASE(ID_SENSOR_AGC_GAIN, set_agc_gain, uint8_t);
        CASE(ID_SENSOR_AGC_GAINCEILING, set_gainceiling, gainceiling_t);
        CASE(ID_SENSOR_EN_AEC, set_exposure_ctrl, uint8_t);
        CASE(ID_SENSOR_EN_AGC, set_gain_ctrl, uint8_t);
        CASE(ID_SENSOR_EN_H_MIRROR, set_hmirror, uint8_t);
        CASE(ID_SENSOR_EN_V_FLIP, set_vflip, uint8_t);
        CASE(ID_SENSOR_EN_COLOR_BAR, set_colorbar, uint8_t);
        default: ret = -1;
    }

    return ret;
#undef CAST_TO
#undef CASE
}

int esp32camscp_excute_cmd(sensor_t* sensor, const char* cmd_buf) {
    if(!cmd_buf && !sensor)
        return false;

    int ret = 0;
    unsigned char index = 0;
    unsigned char count = cmd_buf[index++];

    for(int i = 0; i < count; ++i) {
        CAM_DATA_ID data_id = (CAM_DATA_ID)cmd_buf[index++];
        unsigned char data_len = cmd_buf[index++];
        ret += esp32camscp_set_value(sensor, data_id, (void*)(cmd_buf + index));
        index += data_len;
    }
    return ret;
}

#ifdef __cplusplus
}
#endif
#endif /* ESP32_CAM */

