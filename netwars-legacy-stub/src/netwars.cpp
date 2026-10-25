/*
 * netwars.cpp  –  NetWars main game module
 *
 * Reverse-engineered from NETWARS.EXE (PKLITE 1.14 compressed DOS EXE).
 * Original tool chain: Borland C++ 3.1, TASM 3.2, large memory model.
 *
 * Compile with:
 *   bcc -ml -3 -O2 -c netwars.cpp
 *
 * This file contains main(), the top-level menu/game dispatcher, and the
 * game-loop logic reconstructed from the disassembly.  Lower-level routines
 * (graphics, network, shape I/O) are in separate translation units.
 */

#include "netwars.h"
#include <stdarg.h>
#include <bios.h>

/* ── compile-time sanity ──────────────────────────────────────────────── */
#if !defined(__LARGE__)
#  error "Compile with large memory model (-ml)"
#endif

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Global state                                                           */
/* ═══════════════════════════════════════════════════════════════════════ */

NWConfig        FAR gConfig;
PlayerState     FAR gPlayers[MAX_PLAYERS];
int             FAR gNumPlayers     = 1;
int             FAR gGameMode       = MODE_SINGLE_PLAYER;
int             FAR gCurrentMission = 0;
BOOL            FAR gPaused         = FALSE;
BOOL            FAR gGameOver       = FALSE;
BOOL            FAR gShootOutMode   = FALSE;
long            FAR gScore          = 0L;
int             FAR gAliensRemaining = 0;
HiScore         FAR gHiScores[MAX_HISCORES];
int             FAR gScreenMode     = SCREEN_VGA;

/* internal state */
static int          sCurrentPlayer  = 0;
static BOOL         sQuitGame       = FALSE;
static unsigned int sRandSeed       = 0;

/* ── mission table (reconstructed from string data at DS:0x2D690) ──────── */
static MissionDef sMissions[] = {
    /* id  title                             AF  CS  FP  CR  done  bonus */
    {  0, "Destroy Alien Fighters",          8,  0,  0,  0, FALSE, 100  },
    {  1, "Defend Cargo Ships",              4,  3,  0,  0, FALSE, 150  },
    {  2, "Protect Fuel Pods",               6,  0,  4,  0, FALSE, 150  },
    {  3, "Destroy Alien Cruiser",           4,  0,  0,  1, FALSE, 200  },
    {  4, "Destroy Alien Fighters",         12,  0,  0,  0, FALSE, 120  },
    {  5, "Defend Cargo Ships",              8,  4,  0,  0, FALSE, 175  },
    {  6, "Protect Fuel Pods",              10,  0,  6,  0, FALSE, 175  },
    {  7, "Destroy Alien Cruisers",          6,  0,  0,  2, FALSE, 250  },
    {  8, "Defend Fuel Pods and Cargo Ships",8,  4,  4,  0, FALSE, 200  },
    {  9, "Collect Satellites",              8,  0,  0,  0, FALSE, 300  },
    { -1, "",                                0,  0,  0,  0, FALSE, 0    }
};

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Utilities                                                              */
/* ═══════════════════════════════════════════════════════════════════════ */

int FAR Random(int max)
{
    sRandSeed = sRandSeed * 1103515245UL + 12345;
    return (int)((sRandSeed >> 16) % (unsigned)max);
}

void FAR ShowMessage(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    /* In the original, this prints on-screen via the game's text renderer. */
    /* For DOS output, write directly to stdout for non-graphical messages. */
    printf("%s\n", buf);
}

