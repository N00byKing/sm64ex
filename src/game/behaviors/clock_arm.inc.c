/**
 * Main loop of the hour and minute hands of the Tick Tock Clock painting.
 */

#include "sm64ap.h"

void bhv_rotating_clock_arm_loop(void) {
    int entrance = SM64AP_EntranceToTTC();
    int course = entrance / 10;
    if ((o->oBehParams <= LEVEL_UNKNOWN_38 && o->oBehParams != course) || // This entrance doesn't lead to TTC. Remove.
        (o->oBehParams > LEVEL_UNKNOWN_38 && o->oBehParams != entrance) || // Special handling is done for THI, which has two entrances. If param is greater than max level id, check directly against entrance
        (o->oBehParams == LEVEL_VCUTM && !SM64AP_MoatDrained())) { // If the moat isn't drained the clock hands stick out of the water. Remove them.
        obj_mark_for_deletion(o);
        return;
    } else if (cur_obj_has_behavior(bhvClockMinuteHand) && o->oAction < 2) {
        // This is the correct clock hand. Set the clock action
        SM64AP_SetClockToTTCAction(&(o->oAction));
    }
    
    struct Surface *marioSurface;
    u16 rollAngle = o->oFaceAngleRoll;
    o->oFloorHeight =
        find_floor(gMarioObject->oPosX, gMarioObject->oPosY, gMarioObject->oPosZ, &marioSurface);

    // On first frame, set entrance-specific settings
    if (o->oTimer == 0) {
        switch (o->oBehParams) {
            case LEVEL_BBH:
            case LEVEL_RR:
            case LEVEL_WMOTR:
                cur_obj_scale(1.1f);
                break;
            case LEVEL_CCM:
                cur_obj_scale(1.4f);
                break;
            case LEVEL_HMC:
            case LEVEL_COTMC:
                cur_obj_scale(2.0f);
                o->oFaceAnglePitch = 0xBFA9;
                break;
            case LEVEL_SSL:
            case LEVEL_LLL:
                cur_obj_scale(1.6f);
                break;
            case LEVEL_BOB:
            case LEVEL_JRB:
            case LEVEL_WF:
                cur_obj_scale(1.5f);
                break;
            case LEVEL_SL:
                cur_obj_scale(1.2f);
                break;
            case LEVEL_WDW:
                cur_obj_scale(1.55f);
                break;
            case LEVEL_THI * 10 + 1: // THI Huge
                cur_obj_scale(4.5f);
                break;
            case LEVEL_BITDW:
                cur_obj_scale(1.7f);
                break;
            case LEVEL_VCUTM:
                cur_obj_scale(1.5f);
                break;
            case LEVEL_BITFS:
                cur_obj_scale(0.3f);
                o->oFaceAnglePitch = -0x1300;
                break;
            case LEVEL_SA:
            case LEVEL_PSS:
                cur_obj_scale(0.42f);
                break;
            case LEVEL_DDD:
                cur_obj_scale(2.0f);
                break;
            case LEVEL_TOTWC:
                cur_obj_scale(2.2f);
                o->oFaceAnglePitch = 0x3FAA;
                break;
            case LEVEL_TTM:
                cur_obj_scale(0.65f);
                break;
        }
    }
    // Move back arrows when DDD painting is moved back
    // The painting can be moved at any time due to collect or same-slot multiplayer, so always check this
    if (o->oBehParams == LEVEL_DDD && SM64AP_CheckedLoc(SM64AP_LOCATIONID_BOARDBOWSERSSUB)) {
        o->oPosX = 5529.0f;
    }
    // Seems to make sure Mario is on a default surface & 4 frames pass before
    //   allowing him to change the Tick Tock Clock speed setting.
    // Probably a safety check for when you leave the level through the painting
    //   to make sure the setting isn't accidentally locked in as you fly out.
    if (o->oAction == 0) {
        if (marioSurface->type == SURFACE_DEFAULT)
            if (o->oTimer >= 4)
                o->oAction++;
    } else if (o->oAction == 5) {
        // If Mario is touching the Tick Tock Clock painting...
        if (1) {
            // And this is the minute hand...
            if (cur_obj_has_behavior(bhvClockMinuteHand)) {
                // Set Tick Tick Clock's speed based on the angle of the hand.
                // The angle actually counting down from 0xFFFF to 0 so
                //   11 o'clock is a small value and 1 o'clock is a large value.
                if (rollAngle < 0xAAA) // > 345 degrees from 12 o'clock.
                    gTTCSpeedSetting = TTC_SPEED_STOPPED;
                else if (rollAngle < 0x6aa4) // 210..345 degrees from 12 o'clock.
                    gTTCSpeedSetting = TTC_SPEED_FAST;
                else if (rollAngle < 0x954C) // 150..210 degrees from 12 o'clock.
                    gTTCSpeedSetting = TTC_SPEED_RANDOM;
                else if (rollAngle < 0xf546) // 15..150 degrees from 12 o'clock.
                    gTTCSpeedSetting = TTC_SPEED_SLOW;
                else // < 15 degrees from 12 o'clock.
                    gTTCSpeedSetting = TTC_SPEED_STOPPED;
            }

            // Increment the action to stop animating the hands.
            o->oAction++;
        } else {
        }
    }

    // Only rotate the hands until Mario enters the painting. Don't rotate the hour hand except in TTC
    if (o->oAction < 2 && (o->oBehParams == LEVEL_TTC || cur_obj_has_behavior(bhvClockMinuteHand)))
        cur_obj_rotate_face_angle_using_vel();
}
