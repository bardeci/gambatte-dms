#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>

#include <gambatte.h>
#include "src/blitterwrapper.h"
#include "builddate.h"

#include "libmenu.h"
#include "sfont_gameboy.h"

#include <string.h>
#include <string>
#include <locale>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>

#include "src/audiosink.h"

static SDL_Surface *screen;
static SFont_Font* font;
static SFont_Font* fpsfont;
static SDL_Surface *font_bitmap_surface = NULL;
static SDL_Surface *fpsfont_bitmap_surface = NULL;
static SDL_RWops *RWops;

#ifdef ROM_BROWSER
#ifdef VERSION_OPENDINGUX
static std::string gamedir = ("/media/data/roms");
#elif VERSION_RETROFW
static std::string gamedir = (homedir + "/roms");
#elif defined VERSION_BITTBOY || defined VERSION_POCKETGO
static std::string gamedir = (homedir + "/roms");
#else
static std::string gamedir = (homedir + "/roms");
#endif
#endif

gambatte::GB *gambatte_p;
BlitterWrapper *blitter_p;

void init_globals(gambatte::GB *gambatte, BlitterWrapper *blitter){
    blitter_p = blitter;
    gambatte_p = gambatte;
}

int init_fps_font() {

    SDL_FreeSurface(fpsfont_bitmap_surface);
    RWops = SDL_RWFromMem(sfont_gameboy_fps, 1234);
    fpsfont_bitmap_surface = IMG_LoadPNG_RW(RWops);
    SDL_FreeRW(RWops);
    if (!fpsfont_bitmap_surface) {
        fprintf(stderr, "fps: font load error\n");
        exit(1);
    }
    fpsfont = SFont_InitFont(fpsfont_bitmap_surface);
    if (!fpsfont) {
        fprintf(stderr, "fps: font init error\n");
        exit(1);
    }
    
    return 0;
}

int init_menu() {
    
    SDL_FreeSurface(font_bitmap_surface);
	RWops = SDL_RWFromMem(sfont_gameboy_black, 895);
    font_bitmap_surface = IMG_LoadPNG_RW(RWops);
    SDL_FreeRW(RWops);
    if (!font_bitmap_surface) {
        fprintf(stderr, "menu: font load error\n");
        exit(1);
    }
    font = SFont_InitFont(font_bitmap_surface);
    if (!font) {
        fprintf(stderr, "menu: font init error\n");
        exit(1);
    }

	libmenu_set_font(font);
    
	return 0;
}

void menu_set_screen(SDL_Surface *set_screen) {
	screen = set_screen;
	libmenu_set_screen(screen);
}

void show_fps(SDL_Surface *surface, int fps) {
    char buffer[8];
    sprintf(buffer, "%d", fps);
    if (showfps) {
        SFont_Write(surface, fpsfont, 0, 0, buffer);
    }
}

std::string numtohextext(int num){
    std::locale loc;
    char buffer[4];
    std::string result;
    sprintf(buffer, "%x", num);

    result = std::string(buffer);
    result = std::toupper(buffer[0],loc);

    return result;
}

static int parse_ext_pal(const struct dirent *dir) {
    if(!dir){
        return 0;
    }

    if(dir->d_type == DT_REG) {
        const char *ext = strrchr(dir->d_name,'.');
        if((!ext) || (ext == dir->d_name)) {
            return 0;
        } else {
            if(strcmp(ext, ".pal") == 0){
                return 1;
            }
        }
    }
    return 0;
}

static int parse_ext_fil(const struct dirent *dir) {
    if(!dir){
        return 0;
    }

    if(dir->d_type == DT_REG) {
        const char *ext = strrchr(dir->d_name,'.');
        if((!ext) || (ext == dir->d_name)) {
            return 0;
        } else {
            if(strcmp(ext, ".fil") == 0){
                return 1;
            }
        }
    }
    return 0;
}

static int parse_ext_png(const struct dirent *dir) {
    if(!dir){
        return 0;
    }

    if(dir->d_type == DT_REG) {
        const char *ext = strrchr(dir->d_name,'.');
        if((!ext) || (ext == dir->d_name)) {
            return 0;
        } else {
            if(strcmp(ext, ".png") == 0){
                return 1;
            }
        }
    }
    return 0;
}

#ifdef ROM_BROWSER
static int parse_ext_zip_gb_gbc(const struct dirent *dir) {
    if(!dir){
        return 0;
    }

    if(dir->d_type == DT_REG) {
        const char *ext = strrchr(dir->d_name,'.');
        if((!ext) || (ext == dir->d_name)) {
            return 0;
        } else {
            if((strcmp(ext, ".zip") == 0) || (strcmp(ext, ".gb") == 0) || (strcmp(ext, ".gbc") == 0)){
                return 1;
            }
        }
    }
    return 0;
}
#endif

/* ============================ MAIN MENU =========================== */

#ifdef ROM_BROWSER
static void callback_loadgame(menu_t *caller_menu);
#endif
static void callback_selectstateload(menu_t *caller_menu);
static void callback_selectstatesave(menu_t *caller_menu);
static void callback_restart(menu_t *caller_menu);
static void callback_settings(menu_t *caller_menu);
static void callback_cheats(menu_t *caller_menu);
static void callback_about(menu_t *caller_menu);
static void callback_quit(menu_t *caller_menu);
static void callback_return(menu_t *caller_menu);

std::string menu_main_title = ("GAMBATTE-DMS");

void main_menu_with_anim() {//create a single menu frame to make the entry animation

    menu_t *menu;
    menu_entry_t *menu_entry;
    
    menu = new_menu();
    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Main Menu");

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Load state");
    menu_add_entry(menu, menu_entry);

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Save state");
    menu_add_entry(menu, menu_entry);

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Cheat Codes");
    menu_add_entry(menu, menu_entry);

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Reset game");
    menu_add_entry(menu, menu_entry);

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
#ifdef ROM_BROWSER
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Load Game");
    menu_add_entry(menu, menu_entry);
#endif 
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Settings");
    menu_add_entry(menu, menu_entry);

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Credits");
    menu_add_entry(menu, menu_entry);
#ifdef POWEROFF
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Shutdown");
    menu_add_entry(menu, menu_entry);
#else
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Quit");
    menu_add_entry(menu, menu_entry);
#endif
    

    menuin = 0;
    menu_drawmenuframe(menu);
    delete_menu(menu);
}

void main_menu() {

    //switchToMenuAudio();

    SDL_EnableKeyRepeat(250, 83);
    forcemenuexit = 0;

    menu_t *menu;
    menu_entry_t *menu_entry;
    
    menu = new_menu();
    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Main Menu");
    menu->back_callback = callback_return;

    if(gambatte_p->isLoaded()){
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Load state");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectstateload;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Save state");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectstatesave;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Cheat Codes");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_cheats;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Reset game");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_restart;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "");
        menu_add_entry(menu, menu_entry);
        menu_entry->selectable = 0;
    }

#ifdef ROM_BROWSER
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Load Game");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_loadgame;
#endif    
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Settings");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_settings;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Credits");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_about;
#ifdef POWEROFF
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Shutdown");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_quit;
#else
    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Quit");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_quit;
#endif
    
    switchToMenuAudio();
    menu_main(menu);
    switchToEmulatorAudio();
    delete_menu(menu);
    SDL_EnableKeyRepeat(0, 100);
}

