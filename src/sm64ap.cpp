#include "sm64ap.h"
#include "Archipelago.h"

extern "C" {
    #include "game/print.h"
    #include "gfx_dimensions.h"
    #include "level_table.h"
    #include "game/level_update.h"
}

#include <deque>
#include <string>
#include <vector>
#include <map>

int starsCollected = 0;
bool sm64_locations[SM64AP_NUM_LOCS];
bool sm64_have_key1 = false;
bool sm64_have_key2 = false;
bool sm64_have_wingcap = false;
bool sm64_have_metalcap = false;
bool sm64_have_vanishcap = false;
bool sm64_have_cannon[15];
int* sm64_clockaction = nullptr;
int sm64_cost_firstbowserdoor = 8;
int sm64_cost_basementdoor = 30;
int sm64_cost_secondfloordoor = 50;
int sm64_cost_endlessstairs = 70;
int msg_frame_duration = 90; // 3 Secounds at 30F/s
int cur_msg_frame_duration = msg_frame_duration;

std::map<int,int> map_entrances;
std::map<int,int> map_exits;
std::map<int,int> map_courseidx_coursenum;
std::map<int,int> map_coursenum_courseidx;
std::map<int,int> map_warpnodeid_entranceid;

int sm64_exit_return_to;
int sm64_exit_orig_entrancelvl;

void SM64AP_RecvItem(int idx, bool notify) {
    switch (idx) {
        case SM64AP_ITEMID_STAR:
            starsCollected++;
            break;
        case SM64AP_ID_KEY1:
            sm64_have_key1 = true;
            break;
        case SM64AP_ID_KEY2:
            sm64_have_key2 = true;
            break;
        case SM64AP_ID_KEYPROG:
            sm64_have_key2 = sm64_have_key1;
            sm64_have_key1 = true;
            break;
        case SM64AP_ID_WINGCAP:
            sm64_have_wingcap = true;
            break;
        case SM64AP_ID_METALCAP:
            sm64_have_metalcap = true;
            break;
        case SM64AP_ID_VANISHCAP:
            sm64_have_vanishcap = true;
            break;
        case SM64AP_ITEMID_1UP:
            gMarioState->numLives++;
            break;
        case SM64AP_ID_CANNONUNLOCK(0) ... SM64AP_ID_CANNONUNLOCK(15-1):
            sm64_have_cannon[idx-SM64AP_ID_OFFSET-200] = true;
            break;
    }
}

void SM64AP_CheckLocation(int loc_id) {
    sm64_locations[loc_id - SM64AP_ID_OFFSET] = true;
}

u32 SM64AP_CourseStarFlags(s32 courseIdx) {
    u32 starflags = 0;
    for (int i = 0; i < 7; i++) {
        if (sm64_locations[i + (courseIdx*7)]) {
            starflags |= (1 << i);
        }
    }
    return starflags;
}

void setCourseNodeAndArea(int coursenum, s16* oldnode, bool isDeathWarp) {
    switch (coursenum) {
        case LEVEL_BOB:
            *oldnode = isDeathWarp ? 0x64 : 0x32;
            return;
        case LEVEL_CCM:
            *oldnode = isDeathWarp ? 0x65 : 0x33;
            return;
        case LEVEL_WF:
            *oldnode = isDeathWarp ? 0x66 : 0x34;
            return;
        case LEVEL_JRB:
            *oldnode = isDeathWarp ? 0x67 : 0x35;
            return;
        case LEVEL_BBH:
            *oldnode = isDeathWarp ? 0x0B : 0x0A;
            return;
        case LEVEL_LLL:
            *oldnode = isDeathWarp ? 0x64 : 0x32;
            return;
        case LEVEL_SSL:
            *oldnode = isDeathWarp ? 0x65 : 0x33;
            return;
        case LEVEL_HMC:
            *oldnode = isDeathWarp ? 0x66 : 0x34;
            return;
        case LEVEL_DDD:
            *oldnode = isDeathWarp ? 0x67 : 0x35;
            return;
        case LEVEL_WDW:
            *oldnode = isDeathWarp ? 0x64 : 0x32;
            return;
        case LEVEL_THI:
            *oldnode = isDeathWarp ? 0x65 : 0x33;
            return;
        case LEVEL_TTM:
            *oldnode = isDeathWarp ? 0x66 : 0x34;
            return;
        case LEVEL_TTC:
            *oldnode = isDeathWarp ? 0x67 : 0x35;
            return;
        case LEVEL_SL:
            *oldnode = isDeathWarp ? 0x68 : 0x36;
            return;
        case LEVEL_RR:
            *oldnode = isDeathWarp ? 0x6C : 0x3A;
            return;
        case LEVEL_PSS:
        case LEVEL_TOTWC:
            *oldnode = isDeathWarp ? 0x21 : 0x20;
            return;
        case LEVEL_SA:
            *oldnode = isDeathWarp ? 0x28 : 0x27;
            return;
        case LEVEL_BITDW:
            *oldnode = isDeathWarp ? 0x25 : 0x24;
            return;
        case LEVEL_VCUTM:
            *oldnode = isDeathWarp ? 0x06 : 0x07;
            return;
        case LEVEL_BITFS:
            *oldnode = isDeathWarp ? 0x68 : 0x36;
            return;
        case LEVEL_WMOTR:
            *oldnode = isDeathWarp ? 0x6D : 0x38;
        default:
            return;
    }
}

