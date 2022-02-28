#ifndef ESP32_VISION_COMPUTE_H
#define ESP32_VISION_COMPUTE_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Point16 {
    int16_t x;
    int16_t y;
} Point16;

typedef struct Point2f {
    float x;
    float y;
} Point2f;

typedef struct Point2d {
    double x;
    double y;
} Point2d;

typedef struct Spot {
    unsigned char is_valid;
    Point2f center;
    Point16 left1 /* 0x01 */,   top /* 0x02 */,     right1 /*0x04*/;
    Point16 left2 /* 0x08 */,                       right2 /*0x10*/;
    Point16 left3 /* 0x20 */,   bottom /* 0x40 */,  right3 /*0x80*/;
} Spot;


void find_spot(Spot* ret, int grayimg_w, int grayimg_h, unsigned char* grayimg_data);

int get_offset_spot2center(unsigned char *jpeg_buf, int jpeg_buf_len, Point2f *offset);

int get_relative_distance(unsigned char *jpeg_buf, int jpeg_buf_len, float *distance);
#ifdef __cplusplus
}
#endif

#endif // ESP32_VISION_COMPUTE_H
