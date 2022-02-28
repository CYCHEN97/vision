#include "vision_compute.h"
#include "dl_lib_matrix3d.h"
#include "esp_camera.h"

#define GRAY888
static const uint8_t DOWN_TOLERANCE = 20;
static const uint8_t SPOT_RECOGNIZE_MIN_GRAY_DISTANCE = 50;
static dl_matrix3du_t* gray888 = NULL;

#ifdef GRAY888
#define IMGPTR_MOVE(p_start, p_index) ((p_start) += (p_index) * 3)
#define IMGPTR_GET(p_start, p_index) ((p_start) + (p_index) * 3)
#define IMGPTR_SET(ptr, index, val) {  \
unsigned char* new_ptr = (unsigned char*)(ptr) + (index) * 3; \
unsigned char new_val = (unsigned char)(val);\
*new_ptr = new_val; \
*(new_ptr + 1) = new_val; \
*(new_ptr + 2) = new_val; \
}
#else
#define IMGPTR_MOVE(p_start, p_index) ((p_start) += (p_index))
#define IMGPTR_GET(p_start, p_index) ((p_start) + (p_index))
#define IMGPTR_SET(ptr, index,  val) (*((uint8_t*)(ptr + index)) = ((uint8_t)(val)))
#endif

typedef unsigned char uchar;
#define false  (0)
#define true   (1)

static void _find_cursorily(Point16* maxpoint,
                            int16_t* graymax,
                            int16_t* grayavg,
                            int grayimg_w,
                            int grayimg_h,
                            unsigned char* grayimg_data);

static void _find_spot(Spot* spot,
                       Point16 ave_point,
                       int16_t ave_maxval,
                       int16_t down_tor,
                       int16_t grayimg_w,
                       int16_t grayimg_h,
                       unsigned char* grayimg_data);


void find_spot(Spot* spot, int grayimg_w, int grayimg_h, unsigned char* grayimg_data) {
    int16_t grayval_max;
    int16_t grayval_avg;
    Point16 point_grayval_max;
    _find_cursorily(&point_grayval_max, &grayval_max, &grayval_avg, grayimg_w, grayimg_h, grayimg_data);
    //printf("avg: %d, max: %d (%d, %d)\n", grayval_avg, grayval_max, point_grayval_max.x, point_grayval_max.y);
    if(grayval_max - grayval_avg < SPOT_RECOGNIZE_MIN_GRAY_DISTANCE) {
        spot->is_valid = false;
        return;
    }
    else {
        spot->is_valid = true;
    }

    _find_spot(spot, point_grayval_max, grayval_max, DOWN_TOLERANCE, grayimg_w, grayimg_h, grayimg_data);

    Point16  roi_left_top;
    Point16  roi_right_bottom;
    {
#define MAX_TEMP(x, y) (x > y ? x : y)
#define MIN_TEMP(x, y) (x < y ? x : y)
        int16_t min_x = MIN_TEMP(MIN_TEMP(spot->left1.x,    spot->left2.x),  spot->left3.x);
        int16_t min_y = MIN_TEMP(MIN_TEMP(spot->left1.y,    spot->top.y),    spot->right1.y);
        int16_t max_x = MAX_TEMP(MAX_TEMP(spot->right1.x,   spot->right2.x), spot->right3.x);
        int16_t max_y = MAX_TEMP(MAX_TEMP(spot->left3.y,    spot->bottom.y), spot->right3.y);
#undef MAX_TEMP
#undef MIN_TEMP
        roi_left_top.x = min_x - 5; if(roi_left_top.x < 0) roi_left_top.x = 0;
        roi_left_top.y = min_y - 5; if(roi_left_top.y < 0) roi_left_top.y = 0;
        roi_right_bottom.x = max_x + 5; if(roi_right_bottom.x >= grayimg_w) roi_right_bottom.x = grayimg_w - 1;
        roi_right_bottom.y = max_y + 5; if(roi_right_bottom.y >= grayimg_h) roi_right_bottom.y = grayimg_h - 1;
    }

    uint32_t x_count = 0;
    uint32_t y_count = 0;
    uint32_t p_count = 0;
    for(int y = roi_left_top.y; y <= roi_right_bottom.y; y += 1) {
        uchar* start = IMGPTR_GET(grayimg_data, y * grayimg_w);
        for(int x = roi_left_top.x; x <= roi_right_bottom.x; x += 1) {
            if((int16_t)*IMGPTR_GET(start, x) + (int16_t)DOWN_TOLERANCE >= grayval_max) {
                IMGPTR_SET(start, x, 255);
                x_count += x;
                y_count += y;
                p_count ++;
                continue;
             }
             else {
                IMGPTR_SET(start, x, 0);
             }
        }
    }

    spot->center.x = (float)x_count / (float)p_count;
    spot->center.y = (float)y_count / (float)p_count;
}