void SM64AP_RedirectWarp(s16* curLevel, s16* destLevel, s8* curArea, s16* destArea, s16* destWarpNode, int warpNode_id, bool isDeathWarp) {
    if ((*curLevel == LEVEL_CASTLE || *curLevel == LEVEL_CASTLE_COURTYARD || *curLevel == LEVEL_CASTLE_GROUNDS || *curLevel == LEVEL_HMC) && 
         *destLevel != LEVEL_CASTLE && *destLevel != LEVEL_CASTLE_COURTYARD && *destLevel != LEVEL_CASTLE_GROUNDS) {
        if (sm64_clockaction) *sm64_clockaction = 5;
        int destination;
        switch (*destLevel) {
            case LEVEL_COTMC:
                destination = map_entrances[281];
                break;
            default:
                if (*curLevel == LEVEL_HMC) return; // Safety Check: If in HMC only relevant warp is to COTMC
                destination = map_entrances[*destLevel * 10 + *destArea];
                break;
        }
        if (*curLevel != LEVEL_HMC) { // HMC -> COTMC transition should not set new return point
            sm64_exit_return_to = *curLevel * 10 + *curArea;
            sm64_exit_orig_entrancelvl = *destLevel;
        }
        *destLevel = destination/10; // Cuts off Area Info
        *destArea = destination % 10; // Cuts off Level Info
        *destWarpNode = 0x0A;
        return;
    }
    
    if ((*destLevel == LEVEL_CASTLE || *destLevel == LEVEL_CASTLE_COURTYARD || *destLevel == LEVEL_CASTLE_GROUNDS) && map_coursenum_courseidx.count(*curLevel)) {
        if (*destLevel == LEVEL_CASTLE && (*destWarpNode == 0x1F || *destWarpNode == 0x00)) return; //Exit Course, Inter-Case warp
        *destLevel = sm64_exit_return_to / 10;
        *destArea = sm64_exit_return_to % 10;
        setCourseNodeAndArea(sm64_exit_orig_entrancelvl, destWarpNode, isDeathWarp);
        return;
    }
}

int SM64AP_CourseToTTC() {
    int courseidx = 0;
    for (auto itr : map_entrances) {
        if (itr.second/10 == LEVEL_TTC) {
            courseidx = map_coursenum_courseidx[itr.first/10];
            break;
        }
    }
    return courseidx;
}

void SM64AP_SetClockToTTCAction(int* action) {
    sm64_clockaction = action;
}

void SM64AP_SetFirstBowserDoorCost(int amount) {
    sm64_cost_firstbowserdoor = amount;
}

void SM64AP_SetBasementDoorCost(int amount) {
    sm64_cost_basementdoor = amount;
}

void SM64AP_SetSecondFloorDoorCost(int amount) {
    sm64_cost_secondfloordoor = amount;
}

void SM64AP_SetStarsToFinish(int amount) {
    sm64_cost_endlessstairs = amount;
}

void SM64AP_SetCourseMap(std::map<int,int> map) {
    map_entrances = map;
    for (auto itr : map_entrances) {
        map_exits[itr.second] = itr.first;
    }
}

