#include <stdbool.h>
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

#define SM64AP_ID_CANNONUNLOCK(x) (SM64AP_ID_OFFSET+200+x)

#define SM64AP_NUM_LOCS 215


#ifdef __cplusplus
//Init
extern "C" void SM64AP_InitMW(const char*, const char*, const char*);
extern "C" void SM64AP_InitSP(const char*);

// Local Stars, Keys and Caps
extern "C" int SM64AP_GetStars();
extern "C" int SM64AP_GetRequiredStars(int);
extern "C" u32 SM64AP_CourseStarFlags(s32);
extern "C" void SM64AP_RedirectWarp(s16*,s16*,s8*,s16*,s16*,int);
extern "C" int SM64AP_CourseToTTC();
extern "C" void SM64AP_SetClockToTTCAction(int* action);
extern "C" bool SM64AP_CheckedLoc(int);
extern "C" bool SM64AP_HaveKey1();
extern "C" bool SM64AP_HaveKey2();
extern "C" bool SM64AP_HaveCap(int);
extern "C" bool SM64AP_HaveCannon(int);
extern "C" bool SM64AP_DeathLinkPending();
extern "C" void SM64AP_DeathLinkClear();
extern "C" void SM64AP_DeathLinkSend();

// Send Item
extern "C" void SM64AP_SendItem(int);

// Print Next Message to Screen
extern "C" void SM64AP_PrintNext();

// Called when Story completed, sends StatusUpdate
extern "C" void SM64AP_StoryComplete();
#else
void SM64AP_InitMW(const char*, const char*, const char*);
void SM64AP_InitSP(const char*);
int SM64AP_GetStars();
int SM64AP_GetRequiredStars(int);
u32 SM64AP_CourseStarFlags(s32);
void SM64AP_RedirectWarp(s16*,s16*,s8*,s16*,s16*,int);
int SM64AP_CourseToTTC();
void SM64AP_SetClockToTTCAction(int* action);
bool SM64AP_CheckedLoc(int);
bool SM64AP_HaveKey1();
bool SM64AP_HaveKey2();
bool SM64AP_HaveCap(int);
bool SM64AP_HaveCannon(int);
bool SM64AP_DeathLinkPending();
void SM64AP_DeathLinkClear();
void SM64AP_DeathLinkSend();
void SM64AP_SendItem(int);
void SM64AP_PrintNext();
void SM64AP_StoryComplete();
#endif