static void _find_cursorily(Point16* maxpoint, int16_t* graymax, int16_t* grayavg, int grayimg_w, int grayimg_h, unsigned char* grayimg_data) {
    *graymax = 0;
    *grayavg = 0;
    maxpoint->x = 0;
    maxpoint->y = 0;

    // find max 16 value
    static const int MAX_NUM = 4 * 4 + 16;
    Point16 max_point[MAX_NUM];
    uchar max_val[MAX_NUM];
    for(int i = 0; i < MAX_NUM; ++i)
        max_val[i] = 0;

    // 为了自适应， 寻找当前灰度图中的最大值
    {
        static int START_H = 4;
        static int START_W = 4;
        static int FLITER_SIZE = 4;
        static int STEP = 2;
        int32_t count = 0;
        int32_t grayval_total = 0;
        for(int h = START_H; h < grayimg_h; h += STEP) {
            int w = START_W;
            uchar* start = IMGPTR_GET(grayimg_data, h * grayimg_w + w);
            for(; w < grayimg_w; w += STEP) {

                // 图片没有滤波，过滤噪点
                if(
                   *start > *max_val &&
                   *IMGPTR_GET(start, FLITER_SIZE) > *max_val &&
                   *IMGPTR_GET(start, -FLITER_SIZE) > *max_val &&
                   *IMGPTR_GET(start, FLITER_SIZE * grayimg_w) > *max_val &&
                   *IMGPTR_GET(start, -FLITER_SIZE * grayimg_w) > *max_val
                ) {
                    for(int i = MAX_NUM - 1; i >= 0; --i) {
                        if(*start > max_val[i]) {
                            for(int j = 0; j < i; ++j) {
                                max_val[j] =  max_val[j + 1];
                            }
                            max_val[i] = *start;
                            max_point[i].x = w;
                            max_point[i].y = h;
                        }
                    }
                }
                else {
                    count++;
                    grayval_total += *start;
                }
                IMGPTR_MOVE(start, STEP);
            }
        }

        // 四舍五入求整个ROI区域的均值
        *grayavg = (int32_t)(((float)grayval_total / (float)count) + 0.5f);
    }


    // 去掉8个极大值和8个极小值，计算均值
    for(int i = 8; i < MAX_NUM - 8; ++i) {
        maxpoint->x += max_point[i].x;
        maxpoint->y += max_point[i].y;
        *graymax += max_val[i];
    }
    maxpoint->x = (int16_t)(((float)(maxpoint->x) / (float)(MAX_NUM - 16)) + 0.5f);
    maxpoint->y = (int16_t)(((float)(maxpoint->y) / (float)(MAX_NUM - 16)) + 0.5f);
    *graymax = (int32_t)(((float)*graymax / (float)(MAX_NUM - 16)) + 0.5f);
}


