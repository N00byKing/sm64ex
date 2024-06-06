#include <stdbool.h>

#ifdef __cplusplus
#define AP_EXTERN_C extern "C"
#else
#define AP_EXTERN_C
#endif

#ifdef __cplusplus
extern "C" {
    #include "types.h"
}
#else
#include "types.h"
#endif

#define SM64AP_ID_OFFSET 3626000
#define SM64AP_ITEMID_STAR SM64AP_ID_OFFSET
#define SM64AP_ID_KEY1 (SM64AP_ID_OFFSET+178)
#define SM64AP_ID_KEY2 (SM64AP_ID_KEY1+1)
#define SM64AP_ID_KEYPROG (SM64AP_ID_KEY2+1)
#define SM64AP_ID_WINGCAP (SM64AP_ID_KEYPROG+1)
#define SM64AP_ID_METALCAP (SM64AP_ID_WINGCAP+1)
#define SM64AP_ID_VANISHCAP (SM64AP_ID_METALCAP+1)
#define SM64AP_ITEMID_1UP (SM64AP_ID_VANISHCAP+1)

#define SM64AP_ABILITY_OFFSET (SM64AP_ITEMID_1UP+1)
#define SM64AP_ID_DOUBLEJUMP (SM64AP_ABILITY_OFFSET)
#define SM64AP_ID_TRIPLEJUMP (SM64AP_ABILITY_OFFSET+1)
#define SM64AP_ID_LONGJUMP (SM64AP_ABILITY_OFFSET+2)
#define SM64AP_ID_BACKFLIP (SM64AP_ABILITY_OFFSET+3)
#define SM64AP_ID_SIDEFLIP (SM64AP_ABILITY_OFFSET+4)
#define SM64AP_ID_WALLKICK (SM64AP_ABILITY_OFFSET+5)
#define SM64AP_ID_DIVE (SM64AP_ABILITY_OFFSET+6)
#define SM64AP_ID_GROUNDPOUND (SM64AP_ABILITY_OFFSET+7)
#define SM64AP_ID_KICK (SM64AP_ABILITY_OFFSET+8)
#define SM64AP_ID_CLIMB (SM64AP_ABILITY_OFFSET+9)
#define SM64AP_ID_LEDGEGRAB (SM64AP_ABILITY_OFFSET+10)

#define SM64AP_ID_CANNONUNLOCK(x) (SM64AP_ID_OFFSET+200+x)
#define SM64AP_ID_ABILITY(x) (SM64AP_ABILITY_OFFSET+x)

#define SM64AP_LOCATIONID_BASEMENTTOAD (SM64AP_ID_OFFSET + 168)
#define SM64AP_LOCATIONID_SECONDFLOORTOAD (SM64AP_LOCATIONID_BASEMENTTOAD + 1)
#define SM64AP_LOCATIONID_THIRDFLOORTOAD (SM64AP_LOCATIONID_SECONDFLOORTOAD + 1)
#define SM64AP_LOCATIONID_MIPS1 (SM64AP_LOCATIONID_ATTICTOAD + 1)
#define SM64AP_LOCATIONID_MIPS2 (SM64AP_LOCATIONID_MIPS1 + 1)

#define SM64AP_NUM_LOCS 244

#define SM64AP_NUM_ABILITIES 11

//Init
AP_EXTERN_C void SM64AP_InitMW(const char*, const char*, const char*);
AP_EXTERN_C void SM64AP_InitSP(const char*);

// Local Stars, Keys and Caps
AP_EXTERN_C int SM64AP_GetStars();
AP_EXTERN_C int SM64AP_GetRequiredStars(int);
AP_EXTERN_C u32 SM64AP_CourseStarFlags(s32);
AP_EXTERN_C void SM64AP_RedirectWarp(s16*,s16*,s8*,s16*,s16*,bool,int);
AP_EXTERN_C int SM64AP_CourseToTTC();
AP_EXTERN_C void SM64AP_SetClockToTTCAction(int* action);
AP_EXTERN_C void SM64AP_SetClockToTTCState();
AP_EXTERN_C bool SM64AP_CheckedLoc(int);
AP_EXTERN_C bool SM64AP_HaveKey1();
AP_EXTERN_C bool SM64AP_HaveKey2();
AP_EXTERN_C bool SM64AP_HaveCap(int);
AP_EXTERN_C bool SM64AP_HaveCannon(int);
AP_EXTERN_C bool SM64AP_DeathLinkPending();
AP_EXTERN_C void SM64AP_DeathLinkClear();
AP_EXTERN_C void SM64AP_DeathLinkSend();

// Local Moves
AP_EXTERN_C bool SM64AP_CanDoubleJump();
AP_EXTERN_C bool SM64AP_CanTripleJump();
AP_EXTERN_C bool SM64AP_CanLongJump();
AP_EXTERN_C bool SM64AP_CanBackflip();
AP_EXTERN_C bool SM64AP_CanSideFlip();
AP_EXTERN_C bool SM64AP_CanWallKick();
AP_EXTERN_C bool SM64AP_CanDive();
AP_EXTERN_C bool SM64AP_CanGroundPound();
AP_EXTERN_C bool SM64AP_CanKick();
AP_EXTERN_C bool SM64AP_CanClimb();
AP_EXTERN_C bool SM64AP_CanLedgeGrab();

// Send Item
AP_EXTERN_C void SM64AP_SendByBoxID(int);
AP_EXTERN_C void SM64AP_SendItem(int);

// Print Next Message to Screen
AP_EXTERN_C void SM64AP_PrintNext();

// Called on each Bowser stage completion, i is bowser index. Will send StoryComplete depending on completion option.
AP_EXTERN_C void SM64AP_FinishBowser(int i);

// Check for switch state (used for initial switch state on level load)
AP_EXTERN_C bool SM64AP_PressedSwitch(int);

#undef AP_EXTERN_C
