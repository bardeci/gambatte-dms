/* libmenu.h
 * code for generating simple menus
 * public domain
 * by abhoriel
 */
 
#ifndef _LIBMENU_H
#define _LIBMENU_H

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_GG_CODES 20
#define NUM_GS_CODES 20

#if defined VERSION_OPENDINGUX

	#define BLINK_SPEED 12
	#define FOOTER_ALT_SPEED 100
	#define TEXTANIM_DELAY 46
	#define TEXTANIM_SPEED 10

	#define KEYMAP_MENUCANCEL SDLK_LALT
	#define KEYMAP_MENUACCEPT SDLK_LCTRL

#elif defined VERSION_RETROFW

	#define BLINK_SPEED 8
	#define FOOTER_ALT_SPEED 80
	#define TEXTANIM_DELAY 36
	#define TEXTANIM_SPEED 6

	#define KEYMAP_MENUCANCEL SDLK_LALT
	#define KEYMAP_MENUACCEPT SDLK_LCTRL

#elif defined VERSION_BITTBOY

	#define BLINK_SPEED 16
	#define FOOTER_ALT_SPEED 156
	#define TEXTANIM_DELAY 66
	#define TEXTANIM_SPEED 13

	#define KEYMAP_MENUCANCEL SDLK_SPACE
	#define KEYMAP_MENUACCEPT SDLK_LCTRL

#elif defined VERSION_POCKETGO

	#define BLINK_SPEED 16
	#define FOOTER_ALT_SPEED 156
	#define TEXTANIM_DELAY 66
	#define TEXTANIM_SPEED 13

	#define KEYMAP_MENUCANCEL SDLK_LCTRL
	#define KEYMAP_MENUACCEPT SDLK_LALT

#else

	#define BLINK_SPEED 12
	#define FOOTER_ALT_SPEED 100
	#define TEXTANIM_DELAY 50
	#define TEXTANIM_SPEED 10

	#define KEYMAP_MENUCANCEL SDLK_LALT
	#define KEYMAP_MENUACCEPT SDLK_LCTRL

#endif

#ifdef MIYOO_BATTERY_WARNING
	#define BATTCHECKINTERVAL 60000
#endif

#ifdef BITTBOY_PROPER_BA_LAYOUT
	#undef KEYMAP_MENUCANCEL
	#undef KEYMAP_MENUACCEPT
	#define KEYMAP_MENUCANCEL SDLK_LCTRL
	#define KEYMAP_MENUACCEPT SDLK_LALT
#endif

#include <SDL/SDL.h>
#include <string.h>
#include <string>
#include "SFont.h"

typedef struct Menu_t menu_t;

typedef struct {
	char *text;
	char **entries;
	int is_shiftable;
	int selectable;
	int n_entries;
	int selected_entry;
	void (*callback)(menu_t *);
} menu_entry_t;

struct Menu_t {
	char *header;
	char *title;
	menu_entry_t **entries;
	int n_entries;
	int selected_entry;
	int i;
	int quit;
	void (*back_callback)(menu_t *);
};

extern SDL_Surface *menuscreen;
extern SDL_Surface *surface_menuinout;
extern SDL_Surface *textoverlay;
extern SDL_Surface *textoverlaycolored;
extern int selectedscaler, showfps, ghosting, biosenabled, colorfilter, gameiscgb, buttonlayout, stereosound, prefercgb, ffwhotkey;
extern uint32_t menupalblack, menupaldark, menupallight, menupalwhite;
extern int filtervalue[12];
extern std::string dmgbordername, gbcbordername, palname, filtername, currgamename, homedir, ipuscaling;
extern int numcodes_gg, numcodes_gs, selectedcode, editmode;
extern int ggcheats[NUM_GG_CODES*9];
extern int gscheats[NUM_GS_CODES*8];
extern int gscheatsenabled[NUM_GS_CODES];
extern int menuin, menuout, showoverlay, overlay_inout, is_using_bios, can_reset, forcemenuexit, refreshkeys, ffwdtoggle;
extern int firstframe;

#ifdef ROM_BROWSER
extern std::string gamename;
#endif



void libmenu_set_screen(SDL_Surface *set_screen);
void libmenu_set_font(SFont_Font *set_font);
int menu_drawmenuframe(menu_t *menu);
int menu_main(menu_t *menu);
void menu_message(menu_t *menu);
int menu_cheat(menu_t *menu);
int menu_cheattest(menu_t *menu);
void set_active_menu(menu_t *menu);
menu_t *new_menu();
void delete_menu(menu_t *menu);
void menu_set_header(menu_t *menu, const char *header);
void menu_set_title(menu_t *menu, const char *title);
void menu_add_entry(menu_t *menu, menu_entry_t *entry);
menu_entry_t *new_menu_entry(int is_shiftable);
void delete_menu_entry(menu_entry_t *entry);
void menu_entry_set_text(menu_entry_t *entry, const char *text);
void menu_entry_set_text_no_ext(menu_entry_t *entry, const char *text);
void menu_entry_add_entry(menu_entry_t *entry, const char* text);
void callback_menu_quit(menu_t *menu);
void set_menu_palette(uint32_t valwhite, uint32_t vallight, uint32_t valdark, uint32_t valblack);
void init_menusurfaces();
void free_menusurfaces();
void clean_menu_screen(menu_t *menu);
void paint_titlebar(SDL_Surface *surface);
void paint_titlebar_cheat();
void convert_bw_surface_colors(SDL_Surface *surface, SDL_Surface *surface2, const uint32_t repl_col_black, const uint32_t repl_col_dark, const uint32_t repl_col_light, const uint32_t repl_col_white, int mode);
void load_border(std::string borderfilename);
void paint_border(SDL_Surface *surface);
uint32_t convert_hexcolor(SDL_Surface *surface, const uint32_t color);
int currentEntryInList(menu_t *menu, std::string text);
void clear_surface(SDL_Surface *surface, Uint32 color);
void loadPalette(std::string palettefile);
void loadFilter(std::string filterfile);
void saveConfig(int pergame = 0);
void deleteConfig();
void loadConfig();

void openMenuAudio();
void closeMenuAudio();
void loadMenuSounds();
void freeMenuSounds();

int switchToMenuAudio();
void switchToEmulatorAudio();

void playMenuSound_in();
void playMenuSound_back();
void playMenuSound_move();
void playMenuSound_ok();

std::string getSaveStateFilename(int statenum);
std::string const strip_Extension(std::string const &str);
std::string const strip_Dir(std::string const &str);
void getSaveStatePreview(int statenum);
void printSaveStatePreview(SDL_Surface *surface);
void apply_cfilter(SDL_Surface *surface);
void printOverlay(const char *text);
void clearAllCheats();

#ifdef MIYOO_BATTERY_WARNING
void checkBatt();
#endif


#ifdef __cplusplus
}
#endif


#endif