static void callback_quit(menu_t *caller_menu) {
    playMenuSound_ok();
    SDL_Delay(500);
    printf("exiting...\n");
    forcemenuexit = 0;
    gambatte_p->saveSavedata();
    caller_menu->quit = 1;
    SDL_Quit();
#ifdef POWEROFF
    system("poweroff");
#endif
    exit(0);

}

static void callback_return(menu_t *caller_menu) {
    //playMenuSound_back();
    //SDL_Delay(208);
    if(gambatte_p->isLoaded()){
        forcemenuexit = 0;
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_back(menu_t *caller_menu) {
    playMenuSound_back();
    caller_menu->quit = 1;
}

static void callback_restart(menu_t *caller_menu) {
    playMenuSound_ok();
    SDL_Delay(250);
    if(can_reset == 1){//boot logo already ended, can reset game safely
        gambatte_p->reset();
        printOverlay("Reset ok");//print overlay text
    } else if (can_reset == 0){//boot logo is still running, can't reset game safely
        printOverlay("Unable to reset");//print overlay text
    }
    menuout = 0;
    caller_menu->quit = 1;
}

#ifdef ROM_BROWSER

/* ==================== LOAD GAME MENU ================================ */

static void callback_loaddmggame(menu_t *caller_menu);
static void callback_loadgbcgame(menu_t *caller_menu);

static void callback_loadgame(menu_t *caller_menu) {
    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();
        
    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Select System");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Gameboy");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_loaddmggame;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Gameboy Color");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_loadgbcgame;
    
    playMenuSound_in();
    menu_main(menu);
    
    delete_menu(menu);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

/* ==================== LOAD DMG GAME MENU =========================== */

struct dirent **gamelist = NULL;
int numgames;

static void callback_selecteddmggame(menu_t *caller_menu);

static void callback_loaddmggame(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Select Game");
    menu->back_callback = callback_back;

    std::string romdir = (gamedir + "/gb");
    numgames = scandir(romdir.c_str(), &gamelist, parse_ext_zip_gb_gbc, alphasort);
    if (numgames <= 0) {
        printf("scandir for %s failed.\n", romdir.c_str());
    } else {
        for (int i = 0; i < numgames; ++i){
            menu_entry = new_menu_entry(0);
            menu_entry_set_text_no_ext(menu_entry, gamelist[i]->d_name);
            menu_add_entry(menu, menu_entry);
            menu_entry->callback = callback_selecteddmggame;
        }
    }

    menu->selected_entry = 0; 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    for (int i = 0; i < numgames; ++i){
        free(gamelist[i]);
    }
    free(gamelist);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_selecteddmggame(menu_t *caller_menu) {
    playMenuSound_ok();
    SDL_Delay(250);
    gamename = gamelist[caller_menu->selected_entry]->d_name;
    currgamename = strip_Extension(gamename);
    loadConfig();
    std::string fullgamepath = (gamedir + "/gb/");
    fullgamepath += (gamename);
    if (gambatte_p->load(fullgamepath.c_str(),0 + 0 + 0, prefercgb) < 0) {
        printf("failed to load ROM: %s\n", fullgamepath.c_str());
    } else {
        clearAllCheats(); //clear all cheatcodes from menus
        if(gambatte_p->isCgb()){
            gameiscgb = 1;
            loadFilter(filtername);
        } else {
            gameiscgb = 0;
            loadPalette(palname);
        }
        printOverlay("Game loaded");//print overlay text
        firstframe = 0; //reset the frame counter
        forcemenuexit = 2;
        caller_menu->quit = 1;
    }
}

/* ==================== LOAD GBC GAME MENU =========================== */

static void callback_selectedgbcgame(menu_t *caller_menu);

static void callback_loadgbcgame(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Select Game");
    menu->back_callback = callback_back;

    std::string romdir = (gamedir + "/gbc");
    numgames = scandir(romdir.c_str(), &gamelist, parse_ext_zip_gb_gbc, alphasort);
    if (numgames <= 0) {
        printf("scandir for %s failed.\n", romdir.c_str());
    } else {
        for (int i = 0; i < numgames; ++i){
            menu_entry = new_menu_entry(0);
            menu_entry_set_text_no_ext(menu_entry, gamelist[i]->d_name);
            menu_add_entry(menu, menu_entry);
            menu_entry->callback = callback_selectedgbcgame;
        }
    }

    menu->selected_entry = 0; 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    for (int i = 0; i < numgames; ++i){
        free(gamelist[i]);
    }
    free(gamelist);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_selectedgbcgame(menu_t *caller_menu) {
    playMenuSound_ok();
    SDL_Delay(250);
    gamename = gamelist[caller_menu->selected_entry]->d_name;
    currgamename = strip_Extension(gamename);
    loadConfig();
    std::string fullgamepath = (gamedir + "/gbc/");
    fullgamepath += (gamename);
    if (gambatte_p->load(fullgamepath.c_str(),0 + 0 + 0, prefercgb) < 0) {
        printf("failed to load ROM: %s\n", fullgamepath.c_str());
    } else {
        clearAllCheats(); //clear all cheatcodes from menus
        if(gambatte_p->isCgb()){
            gameiscgb = 1;
            loadFilter(filtername);
        } else {
            gameiscgb = 0;
            loadPalette(palname);
        }
        printOverlay("Game loaded");//print overlay text
        firstframe = 0; //reset the frame counter
        forcemenuexit = 2;
        caller_menu->quit = 1;
    }
}
#endif

/* ==================== SELECT STATE MENU (LOAD) =========================== */

static void callback_selectedstateload(menu_t *caller_menu);

static void callback_selectstateload(menu_t *caller_menu) {
    #define N_STATES 10
    menu_t *menu;
	menu_entry_t *menu_entry;
    int i;
    char buffer[64];
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Load State");
	menu->back_callback = callback_back;
	
    for (i = 0; i < N_STATES; i++) {
        menu_entry = new_menu_entry(0);
        sprintf(buffer, "State %d", i);
        menu_entry_set_text(menu_entry, buffer);
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedstateload;
    }
    menu->selected_entry = gambatte_p->currentState();
    
    playMenuSound_in();
	menu_main(menu);
    
    delete_menu(menu);

    if(forcemenuexit > 0) {
    	menuout = 0;
    	caller_menu->quit = 1;
    }
}

static void callback_selectedstateload(menu_t *caller_menu) {
	gambatte_p->selectState_NoOsd(caller_menu->selected_entry);
	playMenuSound_ok();
    SDL_Delay(250);
    char overlaytext[20];
	if(gambatte_p->loadState_NoOsd()){
        can_reset = 1;//allow user to reset or save state once a savestate is loaded
        sprintf(overlaytext, "State %d loaded", gambatte_p->currentState());
        printOverlay(overlaytext);//print overlay text
    } else {
        sprintf(overlaytext, "State %d empty", gambatte_p->currentState());
        printOverlay(overlaytext);//print overlay text
    }
    forcemenuexit = 2;
    caller_menu->quit = 1;
}


/* ==================== SELECT STATE MENU (SAVE) =========================== */

static void callback_selectedstatesave(menu_t *caller_menu);

static void callback_selectstatesave(menu_t *caller_menu) {
    #define N_STATES 10
    menu_t *menu;
	menu_entry_t *menu_entry;
    int i;
    char buffer[64];
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Save State");
	menu->back_callback = callback_back;
	
    for (i = 0; i < N_STATES; i++) {
        menu_entry = new_menu_entry(0);
        sprintf(buffer, "State %d", i);
        menu_entry_set_text(menu_entry, buffer);
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedstatesave;
    }
    menu->selected_entry = gambatte_p->currentState();
    
    playMenuSound_in();
	menu_main(menu);
    
    delete_menu(menu);

    if(forcemenuexit > 0) {
    	menuout = 0;
    	caller_menu->quit = 1;
    }
}

static void callback_selectedstatesave(menu_t *caller_menu) {
	gambatte_p->selectState_NoOsd(caller_menu->selected_entry);
	playMenuSound_ok();
    SDL_Delay(250);
    if(can_reset == 1){//boot logo already ended, can save state safely
        if(gameiscgb == 0){ //set palette to greyscale
            Uint32 value;
            for (int i = 0; i < 3; ++i) {
                for (int k = 0; k < 4; ++k) {
                    if(k == 0)
                        value = 0xF8FCF8;
                    if(k == 1)
                        value = 0xA8A8A8;
                    if(k == 2)
                        value = 0x505450;
                    if(k == 3)
                        value = 0x000000;
                    gambatte_p->setDmgPaletteColor(i, k, value);
                }
            }
        } else { // disable color filter
            gambatte_p->setColorFilter(0, filtervalue);
        }    
        //run the emulator for 1 frame, so the screen buffer is updated without color palettes
        std::size_t fakesamples = 35112;
        Array<Uint32> const fakeBuf(35112 + 2064);
        gambatte_p->runFor(blitter_p->inBuf().pixels, blitter_p->inBuf().pitch, fakeBuf, fakesamples);
        //save state. the snapshot will now be in greyscale
        gambatte_p->saveState_NoOsd(blitter_p->inBuf().pixels, blitter_p->inBuf().pitch);
        if(gameiscgb == 0){
            loadPalette(palname); //restore palette
        } else {
            loadFilter(filtername); //restore color filter
        }

        char overlaytext[14];
        sprintf(overlaytext, "State %d saved", gambatte_p->currentState());
        printOverlay(overlaytext);//print overlay text
    } else if (can_reset == 0){//boot logo is still running, can't save state
        printOverlay("Unable to save");//print overlay text
    }
    forcemenuexit = 2;
    caller_menu->quit = 1;
}

/* ==================== SETTINGS MENU ================================ */

static void callback_showfps(menu_t *caller_menu);
static void callback_scaler(menu_t *caller_menu);
static void callback_dmgpalette(menu_t *caller_menu);
static void callback_colorfilter(menu_t *caller_menu);
static void callback_dmgborderimage(menu_t *caller_menu);
static void callback_gbcborderimage(menu_t *caller_menu);
static void callback_system(menu_t *caller_menu);
static void callback_usebios(menu_t *caller_menu);
static void callback_ghosting(menu_t *caller_menu);
static void callback_controls(menu_t *caller_menu);
static void callback_sound(menu_t *caller_menu);

static void callback_saveconfig_confirm(menu_t *caller_menu);
static void callback_saveconfig_apply(menu_t *caller_menu);
static void callback_saveconfig_apply_back(menu_t *caller_menu);

static void callback_pergame_settings(menu_t *caller_menu);

static void callback_settings(menu_t *caller_menu) {
    menu_t *menu;
	menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Settings");
	menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Show FPS");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_showfps;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Scaler");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_scaler;

    if (gameiscgb == 0) { // Display only the DMG relevant options.
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Mono Palette");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_dmgpalette;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "DMG Border");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_dmgborderimage;
    } else if (gameiscgb == 1) { // Display only the GBC relevant options.
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Color Filter");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_colorfilter;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "GBC Border");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_gbcborderimage;
    }

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "System");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_system;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Boot logos");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_usebios;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Ghosting");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_ghosting;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Controls");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_controls;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Sound");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_sound;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Save as default");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_saveconfig_confirm;

    if(gambatte_p->isLoaded()){
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Per-Game settings");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_pergame_settings;
    }

	playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
    	menuout = 0;
    	caller_menu->quit = 1;
    }
}

