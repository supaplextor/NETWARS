/*
 * netwars.h  –  NetWars main header
 *
 * Reverse-engineered from NETWARS.EXE (PKLITE 1.14 compressed DOS EXE).
 * Original tool chain: Borland C++ 3.1, TASM 3.2, large memory model.
 *
 * All structures, constants and prototypes are derived from disassembly of
 * the 239,712-byte decompressed image (data-segment base 0x2AE6).
 */

#ifndef NETWARS_H
#define NETWARS_H

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mem.h>
#include <conio.h>
#include <time.h>

/* ── compiler / model helpers ─────────────────────────────────────────── */
#define FAR     far
#define NEAR    near
#define PASCAL  pascal
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef int             BOOL;
#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

/* ── version ──────────────────────────────────────────────────────────── */
#define NW_VERSION_STRING  "NetWars"
#define COPYRIGHT_STRING   "Copyright (c) 1997 Caldera, Inc. All rights reserved."
#define HISCORES_FILE      "HISCORES.DAT"
#define INI_FILE           "NETWARS.INI"
#define README_FILE        "README.NW"
#define INI_SECTION        "[NetWars]"
#define INI_SECTION_SHARED "[NetWars Shared File]"

/* ── screen / video modes ─────────────────────────────────────────────── */
#define SCREEN_EGA   1   /* 640x350, 16 colours  */
#define SCREEN_SVGA  2   /* 640x480, 256 colours */
#define SCREEN_VGA   3   /* 320x200, 256 colours */

#define EGA_WIDTH    640
#define EGA_HEIGHT   350
#define SVGA_WIDTH   640
#define SVGA_HEIGHT  480
#define VGA_WIDTH    320
#define VGA_HEIGHT   200

/* ── sound options ────────────────────────────────────────────────────── */
#define SOUND_NONE         0
#define SOUND_PC_SPEAKER   1
#define SOUND_BLASTER      2

/* ── control options ──────────────────────────────────────────────────── */
#define CTRL_MOUSE    0
#define CTRL_KEYBOARD 1
#define CTRL_JOYSTICK 2

/* ── ship / entity types ──────────────────────────────────────────────── */
#define SHIP_ALIEN_FIGHTER  0
#define SHIP_CARGO_SHIP     1
#define SHIP_ALIEN_CRUISER  2
#define SHIP_TURRET         3
#define SHIP_CUSTOM         4    /* loaded from .NW shape file */

/* ── game modes ───────────────────────────────────────────────────────── */
#define MODE_SINGLE_PLAYER   0
#define MODE_MULTI_PLAYER    1
#define MODE_SPACE_SIM       2
#define MODE_SHAPE_EDITOR    3

/* ── multiplayer networking ───────────────────────────────────────────── */
#define MAX_PLAYERS          6
#define IPX_NODE_LEN         6   /* bytes in an IPX node address */
#define IPX_SOCKET_NW        0x4577   /* NetWars IPX socket (big-endian) */
#define NET_BROADCAST        0   /* broadcast mode */
#define NET_NODE2NODE        1   /* node-to-node (direct) mode */
#define MAX_IPX_PACKET       576

/* IPX node address (5 WORDs as printed: %04X %04X %04X %04X %04X) */
typedef struct {
    WORD w[5];
} IPXNode;

/* ── player state ─────────────────────────────────────────────────────── */
#define MAX_NAME_LEN   16
#define INIT_SHIELDS   100
#define INIT_SHELLS    200
#define INIT_MISSILES  10

typedef struct PlayerState {
    char    name[MAX_NAME_LEN]; /* player name, default "Player N"        */
    BYTE    shipType;           /* SHIP_xxx constant                       */
    int     shields;            /* current shield level (0-100)            */
    int     maxShields;
    int     shells;             /* cannon shells remaining                 */
    int     missiles;           /* guided missiles remaining               */
    int     score;
    int     kills;
    int     deaths;
    BOOL    turboCloakActive;
    BOOL    cloakActive;
    BOOL    retired;
    WORD    x;                  /* world X position (fixed-point)          */
    WORD    y;                  /* world Y position                        */
    int     velX;               /* velocity X                              */
    int     velY;               /* velocity Y                              */
    int     angle;              /* heading in degrees (0-359)              */
    BOOL    active;
    IPXNode node;               /* IPX address of this player's machine    */
} PlayerState;

/* ── mission descriptors ──────────────────────────────────────────────── */
#define MAX_MISSIONS  20

typedef struct MissionDef {
    int     id;
    char    title[40];
    int     alienCount;
    int     cargoShipCount;
    int     fuelPodCount;
    int     cruiserCount;
    BOOL    completed;
    int     bonusMultiplier;
} MissionDef;