void SM64AP_ResetItems() {
    for (int i = 0; i < SM64AP_NUM_LOCS; i++) {
        sm64_locations[i] = false;
    }
    for (int i = 0; i < 15; i++) {
        sm64_have_cannon[i] = false;
    }
    sm64_have_key1 = false;
    sm64_have_key2 = false;
    sm64_have_wingcap = false;
    sm64_have_metalcap = false;
    sm64_have_vanishcap = false;
    starsCollected = 0;
}

void SM64AP_GenericInit() {
    AP_NetworkVersion version = {0,3,0};
    AP_SetClientVersion(&version);
    AP_SetDeathLinkSupported(true);
    AP_SetItemClearCallback(&SM64AP_ResetItems);
    AP_SetLocationCheckedCallback(&SM64AP_CheckLocation);
    AP_SetItemRecvCallback(&SM64AP_RecvItem);
    AP_RegisterSlotDataIntCallback("FirstBowserDoorCost", &SM64AP_SetFirstBowserDoorCost);
    AP_RegisterSlotDataIntCallback("BasementDoorCost", &SM64AP_SetBasementDoorCost);
    AP_RegisterSlotDataIntCallback("SecondFloorDoorCost", &SM64AP_SetSecondFloorDoorCost);
    AP_RegisterSlotDataIntCallback("StarsToFinish", &SM64AP_SetStarsToFinish);
    AP_RegisterSlotDataMapIntIntCallback("AreaRando", &SM64AP_SetCourseMap);

    map_courseidx_coursenum[0] = LEVEL_BOB;
    map_courseidx_coursenum[1] = LEVEL_WF;
    map_courseidx_coursenum[2] = LEVEL_JRB;
    map_courseidx_coursenum[3] = LEVEL_CCM;
    map_courseidx_coursenum[4] = LEVEL_BBH;
    map_courseidx_coursenum[5] = LEVEL_HMC;
    map_courseidx_coursenum[6] = LEVEL_LLL;
    map_courseidx_coursenum[7] = LEVEL_SSL;
    map_courseidx_coursenum[8] = LEVEL_DDD;
    map_courseidx_coursenum[9] = LEVEL_SL;
    map_courseidx_coursenum[10] = LEVEL_WDW;
    map_courseidx_coursenum[11] = LEVEL_TTM;
    map_courseidx_coursenum[12] = LEVEL_THI;
    map_courseidx_coursenum[13] = LEVEL_TTC;
    map_courseidx_coursenum[14] = LEVEL_RR;
    map_courseidx_coursenum[15] = LEVEL_PSS;
    map_courseidx_coursenum[16] = LEVEL_SA;
    map_courseidx_coursenum[17] = LEVEL_BITDW;
    map_courseidx_coursenum[18] = LEVEL_TOTWC;
    map_courseidx_coursenum[19] = LEVEL_COTMC;
    map_courseidx_coursenum[20] = LEVEL_VCUTM;
    map_courseidx_coursenum[21] = LEVEL_BITFS;
    map_courseidx_coursenum[22] = LEVEL_WMOTR;
    for (auto itr : map_courseidx_coursenum) {
        map_coursenum_courseidx[itr.second] = itr.first;
    }
    map_coursenum_courseidx[LEVEL_COTMC] = 5; //Map COTMC to HMC

    // NOT INCLUDED: RR, HMC due to same warp node ID (Magic Num 0x2A)
    map_warpnodeid_entranceid[0x00] = 0; // BOB
    map_warpnodeid_entranceid[0x01] = 0;
    map_warpnodeid_entranceid[0x02] = 0;
    map_warpnodeid_entranceid[0x03] = 3; // CCM
    map_warpnodeid_entranceid[0x04] = 3;
    map_warpnodeid_entranceid[0x05] = 3;
    map_warpnodeid_entranceid[0x06] = 1; // WF
    map_warpnodeid_entranceid[0x07] = 1;
    map_warpnodeid_entranceid[0x08] = 1;
    map_warpnodeid_entranceid[0x09] = 2; // JRB
    map_warpnodeid_entranceid[0x0A] = 2;
    map_warpnodeid_entranceid[0x0B] = 2;
    map_warpnodeid_entranceid[0x0C] = 6; // LLL
    map_warpnodeid_entranceid[0x0D] = 6;
    map_warpnodeid_entranceid[0x0E] = 6;
    map_warpnodeid_entranceid[0x0F] = 7; // SSL
    map_warpnodeid_entranceid[0x10] = 7;
    map_warpnodeid_entranceid[0x11] = 7;
    map_warpnodeid_entranceid[0x12] = 5; // HMC
    map_warpnodeid_entranceid[0x15] = 8; // DDD
    map_warpnodeid_entranceid[0x16] = 8;
    map_warpnodeid_entranceid[0x17] = 8;
    map_warpnodeid_entranceid[0x18] = 10; // WDW
    map_warpnodeid_entranceid[0x19] = 10;
    map_warpnodeid_entranceid[0x1A] = 10;
    map_warpnodeid_entranceid[0x1B] = 12; // THI Huge
    map_warpnodeid_entranceid[0x1C] = 12;
    map_warpnodeid_entranceid[0x1D] = 12;
    map_warpnodeid_entranceid[0x1E] = 11; // TTM
    map_warpnodeid_entranceid[0x1F] = 11;
    map_warpnodeid_entranceid[0x20] = 11;
    map_warpnodeid_entranceid[0x21] = 14; // TTC
    map_warpnodeid_entranceid[0x22] = 14;
    map_warpnodeid_entranceid[0x23] = 14;
    map_warpnodeid_entranceid[0x24] = 9; // SL
    map_warpnodeid_entranceid[0x25] = 9;
    map_warpnodeid_entranceid[0x26] = 9;
    map_warpnodeid_entranceid[0x27] = 13; // THI Tiny
    map_warpnodeid_entranceid[0x28] = 13;
    map_warpnodeid_entranceid[0x29] = 13;
}