static void callback_saveconfig_confirm(menu_t *caller_menu) {

	menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, " Settings ");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Save default");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_apply;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "settings?");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_apply;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
    	forcemenuexit = 0;
    	caller_menu->selected_entry = 0;
    }
}

static void callback_saveconfig_apply(menu_t *caller_menu) {

    playMenuSound_ok();
    saveConfig();

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, " Settings ");
    menu->back_callback = callback_saveconfig_apply_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Done!");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_apply_back;

    menu_message(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
    	menuout = 0;
    	caller_menu->quit = 1;
    }
}

static void callback_saveconfig_apply_back(menu_t *caller_menu) {
	forcemenuexit = 2;
    caller_menu->quit = 1;
}

/* ==================== PER-GAME SETTINGS MENU ======================== */

static void callback_saveconfig_pergame_confirm(menu_t *caller_menu);
static void callback_saveconfig_pergame_apply(menu_t *caller_menu);
static void callback_saveconfig_pergame_apply_back(menu_t *caller_menu);

static void callback_deleteconfig_pergame_confirm(menu_t *caller_menu);
static void callback_deleteconfig_pergame_apply(menu_t *caller_menu);
static void callback_deleteconfig_pergame_apply_back(menu_t *caller_menu);

static void callback_pergame_settings(menu_t *caller_menu) {
    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Per-Game Settings");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Save settings");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_saveconfig_pergame_confirm;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Delete settings");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_deleteconfig_pergame_confirm;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
        forcemenuexit = 0;
        caller_menu->selected_entry = 0;
    }
}

