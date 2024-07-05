// moat_drainer.c.inc
#include "sm64ap.h"

void bhv_invisible_objects_under_bridge_init(void) {
    if (SM64AP_MoatDrained()) {
        gEnvironmentRegions[6] = -800;
        gEnvironmentRegions[12] = -800;
    }
}