void SM64AP_InitMW(const char* ip, const char* player_name, const char* passwd) {
    AP_Init(ip, "Super Mario 64", player_name, passwd);
    SM64AP_GenericInit();
    AP_Start();
}

void SM64AP_InitSP(const char * filename) {
    AP_Init(filename);
    SM64AP_GenericInit();
    AP_Start();
}

void SM64AP_SendItem(int idxNoOffset) {
    AP_SendItem(idxNoOffset + SM64AP_ID_OFFSET);
}

void SM64AP_StoryComplete() {
    AP_StoryComplete();
}

int SM64AP_GetStars() {
    return starsCollected;
}

int SM64AP_GetRequiredStars(int prevAmount) {
    switch (prevAmount) {
        case 8:
            return sm64_cost_firstbowserdoor;
        case 30:
            return sm64_cost_basementdoor;
        case 50:
            return sm64_cost_secondfloordoor;
        case 70:
            return sm64_cost_endlessstairs;
        default:
            return prevAmount;
    }
}

bool SM64AP_CheckedLoc(int x) {
    return sm64_locations[x - SM64AP_ID_OFFSET];
}

bool SM64AP_HaveKey1() {
    return sm64_have_key1;
}

bool SM64AP_HaveKey2() {
    return sm64_have_key2;
}

bool SM64AP_HaveCap(int flag) {
    switch (flag) {
        case 2:
            return sm64_have_wingcap;
            break;
        case 4:
            return sm64_have_metalcap;
            break;
        case 8:
            return sm64_have_vanishcap;
            break;
        default:
            //Probably coin/1up or something
            return true;
    }
}

bool SM64AP_HaveCannon(int courseIdx) {
    if (courseIdx < 15) return sm64_have_cannon[courseIdx];
    return true;
}

bool SM64AP_DeathLinkPending() {
    return AP_DeathLinkPending();
}

void SM64AP_DeathLinkClear() {
    AP_DeathLinkClear();
}

void SM64AP_DeathLinkSend() {
    if (!SM64AP_DeathLinkPending()) {
        return AP_DeathLinkSend();
    } else {
        SM64AP_DeathLinkClear();
    }
}

void SM64AP_PrintNext() {
    if (!AP_IsMessagePending()) return;
    std::vector<std::string> msg = AP_GetLatestMessage();
    for (int i = 0; i < msg.size(); i++) {
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0), (msg.size()-i)*20, msg.at(i).c_str());
    }
    if (cur_msg_frame_duration > 0) {
        cur_msg_frame_duration--;
    } else {
        AP_ClearLatestMessage();
        cur_msg_frame_duration = msg_frame_duration;
    }
}