static void callback_saveconfig_pergame_confirm(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, " Per-Game Settings ");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Save settings");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_pergame_apply;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "for the current");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_pergame_apply;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "game?");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_pergame_apply;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_saveconfig_pergame_apply(menu_t *caller_menu) {

    playMenuSound_ok();
    saveConfig(1);

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, " Per-Game Settings ");
    menu->back_callback = callback_saveconfig_pergame_apply_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Done!");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_saveconfig_pergame_apply_back;

    menu_message(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_saveconfig_pergame_apply_back(menu_t *caller_menu) {
    forcemenuexit = 2;
    caller_menu->quit = 1;
}

static void callback_deleteconfig_pergame_confirm(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "  Per-Game Settings  ");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Delete settings");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_deleteconfig_pergame_apply;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "for the current");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_deleteconfig_pergame_apply;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "game?");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_deleteconfig_pergame_apply;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_deleteconfig_pergame_apply(menu_t *caller_menu) {

    playMenuSound_ok();
    deleteConfig(); // delete custom config
    loadConfig(); // load default config
    refreshkeys = 1;                  //
    if(gameiscgb == 0){               //
        loadPalette(palname);         //
        load_border(dmgbordername);   //
    } else if(gameiscgb == 1){        // apply loaded default config
        loadFilter(filtername);       //
        load_border(gbcbordername);   //
    }                                 //
    blitter_p->setScreenRes();        //

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "  Per-Game Settings  ");
    menu->back_callback = callback_deleteconfig_pergame_apply_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Done!");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_deleteconfig_pergame_apply_back;

    menu_message(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

static void callback_deleteconfig_pergame_apply_back(menu_t *caller_menu) {
    forcemenuexit = 2;
    caller_menu->quit = 1;
}

/* ==================== SHOW FPS MENU =========================== */

static void callback_selectedshowfps(menu_t *caller_menu);

static void callback_showfps(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Show FPS");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "OFF");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedshowfps;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "ON");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedshowfps;

    menu->selected_entry = showfps; 

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedshowfps(menu_t *caller_menu) {
    playMenuSound_ok();
    showfps = caller_menu->selected_entry;
    caller_menu->quit = 1;
}

/* ==================== SCALER MENU =========================== */

static void callback_selectedscaler(menu_t *caller_menu);

static void callback_scaler(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Scaler");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "No Scaling");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "1.50x Fast");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "1.50x Smooth");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "1.66x Fast");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "1.66x Smooth");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "FullScreen Fast");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "FullScreen Smooth");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedscaler;

    if (ipuscaling != "NONE") {
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Hw 1.25x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Hw 1.36x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Hw 1.50x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Hw 1.66x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Hw FullScreen");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;
#ifdef OGA_SCREEN
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Dot Matrix 2x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "CRT 2x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;
#elif VGA_SCREEN
        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Dot Matrix 3x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "CRT 3x");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "CRT FullScreen");
        menu_add_entry(menu, menu_entry);
        menu_entry->callback = callback_selectedscaler;
#endif
    }

    menu->selected_entry = selectedscaler; 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedscaler(menu_t *caller_menu) {
    playMenuSound_ok();
    selectedscaler = caller_menu->selected_entry;
    if(gameiscgb == 0){
        loadPalette(palname);
        load_border(dmgbordername);
    } else if(gameiscgb == 1){
        load_border(gbcbordername);
    }
    blitter_p->setScreenRes(); /* switch to selected resolution */
    clean_menu_screen(caller_menu);
    
    caller_menu->quit = 0;
}

/* ==================== DMG PALETTE MENU =========================== */

struct dirent **palettelist = NULL;
int numpalettes;

static void callback_nopalette(menu_t *caller_menu);
static void callback_defaultpalette(menu_t *caller_menu);
static void callback_autopalette(menu_t *caller_menu);
static void callback_selectedpalette(menu_t *caller_menu);

static void callback_dmgpalette(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Mono Palette");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "No palette");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_nopalette;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Default");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_defaultpalette;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Auto");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_autopalette;

    std::string palettedir = (homedir + "/.gambatte/palettes");
    numpalettes = scandir(palettedir.c_str(), &palettelist, parse_ext_pal, alphasort);
    if (numpalettes <= 0) {
        printf("scandir for ./gambatte/palettes/ failed.");
    } else {
        for (int i = 0; i < numpalettes; ++i){
            menu_entry = new_menu_entry(0);
            menu_entry_set_text_no_ext(menu_entry, palettelist[i]->d_name);
            menu_add_entry(menu, menu_entry);
            menu_entry->callback = callback_selectedpalette;
        }
    }

    menu->selected_entry = currentEntryInList(menu, palname); 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    for (int i = 0; i < numpalettes; ++i){
        free(palettelist[i]);
    }
    free(palettelist);
}

static void callback_nopalette(menu_t *caller_menu) {
    playMenuSound_ok();
    palname = "NONE";
    loadPalette(palname);
    if(gameiscgb == 1){
        load_border(gbcbordername);
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        caller_menu->quit = 0;
    }
}

static void callback_defaultpalette(menu_t *caller_menu) {
    playMenuSound_ok();
    palname = "DEFAULT";
    loadPalette(palname);
    if(gameiscgb == 1){
        load_border(gbcbordername);
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        caller_menu->quit = 0;
    }
}

static void callback_autopalette(menu_t *caller_menu) {
    playMenuSound_ok();
    palname = "AUTO";
    loadPalette(palname);
    if(gameiscgb == 1){
        load_border(gbcbordername);
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        caller_menu->quit = 0;
    }
}

static void callback_selectedpalette(menu_t *caller_menu) {
    playMenuSound_ok();
    palname = palettelist[caller_menu->selected_entry - 3]->d_name; // we added 3 extra entries before the list, so we do (-3).
    loadPalette(palname);
    if(gameiscgb == 1){
        load_border(gbcbordername);
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        caller_menu->quit = 0;
    }
}

/* ==================== COLOR FILTER MENU =========================== */

struct dirent **filterlist = NULL;
int numfilters;

static void callback_nofilter(menu_t *caller_menu);
static void callback_defaultfilter(menu_t *caller_menu);
static void callback_selectedfilter(menu_t *caller_menu);

static void callback_colorfilter(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Color Filter");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "No filter");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_nofilter;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Default");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_defaultfilter;

    std::string filterdir = (homedir + "/.gambatte/filters");
    numfilters = scandir(filterdir.c_str(), &filterlist, parse_ext_fil, alphasort);
    if (numfilters <= 0) {
        printf("scandir for ./gambatte/filters/ failed.");
    } else {
        for (int i = 0; i < numfilters; ++i){
            menu_entry = new_menu_entry(0);
            menu_entry_set_text_no_ext(menu_entry, filterlist[i]->d_name);
            menu_add_entry(menu, menu_entry);
            menu_entry->callback = callback_selectedfilter;
        }
    }

    menu->selected_entry = currentEntryInList(menu, filtername); 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    for (int i = 0; i < numfilters; ++i){
        free(filterlist[i]);
    }
    free(filterlist);
}