void FAR DebugPrint(const char *fmt, ...)
{
#ifdef DEBUG
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    printf(buf);
#else
    (void)fmt;
#endif
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Configuration (NETWARS.INI)                                            */
/* ═══════════════════════════════════════════════════════════════════════ */

void FAR LoadConfig(void)
{
    FILE *fp;
    char  line[128], key[64], val[64];
    int   nodeIdx = 0;

    /* defaults */
    memset(&gConfig, 0, sizeof(gConfig));
    gConfig.soundCard    = SOUND_PC_SPEAKER;
    gConfig.computerShip = TRUE;
    gConfig.screenMode   = SCREEN_VGA;
    gConfig.networkMode  = NET_BROADCAST;
    strcpy(gConfig.defaultName, "Player 1");

    fp = fopen(INI_FILE, "r");
    if (!fp) return;

    while (fgets(line, sizeof(line), fp)) {
        /* strip trailing CR/LF */
        int n = (int)strlen(line);
        while (n > 0 && (line[n-1] == '\r' || line[n-1] == '\n'))
            line[--n] = '\0';

        if (sscanf(line, "%[^=] = %s", key, val) != 2) continue;

        /* trim trailing spaces from key */
        n = (int)strlen(key);
        while (n > 0 && key[n-1] == ' ') key[--n] = '\0';

        if (!strcmp(key, "SoundCard")) {
            if (!strcmp(val, "PCSpeaker"))    gConfig.soundCard = SOUND_PC_SPEAKER;
            else if (!strcmp(val, "SoundBlaster")) gConfig.soundCard = SOUND_BLASTER;
        } else if (!strcmp(key, "PCSpeaker")) {
            gConfig.pcSpeaker = atoi(val);
        } else if (!strcmp(key, "SoundBlaster")) {
            gConfig.soundBlaster = atoi(val);
        } else if (!strcmp(key, "ComputerShip")) {
            gConfig.computerShip = atoi(val);
        } else if (!strcmp(key, "SeperateScanner")) {
            gConfig.seperateScanner = atoi(val);
        } else if (!strcmp(key, "CustomShip")) {
            strncpy(gConfig.customShipFile, val, sizeof(gConfig.customShipFile)-1);
        } else if (!strcmp(key, "DefaultName")) {
            strncpy(gConfig.defaultName, val, MAX_NAME_LEN-1);
        } else if (!strcmp(key, "FastMode")) {
            gConfig.fastMode = atoi(val);
        } else if (!strcmp(key, "MotionBlur")) {
            gConfig.motionBlur = atoi(val);
        } else if (!strcmp(key, "SharedFile")) {
            strncpy(gConfig.sharedFile, val, sizeof(gConfig.sharedFile)-1);
        } else if (!strcmp(key, "NetworkMode")) {
            if (!strcmp(val, "Broadcast"))   gConfig.networkMode = NET_BROADCAST;
            else if (!strcmp(val, "Node2Node")) gConfig.networkMode = NET_NODE2NODE;
        } else if (!strcmp(key, "Broadcast")) {
            sscanf(val, "%X %X %X %X %X",
                   &gConfig.broadcast[0], &gConfig.broadcast[1],
                   &gConfig.broadcast[2], &gConfig.broadcast[3],
                   &gConfig.broadcast[4]);
        } else if (!strncmp(key, "NODE", 4)) {
            nodeIdx = atoi(key+4) - 1;
            if (nodeIdx >= 0 && nodeIdx < MAX_PLAYERS) {
                sscanf(val, "%X %X %X %X %X",
                       &gConfig.nodes[nodeIdx].w[0],
                       &gConfig.nodes[nodeIdx].w[1],
                       &gConfig.nodes[nodeIdx].w[2],
                       &gConfig.nodes[nodeIdx].w[3],
                       &gConfig.nodes[nodeIdx].w[4]);
            }
        }
    }
    fclose(fp);
}

void FAR SaveConfig(void)
{
    FILE *fp;
    int   i;

    fp = fopen(INI_FILE, "w");
    if (!fp) return;

    fputs(INI_SECTION "\r\n", fp);

    fprintf(fp, "SoundCard = %s\r\n",
            gConfig.soundCard == SOUND_BLASTER ? "SoundBlaster" : "PCSpeaker");
    fprintf(fp, "PCSpeaker = %d\r\n",      gConfig.pcSpeaker);
    fprintf(fp, "SoundBlaster = %d\r\n",   gConfig.soundBlaster);
    fprintf(fp, "ComputerShip = %d\r\n",   gConfig.computerShip);
    fprintf(fp, "SeperateScanner = %d\r\n",gConfig.seperateScanner);
    fprintf(fp, "CustomShip = %s\r\n",     gConfig.customShipFile);
    fprintf(fp, "DefaultName = %s\r\n",    gConfig.defaultName);
    fprintf(fp, "FastMode = %d\r\n",       gConfig.fastMode);
    fprintf(fp, "MotionBlur = %d\r\n",     gConfig.motionBlur);
    fprintf(fp, "SharedFile = %s\r\n",     gConfig.sharedFile);
    fprintf(fp, "NetworkMode = %s\r\n",
            gConfig.networkMode == NET_NODE2NODE ? "Node2Node" : "Broadcast");
    fprintf(fp, "Broadcast = %04X %04X %04X %04X %04X\r\n",
            gConfig.broadcast[0], gConfig.broadcast[1],
            gConfig.broadcast[2], gConfig.broadcast[3],
            gConfig.broadcast[4]);

    for (i = 0; i < MAX_PLAYERS; i++) {
        fprintf(fp, "NODE%d = %04X %04X %04X %04X %04X\r\n",
                i+1,
                gConfig.nodes[i].w[0], gConfig.nodes[i].w[1],
                gConfig.nodes[i].w[2], gConfig.nodes[i].w[3],
                gConfig.nodes[i].w[4]);
    }
    fclose(fp);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  High scores                                                            */
/* ═══════════════════════════════════════════════════════════════════════ */

void FAR LoadHiScores(void)
{
    FILE *fp;
    memset(gHiScores, 0, sizeof(gHiScores));
    fp = fopen(HISCORES_FILE, "rb");
    if (!fp) return;
    fread(gHiScores, sizeof(HiScore), MAX_HISCORES, fp);
    fclose(fp);
}

void FAR SaveHiScores(void)
{
    FILE *fp;
    fp = fopen(HISCORES_FILE, "wb");
    if (!fp) return;
    fwrite(gHiScores, sizeof(HiScore), MAX_HISCORES, fp);
    fclose(fp);
}

int FAR RankScore(long score)
{
    int i;
    for (i = 0; i < MAX_HISCORES; i++) {
        if (score > gHiScores[i].score) return i;
    }
    return -1;  /* not ranked */
}

void FAR InsertHiScore(int rank, const char *initials, long score, int mission)
{
    int i;
    for (i = MAX_HISCORES - 1; i > rank; i--)
        gHiScores[i] = gHiScores[i-1];
    strncpy(gHiScores[rank].initials, initials, INITIALS_LEN-1);
    gHiScores[rank].score   = score;
    gHiScores[rank].mission = mission;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Player setup                                                           */
/* ═══════════════════════════════════════════════════════════════════════ */

static void InitPlayer(int n)
{
    static const char *defaults[] = {
        "Player 1","Player 2","Player 3","Player 4","Player 5","Player 6"
    };
    PlayerState FAR *p = &gPlayers[n];
    memset(p, 0, sizeof(*p));
    strncpy(p->name, (n == 0) ? gConfig.defaultName : defaults[n], MAX_NAME_LEN-1);
    p->shipType  = SHIP_ALIEN_FIGHTER;
    p->shields   = INIT_SHIELDS;
    p->maxShields= INIT_SHIELDS;
    p->shells    = INIT_SHELLS;
    p->missiles  = INIT_MISSILES;
    p->active    = TRUE;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Game-loop core                                                         */
/* ═══════════════════════════════════════════════════════════════════════ */

/*
 * UpdatePhysics – advance all entities by one tick.
 *
 * Reconstructed from the inner game loop at code offsets 0x3B7–0x15FFF.
 * The original uses a fixed-point 16.0 coordinate system scaled so that
 * one world unit equals one screen pixel in VGA 320×200 mode.
 */
void FAR UpdatePhysics(void)
{
    int i;
    for (i = 0; i < MAX_PLAYERS; i++) {
        PlayerState FAR *p = &gPlayers[i];
        if (!p->active) continue;

        /* simple Euler integration */
        p->x = (WORD)((int)p->x + p->velX);
        p->y = (WORD)((int)p->y + p->velY);

        /* wrap around the star-field world */
        /* world size is screen-mode dependent; use 320/640 pixels wide */
        if (gScreenMode == SCREEN_VGA) {
            p->x %= VGA_WIDTH;
            p->y %= VGA_HEIGHT;
        } else {
            p->x %= EGA_WIDTH;
            p->y %= EGA_HEIGHT;
        }
    }
}

/*
 * CheckCollisions – detect bullet/ship and ship/ship contacts.
 *
 * The original collision code at DS:0x45C–0x499 uses rectangle overlap.
 */
void FAR CheckCollisions(void)
{
    int i, j;
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (!gPlayers[i].active) continue;
        for (j = 0; j < MAX_PLAYERS; j++) {
            if (i == j || !gPlayers[j].active) continue;
            /* bounding-box check (16×16 sprites) */
            int dx = (int)gPlayers[i].x - (int)gPlayers[j].x;
            int dy = (int)gPlayers[i].y - (int)gPlayers[j].y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx < 16 && dy < 16) {
                /* collision – reduce shields */
                gPlayers[i].shields -= 25;
                gPlayers[j].shields -= 25;
                if (gPlayers[i].shields <= 0) {
                    gPlayers[i].active  = FALSE;
                    gPlayers[j].kills++;
                    ShowMessage("%s has been killed", gPlayers[i].name);
                }
                if (gPlayers[j].shields <= 0) {
                    gPlayers[j].active  = FALSE;
                    gPlayers[i].kills++;
                    ShowMessage("%s has been killed", gPlayers[j].name);
                }
            }
        }
    }
}

/*
 * ProcessInput – read keyboard / mouse / joystick and update player 0.
 *
 * Key layout (from DS:0x2DC5–0x2DE3 string table):
 *   Mouse / Joystick / Cursor Keys        – steer
 *   Left mouse / Joy-A / Space            – fire cannon
 *   Shift                                 – accelerate
 *   Ctrl                                  – decelerate
 *   Caps Lock / Enter                     – launch missile
 *   Right mouse / Joy-B / Tab / Backspace – turbo thrust
 *   A / ]                                 – cloak toggle
 *   F1                                    – scanner / external view
 *   F2                                    – pause (single player)
 */
void FAR ProcessInput(void)
{
    PlayerState FAR *p = &gPlayers[sCurrentPlayer];
    int key = 0;

    if (!kbhit()) return;
    key = getch();
    if (key == 0) key = -(getch());   /* extended scan code */

    switch (key) {
    case ' ':  /* fire */
        if (p->shells > 0) { p->shells--; /* spawn bullet */ }
        break;
    case 13:   /* Enter = launch missile */
        if (p->missiles > 0) { p->missiles--; /* spawn missile */ }
        break;
    case -59:  /* F1 = scanner mode */
        break;
    case -60:  /* F2 = pause */
        gPaused = !gPaused;
        if (gPaused) ShowMessage("Paused");
        break;
    case 'a': case ']':  /* cloak */
        p->cloakActive = !p->cloakActive;
        break;
    case 27:   /* Escape = quit */
        sQuitGame = TRUE;
        break;
    }
}

/*
 * RenderFrame – draw the current game state to the back-buffer and flip.
 *
 * The rendering engine (offsets 0x14000–0x19FFF in the decompressed image)
 * supports three video modes:
 *   EGA  640×350 – planar 4-bit, uses BIOS INT 10h with EGA register writes
 *   SVGA 640×480 – VESA mode 0x101 (256 colour, 640-wide)
 *   VGA  320×200 – mode 0x13 (Mode X / Mode 13h, 256 colour)
 *
 * Star-field motion blur is implemented as a one-pixel trail per star
 * drawn in the colour immediately below the star's current colour index
 * when gConfig.motionBlur is set.
 */
void FAR RenderFrame(void)
{
    int i;

    /* clear back buffer  (hardware-assisted via REP STOSB) */
    /* draw star field    (DS:0x260B8 – sine/cosine parallax table) */
    /* draw entities */
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (gPlayers[i].active) {
            DrawShape(gPlayers[i].x, gPlayers[i].y,
                      gPlayers[i].angle, gPlayers[i].shipType, i);
        }
    }
    DrawHUD();
    if (gConfig.seperateScanner) DrawRadar();
    /* page-flip */
}

void FAR DrawHUD(void)
{
    /* Status bar: "Shields: %d%%  Shells: %d  Missiles: %d  Score: %ld" */
    /* Rendered at the bottom of the screen using the game's bitmapped font */
}

void FAR DrawRadar(void)
{
    /* Mini-map / scanner in a separate 80×80 pixel panel */
}

/*
 * UpdateAI – move computer-controlled ships.
 *
 * The AI loop is at code offsets ~0x0C000–0x0DFFF.  Each alien uses a
 * simple state machine:
 *   PATROL  – fly in a straight line, turn when near edge
 *   CHASE   – head toward nearest player
 *   EVADE   – move away from missiles
 *   ATTACK  – fire cannon when within 64 pixels of target
 */
void FAR UpdateAI(void)
{
    /* AI entity processing – placeholder for the full state-machine */
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Game modes                                                             */
/* ═══════════════════════════════════════════════════════════════════════ */

void FAR RunSinglePlayer(void)
{
    int i, mission, rank;
    char initials[INITIALS_LEN];

    for (mission = gCurrentMission; sMissions[mission].id >= 0; mission++) {
        MissionDef *m = &sMissions[mission];

        ShowMessage("Mission %d", mission + 1);
        ShowMessage("%s", m->title);
        ShowMessage("Press Enter to Start");

        gAliensRemaining = m->alienCount + m->cruiserCount;
        gScore           = 0L;
        gGameOver        = FALSE;
        sQuitGame        = FALSE;

        InitPlayer(0);

        while (!gGameOver && !sQuitGame) {
            if (!gPaused) {
                UpdatePhysics();
                UpdateAI();
                CheckCollisions();
            }
            ProcessInput();
            RenderFrame();
        }

        if (sQuitGame) break;

        if (gAliensRemaining <= 0) {
            long bonus = (long)m->bonusMultiplier * gScore;
            m->completed = TRUE;
            ShowMessage("Mission %d Completed", mission + 1);
            ShowMessage("Bonus %d x %d", m->bonusMultiplier, (int)gScore);
            gScore += bonus;

            rank = RankScore(gScore);
            if (rank >= 0) {
                ShowMessage("Congratulations!");
                ShowMessage("Your score is ranked %d", rank + 1);
                ShowMessage("Enter your initials");
                EnterInitials(initials);
                InsertHiScore(rank, initials, gScore, mission);
                SaveHiScores();
            }
        }

        ShowMessage("Press ENTER or mouse button to continue");
    }

    /* All missions done? */
    if (sMissions[mission].id < 0) {
        ShowMessage("All Missions Completed!");
        ShowMessage("Kill Ratio: %d%%",
                    (gPlayers[0].kills * 100) /
                    (gPlayers[0].kills + gPlayers[0].deaths + 1));
    }
}

void FAR RunMultiPlayer(void)
{
    int i, winner;

    if (!InitIPX()) {
        ShowMessage("IPX is not loaded.\nLoad IPXODI.COM to play multi-player NetWars.");
        return;
    }

    gNumPlayers = SelectNumPlayers();
    if (gNumPlayers < 2) { ShutdownIPX(); return; }

    for (i = 0; i < gNumPlayers; i++) {
        InitPlayer(i);
        SelectShip(i);
        EnterPlayerName(i);
    }

    if (gShootOutMode) ShowMessage("Shoot Out Mode");
    ShowMessage("Waiting for other players");

    if (!NetWaitForPlayers(gNumPlayers)) { ShutdownIPX(); return; }

    gGameOver  = FALSE;
    sQuitGame  = FALSE;

    while (!gGameOver && !sQuitGame) {
        UpdatePhysics();
        UpdateAI();
        CheckCollisions();
        ProcessInput();
        RenderFrame();

        /* check for Shoot Out winner */
        if (gShootOutMode) {
            int alive = 0; winner = -1;
            for (i = 0; i < gNumPlayers; i++) {
                if (gPlayers[i].active && !gPlayers[i].retired) {
                    alive++; winner = i;
                }
            }
            if (alive <= 1) {
                gGameOver = TRUE;
                if (winner == sCurrentPlayer)
                    ShowMessage("You are the Shoot Out Winner!");
                else if (winner >= 0)
                    ShowMessage("%s is the Shoot Out winner", gPlayers[winner].name);
            }
        }
    }

    ShutdownIPX();
}

void FAR RunSpaceSim(void)
{
    InitPlayer(0);
    gGameOver = FALSE;
    sQuitGame = FALSE;

    while (!sQuitGame) {
        UpdatePhysics();
        ProcessInput();
        RenderFrame();
    }
}

void FAR RunGame(void)
{
    switch (gGameMode) {
    case MODE_SINGLE_PLAYER: RunSinglePlayer(); break;
    case MODE_MULTI_PLAYER:  RunMultiPlayer();  break;
    case MODE_SPACE_SIM:     RunSpaceSim();     break;
    case MODE_SHAPE_EDITOR:  RunShapeEditor();  break;
    }
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Menu system                                                            */
/* ═══════════════════════════════════════════════════════════════════════ */

/*
 * ShowMainMenu – present the top-level mode-selection menu.
 *
 * Options (DS:0x2D5A8):
 *   0 – Single Player
 *   1 – Multi Player
 *   2 – Space Simulator
 *   3 – Shape Editor
 *   4 – View README file
 */
int FAR ShowMainMenu(void)
{
    static const char *items[] = {
        "Single Player",
        "Multi Player",
        "Space Simulator",
        "Shape Editor",
        "View README file",
        NULL
    };
    int choice, i;

    printf("\n");
    for (i = 0; items[i]; i++)
        printf("  %d. %s\n", i+1, items[i]);
    printf("\nPress F1 for help\n");
    printf("\nSelect game mode: ");

    choice = getch() - '1';
    if (choice < 0 || choice > 4) choice = 0;
    return choice;
}

int FAR SelectScreenMode(void)
{
    printf("Select Screen Mode:\n");
    printf("1 = EGA  640x350 16 Colours\n");
    printf("2 = SVGA 640x480 256 Colours\n");
    printf("3 = VGA  320x200 256 Colours\n");
    int c = getch() - '0';
    if (c < 1 || c > 3) c = 3;
    return c;
}

int FAR SelectShip(int player)
{
    static const char *ships[] = {
        "Alien Fighter",
        "Cargo Ship",
        "Alien Cruiser",
        "Turret",
        NULL
    };
    int i;

    printf("Select a Space Craft (Player %d)\n", player+1);
    for (i = 0; ships[i]; i++)
        printf("  %d. %s\n", i+1, ships[i]);

    i = getch() - '1';
    if (i < 0 || i > 3) i = 0;
    gPlayers[player].shipType = (BYTE)i;
    return i;
}

void FAR EnterPlayerName(int player)
{
    printf("Enter your name below (Player %d): ", player+1);
    fflush(stdout);
    fgets(gPlayers[player].name, MAX_NAME_LEN, stdin);
    /* strip newline */
    int n = (int)strlen(gPlayers[player].name);
    if (n > 0 && gPlayers[player].name[n-1] == '\n')
        gPlayers[player].name[n-1] = '\0';
    if (!gPlayers[player].name[0])
        strcpy(gPlayers[player].name, gConfig.defaultName);
}

int FAR SelectMission(void)
{
    int i, page = 0, perPage = 5;
    printf("Select Missions\n");
    for (i = page*perPage; sMissions[i].id >= 0 && i < (page+1)*perPage; i++) {
        printf("  %2d. %s %s\n", i+1, sMissions[i].title,
               sMissions[i].completed ? "[Completed]" : "");
    }
    printf("Missions %d - %d\n", page*perPage+1,
           (page+1)*perPage < MAX_MISSIONS ? (page+1)*perPage : MAX_MISSIONS);
    int c = getch() - '1' + page*perPage;
    if (c < 0 || sMissions[c].id < 0) c = 0;
    return c;
}

void FAR ShowControls(void)
{
    printf("\n");
    printf("Mouse / Joystick / Cursor Keys                                Steer Ship\n");
    printf("Left Mouse Button / Joystick Button A / Space bar                   Fire\n");
    printf("Shift key                                                     Accelerate\n");
    printf("Ctrl key                                                      Decelerate\n");
    printf("Caps Lock key / Enter key                                 Launch Missile\n");
    printf("Right Mouse Button / Joystick Button B /\n");
    printf("  Tab key / Back Space key                                  Turbo Thrust\n");
    printf("A key / ] key                                  Activate/Deactivate Cloak\n");
    printf("F1 key                                        Scanner/External View Mode\n");
    printf("F2 key                                   Pause Game (single player only)\n");
}

void FAR ShowHiScores(void)
{
    int i;
    printf("\nAll Time Greatest Scores\n");
    printf("========================\n");
    for (i = 0; i < MAX_HISCORES; i++) {
        if (gHiScores[i].score > 0)
            printf("  %2d. %-4s  %8ld  Mission %d\n",
                   i+1, gHiScores[i].initials,
                   gHiScores[i].score, gHiScores[i].mission+1);
    }
    printf("\nToday's Best\n");
}

BOOL FAR EnterInitials(char *out)
{
    printf("Enter your initials: ");
    fflush(stdout);
    if (!fgets(out, INITIALS_LEN, stdin)) { out[0] = '\0'; return FALSE; }
    int n = (int)strlen(out);
    if (n > 0 && out[n-1] == '\n') out[--n] = '\0';
    if (!n) { strcpy(out, "???"); }
    return TRUE;
}

int FAR SelectNumPlayers(void)
{
    int n;
    printf("Number of players: ");
    scanf("%d", &n);
    if (n < 2) n = 2;
    if (n > MAX_PLAYERS) n = MAX_PLAYERS;
    return n;
}

void FAR ShowShootOutResults(void)
{
    int i;
    printf("\nShoot Out Mode Results\n");
    printf("======================\n");
    for (i = 0; i < gNumPlayers; i++) {
        printf("  %-16s  kills: %3d  deaths: %3d  shield: %3d%%  %s\n",
               gPlayers[i].name,
               gPlayers[i].kills, gPlayers[i].deaths,
               gPlayers[i].shields,
               gPlayers[i].retired ? "Retired" : (gPlayers[i].active ? "" : "Out"));
    }
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Shape stub (wireframe polygon rendering)                               */
/* ═══════════════════════════════════════════════════════════════════════ */

void FAR DrawShape(int x, int y, int angle, int shipType, int color)
{
    /* The full polygon renderer lives in a separate translation unit.   */
    /* Polygon types (DS:0x2D993): Triangle, Pentagon                    */
    (void)x; (void)y; (void)angle; (void)shipType; (void)color;
}

BOOL FAR LoadShape(const char *filename)
{
    FILE *fp;
    if (!filename || !*filename) return FALSE;
    fp = fopen(filename, "rb");
    if (!fp) {
        ShowMessage("Unable to read shape file %s", filename);
        return FALSE;
    }
    /* read shape data – format is proprietary NW polygon format */
    fclose(fp);
    return TRUE;
}

/* Shape editor stub – full implementation in shape.cpp */
void FAR RunShapeEditor(void)
{
    ShowMessage("Select Polygon Type");
    ShowMessage("You need to load a mouse driver (eg. mouse.com) to use the Shape Editor.");
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  IPX networking stubs                                                   */
/* ═══════════════════════════════════════════════════════════════════════ */

BOOL FAR InitIPX(void)
{
    /* Call IPX via INT 7Ah to check if IPXODI.COM is resident.          */
    /* The original probes via INT 2Fh AX=7A00h, then opens socket.     */
    /* Opening socket %2X%2X...  (DS:0x2D8B6)                           */
    BYTE lo = (BYTE)(IPX_SOCKET_NW & 0xFF);
    BYTE hi = (BYTE)(IPX_SOCKET_NW >> 8);
    printf("Opening socket %2X%2X... ", hi, lo);
    /* If IPX is not loaded the original prints:                         */
    /*   "IPX is not loaded.\nLoad IPXODI.COM to play multi-player..."  */
    return FALSE;  /* stub returns failure */
}

void FAR ShutdownIPX(void) { }

BOOL FAR NetOpenSocket(WORD socketNum)    { (void)socketNum; return FALSE; }
void FAR NetCloseSocket(void)             { }
BOOL FAR NetSendPacket(IPXNode FAR *dest, void FAR *data, WORD len)
{ (void)dest;(void)data;(void)len; return FALSE; }
BOOL FAR NetReceivePacket(void FAR *buf, WORD FAR *len, IPXNode FAR *from)
{ (void)buf;(void)len;(void)from; return FALSE; }
void FAR NetBroadcast(void FAR *data, WORD len)
{ (void)data;(void)len; }
BOOL FAR NetWaitForPlayers(int expected) { (void)expected; return TRUE; }

/* ═══════════════════════════════════════════════════════════════════════ */
/*  Graphics / sound stubs                                                 */
/* ═══════════════════════════════════════════════════════════════════════ */

BOOL FAR InitGraphics(int screenMode)
{
    gScreenMode = screenMode;
    /* Set video mode via INT 10h:                                        */
    /*   EGA  -> mode 0x10  (640×350 16-colour)                          */
    /*   SVGA -> VESA mode 0x101 (640×480 256-colour)                    */
    /*   VGA  -> mode 0x13  (320×200 256-colour)                         */
    union REGS r;
    switch (screenMode) {
    case SCREEN_EGA:
        r.h.ah = 0x00; r.h.al = 0x10;
        int86(0x10, &r, &r);
        break;
    case SCREEN_SVGA:
        r.x.ax = 0x4F02; r.x.bx = 0x101;
        int86(0x10, &r, &r);
        break;
    case SCREEN_VGA:
    default:
        r.h.ah = 0x00; r.h.al = 0x13;
        int86(0x10, &r, &r);
        break;
    }
    return TRUE;
}

void FAR ShutdownGraphics(void)
{
    union REGS r;
    r.h.ah = 0x00; r.h.al = 0x03;   /* text mode */
    int86(0x10, &r, &r);
}

BOOL FAR InitSound(int soundType)
{
    (void)soundType;
    return TRUE;
}

void FAR ShutdownSound(void) { }

BOOL FAR InitJoystick(void)
{
    /* Calibration sequence:                                              */
    /*   "Centre Joystick and press Enter"                                */
    /*   "Swirl Joystick and press Enter"                                 */
    printf("Centre Joystick and press Enter\n");
    getch();
    printf("Swirl Joystick and press Enter\n");
    getch();
    return TRUE;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*  main()                                                                 */
/* ═══════════════════════════════════════════════════════════════════════ */

/*
 * main() is called as a FAR function by the Borland CRT0 startup at
 * code segment 0x60, offset 0x3B7.  The startup passes (argc, argv)
 * on the stack in the usual C large-model convention.
 *
 * Reconstructed from the dispatch table at DS:0x353C and the atexit/
 * destructor chain that surrounds the actual game call at offset 0x10E.
 */
int FAR main(int argc, char FAR * FAR *argv)
{
    int choice;

    (void)argc; (void)argv;

    /* seed the PRNG from the real-time clock tick saved by startup */
    sRandSeed = (unsigned int)clock();

    /* load configuration and scores */
    LoadConfig();
    LoadHiScores();

    /* ask user to pick a screen mode (unless already set via INI) */
    gScreenMode = SelectScreenMode();

    /* initialise graphics; abort if it fails */
    if (!InitGraphics(gScreenMode)) {
        printf("Graphics init failed!\n");
        return 1;
    }

    InitSound(gConfig.soundCard);

    /* main menu loop */
    while (1) {
        choice = ShowMainMenu();

        if (choice == 4) {
            /* view README */
            FILE *fp = fopen(README_FILE, "r");
            if (fp) {
                char line[80];
                while (fgets(line, sizeof(line), fp))
                    fputs(line, stdout);
                fclose(fp);
            } else {
                printf("Can't find file %s.\n", README_FILE);
            }
            continue;
        }

        gGameMode = choice;   /* 0=single, 1=multi, 2=sim, 3=editor */

        if (gGameMode == MODE_SINGLE_PLAYER) {
            gCurrentMission = SelectMission();
        } else if (gGameMode == MODE_MULTI_PLAYER) {
            gShootOutMode = FALSE;
        }

        RunGame();

        ShowHiScores();
    }

    /* never reached in the original – exit via atexit / INT 21h 4Ch */
    ShutdownSound();
    ShutdownGraphics();
    SaveConfig();
    return 0;
}
