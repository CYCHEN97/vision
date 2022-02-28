#ifndef APP_CAL_OFFSET_H
#define APP_CAL_OFFSET_H
#include "vision_compute.h"


bool cal_offset_init(void);
bool cal_centr_offset(void);

extern Point2f offset;

#endif // APP_CAL_OFFSET_H