static void callback_nofilter(menu_t *caller_menu) {
    playMenuSound_ok();
    filtername = "NONE";
    loadFilter(filtername);
    if(gameiscgb == 1){
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

static void callback_defaultfilter(menu_t *caller_menu) {
    playMenuSound_ok();
    filtername = "DEFAULT";
    loadFilter(filtername);
    if(gameiscgb == 1){
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

static void callback_selectedfilter(menu_t *caller_menu) {
    playMenuSound_ok();
    filtername = filterlist[caller_menu->selected_entry - 2]->d_name; // we added 2 extra entries before the list, so we do (-2).
    loadFilter(filtername);
    if(gameiscgb == 1){
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

/* ==================== DMG BORDER IMAGE MENU =========================== */

struct dirent **dmgborderlist = NULL;
int numdmgborders;

static void callback_nodmgborder(menu_t *caller_menu);
static void callback_defaultdmgborder(menu_t *caller_menu);
static void callback_autodmgborder(menu_t *caller_menu);
static void callback_selecteddmgborder(menu_t *caller_menu);

static void callback_dmgborderimage(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "DMG Border");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "No border");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_nodmgborder;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Default");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_defaultdmgborder;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Auto");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_autodmgborder;

    std::string borderdir = (homedir + "/.gambatte/borders");
    numdmgborders = scandir(borderdir.c_str(), &dmgborderlist, parse_ext_png, alphasort);
    if (numdmgborders <= 0) {
        printf("scandir for ./gambatte/borders/ failed.");
    } else {
        for (int i = 0; i < numdmgborders; ++i){
            menu_entry = new_menu_entry(0);
            menu_entry_set_text_no_ext(menu_entry, dmgborderlist[i]->d_name);
            menu_add_entry(menu, menu_entry);
            menu_entry->callback = callback_selecteddmgborder;
        }
    }

    menu->selected_entry = currentEntryInList(menu, dmgbordername); 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    for (int i = 0; i < numdmgborders; ++i){
        free(dmgborderlist[i]);
    }
    free(dmgborderlist);
}

static void callback_nodmgborder(menu_t *caller_menu) {
    playMenuSound_ok();
    dmgbordername = "NONE";
    if(gameiscgb == 1){
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    }
}

static void callback_defaultdmgborder(menu_t *caller_menu) {
    playMenuSound_ok();
    dmgbordername = "DEFAULT";
    if(gameiscgb == 1){
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    }
}

static void callback_autodmgborder(menu_t *caller_menu) {
    playMenuSound_ok();
    dmgbordername = "AUTO";
    if(gameiscgb == 1){
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    }
}

static void callback_selecteddmgborder(menu_t *caller_menu) {
    playMenuSound_ok();
    dmgbordername = dmgborderlist[caller_menu->selected_entry - 3]->d_name; // we added 3 extra entries before the list, so we do (-3).
    if(gameiscgb == 1){
        caller_menu->quit = 1;
    } else {
        load_border(dmgbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    }
}

/* ==================== GBC BORDER IMAGE MENU =========================== */

struct dirent **gbcborderlist = NULL;
int numgbcborders;

static void callback_nogbcborder(menu_t *caller_menu);
static void callback_defaultgbcborder(menu_t *caller_menu);
static void callback_autogbcborder(menu_t *caller_menu);
static void callback_selectedgbcborder(menu_t *caller_menu);

static void callback_gbcborderimage(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "GBC Border");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "No border");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_nogbcborder;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Default");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_defaultgbcborder;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Auto");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_autogbcborder;

    std::string borderdir = (homedir + "/.gambatte/borders");
    numgbcborders = scandir(borderdir.c_str(), &gbcborderlist, parse_ext_png, alphasort);
    if (numgbcborders <= 0) {
        printf("scandir for ./gambatte/borders/ failed.");
    } else {
        for (int i = 0; i < numgbcborders; ++i){
            menu_entry = new_menu_entry(0);
            menu_entry_set_text_no_ext(menu_entry, gbcborderlist[i]->d_name);
            menu_add_entry(menu, menu_entry);
            menu_entry->callback = callback_selectedgbcborder;
        }
    }

    menu->selected_entry = currentEntryInList(menu, gbcbordername); 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    for (int i = 0; i < numgbcborders; ++i){
        free(gbcborderlist[i]);
    }
    free(gbcborderlist);
}

static void callback_nogbcborder(menu_t *caller_menu) {
    playMenuSound_ok();
    gbcbordername = "NONE";
    if(gameiscgb == 1){
        load_border(gbcbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

static void callback_defaultgbcborder(menu_t *caller_menu) {
    playMenuSound_ok();
    gbcbordername = "DEFAULT";
    if(gameiscgb == 1){
        load_border(gbcbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

static void callback_autogbcborder(menu_t *caller_menu) {
    playMenuSound_ok();
    gbcbordername = "AUTO";
    if(gameiscgb == 1){
        load_border(gbcbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

static void callback_selectedgbcborder(menu_t *caller_menu) {
    playMenuSound_ok();
    gbcbordername = gbcborderlist[caller_menu->selected_entry - 3]->d_name; // we added 3 extra entries before the list, so we do (-3).
    if(gameiscgb == 1){
        load_border(gbcbordername);
        clean_menu_screen(caller_menu);
        caller_menu->quit = 0;
    } else {
        caller_menu->quit = 1;
    }
}

/* ==================== SYSTEM MENU =========================== */

static void callback_selectedsystem(menu_t *caller_menu);

static void callback_system(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "System");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Priority DMG");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedsystem;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Priority GBC");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedsystem;

    menu->selected_entry = prefercgb;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedsystem(menu_t *caller_menu) {
    playMenuSound_ok();
    prefercgb = caller_menu->selected_entry;
    caller_menu->quit = 1;
}

/* ==================== BOOT LOGOS MENU =========================== */

static void callback_selectedbios(menu_t *caller_menu);

static void callback_usebios(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Boot Logos");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "OFF");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedbios;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "ON");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedbios;

    menu->selected_entry = biosenabled; 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedbios(menu_t *caller_menu) {
    playMenuSound_ok();
    biosenabled = caller_menu->selected_entry;
    caller_menu->quit = 1;
}

/* ==================== GHOSTING MENU =========================== */

static void callback_selectedghosting(menu_t *caller_menu);

static void callback_ghosting(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Ghosting");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "OFF");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedghosting;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "DMG");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedghosting;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "GBC");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedghosting;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "ALL");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedghosting;

    menu->selected_entry = ghosting; 

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedghosting(menu_t *caller_menu) {
    playMenuSound_ok();
    ghosting = caller_menu->selected_entry;
    caller_menu->quit = 1;
}

/* ==================== CONTROLS MENU ================================ */

static void callback_buttonlayout(menu_t *caller_menu);
static void callback_ffwhotkey(menu_t *caller_menu);

static void callback_controls(menu_t *caller_menu) {
    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Controls");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Button Layout");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_buttonlayout;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Fast Forward");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_ffwhotkey;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
        menuout = 0;
        caller_menu->quit = 1;
    }
}

/* ==================== BUTTON LAYOUT MENU =========================== */

static void callback_selectedbuttonlayout(menu_t *caller_menu);

static void callback_buttonlayout(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Button Layout");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Default");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedbuttonlayout;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Alternate");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedbuttonlayout;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Alternate 2");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedbuttonlayout;

    menu->selected_entry = buttonlayout; 
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedbuttonlayout(menu_t *caller_menu) {
    playMenuSound_ok();
    buttonlayout = caller_menu->selected_entry;
    refreshkeys = 1;
    caller_menu->quit = 1;
}

/* ==================== FAST FORWARD MENU =========================== */

static void callback_selectedffwhotkey(menu_t *caller_menu);

static void callback_ffwhotkey(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Fast Forward");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Hotkey OFF");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedffwhotkey;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Hotkey HOLD");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedffwhotkey;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Hotkey TOGGLE");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedffwhotkey;

    menu->selected_entry = ffwhotkey;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedffwhotkey(menu_t *caller_menu) {
    playMenuSound_ok();
    ffwhotkey = caller_menu->selected_entry;
    caller_menu->quit = 1;
}

/* ==================== SOUND MENU =========================== */

static void callback_selectedsound(menu_t *caller_menu);

static void callback_sound(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Sound");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Mono");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedsound;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Stereo");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_selectedsound;

    menu->selected_entry = stereosound;

    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

static void callback_selectedsound(menu_t *caller_menu) {
    playMenuSound_ok();
    stereosound = caller_menu->selected_entry;
    caller_menu->quit = 1;
}

/* ==================== ABOUT MENU =========================== */

static void callback_about(menu_t *caller_menu) {

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Credits");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Gambatte");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "by Sindre Aamas");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Gambatte-DMS fork");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "by Hi-Ban");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Special thanks to");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Surkow, Senquack");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "and Pingflood");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "build version:");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, BUILDDATE);
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_back;
    
    playMenuSound_in();
    menu_main(menu);

    delete_menu(menu);
}

/* ==================== CHEATS MENU ================================ */

static void callback_gamegenie(menu_t *caller_menu);
static void callback_gameshark(menu_t *caller_menu);

static void callback_cheats(menu_t *caller_menu) {
    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();
        
    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Cheat Codes");
    menu->back_callback = callback_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Game Genie");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_gamegenie;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Game Shark");
    menu_add_entry(menu, menu_entry);
    menu_entry->callback = callback_gameshark;
    
    playMenuSound_in();
    menu_main(menu);
    
    delete_menu(menu);

    if(forcemenuexit > 0) {
    	menuout = 0;
    	caller_menu->quit = 1;
    }
}

/* ==================== GAME GENIE MENU ================================ */

static void callback_gamegenie_confirm(menu_t *caller_menu);
static void callback_gamegenie_apply(menu_t *caller_menu);
static void callback_gamegenie_apply_back(menu_t *caller_menu);
static void callback_gamegenie_edit(menu_t *caller_menu);
static void callback_gamegenie_back(menu_t *caller_menu);

static void callback_gamegenie(menu_t *caller_menu) {

    int i, j, offset, offset2;

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Game Genie");
    menu->back_callback = callback_gamegenie_back;

    for (i = 0; i < numcodes_gg; i++) {

        for (j = 0; j < 3; j++) {
            menu_entry = new_menu_entry(1);
            menu_entry_set_text(menu_entry, "");
            menu_add_entry(menu, menu_entry);
            menu_entry_add_entry(menu_entry, "0");
            menu_entry_add_entry(menu_entry, "1");
            menu_entry_add_entry(menu_entry, "2");
            menu_entry_add_entry(menu_entry, "3");
            menu_entry_add_entry(menu_entry, "4");
            menu_entry_add_entry(menu_entry, "5");
            menu_entry_add_entry(menu_entry, "6");
            menu_entry_add_entry(menu_entry, "7");
            menu_entry_add_entry(menu_entry, "8");
            menu_entry_add_entry(menu_entry, "9");
            menu_entry_add_entry(menu_entry, "A");
            menu_entry_add_entry(menu_entry, "B");
            menu_entry_add_entry(menu_entry, "C");
            menu_entry_add_entry(menu_entry, "D");
            menu_entry_add_entry(menu_entry, "E");
            menu_entry_add_entry(menu_entry, "F");
            menu_entry->selected_entry = 0;
            menu_entry->selectable = 2;
            menu_entry->callback = callback_gamegenie_edit;
        }

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "-");
        menu_add_entry(menu, menu_entry);
        menu_entry->selectable = 0;

        for (j = 0; j < 3; j++) {
            menu_entry = new_menu_entry(1);
            menu_entry_set_text(menu_entry, "");
            menu_add_entry(menu, menu_entry);
            menu_entry_add_entry(menu_entry, "0");
            menu_entry_add_entry(menu_entry, "1");
            menu_entry_add_entry(menu_entry, "2");
            menu_entry_add_entry(menu_entry, "3");
            menu_entry_add_entry(menu_entry, "4");
            menu_entry_add_entry(menu_entry, "5");
            menu_entry_add_entry(menu_entry, "6");
            menu_entry_add_entry(menu_entry, "7");
            menu_entry_add_entry(menu_entry, "8");
            menu_entry_add_entry(menu_entry, "9");
            menu_entry_add_entry(menu_entry, "A");
            menu_entry_add_entry(menu_entry, "B");
            menu_entry_add_entry(menu_entry, "C");
            menu_entry_add_entry(menu_entry, "D");
            menu_entry_add_entry(menu_entry, "E");
            menu_entry_add_entry(menu_entry, "F");
            menu_entry->selected_entry = 0;
            menu_entry->selectable = 2;
            menu_entry->callback = callback_gamegenie_edit;
        }

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "-");
        menu_add_entry(menu, menu_entry);
        menu_entry->selectable = 0;

        for (j = 0; j < 3; j++) {
            menu_entry = new_menu_entry(1);
            menu_entry_set_text(menu_entry, "");
            menu_add_entry(menu, menu_entry);
            menu_entry_add_entry(menu_entry, "0");
            menu_entry_add_entry(menu_entry, "1");
            menu_entry_add_entry(menu_entry, "2");
            menu_entry_add_entry(menu_entry, "3");
            menu_entry_add_entry(menu_entry, "4");
            menu_entry_add_entry(menu_entry, "5");
            menu_entry_add_entry(menu_entry, "6");
            menu_entry_add_entry(menu_entry, "7");
            menu_entry_add_entry(menu_entry, "8");
            menu_entry_add_entry(menu_entry, "9");
            menu_entry_add_entry(menu_entry, "A");
            menu_entry_add_entry(menu_entry, "B");
            menu_entry_add_entry(menu_entry, "C");
            menu_entry_add_entry(menu_entry, "D");
            menu_entry_add_entry(menu_entry, "E");
            menu_entry_add_entry(menu_entry, "F");
            menu_entry->selected_entry = 0;
            menu_entry->selectable = 2;
            menu_entry->callback = callback_gamegenie_edit;
        }
    }

    menu_entry->callback = callback_gamegenie_confirm; // set last entry callback to "confirm" function

    // get code values from stored
    for (i = 0; i < numcodes_gg; i++) {
        offset = 11 * i;
        offset2 = 9 * i;

        menu->entries[0 + offset]->selected_entry = ggcheats[0 + offset2];
        menu->entries[1 + offset]->selected_entry = ggcheats[1 + offset2];
        menu->entries[2 + offset]->selected_entry = ggcheats[2 + offset2];
        menu->entries[4 + offset]->selected_entry = ggcheats[3 + offset2];
        menu->entries[5 + offset]->selected_entry = ggcheats[4 + offset2];
        menu->entries[6 + offset]->selected_entry = ggcheats[5 + offset2];
        menu->entries[8 + offset]->selected_entry = ggcheats[6 + offset2];
        menu->entries[9 + offset]->selected_entry = ggcheats[7 + offset2];
        menu->entries[10 + offset]->selected_entry = ggcheats[8 + offset2];
    }
    
    playMenuSound_in();
    menu_cheat(menu);

    delete_menu(menu);
}

static void callback_gamegenie_confirm(menu_t *caller_menu) {

    if (editmode == 0){ //user pressed START, he wants to apply cheats

        menu_t *menu;
        menu_entry_t *menu_entry;
        (void) caller_menu;
        menu = new_menu();

        menu_set_header(menu, menu_main_title.c_str());
        menu_set_title(menu, " Game Genie ");
        menu->back_callback = callback_gamegenie_back;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, "Apply Codes?");
        menu_add_entry(menu, menu_entry);
        menu_entry->selectable = 0;
        menu_entry->callback = callback_gamegenie_apply;
        
        playMenuSound_in();
        menu_main(menu);

        delete_menu(menu);

        int i, offset, offset2;

        // after applying cheats the stored codes are cleared, so we must reload code values
        for (i = 0; i < numcodes_gg; i++) {
            offset = 11 * i;
            offset2 = 9 * i;

            caller_menu->entries[0 + offset]->selected_entry = ggcheats[0 + offset2];
            caller_menu->entries[1 + offset]->selected_entry = ggcheats[1 + offset2];
            caller_menu->entries[2 + offset]->selected_entry = ggcheats[2 + offset2];
            caller_menu->entries[4 + offset]->selected_entry = ggcheats[3 + offset2];
            caller_menu->entries[5 + offset]->selected_entry = ggcheats[4 + offset2];
            caller_menu->entries[6 + offset]->selected_entry = ggcheats[5 + offset2];
            caller_menu->entries[8 + offset]->selected_entry = ggcheats[6 + offset2];
            caller_menu->entries[9 + offset]->selected_entry = ggcheats[7 + offset2];
            caller_menu->entries[10 + offset]->selected_entry = ggcheats[8 + offset2]; 
        }

        if(forcemenuexit > 0) {
	    	forcemenuexit = 0;
        	caller_menu->selected_entry = 0;
	    }

    } else if (editmode == 1){ //user is in edit mode, he wants to exit edit mode

        callback_gamegenie_edit(caller_menu);
    }
}

