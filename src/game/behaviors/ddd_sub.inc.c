// ddd_sub.c.inc
#include "../../sm64ap.h"
void bhv_bowsers_sub_loop(void) {
    if (SM64AP_CheckedLoc(SM64AP_ID_KEY2) && SM64AP_CheckedLoc(SM64AP_LOCATIONID_BOARDBOWSERSSUB))
        obj_mark_for_deletion(o);
}
