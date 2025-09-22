#pragma once

#include <lvgl.h>
#include <UI/components/general.h>

#define MAX_SPINBOX_SPEED 18000
#define MIN_SPINBOX_SPEED 0
#define MAX_SPINBOX_WEIGHT 5000

void build_settingsTab(lv_obj_t *parent);