static void callback_gamegenie_apply(menu_t *caller_menu) {

    std::string a1 = "0", a2 = "0", a3 = "0", a4 = "0", a5 = "0", a6 = "0", a7 = "0", a8 = "0", a9 = "0";
    int i, offset;
    std::string cheat_a = "", multicheat = "";

    for (i = 0; i < numcodes_gg; i++) {
        offset = 9 * i;

        // get code values from stored
        a1 = numtohextext(ggcheats[0 + offset]);
        a2 = numtohextext(ggcheats[1 + offset]);
        a3 = numtohextext(ggcheats[2 + offset]);
        a4 = numtohextext(ggcheats[3 + offset]);
        a5 = numtohextext(ggcheats[4 + offset]);
        a6 = numtohextext(ggcheats[5 + offset]);
        a7 = numtohextext(ggcheats[6 + offset]);
        a8 = numtohextext(ggcheats[7 + offset]);
        a9 = numtohextext(ggcheats[8 + offset]);

        if ((a7 == "0") && (a8 == "0") && (a9 == "0")){
            cheat_a = a1 + a2 + a3 + "-" + a4 + a5 + a6 + ";";
        } else {
            cheat_a = a1 + a2 + a3 + "-" + a4 + a5 + a6 + "-" + a7 + a8 + a9 + ";";
        }

        if ((cheat_a != "000-000-000;") && (cheat_a != "000-000;")){
            multicheat += cheat_a;
        } 
    }

    playMenuSound_ok();
    gambatte_p->setGameGenie(multicheat); // apply cheats

    // clear all codes after applying them
    for (i = 0; i < (numcodes_gg * 9); i++) {
        ggcheats[i] = 0;
    }

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();
        
    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, " Game Genie ");
    menu->back_callback = callback_gamegenie_apply_back;

    menu_entry = new_menu_entry(0);
    menu_entry_set_text(menu_entry, "Done!");
    menu_add_entry(menu, menu_entry);
    menu_entry->selectable = 0;
    menu_entry->callback = callback_gamegenie_apply_back;
    
    menu_message(menu);

    delete_menu(menu);

    if(forcemenuexit > 0) {
    	menuout = 0;
    	caller_menu->quit = 1;
    }
}

