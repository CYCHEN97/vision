#ifndef CAM_SIMPLE_CTRL_PROTOCOL_H
#define CAM_SIMPLE_CTRL_PROTOCOL_H

//#define MASTER_PC
#define ESP32_CAM

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* DataTable */
#ifdef ESP32_CAM
#include "sensor.h"
typedef enum  {
#endif

#ifdef MASTER_PC
typedef enum : unsigned char {
#endif
    ID__XCLK_FREQ_MHZ = 0,
    ID__PIXFORMAT,

    ID_DSP_FRAMESIZE,
    ID_DSP_QUALITY,
    ID_DSP_SDE,
    ID_DSP_WB_MODE,
    ID_DSP_BRIGHTNESS,
    ID_DSP_CONTRAST,
    ID_DSP_SATURATION,

    ID_DSP_EN_AWB,
    ID_DSP_EN_AWB_GAIN,
    ID_DSP_EN_AEC,
    ID_DSP_EN_BPC,
    ID_DSP_EN_WPC,
    ID_DSP_EN_RAW_GMA,
    ID_DSP_EN_LENC, /* lens corrector */
    ID_DSP_EN_DCW,

    ID_SENSOR_AEC_VALUE,
    ID_SENSOR_AE_LEVEL,
    ID_SENSOR_AGC_GAIN,
    ID_SENSOR_AGC_GAINCEILING,

    ID_SENSOR_EN_AEC,
    ID_SENSOR_EN_AGC,
    ID_SENSOR_EN_H_MIRROR,
    ID_SENSOR_EN_V_FLIP,
    ID_SENSOR_EN_COLOR_BAR,

    ID_IMG_BYTELEN,
    ID_IMG_WIDTH,
    ID_IMG_HEIGHT,
    ID_IMG_PIXFORMAT,
    ID_IMG_TIMESTAMP_SEC,
    ID_IMG_TIMESTAMP_USEC
} CAM_DATA_ID;

#define CAM_ID_DSP_START      ID_DSP_FRAMESIZE
#define CAM_ID_DSP_END        ID_DSP_EN_DCW
#define CAM_ID_DSP_LEN        (ID_DSP_EN_DCW - ID_DSP_FRAMESIZE + 1)

#define CAM_ID_SENSOR_START   ID_SENSOR_AEC_VALUE
#define CAM_ID_SENSOR_END     ID_SENSOR_EN_COLOR_BAR
#define CAM_ID_SENSOR_LEN     (ID_SENSOR_EN_COLOR_BAR - ID_SENSOR_AEC_VALUE + 2)

#ifdef MASTER_PC
typedef enum : unsigned char {
    TYPE_BOOL,
    TYPE_ENUM,
    TYPE_LONG,
    TYPE_UINT,
    TYPE_INT,
} CAM_DATA_TYPE;

typedef enum : unsigned char {
    ATTR_READWRITE,
    ATTR_ONLYREAD,
} CAM_DATA_ATTR;

typedef struct {
    const char*     namestr;
    const char**    opt_list;
    unsigned char   opt_list_len;
    CAM_DATA_ID     id;
    CAM_DATA_TYPE   type;
    CAM_DATA_ATTR   attr;
    unsigned char   bytelen_translate;
} CAM_DATAINFO;

//const CAM_STATUS_INFO* CAM_TABLE();
extern const CAM_DATAINFO CAM_TABLE[];
extern const int CAM_TABLE_LEN;

#endif /* MASTER_PC */

#ifdef ESP32_CAM

int esp32camscp_set_value(sensor_t* sensor, CAM_DATA_ID id, void* val_buf);
int esp32camscp_excute_cmd(sensor_t* sensor, const char* cmd_buf);

#endif /* ESP32_CAM */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //  CAM_SIMPLE_CTRL_PROTOCOL_H

