// moat_grill.c.inc
#include "sm64ap.h"

void bhv_moat_grills_loop(void) {
    if ((save_file_get_flags() & SAVE_FLAG_MOAT_DRAINED) && SM64AP_CanDrainMoat())
        cur_obj_set_model(MODEL_NONE);
    else
        load_object_collision_model();
}