static void callback_gamegenie_apply_back(menu_t *caller_menu) {
	forcemenuexit = 2;
    caller_menu->quit = 1;
}

static void callback_gamegenie_edit(menu_t *caller_menu) {

    if(editmode == 0){ //enter edit mode

        playMenuSound_in();
        selectedcode = floor(caller_menu->selected_entry / 11);
        editmode = 1;

    } else if (editmode == 1){ //exit edit mode

        playMenuSound_ok();
        editmode = 0;
        int loop = 0;
        int i, offset, offset2;

        do {
            if ((caller_menu->selected_entry % 11 == 0) || (caller_menu->selected_entry == 0)){
                //do nothing
            } else {
                --caller_menu->selected_entry;
            }
            loop++;
        } while ((caller_menu->selected_entry % 11 != 0) && (loop < 11)); //go to first entry in that line

        // save code values
        for (i = 0; i < numcodes_gg; i++) {
            offset = 11 * i;
            offset2 = 9 * i; 

            ggcheats[0 + offset2] = caller_menu->entries[0 + offset]->selected_entry;
            ggcheats[1 + offset2] = caller_menu->entries[1 + offset]->selected_entry;
            ggcheats[2 + offset2] = caller_menu->entries[2 + offset]->selected_entry;
            ggcheats[3 + offset2] = caller_menu->entries[4 + offset]->selected_entry;
            ggcheats[4 + offset2] = caller_menu->entries[5 + offset]->selected_entry;
            ggcheats[5 + offset2] = caller_menu->entries[6 + offset]->selected_entry;
            ggcheats[6 + offset2] = caller_menu->entries[8 + offset]->selected_entry;
            ggcheats[7 + offset2] = caller_menu->entries[9 + offset]->selected_entry;
            ggcheats[8 + offset2] = caller_menu->entries[10 + offset]->selected_entry;
        }
    }

    caller_menu->quit = 0;
}

static void callback_gamegenie_back(menu_t *caller_menu) {

    if(editmode == 0){ // exit to previous menu screen

        playMenuSound_back();
        selectedcode = 0;

        caller_menu->quit = 1;

    } else if (editmode == 1){ // exit edit mode without saving changes

        playMenuSound_back();
        editmode = 0;
        int loop = 0;
        int i, offset, offset2;

        do {
            if ((caller_menu->selected_entry % 11 == 0) || (caller_menu->selected_entry == 0)){
                //do nothing
            } else {
                --caller_menu->selected_entry;
            }
            loop++;
        } while ((caller_menu->selected_entry % 11 != 0) && (loop < 11)); //go to first entry in that line

        // reload code values from stored
        for (i = 0; i < numcodes_gg; i++) {
            offset = 11 * i;
            offset2 = 9 * i;

            caller_menu->entries[0 + offset]->selected_entry = ggcheats[0 + offset2];
            caller_menu->entries[1 + offset]->selected_entry = ggcheats[1 + offset2];
            caller_menu->entries[2 + offset]->selected_entry = ggcheats[2 + offset2];
            caller_menu->entries[4 + offset]->selected_entry = ggcheats[3 + offset2];
            caller_menu->entries[5 + offset]->selected_entry = ggcheats[4 + offset2];
            caller_menu->entries[6 + offset]->selected_entry = ggcheats[5 + offset2];
            caller_menu->entries[8 + offset]->selected_entry = ggcheats[6 + offset2];
            caller_menu->entries[9 + offset]->selected_entry = ggcheats[7 + offset2];
            caller_menu->entries[10 + offset]->selected_entry = ggcheats[8 + offset2]; 
        }

        caller_menu->quit = 0;
    }
}

/* ==================== GAME SHARK MENU =========================== */

static void callback_gameshark_enabledisable(menu_t *caller_menu);
static void callback_gameshark_edit(menu_t *caller_menu);
static void callback_gameshark_back(menu_t *caller_menu);

