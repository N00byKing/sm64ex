#include "sm64ap.h"
#include "Archipelago.h"

extern "C" {
    #include "game/print.h"
    #include "gfx_dimensions.h"
    #include "level_table.h"
    #include "game/level_update.h"
}

#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <bitset>
#include <set>

#define WARP_NODE_CREDITS_MIN 0xF8 // level_update.c

// Set to false on some branch for compat with patches
static constexpr bool SM64AP_SUPPORT_MOVE_RANDO = true;

int starsCollected = 0;
bool sm64_locations[SM64AP_NUM_LOCS];
bool sm64_have_key1 = false;
bool sm64_have_key2 = false;
bool sm64_have_wingcap = false;
bool sm64_have_metalcap = false;
bool sm64_have_vanishcap = false;
int sm64_moat_state = 0;
bool sm64_have_cannon[15];
int sm64_completion_type = 0;
std::bitset<SM64AP_NUM_ABILITIES> sm64_have_abilities;
int* sm64_clockaction = nullptr;
int sm64_cost_firstbowserdoor = 8;
int sm64_cost_basementdoor = 30;
int sm64_cost_secondfloordoor = 50;
int sm64_cost_endlessstairs = 70;
int sm64_cost_mips1 = 15;
int sm64_cost_mips2 = 50;
int msg_frame_duration = 90; // 3 Secounds at 30F/s
int cur_msg_frame_duration = msg_frame_duration;

std::map<int,int> map_entrances;
std::set<int> course_dest_supported;

std::map<int,int> map_boxid_locid;

int sm64_exit_return_to;
int sm64_exit_orig_entrancelvl;

void SM64AP_RecvItem(int64_t idx, bool notify) {
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
            sm64_have_cannon[idx-(SM64AP_ID_CANNONUNLOCK(0))] = true;
            break;
        case SM64AP_ID_ABILITY(0) ... SM64AP_ID_ABILITY(SM64AP_NUM_ABILITIES-1):
            sm64_have_abilities[idx-(SM64AP_ID_ABILITY(0))] = true;
            break;
    }
}

void SM64AP_CheckLocation(int64_t loc_id) {
    sm64_locations[loc_id - SM64AP_ID_OFFSET] = true;
}

u32 SM64AP_CourseStarFlags(s32 courseIdx) {
    u32 starflags = 0;
    s32 courseIndex = courseIdx;
    if (courseIdx == -1) {
        courseIndex = 24;
    }
    for (int i = 0; i < 7; i++) {
        if (sm64_locations[i + (courseIndex * 7)]) {
            starflags |= (1 << i);
        }
    }
    return starflags;
}

void setCourseNodeAndArea(int coursenum, s16* oldnode, bool isDeathWarp, int warpOp) {
    switch (coursenum) {
        case LEVEL_BOB:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x64 : 0x32;
            return;
        case LEVEL_CCM:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x65 : 0x33;
            return;
        case LEVEL_WF:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x66 : 0x34;
            return;
        case LEVEL_JRB:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x67 : 0x35;
            return;
        case LEVEL_BBH:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x0B : 0x0A;
            return;
        case LEVEL_LLL:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x64 : 0x32;
            return;
        case LEVEL_SSL:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x65 : 0x33;
            return;
        case LEVEL_HMC:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x66 : 0x34;
            return;
        case LEVEL_DDD:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x67 : 0x35;
            return;
        case LEVEL_WDW:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x64 : 0x32;
            return;
        case LEVEL_THI:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x65 : 0x33;
            return;
        case LEVEL_TTM:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x66 : 0x34;
            return;
        case LEVEL_TTC:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x67 : 0x35;
            return;
        case LEVEL_SL:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x68 : 0x36;
            return;
        case LEVEL_RR:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x6C : 0x3A;
            return;
        case LEVEL_PSS:
        case LEVEL_TOTWC:
            *oldnode = isDeathWarp ? 0x21 : (warpOp == WARP_OP_STAR_EXIT ? 0x26: 0x20);
            return;
        case LEVEL_SA:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x28 : 0x27;
            return;
        case LEVEL_BITDW:
        case LEVEL_BOWSER_1:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x25 : 0x24;
            return;
        case LEVEL_VCUTM:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x06 : 0x07;
            return;
        case LEVEL_BITFS:
        case LEVEL_BOWSER_2:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x68 : 0x36;
            return;
        case LEVEL_WMOTR:
            *oldnode = (isDeathWarp || warpOp != WARP_OP_STAR_EXIT) ? 0x6D : 0x38;
        default:
            return;
    }
}

