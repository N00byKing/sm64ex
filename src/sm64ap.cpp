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
std::map<int,int> map_paintingidx_coursenum;
std::map<int,int> map_coursenum_paintingidx;
std::map<int,int> map_warpnodeid_paintingid;

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

void setCourseNodeAndArea(int coursenum, s16* oldnode, s16* oldarea) {
    bool isDeathWarp = *oldnode >= 64 || *oldnode == 0x0B;
    switch (coursenum) {
        case LEVEL_BOB:
            *oldnode = isDeathWarp ? 0x64 : 0x32;
            *oldarea = 0x01;
            return;
        case LEVEL_CCM:
            *oldnode = isDeathWarp ? 0x65 : 0x33;
            *oldarea = 0x01;
            return;
        case LEVEL_WF:
            *oldnode = isDeathWarp ? 0x66 : 0x34;
            *oldarea = 0x01;
            return;
        case LEVEL_JRB:
            *oldnode = isDeathWarp ? 0x67 : 0x35;
            *oldarea = 0x01;
            return;
        case LEVEL_BBH:
            *oldnode = isDeathWarp ? 0x0B : 0x0A;
            *oldarea = 0x01;
            return;
        case LEVEL_LLL:
            *oldnode = isDeathWarp ? 0x64 : 0x32;
            *oldarea = 0x03;
            return;
        case LEVEL_SSL:
            *oldnode = isDeathWarp ? 0x65 : 0x33;
            *oldarea = 0x03;
            return;
        case LEVEL_HMC:
            *oldnode = isDeathWarp ? 0x66 : 0x34;
            *oldarea = 0x03;
            return;
        case LEVEL_DDD:
            *oldnode = isDeathWarp ? 0x67 : 0x35;
            *oldarea = 0x03;
            return;
        case LEVEL_WDW:
            *oldnode = isDeathWarp ? 0x64 : 0x32;
            *oldarea = 0x02;
            return;
        case LEVEL_THI:
            *oldnode = isDeathWarp ? 0x65 : 0x33;
            *oldarea = 0x02;
            return;
        case LEVEL_TTM:
            *oldnode = isDeathWarp ? 0x66 : 0x34;
            *oldarea = 0x02;
            return;
        case LEVEL_TTC:
            *oldnode = isDeathWarp ? 0x67 : 0x35;
            *oldarea = 0x02;
            return;
        case LEVEL_SL:
            *oldnode = isDeathWarp ? 0x68 : 0x36;
            *oldarea = 0x02;
            return;
        case LEVEL_RR:
            *oldnode = isDeathWarp ? 0x6C : 0x3A;
            *oldarea = 0x02;
            return;
        default:
            return;
    }
}

void SM64AP_RedirectWarp(s16* curLevel, s16* destLevel, s8* curArea, s16* destArea, s16* destWarpNode, int warpNode_id) {
    if ((*curLevel == LEVEL_CASTLE || *curLevel == LEVEL_CASTLE_COURTYARD) && map_coursenum_paintingidx.count(*destLevel)) {
        if (sm64_clockaction) *sm64_clockaction = 5;
        int destination;
        if (warpNode_id != -1 && map_warpnodeid_paintingid.count(warpNode_id)) {
            // If available, use more accurate painting source information
            destination = map_entrances.at(map_warpnodeid_paintingid.at(warpNode_id));
        } else {
            // Extrapolate painting from destination, if nothing else works...
            destination = map_entrances.at(map_coursenum_paintingidx.at(*destLevel));
        }
        *destLevel = map_paintingidx_coursenum.at(destination/10); // Cuts off Area Info
        *destArea = destination % 10; // Cuts off Level Info
        *destWarpNode = 0x0A;
        return;
    }
    
    if ((*destLevel == LEVEL_CASTLE || *destLevel == LEVEL_CASTLE_COURTYARD) && map_coursenum_paintingidx.count(*curLevel)) {
        if (*destLevel == LEVEL_CASTLE && *destArea == 0x01 && *destWarpNode == 0x1F) return; //Exit Course
        if (*curLevel == LEVEL_COTMC) *curLevel = LEVEL_HMC;
        int orig_destination = map_coursenum_paintingidx.at(*curLevel)*10 + *curArea;
        int exit = map_paintingidx_coursenum.at(map_exits.at(orig_destination));
        if (exit == LEVEL_BBH) {
            *destLevel = LEVEL_CASTLE_COURTYARD;
        } else {
            *destLevel = LEVEL_CASTLE;
        }
        setCourseNodeAndArea(exit, destWarpNode, destArea);
    }
}