static void callback_gameshark(menu_t *caller_menu) {

    int i, j, offset, offset2, enabled;

    menu_t *menu;
    menu_entry_t *menu_entry;
    (void) caller_menu;
    menu = new_menu();

    menu_set_header(menu, menu_main_title.c_str());
    menu_set_title(menu, "Game Shark");
    menu->back_callback = callback_gameshark_back;

    for (i = 0; i < numcodes_gs; i++) {

        enabled = gscheatsenabled[i];

        menu_entry = new_menu_entry(1);
        menu_entry_set_text(menu_entry, "");
        menu_add_entry(menu, menu_entry);
        menu_entry_add_entry(menu_entry, "[ ]");
        menu_entry_add_entry(menu_entry, "[~]");
        menu_entry->selected_entry = enabled;
        menu_entry->callback = callback_gameshark_enabledisable;

        menu_entry = new_menu_entry(0);
        menu_entry_set_text(menu_entry, " ");
        menu_add_entry(menu, menu_entry);
        menu_entry->selectable = 0;

        for (j = 0; j < 8; j++) {
            menu_entry = new_menu_entry(1);
            menu_entry_set_text(menu_entry, "");
            menu_add_entry(menu, menu_entry);
            menu_entry_add_entry(menu_entry, "0");
            menu_entry_add_entry(menu_entry, "1");
            menu_entry_add_entry(menu_entry, "2");
            menu_entry_add_entry(menu_entry, "3");
            menu_entry_add_entry(menu_entry, "4");
            menu_entry_add_entry(menu_entry, "5");
            menu_entry_add_entry(menu_entry, "6");
            menu_entry_add_entry(menu_entry, "7");
            menu_entry_add_entry(menu_entry, "8");
            menu_entry_add_entry(menu_entry, "9");
            menu_entry_add_entry(menu_entry, "A");
            menu_entry_add_entry(menu_entry, "B");
            menu_entry_add_entry(menu_entry, "C");
            menu_entry_add_entry(menu_entry, "D");
            menu_entry_add_entry(menu_entry, "E");
            menu_entry_add_entry(menu_entry, "F");
            menu_entry->selected_entry = 0;
            menu_entry->selectable = 2;
            menu_entry->callback = callback_gameshark_edit;
        }
    }

    // get code values from stored
    for (i = 0; i < numcodes_gs; i++) {
        offset = 10 * i;
        offset2 = 8 * i;

        menu->entries[0 + offset]->selected_entry = gscheatsenabled[0 + i];

        menu->entries[2 + offset]->selected_entry = gscheats[0 + offset2];
        menu->entries[3 + offset]->selected_entry = gscheats[1 + offset2];
        menu->entries[4 + offset]->selected_entry = gscheats[2 + offset2];
        menu->entries[5 + offset]->selected_entry = gscheats[3 + offset2];
        menu->entries[6 + offset]->selected_entry = gscheats[4 + offset2];
        menu->entries[7 + offset]->selected_entry = gscheats[5 + offset2];
        menu->entries[8 + offset]->selected_entry = gscheats[6 + offset2];
        menu->entries[9 + offset]->selected_entry = gscheats[7 + offset2]; 
    }
    
    playMenuSound_in();
    menu_cheat(menu);

    delete_menu(menu);
}

static void callback_gameshark_enabledisable(menu_t *caller_menu) {

    if (caller_menu->entries[caller_menu->selected_entry]->selected_entry == 0) {
        playMenuSound_in();
        caller_menu->entries[caller_menu->selected_entry]->selected_entry = 1;
    } else {
        playMenuSound_back();
        caller_menu->entries[caller_menu->selected_entry]->selected_entry = 0;
    }
    gscheatsenabled[(caller_menu->selected_entry / 10)] = caller_menu->entries[caller_menu->selected_entry]->selected_entry; //store the value

    caller_menu->quit = 0;
}

static void callback_gameshark_edit(menu_t *caller_menu) {

    if(editmode == 0){ //enter edit mode

        playMenuSound_in();
        selectedcode = floor(caller_menu->selected_entry / 10);
        editmode = 1;

    } else if (editmode == 1){ //exit edit mode

        playMenuSound_ok();
        editmode = 0;
        int loop = 0;
        int i, offset, offset2;

        do {
            if ((caller_menu->selected_entry % 10 == 0) || (caller_menu->selected_entry == 0)){
                //do nothing
            } else {
                --caller_menu->selected_entry;
            }
            loop++;
        } while ((caller_menu->selected_entry % 10 != 0) && (loop < 10)); //go to first entry in that line
        caller_menu->selected_entry += 2; //go to second entry in that line

        // save code values
        for (i = 0; i < numcodes_gs; i++) {
            offset = 10 * i;
            offset2 = 8 * i; 

            gscheats[0 + offset2] = caller_menu->entries[2 + offset]->selected_entry;
            gscheats[1 + offset2] = caller_menu->entries[3 + offset]->selected_entry;
            gscheats[2 + offset2] = caller_menu->entries[4 + offset]->selected_entry;
            gscheats[3 + offset2] = caller_menu->entries[5 + offset]->selected_entry;
            gscheats[4 + offset2] = caller_menu->entries[6 + offset]->selected_entry;
            gscheats[5 + offset2] = caller_menu->entries[7 + offset]->selected_entry;
            gscheats[6 + offset2] = caller_menu->entries[8 + offset]->selected_entry;
            gscheats[7 + offset2] = caller_menu->entries[9 + offset]->selected_entry;
        }
    }

    caller_menu->quit = 0;
}

static void callback_gameshark_back(menu_t *caller_menu) {

    if(editmode == 0){ // exit to previous menu screen and apply cheats

        playMenuSound_back();
        selectedcode = 0;
        std::string a1 = "0", a2 = "0", a3 = "0", a4 = "0", a5 = "0", a6 = "0", a7 = "0", a8 = "0";
        int i, offset, enabled;
        std::string cheat_a = "", multicheat = "";

        for (i = 0; i < numcodes_gs; i++) {
            offset = 8 * i;

            // get code values from stored
            a1 = numtohextext(gscheats[0 + offset]);
            a2 = numtohextext(gscheats[1 + offset]);
            a3 = numtohextext(gscheats[2 + offset]);
            a4 = numtohextext(gscheats[3 + offset]);
            a5 = numtohextext(gscheats[4 + offset]);
            a6 = numtohextext(gscheats[5 + offset]);
            a7 = numtohextext(gscheats[6 + offset]);
            a8 = numtohextext(gscheats[7 + offset]);

            cheat_a = a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + ";";
            enabled = gscheatsenabled[i];

            if ((cheat_a != "00000000;") && (enabled == 1)){
                multicheat += cheat_a;
            } 
        }

        gambatte_p->setGameShark(multicheat); // apply cheats

        caller_menu->quit = 1;

    } else if (editmode == 1){ // exit edit mode without saving changes

        playMenuSound_back();
        editmode = 0;
        int loop = 0;
        int i, offset, offset2;

        do {
            if ((caller_menu->selected_entry % 10 == 0) || (caller_menu->selected_entry == 0)){
                //do nothing
            } else {
                --caller_menu->selected_entry;
            }
            loop++;
        } while ((caller_menu->selected_entry % 10 != 0) && (loop < 10)); //go to first entry in that line
        caller_menu->selected_entry += 2; //go to second entry in that line

        // reload code values from stored
        for (i = 0; i < numcodes_gs; i++) {
            offset = 10 * i;
            offset2 = 8 * i;

            caller_menu->entries[0 + offset]->selected_entry = gscheatsenabled[0 + i];

            caller_menu->entries[2 + offset]->selected_entry = gscheats[0 + offset2];
            caller_menu->entries[3 + offset]->selected_entry = gscheats[1 + offset2];
            caller_menu->entries[4 + offset]->selected_entry = gscheats[2 + offset2];
            caller_menu->entries[5 + offset]->selected_entry = gscheats[3 + offset2];
            caller_menu->entries[6 + offset]->selected_entry = gscheats[4 + offset2];
            caller_menu->entries[7 + offset]->selected_entry = gscheats[5 + offset2];
            caller_menu->entries[8 + offset]->selected_entry = gscheats[6 + offset2];
            caller_menu->entries[9 + offset]->selected_entry = gscheats[7 + offset2]; 
        }
        caller_menu->quit = 0;
    }
}
