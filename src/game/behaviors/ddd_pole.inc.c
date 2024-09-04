#include "../../sm64ap.h"

void bhv_ddd_pole_init(void) {
    o->oHomeY = o->oPosY;
    o->hitboxDownOffset = 100.0f;
    o->oDDDPoleMaxOffset = 100.0f * o->oBehParams2ndByte;
}

void bhv_ddd_pole_update(void) {
    if (SM64AP_CheckedLoc(SM64AP_ID_KEY2) && SM64AP_CheckedLoc(SM64AP_LOCATIONID_BOARDBOWSERSSUB)) {
        if (o->oPosY != o->oHomeY) {
            o->oPosY = o->oHomeY;
        }
        if (o->oTimer > 20) {
            o->oDDDPoleOffset += o->oDDDPoleVel;

            if (clamp_f32(&o->oDDDPoleOffset, 0.0f, o->oDDDPoleMaxOffset)) {
                o->oDDDPoleVel = -o->oDDDPoleVel;
                o->oTimer = 0;
            }
        }

        obj_set_dist_from_home(o->oDDDPoleOffset);
    } else {
        o->oPosY = -10000.0f;
    }
}