int SM64AP_CourseToTTC() {
    int i = 0;
    for (i = 0; i < 15; i++) {
        if (map_paintingidx_coursenum.at(map_entrances.at(i)/10) == LEVEL_TTC) {
            break;
        }
    }
    return i;
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
    for (int i = 0; i < map.size(); i++) {
        map_exits[map_entrances.at(i)] = i;
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
    AP_SetDeathLinkSupported(true);
    AP_SetItemClearCallback(&SM64AP_ResetItems);
    AP_SetLocationCheckedCallback(&SM64AP_CheckLocation);
    AP_SetItemRecvCallback(&SM64AP_RecvItem);
    AP_RegisterSlotDataIntCallback("FirstBowserDoorCost", &SM64AP_SetFirstBowserDoorCost);
    AP_RegisterSlotDataIntCallback("BasementDoorCost", &SM64AP_SetBasementDoorCost);
    AP_RegisterSlotDataIntCallback("SecondFloorDoorCost", &SM64AP_SetSecondFloorDoorCost);
    AP_RegisterSlotDataIntCallback("StarsToFinish", &SM64AP_SetStarsToFinish);
    AP_RegisterSlotDataMapIntIntCallback("AreaRando", &SM64AP_SetCourseMap);

    map_paintingidx_coursenum.insert(std::pair<int,int>(0,LEVEL_BOB));
    map_paintingidx_coursenum.insert(std::pair<int,int>(1,LEVEL_WF));
    map_paintingidx_coursenum.insert(std::pair<int,int>(2,LEVEL_JRB));
    map_paintingidx_coursenum.insert(std::pair<int,int>(3,LEVEL_CCM));
    map_paintingidx_coursenum.insert(std::pair<int,int>(4,LEVEL_BBH));
    map_paintingidx_coursenum.insert(std::pair<int,int>(5,LEVEL_HMC));
    map_paintingidx_coursenum.insert(std::pair<int,int>(6,LEVEL_LLL));
    map_paintingidx_coursenum.insert(std::pair<int,int>(7,LEVEL_SSL));
    map_paintingidx_coursenum.insert(std::pair<int,int>(8,LEVEL_DDD));
    map_paintingidx_coursenum.insert(std::pair<int,int>(9,LEVEL_SL));
    map_paintingidx_coursenum.insert(std::pair<int,int>(10,LEVEL_WDW));
    map_paintingidx_coursenum.insert(std::pair<int,int>(11,LEVEL_TTM));
    map_paintingidx_coursenum.insert(std::pair<int,int>(12,LEVEL_THI));
    map_paintingidx_coursenum.insert(std::pair<int,int>(13,LEVEL_THI));
    map_paintingidx_coursenum.insert(std::pair<int,int>(14,LEVEL_TTC));
    map_paintingidx_coursenum.insert(std::pair<int,int>(15,LEVEL_RR));
    for (int i = 0; i < map_paintingidx_coursenum.size(); i++) {
        map_coursenum_paintingidx[map_paintingidx_coursenum.at(i)] = i; // Breaks correct THI but who cares for now
    }
    map_coursenum_paintingidx.insert(std::pair<int,int>(LEVEL_COTMC,5)); //Map COTMC to HMC

    // NOT INCLUDED: RR, HMC due to same warp node ID (Magic Num 0x2A)
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x00,0)); // BOB
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x01,0));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x02,0));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x03,3)); // CCM
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x04,3));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x05,3));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x06,1)); // WF
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x07,1));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x08,1));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x09,2)); // JRB
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x0A,2));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x0B,2));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x0C,6)); // LLL
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x0D,6));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x0E,6));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x0F,7)); // SSL
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x10,7));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x11,7));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x15,8)); // DDD
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x16,8));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x17,8));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x18,10)); // WDW
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x19,10));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x1A,10));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x1B,12)); // THI Huge
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x1C,12));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x1D,12));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x1E,11)); // TTM
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x1F,11));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x20,11));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x21,14)); // TTC
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x22,14));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x23,14));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x24,9)); // SL
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x25,9));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x26,9));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x27,13)); // THI Tiny
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x28,13));
    map_warpnodeid_paintingid.insert(std::pair<int, int>(0x29,13));
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