static void _find_spot(Spot* spot,
                       Point16 ave_point,
                       int16_t ave_maxval,
                       int16_t down_tor,
                       int16_t grayimg_w,
                       int16_t grayimg_h,
                       unsigned char* grayimg_data) {
    uint8_t flag = 0xff;
    int16_t b13 = ave_point.y - ave_point.x;
    int16_t b31 = ave_point.y + ave_point.x;
    int16_t i = 2;
    int16_t x, y;
    uchar* start;
    while(flag) {
        y = ave_point.y - i; start = IMGPTR_GET(grayimg_data , y * grayimg_w);
        if(y < 0) { flag &= (~(0x01 | 0x02 | 0x04)); }

        if(flag & 0x01) {
            x = y - b13;
            if(x >= 0 && (int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->left1.x = x;
                spot->left1.y = y;
            }
            else { flag &= (~0x01); }
        }
        if(flag & 0x02) {
            x = ave_point.x;
            if((int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->top.x = x;
                spot->top.y = y;
            }
            else { flag &= (~0x02); }
        }
        if(flag & 0x04) {
            x = b31 - y;
            if(x < grayimg_w && (int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->right1.x = x;
                spot->right1.y = y;
            }
            else { flag &= (~0x04); }
        }

        y = ave_point.y + i; start = IMGPTR_GET(grayimg_data, y * grayimg_w);
        if(y >= grayimg_h) { flag &= (~(0x20 | 0x40 | 0x80)); }
        if(flag & 0x20) {
            x = b31 - y;
            if(x >= 0 && (int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->left3.x = x;
                spot->left3.y = y;
            }
            else { flag &= (~0x20); }
        }
        if(flag & 0x40) {
            x = ave_point.x;
            if((int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->bottom.x = x;
                spot->bottom.y = y;
            }
            else { flag &= (~0x40); }
        }
        if(flag & 0x80) {
            x = y - b13;
            if(x < grayimg_w && (uint16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->right3.x = x;
                spot->right3.y = y;
            }
            else { flag &= (~0x80); }
        }

        y = ave_point.y; start = IMGPTR_GET(grayimg_data, y * grayimg_w);
        x = ave_point.x + i;
        if(flag & 0x10) {
            if(x < grayimg_w && (int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->right2.x = x;
                spot->right2.y = y;
            }
            else { flag &= (~0x10); }
        }
        x = ave_point.x - i;
        if(flag & 0x08) {
            if(x >= 0 && (int16_t)*IMGPTR_GET(start, x) + down_tor > ave_maxval) {
                spot->left2.x = x;
                spot->left2.y = y;
            }
            else { flag &= (~0x08); }
        }

        i+=2;
    }
}

int get_offset_spot2center(unsigned char *jpeg_buf, int jpeg_buf_len, Point2f *offset) {
	if(NULL == jpeg_buf || jpeg_buf_len <= 0) { return -1; }
	if(NULL == offset) { return -2; }

    Spot spot;
	int square_width = 256;
	float pixel_per_mm = square_width / 122.5f;

	// machine 1
	//float center_x = square_width / 2;
	//float center_y = 20 + 248 / 2;

	// machine 2
	float center_x = square_width / 2 + 10;
	float center_y = 20 + 248 / 2 + 26;

	if(NULL == gray888) {
		gray888 = dl_matrix3du_alloc(1, square_width, square_width, 3);
		if(NULL == gray888) {
			return -5;
		}
	}
	if(false == fmt2rgb888(jpeg_buf, jpeg_buf_len, PIXFORMAT_JPEG, gray888->item)) {
		return -6;
	}
	find_spot(&spot, gray888->w, gray888->h, gray888->item);
	if(spot.is_valid) {
		offset->x = (spot.center.x - center_x) / pixel_per_mm;
		offset->y = (spot.center.y - center_y) / pixel_per_mm;
		return 0;
	}
	else {
		return -7;
	}
}

#include <stdio.h>
int get_relative_distance(unsigned char *jpeg_buf, int jpeg_buf_len, float *distance) {
	if(NULL == jpeg_buf || jpeg_buf_len <= 0) { return -1; }
	if(NULL == distance) { return -2; }

    Spot spot;
	int square_width = 256;
	float y_185 = 126.5f;
	float pixel_per_mm = 1.1f;

	if(NULL == gray888) {
		gray888 = dl_matrix3du_alloc(1, square_width, square_width, 3);
		if(NULL == gray888) {
			return -5;
		}
	}
	if(false == fmt2rgb888(jpeg_buf, jpeg_buf_len, PIXFORMAT_JPEG, gray888->item)) {
		return -6;
	}
	find_spot(&spot, gray888->w, gray888->h, gray888->item);
	if(spot.is_valid) {
		*distance = (spot.center.y - y_185) / pixel_per_mm;
		printf("%.2f %.2f %.2f %.2f\n", *distance, spot.center.y, y_185, pixel_per_mm);
		return 0;
	}
	else {
		return -7;
	}
}

