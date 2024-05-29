// moat_drainer.c.inc
#include "sm64ap.h"

void bhv_invisible_objects_under_bridge_init(void) {
    if ((save_file_get_flags() & SAVE_FLAG_MOAT_DRAINED) && SM64AP_CanDrainMoat()) {
        gEnvironmentRegions[6] = -800;
        gEnvironmentRegions[12] = -800;
    }
}