/* mission objective IDs */
#define OBJ_DESTROY_FIGHTERS   0
#define OBJ_DEFEND_CARGO       1
#define OBJ_PROTECT_FUELPODS   2
#define OBJ_DESTROY_CRUISER    3
#define OBJ_COLLECT_SATELLITES 4
#define OBJ_DEFEND_ALL         5

/* ── high-score table ─────────────────────────────────────────────────── */
#define MAX_HISCORES  10
#define INITIALS_LEN  4

typedef struct HiScore {
    char    initials[INITIALS_LEN];
    long    score;
    int     mission;
} HiScore;

/* ── game configuration (mirrors NETWARS.INI) ─────────────────────────── */
typedef struct NWConfig {
    int     soundCard;          /* SOUND_xxx */
    BOOL    pcSpeaker;
    BOOL    soundBlaster;
    BOOL    computerShip;
    BOOL    seperateScanner;
    char    customShipFile[64]; /* custom ship shape file name */
    char    defaultName[MAX_NAME_LEN];
    BOOL    fastMode;
    BOOL    motionBlur;
    char    sharedFile[64];
    int     networkMode;        /* NET_BROADCAST or NET_NODE2NODE */
    int     broadcast[5];       /* broadcast IPX address (5 words) */
    IPXNode nodes[MAX_PLAYERS]; /* node addresses for NODE2NODE mode */
    int     screenMode;         /* SCREEN_xxx */
    int     controlType;        /* CTRL_xxx */
    BOOL    polygons;           /* use polygon (wireframe) mode */
    BOOL    remoteView;
} NWConfig;

/* ── global state ─────────────────────────────────────────────────────── */
extern NWConfig     FAR gConfig;
extern PlayerState  FAR gPlayers[MAX_PLAYERS];
extern int          FAR gNumPlayers;
extern int          FAR gGameMode;
extern int          FAR gCurrentMission;
extern BOOL         FAR gPaused;
extern BOOL         FAR gGameOver;
extern BOOL         FAR gShootOutMode;
extern long         FAR gScore;
extern int          FAR gAliensRemaining;
extern HiScore      FAR gHiScores[MAX_HISCORES];
extern int          FAR gScreenMode;

/* ── function prototypes ──────────────────────────────────────────────── */

/* main.cpp */
int     FAR main(int argc, char FAR * FAR *argv);

/* init.cpp  (graphics / hardware initialisation) */
BOOL    FAR InitGraphics(int screenMode);
void    FAR ShutdownGraphics(void);
BOOL    FAR InitSound(int soundType);
void    FAR ShutdownSound(void);
BOOL    FAR InitIPX(void);
void    FAR ShutdownIPX(void);
BOOL    FAR InitJoystick(void);

/* menu.cpp  (all interactive menus) */
int     FAR ShowMainMenu(void);
int     FAR SelectScreenMode(void);
int     FAR SelectShip(int player);
void    FAR EnterPlayerName(int player);
int     FAR SelectMission(void);
void    FAR ShowControls(void);
void    FAR ShowHiScores(void);
BOOL    FAR EnterInitials(char *out);
int     FAR SelectNumPlayers(void);
void    FAR ShowShootOutResults(void);

/* game.cpp  (core game loop) */
void    FAR RunGame(void);
void    FAR RunSinglePlayer(void);
void    FAR RunMultiPlayer(void);
void    FAR RunSpaceSim(void);
void    FAR PauseGame(void);
void    FAR UpdatePhysics(void);
void    FAR UpdateAI(void);
void    FAR CheckCollisions(void);
void    FAR RenderFrame(void);
void    FAR ProcessInput(void);

/* net.cpp   (IPX networking) */
BOOL    FAR NetOpenSocket(WORD socketNum);
void    FAR NetCloseSocket(void);
BOOL    FAR NetSendPacket(IPXNode FAR *dest, void FAR *data, WORD len);
BOOL    FAR NetReceivePacket(void FAR *buf, WORD FAR *len, IPXNode FAR *from);
void    FAR NetBroadcast(void FAR *data, WORD len);
BOOL    FAR NetWaitForPlayers(int expected);

/* shape.cpp  (ship shape loading and rendering) */
BOOL    FAR LoadShape(const char *filename);
void    FAR DrawShape(int x, int y, int angle, int shipType, int color);
void    FAR RunShapeEditor(void);

/* config.cpp  (INI file read/write) */
void    FAR LoadConfig(void);
void    FAR SaveConfig(void);

/* score.cpp  (high-score management) */
void    FAR LoadHiScores(void);
void    FAR SaveHiScores(void);
int     FAR RankScore(long score);
void    FAR InsertHiScore(int rank, const char *initials, long score, int mission);

/* util.cpp  (general utilities) */
void    FAR ShowMessage(const char *fmt, ...);
void    FAR DebugPrint(const char *fmt, ...);
void    FAR DrawStatusBar(int player);
void    FAR DrawHUD(void);
void    FAR DrawRadar(void);
int     FAR Random(int max);

#endif /* NETWARS_H */
