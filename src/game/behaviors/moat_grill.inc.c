// moat_grill.c.inc
#include "sm64ap.h"

void bhv_moat_grills_loop(void) {
    if (SM64AP_MoatDrained())
        cur_obj_set_model(MODEL_NONE);
    else
        load_object_collision_model();
}