void SM64AP_RedirectWarp(s16* curLevel, s16* destLevel, s8* curArea, s16* destArea, s16* destWarpNode, bool isDeathWarp, int warpOp) {
    // When warping, always lock the clock and reset var to avoid segfault if old clock val is not in new area
    SM64AP_SetClockToTTCState();
    if (*destLevel == LEVEL_BOWSER_3 || *curLevel == LEVEL_BOWSER_3 ||
        *destLevel == LEVEL_BITS || *curLevel == LEVEL_BITS) return; // Dont play around with this one
    if (*destWarpNode >= WARP_NODE_CREDITS_MIN) return; // Credit Warps
    if ((*curLevel == LEVEL_CASTLE || *curLevel == LEVEL_CASTLE_COURTYARD || *curLevel == LEVEL_CASTLE_GROUNDS || *curLevel == LEVEL_HMC) && 
         *destLevel != LEVEL_CASTLE && *destLevel != LEVEL_CASTLE_COURTYARD && *destLevel != LEVEL_CASTLE_GROUNDS) {
        int destination;
        switch (*destLevel) {
            case LEVEL_LLL:
            case LEVEL_SSL:
            case LEVEL_TTM:
            case LEVEL_COTMC:
                destination = map_entrances[*destLevel * 10 + 1];
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

    if ((*destLevel == LEVEL_CASTLE || *destLevel == LEVEL_CASTLE_COURTYARD || *destLevel == LEVEL_CASTLE_GROUNDS) && course_dest_supported.find(*curLevel) != course_dest_supported.end()) {
        if (*destLevel == LEVEL_CASTLE && (*destWarpNode == 0x1F || *destWarpNode == 0x00)) return; //Exit Course or Inter-Castle warp
        *destLevel = sm64_exit_return_to / 10;
        *destArea = sm64_exit_return_to % 10;
        setCourseNodeAndArea(sm64_exit_orig_entrancelvl, destWarpNode, isDeathWarp, warpOp);
        return;
    }
}

int SM64AP_CourseToTTC() {
    int level = 0;
    for (auto itr : map_entrances) {
        if (itr.second/10 == LEVEL_TTC) {
            return itr.first/10;
        }
    }
    return -1; // Error Cond
}

void SM64AP_SetMoatState(int action) {
    sm64_moat_state = action;
}

void SM64AP_SetClockToTTCAction(int* action) {
    sm64_clockaction = action;
}

void SM64AP_SetClockToTTCState() {
    if (sm64_clockaction) *sm64_clockaction = 5;
    sm64_clockaction = nullptr;
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

void SM64AP_SetMIPS1Cost(int amount) {
    sm64_cost_mips1 = amount;
}

void SM64AP_SetMIPS2Cost(int amount) {
    sm64_cost_mips2 = amount;
}

void SM64AP_SetStarsToFinish(int amount) {
    sm64_cost_endlessstairs = amount;
}

void SM64AP_SetCompletionType(int type) {
    sm64_completion_type = type;
}

void SM64AP_SetCourseMap(std::map<int,int> map) {
    map_entrances = map;
}

void SM64AP_SetMoveRandoVec(int vec) {
    for (int i = 0; i < SM64AP_NUM_ABILITIES; i++) {
        sm64_have_abilities[i] = !std::bitset<SM64AP_NUM_ABILITIES>(vec).test(i) || sm64_have_abilities[i];
    }
}

void SM64AP_ResetItems() {
    for (int i = 0; i < SM64AP_NUM_LOCS; i++) {
        sm64_locations[i] = false;
    }
    for (int i = 0; i < 15; i++) {
        sm64_have_cannon[i] = false;
    }
    sm64_have_abilities.reset();
    sm64_have_key1 = false;
    sm64_have_key2 = false;
    sm64_have_wingcap = false;
    sm64_have_metalcap = false;
    sm64_have_vanishcap = false;
    starsCollected = 0;
}

void SM64AP_SetReplyHandler(AP_SetReply reply) {
    if (reply.key == AP_GetPrivateServerDataPrefix() + "FinishedBowser") {
        switch (sm64_completion_type) {
            case 0: // Only BitS
                if ((*(int*)(reply.value) & 0b100) > 0) AP_StoryComplete();
                break;
            case 1: // All Bowser Stages
                if (*(int*)(reply.value) == 0b111) AP_StoryComplete();
                break;
        }
    }
    else if (reply.key == AP_GetPrivateServerDataPrefix() + "MoatDrained") {
        sm64_moat_state = *(int *) (reply.value);
    }
}

void SM64AP_GenericInit() {
    AP_NetworkVersion version = {0,3,5};
    AP_SetClientVersion(&version);
    AP_SetDeathLinkSupported(true);
    AP_SetItemClearCallback(&SM64AP_ResetItems);
    AP_SetLocationCheckedCallback(&SM64AP_CheckLocation);
    AP_SetItemRecvCallback(&SM64AP_RecvItem);
    AP_RegisterSetReplyCallback(&SM64AP_SetReplyHandler);
    AP_SetNotify(AP_GetPrivateServerDataPrefix() + "FinishedBowser", AP_DataType::Int);
    AP_SetNotify(AP_GetPrivateServerDataPrefix() + "MoatDrained", AP_DataType::Int);

    AP_RegisterSlotDataIntCallback("FirstBowserDoorCost", &SM64AP_SetFirstBowserDoorCost);
    AP_RegisterSlotDataIntCallback("BasementDoorCost", &SM64AP_SetBasementDoorCost);
    AP_RegisterSlotDataIntCallback("SecondFloorDoorCost", &SM64AP_SetSecondFloorDoorCost);
    AP_RegisterSlotDataIntCallback("MIPS1Cost", &SM64AP_SetMIPS1Cost);
    AP_RegisterSlotDataIntCallback("MIPS2Cost", &SM64AP_SetMIPS2Cost);
    AP_RegisterSlotDataIntCallback("StarsToFinish", &SM64AP_SetStarsToFinish);
    AP_RegisterSlotDataIntCallback("CompletionType", &SM64AP_SetCompletionType);
    AP_RegisterSlotDataIntCallback("MoveRandoVec", &SM64AP_SetMoveRandoVec);
    AP_RegisterSlotDataMapIntIntCallback("AreaRando", &SM64AP_SetCourseMap);

    course_dest_supported = {
        LEVEL_BOB, LEVEL_WF, LEVEL_JRB, LEVEL_CCM, LEVEL_BBH, LEVEL_HMC, LEVEL_LLL, LEVEL_SSL, LEVEL_DDD, LEVEL_SL,
        LEVEL_WDW, LEVEL_TTM, LEVEL_THI, LEVEL_TTC, LEVEL_RR, LEVEL_PSS, LEVEL_SA, LEVEL_BITDW, LEVEL_TOTWC, LEVEL_COTMC,
        LEVEL_VCUTM, LEVEL_BITFS, LEVEL_WMOTR, LEVEL_BOWSER_1, LEVEL_BOWSER_2, LEVEL_BOWSER_3
    };
    
    map_boxid_locid[LEVEL_CCM*10 + 1] = 3626215;
    map_boxid_locid[LEVEL_CCM*10 + 2] = 3626216;
    map_boxid_locid[LEVEL_CCM*10 + 3] = 3626217;
    map_boxid_locid[LEVEL_BBH*10 + 1] = 3626218;
    map_boxid_locid[LEVEL_HMC*10 + 1] = 3626219;
    map_boxid_locid[LEVEL_HMC*10 + 2] = 3626220;
    map_boxid_locid[LEVEL_SSL*10 + 1] = 3626221;
    map_boxid_locid[LEVEL_SSL*10 + 2] = 3626222;
    map_boxid_locid[LEVEL_SSL*10 + 3] = 3626223;
    map_boxid_locid[LEVEL_SL*10 + 1] = 3626224;
    map_boxid_locid[LEVEL_SL*10 + 2] = 3626225;
    map_boxid_locid[LEVEL_WDW*10 + 2] = 3626226; // Uses first bit as flag for something, makes mario invisible :/
    map_boxid_locid[LEVEL_TTM*10 + 1] = 3626227;
    map_boxid_locid[LEVEL_THI*10 + 1] = 3626228;
    map_boxid_locid[LEVEL_THI*10 + 2] = 3626229;
    map_boxid_locid[LEVEL_THI*10 + 3] = 3626230;
    map_boxid_locid[LEVEL_TTC*10 + 1] = 3626231;
    map_boxid_locid[LEVEL_TTC*10 + 2] = 3626232;
    map_boxid_locid[LEVEL_RR*10 + 1] = 3626233;
    map_boxid_locid[LEVEL_RR*10 + 2] = 3626234;
    map_boxid_locid[LEVEL_RR*10 + 3] = 3626235;
    map_boxid_locid[LEVEL_BITDW*10 + 1] = 3626236;
    map_boxid_locid[LEVEL_BITDW*10 + 2] = 3626237;
    map_boxid_locid[LEVEL_BITFS*10 + 1] = 3626238;
    map_boxid_locid[LEVEL_BITFS*10 + 2] = 3626239;
    map_boxid_locid[LEVEL_BITS*10 + 1] = 3626240;
    map_boxid_locid[LEVEL_COTMC*10 + 1] = 3626241;
    map_boxid_locid[LEVEL_VCUTM*10 + 1] = 3626242;
    map_boxid_locid[LEVEL_WMOTR*10 + 1] = 3626243;
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

void SM64AP_SendByBoxID(int id) {
    SM64AP_SendItem(map_boxid_locid[id]);
}

void SM64AP_SendItem(int idx) {
    AP_SendItem(idx);
}

void SM64AP_FinishBowser(int i) {
    AP_SetServerDataRequest req;
    req.key = AP_GetPrivateServerDataPrefix() + "FinishedBowser";
    int def_val = 0;
    req.default_value = &def_val;
    req.type = AP_DataType::Int;
    req.want_reply = true;
    int flag = 0b001 << i;
    req.operations = std::vector<AP_DataStorageOperation>{{{"or", &flag}}};
    AP_SetServerData(&req);
}

AP_GetServerDataRequest moat_request;
void SM64AP_CheckMoatState() {
    if (moat_request.key != "") {
        return;
    }
    moat_request.key = AP_GetPrivateServerDataPrefix() + "MoatDrained";
    moat_request.type = AP_DataType::Int;
    moat_request.value = &sm64_moat_state;
    AP_GetServerData(&moat_request);
}

void SM64AP_SetMoatDrained(int flag) {
    AP_SetServerDataRequest req;
    req.key = AP_GetPrivateServerDataPrefix() + "MoatDrained";
    int def_val = 0;
    req.default_value = &def_val;
    req.type = AP_DataType::Int;
    req.want_reply = true;
    req.operations = std::vector<AP_DataStorageOperation>{ { { "or", &flag } } };
    AP_SetServerData(&req);
}

int SM64AP_GetStars() {
    return starsCollected;
}

int SM64AP_GetRequiredStars(int idprx) {
    switch (idprx) {
        case 8: // Star Door 8
            return sm64_cost_firstbowserdoor;
        case 30: // Star Door 30
            return sm64_cost_basementdoor;
        case 50: // Star Door 50
            return sm64_cost_secondfloordoor;
        case 70: // Star Door 70
            return sm64_cost_endlessstairs;
        case 3626171: // MIPS 1
            return sm64_cost_mips1;
        case 3626172: // MIPS 2
            return sm64_cost_mips2;
        default:
            return idprx;
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

bool SM64AP_PressedSwitch(int flag) {
    switch (flag) {
        case 2:
            return SM64AP_CheckedLoc(SM64AP_ID_WINGCAP);
        case 4:
            return SM64AP_CheckedLoc(SM64AP_ID_METALCAP);
        case 8:
            return SM64AP_CheckedLoc(SM64AP_ID_VANISHCAP);
        default:
            // Shouldn't happen, but just in case, this shouldn't be pressed
            return false;
    }
}

bool SM64AP_HaveCannon(int courseIdx) {
    if (courseIdx < 15) return sm64_have_cannon[courseIdx];
    return true;
}

bool SM64AP_MoatDrained() {
    while (moat_request.key == "" || moat_request.status == AP_RequestStatus::Pending) {
        SM64AP_CheckMoatState();
    }
    return sm64_moat_state != 0;
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

bool SM64AP_CanDoubleJump() {
    #warning Use doublejump logic once implemented
    return sm64_have_abilities[SM64AP_ID_TRIPLEJUMP - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanTripleJump() {
    return sm64_have_abilities[SM64AP_ID_TRIPLEJUMP - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanLongJump() {
    return sm64_have_abilities[SM64AP_ID_LONGJUMP - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanBackflip() {
    return sm64_have_abilities[SM64AP_ID_BACKFLIP - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanSideFlip() {
    return sm64_have_abilities[SM64AP_ID_SIDEFLIP - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanWallKick() {
    return sm64_have_abilities[SM64AP_ID_WALLKICK - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanDive() {
    return sm64_have_abilities[SM64AP_ID_DIVE - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanGroundPound() {
    return sm64_have_abilities[SM64AP_ID_GROUNDPOUND - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanKick() {
    return sm64_have_abilities[SM64AP_ID_KICK - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanClimb() {
    return sm64_have_abilities[SM64AP_ID_CLIMB - SM64AP_ABILITY_OFFSET];
}

bool SM64AP_CanLedgeGrab() {
    return sm64_have_abilities[SM64AP_ID_LEDGEGRAB - SM64AP_ABILITY_OFFSET];
}


void SM64AP_PrintNext() {
    if (AP_GetConnectionStatus() == AP_ConnectionStatus::Disconnected) {
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(SCREEN_WIDTH / 2) - 7, SCREEN_HEIGHT / 2, "Connecting");
    }
    if (AP_GetConnectionStatus() == AP_ConnectionStatus::ConnectionRefused) {
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(SCREEN_WIDTH / 2) - 10, SCREEN_HEIGHT / 2, "CONNECTION REFUSED");
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(SCREEN_WIDTH / 2) - 10, SCREEN_HEIGHT / 2 - 20, "CHECK ARGS");
    }
    if (!sm64_have_abilities.all() && !SM64AP_SUPPORT_MOVE_RANDO) {
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(SCREEN_WIDTH / 2) - 10, SCREEN_HEIGHT / 2, "INCOMPATIBLE WITH");
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(SCREEN_WIDTH / 2) - 10, SCREEN_HEIGHT / 2 - 20, "MOUE RANDO");
    }
    if (!AP_IsMessagePending()) return;
    AP_Message* msg = AP_GetLatestMessage();
    if (msg->type == AP_MessageType::ItemSend) {
        AP_ItemSendMessage* o_msg = static_cast<AP_ItemSendMessage*>(msg);
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0), (1-0)*20, (o_msg->item + std::string(" was sent")).c_str());
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0), (1-1)*20, (std::string("to ") + o_msg->recvPlayer).c_str());
    } else if (msg->type == AP_MessageType::ItemRecv) {
        AP_ItemRecvMessage* o_msg = static_cast<AP_ItemRecvMessage*>(msg);
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0), (1-0)*20, (std::string("Got ") + o_msg->item).c_str());
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0), (1-1)*20, (std::string("From ") + o_msg->sendPlayer).c_str());
    } else if (msg->type == AP_MessageType::Countdown) {
        cur_msg_frame_duration = std::min(cur_msg_frame_duration, 30);
        AP_CountdownMessage* o_msg = static_cast<AP_CountdownMessage*>(msg);
        print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0) + SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, std::to_string(o_msg->timer).c_str());
    } else {
        //print_text(GFX_DIMENSIONS_FROM_LEFT_EDGE(0), (1-0)*20, msg->text.c_str());
    }
    if (cur_msg_frame_duration > 0) {
        cur_msg_frame_duration--;
    } else {
        AP_ClearLatestMessage();
        cur_msg_frame_duration = msg_frame_duration;
    }
}