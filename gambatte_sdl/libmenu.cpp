/* libmenu.c
 * code for generating simple menus
 * public domain
 * by abhoriel
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <math.h>

#include "libmenu.h"
#include "SFont.h"
#include "menu.h"
#include "scaler.h"

#include "src/audiosink.h"
#include "menusounds.h"
#include "defaultborders.h"

#include <fstream>

static void display_menu(SDL_Surface *surface, menu_t *menu);
static void display_menu_cheat(SDL_Surface *surface, menu_t *menu);
static void redraw(menu_t *menu);
static void redraw_blank(menu_t *menu);
static void redraw_cheat(menu_t *menu);
static void invert_rect(SDL_Surface* surface, SDL_Rect *rect);
static void put_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
static Uint32 get_pixel(SDL_Surface *surface, int x, int y);

void load_border(std::string borderfilename);

static SDL_Surface *screen = NULL;
static SFont_Font* font = NULL;
static SDL_RWops *RWops;
static SDL_RWops *RWops1;
static SDL_RWops *RWops2;
static SDL_RWops *RWops3;
static SDL_RWops *RWops4;

SDL_Surface *menuscreen;
SDL_Surface *surface_menuinout;
SDL_Surface *statepreview;
SDL_Surface *textoverlay;
SDL_Surface *textoverlaycolored;
SDL_Surface *temp;

Mix_Chunk *menusound_in = NULL;
Mix_Chunk *menusound_back = NULL;
Mix_Chunk *menusound_move = NULL;
Mix_Chunk *menusound_ok = NULL;

// Default config values
int selectedscaler = 0, showfps = 0, ghosting = 1, biosenabled = 0, colorfilter = 0, gameiscgb = 0, buttonlayout = 0, stereosound = 0, prefercgb = 0, ffwhotkey = 1;
uint32_t menupalblack = 0x000000, menupaldark = 0x505450, menupallight = 0xA8A8A8, menupalwhite = 0xF8FCF8;
int filtervalue[12] = {135, 20, 0, 25, 0, 125, 20, 25, 0, 20, 105, 30};
std::string dmgbordername = "DEFAULT", gbcbordername = "DEFAULT", palname = "DEFAULT", filtername = "NONE", currgamename = "default";
std::string homedir = getenv("HOME");
std::string ipuscaling = "NONE";
int numcodes_gg = NUM_GG_CODES, numcodes_gs = NUM_GS_CODES, selectedcode = 0, editmode = 0, blink = 0, footer_alt = 0;
int textanim = 0, textanimpos = 0, textanimdirection = 0, textanimtimer = 0, textanimwidth = 0, textanimspeed = 0;
int ggcheats[NUM_GG_CODES *9] = {0};
int gscheats[NUM_GS_CODES *8] = {0};
int gscheatsenabled[NUM_GS_CODES] = {0};
int menuin = -1, menuout = -1, showoverlay = -1, overlay_inout = 0, is_using_bios = 0, can_reset = 1, forcemenuexit = 0, refreshkeys = 0, ffwdtoggle = 0;
int firstframe = 0;

#ifdef ROM_BROWSER
std::string gamename = "NONE";
#endif

#ifdef MIYOO_BATTERY_WARNING
uint32_t curr_batt_time = SDL_GetTicks();
uint32_t old_batt_time = curr_batt_time;
bool lowbattery = false;
#endif

std::string getSaveStateFilename(int statenum){
	std::string result = gambatte_p->getSaveStatePath(statenum);
	return result;
}

std::string const strip_Extension(std::string const &str) {
	std::string::size_type const lastDot = str.find_last_of('.');
	std::string::size_type const lastSlash = str.find_last_of('/');

	if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastSlash < lastDot))
		return str.substr(0, lastDot);

	return str;
}

std::string const strip_Dir(std::string const &str) {
	std::string::size_type const lastSlash = str.find_last_of('/');
	if (lastSlash != std::string::npos)
		return str.substr(lastSlash + 1);

	return str;
}

#ifdef MIYOO_BATTERY_WARNING
void saveBattLog(const char *text){
	std::string battlogfile = (homedir + "/.gambatte/batterylog.log");
	FILE * cfile;
    cfile = fopen(battlogfile.c_str(), "a");
    if (cfile == NULL) {
		printf("Failed to open log file for writing.\n");
		return;
	}
    if (fprintf(cfile,
		"%s\n",
		text) < 0) {
    	printf("Failed to write log file.\n");
    } else {
    	//printf("Log file successfully updated.\n");
    }
    fclose(cfile);
}

void checkBatt(){
	curr_batt_time = SDL_GetTicks();
	if ((curr_batt_time >= old_batt_time + BATTCHECKINTERVAL) || ((lowbattery == true) && (curr_batt_time >= old_batt_time + (BATTCHECKINTERVAL / 8)))) {
		old_batt_time = curr_batt_time;
		FILE *battery_file;
		FILE *status_file;
		char battlvl[8];
		char battstatus[16];
		int battery_level;

		battery_file = fopen("/sys/class/power_supply/miyoo-battery/voltage_now", "r");
		if (battery_file != NULL) {
			if ((fgets(battlvl,8,battery_file)) != NULL ) {
				battery_level = atoi(battlvl);
			} else {
				battery_level = 9999;
				sprintf(battlvl, "%d", battery_level);
			}
			fclose(battery_file);

			if (battery_level < 3300) {
				lowbattery = true;
				printOverlay("Low Battery!"); //low battery warning
				/*if (battery_level < 3000) {
				    printf("emergency shutdown!\n");
				    gambatte_p->saveSavedata();
				    SDL_Quit();
				    system("poweroff"); //emergency shutdown
				    exit(0);
				}*/
			} else {
				lowbattery = false;
			}
		}

		/*status_file = fopen("/sys/class/power_supply/miyoo-battery/batt_tune_intput_charge_current", "r");
		if (status_file != NULL) {
			if ((fgets(battstatus,16,status_file)) != NULL ) {
				//cool, there is content in the file
				printOverlay(battstatus);
			} else {
				//not cool, file is empty
			}
			fclose(status_file);
		} else {
			printOverlay("status not found");
		}*/

		//printOverlay(battstatus);
		//saveBattLog(battlvl); //save battery values in log file (FOR TESTING)
	}
}
#endif

void getSaveStatePreview(int statenum){
	uint32_t pixels[80 * 72];
	std::ifstream file(getSaveStateFilename(statenum).c_str(), std::ios_base::binary);
	if (file) {
		file.ignore(5);
		file.read(reinterpret_cast<char*>(pixels), sizeof pixels);
		SDL_FillRect(statepreview, NULL, 0xA0A0A0);
		memcpy((uint32_t*)statepreview->pixels, pixels, statepreview->h * statepreview->pitch );	
	} else {
		uint32_t hlcolor;
		if(gameiscgb == 1){
			hlcolor = SDL_MapRGB(statepreview->format, 232, 16, 16);
		} else {
			hlcolor = SDL_MapRGB(statepreview->format, 168, 168, 168);
		}
		SDL_FillRect(statepreview, NULL, hlcolor);
		SFont_WriteCenter(statepreview, font, 32, "No data");
	}
}

void printSaveStatePreview(SDL_Surface *surface, int posx, int posy){
	SFont_Write(surface, font, posx + 13, posy, "Preview");
	SDL_Rect rect;
	rect.x = posx;
    rect.y = posy;
    rect.w = 82;
    rect.h = 81;
    invert_rect(surface, &rect);
    rect.x = posx + 1;
    rect.y = posy + 8;
    rect.w = 80;
    rect.h = 72;
    SDL_BlitSurface(statepreview, NULL, surface, &rect);
}

void apply_cfilter(SDL_Surface *surface) { 
	uint8_t r_initial, g_initial, b_initial;
	uint16_t *src = (uint16_t*)surface->pixels;
	for (int y = 0; y < (surface->h * surface->w); y++)
	{
		r_initial = (*src & 0xf800) >> 8;
		g_initial = (*src & 0x7e0) >> 3;
		b_initial = (*src & 0x1f) << 3;
		*src = ((((r_initial * filtervalue[0] + g_initial * filtervalue[1] + b_initial * filtervalue[2]) >> 8) + filtervalue[3]) & 0xf8) << 8 | 
			   ((((r_initial * filtervalue[4] + g_initial * filtervalue[5] + b_initial * filtervalue[6]) >> 8) + filtervalue[7]) & 0xfc) << 3 | 
			   ((((r_initial * filtervalue[8] + g_initial * filtervalue[9] + b_initial * filtervalue[10]) >> 8) + filtervalue[11]) & 0xf8) >> 3;
	    src++;
	}
}

void printOverlay(const char *text){
	uint32_t hlcolor = SDL_MapRGB(textoverlay->format, 248, 252, 248);
	SDL_FillRect(textoverlay, NULL, hlcolor);
	SFont_WriteCenter(textoverlay, font, 0, text);
	SDL_Rect rect;
	rect.x = 0;
    rect.y = 0;
    rect.w = 160;
    rect.h = 8;
	invert_rect(textoverlay, &rect);
	if(gameiscgb == 0){
		convert_bw_surface_colors(textoverlay, textoverlaycolored, menupalblack, menupaldark, menupallight, menupalwhite, 0); //if game is DMG, then apply DMG palette to overlay
	} else if (gameiscgb == 1){
		SDL_BlitSurface(textoverlay, NULL, textoverlaycolored, NULL);
		if(colorfilter == 1){
			apply_cfilter(textoverlaycolored);
		}
	}
	overlay_inout = 0;
	showoverlay = 0; //start animation
}

void clearAllCheats(){ // NOTE: This does not turn off cheats from the game, it just clears the codes from the menu
	for (int i = 0; i < NUM_GG_CODES * 9; ++i){
		ggcheats[i] = 0;
	}
	for (int i = 0; i < NUM_GS_CODES * 8; ++i){
		gscheats[i] = 0;
	}
	for (int i = 0; i < NUM_GS_CODES; ++i){
		gscheatsenabled[i] = 0;
	}
}

void openMenuAudio(){
#ifdef VERSION_OPENDINGUX
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1792);
#elif VERSION_RETROFW
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024);
#elif defined VERSION_BITTBOY || defined VERSION_POCKETGO
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024);
#else
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1792);
#endif
	Mix_AllocateChannels(2);
}

void closeMenuAudio(){
	Mix_CloseAudio();
}

void loadMenuSounds() {
    RWops1 = SDL_RWFromMem(wav_menu_in, sizeof(wav_menu_in));
    menusound_in = Mix_LoadWAV_RW(RWops1, 1);
    RWops2 = SDL_RWFromMem(wav_menu_back, sizeof(wav_menu_back));
    menusound_back = Mix_LoadWAV_RW(RWops2, 1);
    RWops3 = SDL_RWFromMem(wav_menu_move, sizeof(wav_menu_move));
    menusound_move = Mix_LoadWAV_RW(RWops3, 1);
    RWops4 = SDL_RWFromMem(wav_menu_ok, sizeof(wav_menu_ok));
    menusound_ok = Mix_LoadWAV_RW(RWops4, 1);
    
    if(menusound_in == NULL){
    	printf("Error loading sound menusound_in: %s\n", Mix_GetError());
    }
    if(menusound_back == NULL){
    	printf("Error loading sound menusound_back: %s\n", Mix_GetError());
    }
    if(menusound_move == NULL){
    	printf("Error loading sound menusound_move: %s\n", Mix_GetError());
    }
    if(menusound_ok == NULL){
    	printf("Error loading sound menusound_ok: %s\n", Mix_GetError());
    }
}

void freeMenuSounds(){
	Mix_FreeChunk(menusound_in);
	Mix_FreeChunk(menusound_back);
	Mix_FreeChunk(menusound_move);
	Mix_FreeChunk(menusound_ok);
}

void playMenuSound_in(){
	Mix_PlayChannel(-1, menusound_in, 0);
}

void playMenuSound_back(){
	Mix_PlayChannel(-1, menusound_back, 0);
}

void playMenuSound_move(){
	Mix_PlayChannel(-1, menusound_move, 0);
}

void playMenuSound_ok(){
	Mix_PlayChannel(-1, menusound_ok, 0);
}

int switchToMenuAudio(){
	//SDL_PauseAudio(1);
    SDL_CloseAudio(); //disable emulator audio, otherwise menu audio wont work
    openMenuAudio(); //enable menu audio
    loadMenuSounds();
    return 0;
}

void switchToEmulatorAudio(){
	freeMenuSounds();
    closeMenuAudio();//disable menu audio, otherwise emulator audio wont work
    reopenAudio(); //re-enable emulator audio before resuming emulation, otherwise gambatte will freeze.
}

void libmenu_set_screen(SDL_Surface *set_screen) {
	screen = set_screen;
}

void libmenu_set_font(SFont_Font *set_font) {
	font = set_font;
}

void clean_menu_screen(menu_t *menu){
	/* doing this twice is just an ugly hack to get round an 
	 * opendingux pre-release hardware surfaces bug */
	clear_surface(screen, 0);
	redraw(menu);
	//SDL_Flip(screen);
	clear_surface(screen, 0);
	redraw(menu); // redraw function also flips the screen. delete and restore sdl_flip if problematic
	//SDL_Flip(screen);
	clear_surface(screen, 0);
	redraw(menu); // this third one is for triple-buffer
	//SDL_Flip(screen);
}

void clean_menu_screen_cheat(menu_t *menu){
	/* doing this twice is just an ugly hack to get round an 
	 * opendingux pre-release hardware surfaces bug */
	clear_surface(screen, 0);
	redraw_cheat(menu);
	//SDL_Flip(screen);
	clear_surface(screen, 0);
	redraw_cheat(menu); // redraw function also flips the screen. delete and restore sdl_flip if problematic
	//SDL_Flip(screen);
	clear_surface(screen, 0);
	redraw_cheat(menu); // this third one is for triple-buffer
	//SDL_Flip(screen);
}

void menu_message(menu_t *menu) {
	SDL_Event event;
	for (int i = 0; i < 5; ++i)
	{
		redraw(menu);
    	SDL_Delay(100);
    	redraw_blank(menu);
    	SDL_Delay(100);
	}
    while (menu->quit == 0){
    	while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						default:
							break;
					}
				default:
					break;
			}
		}
		menu->back_callback(menu);
	}
	if(forcemenuexit == 2) {
		forcemenuexit = 1;
		SDL_BlitSurface(menuscreen, NULL, surface_menuinout, NULL);
	} else if(forcemenuexit == 1) {
		// do nothing
	} else {
		SDL_BlitSurface(menuscreen, NULL, surface_menuinout, NULL);
		clean_menu_screen(menu);
	}
}

int menu_drawmenuframe(menu_t *menu) {
	int loop, i;
	loop = 0;
	int num_selectable = 0;
	for (i = 0; i < menu->n_entries; i++) {
		if(menu->entries[i]->selectable == 1){
			num_selectable++; // count num of selectable entries
		}
	}
	while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)) { //ensure we select a selectable entry, if there is any.
		if (menu->selected_entry < menu->n_entries - 1) {
			++menu->selected_entry;
		} else {
			menu->selected_entry = 0;
		}
		loop++;
	}
	clear_surface(surface_menuinout, 0xFFFFFF);
    display_menu(surface_menuinout, menu);
	return menu->selected_entry;
}

int textanim_reset(){
	textanimdirection = 0;
	textanimtimer = 0;
	textanimspeed = 0;
	textanimpos = 0;
	textanimwidth = 0;
	return 0;
}

int menu_main(menu_t *menu) {
    SDL_Event event;
	int dirty, loop, i;
	loop = 0;
	int num_selectable = 0;
	textanim_reset();
	for (i = 0; i < menu->n_entries; i++) {
		if(menu->entries[i]->selectable == 1){
			num_selectable++; // count num of selectable entries
		}
	}
	while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)) { //ensure we select a selectable entry, if there is any.
		if (menu->selected_entry < menu->n_entries - 1) {
			++menu->selected_entry;
		} else {
			menu->selected_entry = 0;
		}
		loop++;
	}
    redraw(menu);
    while (menu->quit == 0){
#ifdef MIYOO_BATTERY_WARNING
		checkBatt();
#endif
        dirty = 0;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					exit(0);
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_UP:
							if(num_selectable > 0){
								playMenuSound_move();
							}
							loop = 0;
							do {
								if (menu->selected_entry > 0) {
									--menu->selected_entry;
								} else {
									menu->selected_entry = menu->n_entries - 1;
								}
								loop++;
							} while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)); //ensure we select a selectable entry, if there is any.
							dirty = 1;
							textanim_reset();
							break;
						case SDLK_DOWN:
							if(num_selectable > 0){
								playMenuSound_move();
							}
							loop = 0;
							do {
								if (menu->selected_entry < menu->n_entries - 1) {
									++menu->selected_entry;
								} else {
									menu->selected_entry = 0;
								}
								loop++;
							} while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)); //ensure we select a selectable entry, if there is any.
							dirty = 1;
							textanim_reset();
							break;
						case SDLK_LEFT:
							if (menu->entries[menu->selected_entry]->is_shiftable) {
								if (menu->entries[menu->selected_entry]->selected_entry > 0) {
									--menu->entries[menu->selected_entry]->selected_entry;
									dirty = 1;
								} else {
									menu->entries[menu->selected_entry]->selected_entry = menu->entries[menu->selected_entry]->n_entries - 1;
									dirty = 1;
								}
							} else if (menu->n_entries >= 26){ // Enable page scrolling if too many menu items
								if(num_selectable > 0){
									playMenuSound_move();
								}
								loop = 0;
								do {
									if (menu->selected_entry > 12) {
										menu->selected_entry -= 13;
									} else if ((menu->selected_entry <= 12) && (menu->selected_entry > 0)){
										menu->selected_entry = 0;
									} else {
										menu->selected_entry = menu->n_entries - 1;
									}
									loop++;
								} while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)); //ensure we select a selectable entry, if there is any.
								dirty = 1;
								textanim_reset();
							}			
							break;
						case SDLK_RIGHT:
							if (menu->entries[menu->selected_entry]->is_shiftable) {
								if (menu->entries[menu->selected_entry]->selected_entry < menu->entries[menu->selected_entry]->n_entries - 1) {
									++menu->entries[menu->selected_entry]->selected_entry;
									dirty = 1;
								} else {
									menu->entries[menu->selected_entry]->selected_entry = 0;
									dirty = 1;
								}
							} else if (menu->n_entries >= 26){ // Enable page scrolling if too many menu items
								if(num_selectable > 0){
									playMenuSound_move();
								}
								loop = 0;
								do {
									if (menu->selected_entry < menu->n_entries - 14) {
										menu->selected_entry += 13;
									} else if ((menu->selected_entry >= menu->n_entries - 14) && (menu->selected_entry < menu->n_entries - 1)){
										menu->selected_entry = menu->n_entries - 1;
									} else {
										menu->selected_entry = 0;
									}
									loop++;
								} while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)); //ensure we select a selectable entry, if there is any.
								dirty = 1;
								textanim_reset();
							}
							break;
						case KEYMAP_MENUACCEPT:	// button used to accept/select a menu item
							if(menuin == -1){
								if (menu->entries[menu->selected_entry]->callback != NULL) {
									footer_alt = 0;
									menu->entries[menu->selected_entry]->callback(menu);
									redraw(menu);
								}
							}
							break;
						case KEYMAP_MENUCANCEL: // button used to cancel/go back
							if(menuin == -1){
								if (menu->back_callback != NULL) {
									footer_alt = 0;
									menu->back_callback(menu);
								}
							}
							break;
						default:
							break;
					}
				default:
					break;
			}
		}
		if(menuin >= 0){
			dirty = 1;
		}
		if(textanim == 1){ //Text animation
			if(textanimdirection == 0){
				if(textanimtimer < TEXTANIM_DELAY){
					textanimtimer++;
				} else {
					textanimtimer = 0;
					textanimdirection = 1;
				}
			} else if (textanimdirection == 1){
				if(textanimpos < textanimwidth){
					if(textanimspeed < TEXTANIM_SPEED){
						textanimspeed++;
					} else {
						textanimspeed = 0;
						textanimpos += 8;
						dirty = 1;
					}
				} else {
					textanimdirection = 2;
					dirty = 1;
				}
			} else if (textanimdirection == 2){
				if(textanimtimer < TEXTANIM_DELAY){
					textanimtimer++;
				} else {
					textanimtimer = 0;
					textanimdirection = 3;
				}
			} else if (textanimdirection == 3){
				if(textanimpos > 0){
					if(textanimspeed < TEXTANIM_SPEED){
						textanimspeed++;
					} else {
						textanimspeed = 0;
						textanimpos -= 8;
						dirty = 1;
					}
				} else {
					textanimdirection = 0;
					dirty = 1;
				}
			}
		}
		if (dirty) {
			redraw(menu);
		}
		if(menuin == -1){
			SDL_Delay(10);
		} else {
			SDL_Delay(0);
		}	
	}
	if(forcemenuexit == 2) {
		forcemenuexit = 1;
		SDL_BlitSurface(menuscreen, NULL, surface_menuinout, NULL);
	} else if(forcemenuexit == 1) {
		// do nothing
	} else {
		SDL_BlitSurface(menuscreen, NULL, surface_menuinout, NULL);
		clean_menu_screen(menu);
	}
	return menu->selected_entry;
}

int menu_cheat(menu_t *menu) {
    SDL_Event event;
	int dirty, loop;
	int i, collimit;
    if (menu->entries[0]->selectable == 2){
    	collimit = 11; //gamegenie codes
    } else if (menu->entries[0]->selectable == 1){
    	collimit = 10; //gameshark codes
    } else {
    	collimit = 1; // this should never happen
    }
	loop = 0;
	while((menu->entries[menu->selected_entry]->selectable == 0) && (loop < menu->n_entries)) { //ensure we select a selectable entry, if there is any.
		if (menu->selected_entry < menu->n_entries - 1) {
			++menu->selected_entry;
		} else {
			menu->selected_entry = 0;
		}
		loop++;
	}
	footer_alt = 0;
    redraw_cheat(menu);
    while (menu->quit == 0) {
#ifdef MIYOO_BATTERY_WARNING
		checkBatt();
#endif
        dirty = 0;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					exit(0);
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_LEFT:
							playMenuSound_move();
							if (editmode == 0){
								if (collimit == 10){ // gameshark
									if ((menu->selected_entry % collimit == 0) || (menu->selected_entry == 0)){
										menu->selected_entry += 2;	
									} else {
										menu->selected_entry -= 2;
									}
								} else if (collimit == 11){ // gamegenie
									// do nothing
								}	
							} else if (editmode == 1){
								loop = 0;
								do {
									if ((menu->selected_entry % collimit == 0) || (menu->selected_entry == 0)){
										menu->selected_entry += (collimit -1);
									} else {
										--menu->selected_entry;
									}
									loop++;
								} while((menu->entries[menu->selected_entry]->selectable != 2) && (loop < collimit)); //ensure we select a selectable entry, if there is any.
							}
							dirty = 1;
							break;
						case SDLK_RIGHT:
							playMenuSound_move();
							if (editmode == 0){
								if (collimit == 10){ // gameshark
									if ((menu->selected_entry % collimit == 0) || (menu->selected_entry == 0)){
										menu->selected_entry += 2;	
									} else {
										menu->selected_entry -= 2;
									}
								} else if (collimit == 11){ // gamegenie
									// do nothing
								}
							} else if (editmode == 1){
								loop = 0;
								do {
									if ((menu->selected_entry + 1) % collimit == 0){
										menu->selected_entry -= (collimit - 1);	
									} else {
										++menu->selected_entry;
									}
									loop++;
								} while((menu->entries[menu->selected_entry]->selectable != 2) && (loop < collimit)); //ensure we select a selectable entry, if there is any.
							}
							dirty = 1;
							break;
						case SDLK_UP:
							playMenuSound_move();
							if (editmode == 0){
								for (i = 0; i < collimit; i++) { // go up 1 line
									if (menu->selected_entry > 0) {
										--menu->selected_entry;
									} else {
										menu->selected_entry = menu->n_entries - 1;
									}
								}
							} else if (editmode == 1){
								if (menu->entries[menu->selected_entry]->is_shiftable) { //cycle entry values forward
									if (menu->entries[menu->selected_entry]->selected_entry < menu->entries[menu->selected_entry]->n_entries - 1) {
										++menu->entries[menu->selected_entry]->selected_entry;
										dirty = 1;
									} else {
										menu->entries[menu->selected_entry]->selected_entry = 0;
										dirty = 1;
									}
								}
							}
							dirty = 1;
							break;
						case SDLK_DOWN:
							playMenuSound_move();
							if (editmode == 0){
								for (i = 0; i < collimit; i++) { // go down 1 line
									if (menu->selected_entry < menu->n_entries - 1) {
										++menu->selected_entry;
									} else {
										menu->selected_entry = 0;
									}
								}
							} else if (editmode == 1){
								if (menu->entries[menu->selected_entry]->is_shiftable) { //cycle entry values backwards
									if (menu->entries[menu->selected_entry]->selected_entry > 0) {
										--menu->entries[menu->selected_entry]->selected_entry;
										dirty = 1;
									} else {
										menu->entries[menu->selected_entry]->selected_entry = menu->entries[menu->selected_entry]->n_entries - 1;
										dirty = 1;
									}
								}
							}
							dirty = 1;
							break;
						case KEYMAP_MENUACCEPT:	// button used to accept/select a menu item
							if (menu->entries[menu->selected_entry]->callback != NULL) {
								footer_alt = 0;
								menu->entries[menu->selected_entry]->callback(menu);
								redraw_cheat(menu);
							}
							break;
						case KEYMAP_MENUCANCEL: // button used to cancel/go back
							if (menu->back_callback != NULL) {
								if (editmode == 1){
									footer_alt = 0;
								}
								menu->back_callback(menu);
								
							}
							dirty = 1;
							break;
						case SDLK_RETURN: 	// start button - Apply
							if (collimit == 11){ // if gamegenie
								if (menu->entries[menu->n_entries -1]->callback != NULL) {
									menu->entries[menu->n_entries -1]->callback(menu);
									redraw_cheat(menu);
								}
							}
							break;
						default:
							break;
					}
				default:
					break;
			}
		}
		if(blink < BLINK_SPEED){ // for blinking animation
			blink++;
		} else if (blink == BLINK_SPEED){
			blink = 0;
		}
		if((collimit == 11) && (footer_alt < FOOTER_ALT_SPEED * 2)){ // for footer alternating animation on gamegenie menu
			footer_alt++;
		} else if ((collimit == 11) && (footer_alt == FOOTER_ALT_SPEED * 2)){
			footer_alt = 0;
		}
		if ((dirty) || ((editmode == 1) && ((blink == 0) || (blink == floor(BLINK_SPEED * 3 / 4)))) || ((collimit == 11) && ((footer_alt == 0) || (footer_alt == FOOTER_ALT_SPEED)))) {
			redraw_cheat(menu);
		}
		if(menuin == -1){
			SDL_Delay(10);
		} else {
			SDL_Delay(0);
		}
	}
	clean_menu_screen_cheat(menu);
	return menu->selected_entry;
}

static void display_menu(SDL_Surface *surface, menu_t *menu) {
    int font_height = SFont_TextHeight(font);
    int i;
    int line =  0;
    SDL_Rect highlight;
    char *text;
    char buffer[64];
    int width;
    int linelimit = 13;
    int posbegin = 0;
    int posend = menu->n_entries;
    int uparrow = 0;
    int downarrow = 0;
    int num_selectable = 0;
    if((menu->n_entries > linelimit) && (linelimit % 2 != 0)){ // menu scrolling, line limit is not multiple of 2
    	if(menu->selected_entry <= floor(linelimit / 2)){
    		posbegin = 0;
    		posend = posbegin + linelimit;
    		uparrow = 0;
    		downarrow = 1;
    	} else if(menu->selected_entry > floor(linelimit / 2)){
    		if((menu->selected_entry + floor(linelimit / 2)) < menu->n_entries){
    			posbegin = menu->selected_entry - floor(linelimit / 2);
    			posend = menu->selected_entry + floor(linelimit / 2) + 1;
    		} else {
    			posbegin = menu->n_entries - linelimit;
    			posend = menu->n_entries;
    		}
    		if((menu->selected_entry + floor(linelimit / 2) + 1) < menu->n_entries){
    			uparrow = 1;
    			downarrow = 1;
    		} else {
    			uparrow = 1;
    			downarrow = 0;
    		}
    	}
    } else if((menu->n_entries > linelimit) && (linelimit % 2 == 0)){ // menu scrolling, line limit is multiple of 2
    	if(menu->selected_entry <= floor(linelimit / 2)){
    		posbegin = 0;
    		posend = posbegin + linelimit;
    		uparrow = 0;
    		downarrow = 1;
    	} else if(menu->selected_entry > floor(linelimit / 2)){
    		if((menu->selected_entry + floor(linelimit / 2)) < menu->n_entries){
    			posbegin = menu->selected_entry - floor(linelimit / 2);
    			posend = menu->selected_entry + floor(linelimit / 2);
    		} else {
    			posbegin = menu->n_entries - linelimit;
    			posend = menu->n_entries;
    		}
    		if((menu->selected_entry + floor(linelimit / 2)) < menu->n_entries){
    			uparrow = 1;
    			downarrow = 1;
    		} else {
    			uparrow = 1;
    			downarrow = 0;
    		}
    	}
    } else {
    	uparrow = 0;
    	downarrow = 0;
    }
    const int highlight_margin = 0;
    paint_titlebar(surface);
    SFont_WriteCenter(surface, font, (line * font_height), menu->header);
    line ++;
    SFont_WriteCenter(surface, font, (line * font_height), menu->title);
    if((gambatte_p->isLoaded()) && (currgamename != "default") && ((strcmp(menu->title, "Settings") == 0) || (strcmp(menu->title, " Settings ") == 0) || (strcmp(menu->title, "  Settings  ") == 0))){
    	std::string tempconfigfile = (homedir + "/.gambatte/settings/");
        tempconfigfile += (currgamename + ".cfg");
        FILE * cfile;
        cfile = fopen(tempconfigfile.c_str(), "r");
        if (cfile != NULL) { //if per-game config file exists for the current ROM, draw PG (per-game) marker on settings menu title
            fclose(cfile);
            SFont_Write(surface, font, 111, 4, ".");
        }
    }
    if(uparrow == 1){
    	line ++;
    	SFont_WriteCenter(surface, font, line * font_height, "{"); // up arrow
    	line ++;
    } else {
    	line += 2;
    }
    if(menu->n_entries < linelimit){ // few menu items, require centering
		int posoffset = floor((linelimit - menu->n_entries) / 2);
		line += posoffset;
	}
	for (i = posbegin; i < posend; i++) {
		if (menu->entries[i]->is_shiftable) {
			sprintf(buffer, "%s <%s>", menu->entries[i]->text, menu->entries[i]->entries[menu->entries[i]->selected_entry]);
			text = buffer;
		} else {
			text = menu->entries[i]->text;
		}
		if((strcmp(menu->title, "Load State") == 0) || (strcmp(menu->title, "Save State") == 0)) { // load/save state screen
			SFont_Write(surface, font, 8, line * font_height, text);
			if ((menu->selected_entry == i) && (menu->entries[i]->selectable == 1)){ // only highlight selected entry if it's selectable
				width = SFont_TextWidth(font, text);
				highlight.x = 8 - highlight_margin;
				highlight.y = line * font_height;
				highlight.w = width + (highlight_margin * 2);
				highlight.h = font_height;
				invert_rect(surface, &highlight);
			}
		} else { // rest of menu screens
			if((SFont_TextWidth(font, text) + (highlight_margin * 2) > surface->w) || (strcmp(menu->title, "Select Game") == 0)){ //if in rom browser OR text does not fit in the screen
				if ((menu->selected_entry == i) && (menu->entries[i]->selectable == 1)){
					SFont_Write(surface, font, (0 - textanimpos), line * font_height, text); //if text is selected entry, animate
				} else {
					SFont_Write(surface, font, 0, line * font_height, text); //if text is not selected entry, dont animate
				}	
			} else {
				SFont_WriteCenter(surface, font, line * font_height, text);
			}			
			if ((menu->selected_entry == i) && (menu->entries[i]->selectable == 1)){ // only highlight selected entry if it's selectable
				width = SFont_TextWidth(font, text);
				highlight.x = ((surface->w - width) / 2) - highlight_margin;
				highlight.y = line * font_height;
				highlight.w = width + (highlight_margin * 2);
				if (highlight.w > surface->w){ //If text does not fit in the screen
					highlight.x = 0;
					highlight.w = surface->w;
					textanim = 1;
					textanimwidth = (width - surface->w);
				} else if (strcmp(menu->title, "Select Game") == 0) { //In ROM browser
					highlight.x = 0;
					textanim = 0;
					textanim_reset();
				} else {
					textanim = 0;
					textanim_reset();
				}
				highlight.h = font_height;
				invert_rect(surface, &highlight);
			}
		}
		line++;
	}
	if(downarrow == 1){
	    SFont_WriteCenter(surface, font, line * font_height, "}"); // down arrow
	}
	if((strcmp(menu->title, "Load State") == 0) || (strcmp(menu->title, "Save State") == 0)) { // load/save state screen
		getSaveStatePreview(menu->selected_entry);
		printSaveStatePreview(surface, 71, 32);
	}
	for (i = 0; i < menu->n_entries; i++) {
		if(menu->entries[i]->selectable == 1){
			num_selectable++; // count num of selectable entries
		}
	}
	if(strcmp(menu->title, " Game Genie ") == 0){
		SFont_WriteCenter(surface, font, 17 * font_height, "B-Cancel     Apply-A"); // footer while in "Apply Cheats" confirmation screen
	} else if((strcmp(menu->title, " Settings ") == 0) || (strcmp(menu->title, " Per-Game Settings ") == 0)){
		SFont_WriteCenter(surface, font, 17 * font_height, "B-Cancel      Save-A"); // footer while in "Save Settings" confirmation screen
	} else if((strcmp(menu->title, "  Settings  ") == 0) || (strcmp(menu->title, "  Per-Game Settings  ") == 0)){
		SFont_WriteCenter(surface, font, 17 * font_height, "B-Cancel    Delete-A"); // footer while in "Delete Settings" confirmation screen
	} else if(strcmp(menu->title, "Credits") == 0){
		SFont_WriteCenter(surface, font, 17 * font_height, "B-Back        Back-A"); // footer while in "Credits" screen
	} else {
		if((gambatte_p->isLoaded()) || (strcmp(menu->title, "Main Menu") != 0)){
			SFont_WriteCenter(surface, font, 17 * font_height, "B-Back      Select-A"); // footer in normal menus
		} else {
			SFont_WriteCenter(surface, font, 17 * font_height, "            Select-A"); // footer in main menu while no rom is loaded
		}
	}
}

static void display_menu_cheat(SDL_Surface *surface, menu_t *menu) {
    int font_height = SFont_TextHeight(font);
    int font_width = SFont_TextWidth(font, "F");
    int i, j, collimit, numcodes, currcode;
    int line = 0, column = 0;
    int h_offset = 0;
    SDL_Rect linehighlight, highlight;
    char *text;
    int width;
    std::string totaltext;
    char buffer[32];
    int linelimit = 13;
    int posbegin = 0;
    int posend = 1;
    int uparrow = 0;
    int downarrow = 0;
    uint32_t hlcolor;
    if (menu->entries[0]->selectable == 2){
    	collimit = 11; //gamegenie codes
    } else if (menu->entries[0]->selectable == 1){
    	collimit = 10; //gameshark codes
    } else {
    	collimit = 1; // this should never happen
    }
    numcodes = floor(menu->n_entries / collimit);
    currcode = floor(menu->selected_entry / collimit);
    if((numcodes > linelimit) && (linelimit % 2 != 0)){ // menu scrolling, line limit is not multiple of 2
    	if(currcode <= floor(linelimit / 2)){
    		posbegin = 0;
    		posend = posbegin + linelimit;
    		uparrow = 0;
    		downarrow = 1;
    	} else if(currcode > floor(linelimit / 2)){
    		if((currcode + floor(linelimit / 2)) < numcodes){
    			posbegin = currcode - floor(linelimit / 2);
    			posend = currcode + floor(linelimit / 2) + 1;
    		} else {
    			posbegin = numcodes - linelimit;
    			posend = numcodes;
    		}
    		if((currcode + floor(linelimit / 2) + 1) < numcodes){
    			uparrow = 1;
    			downarrow = 1;
    		} else {
    			uparrow = 1;
    			downarrow = 0;
    		}
    	}
    } else if((numcodes > linelimit) && (linelimit % 2 == 0)){ // menu scrolling, line limit is multiple of 2
    	if(currcode <= floor(linelimit / 2)){
    		posbegin = 0;
    		posend = posbegin + linelimit;
    		uparrow = 0;
    		downarrow = 1;
    	} else if(currcode > floor(linelimit / 2)){
    		if((currcode + floor(linelimit / 2)) < numcodes){
    			posbegin = currcode - floor(linelimit / 2);
    			posend = currcode + floor(linelimit / 2);
    		} else {
    			posbegin = numcodes - linelimit;
    			posend = numcodes;
    		}
    		if((currcode + floor(linelimit / 2)) < numcodes){
    			uparrow = 1;
    			downarrow = 1;
    		} else {
    			uparrow = 1;
    			downarrow = 0;
    		}
    	}
    } else {
    	posbegin = 0;
    	posend = numcodes;
    	uparrow = 0;
    	downarrow = 0;
    }
    const int highlight_margin = 0;
    paint_titlebar(surface);
    SFont_WriteCenter(surface, font, (line * font_height), menu->header);
    line ++;
    if(editmode == 1){
    	SFont_WriteCenter(surface, font, (line * font_height), "Edit Code");
    } else {
    	SFont_WriteCenter(surface, font, (line * font_height), menu->title);
    }
    if(uparrow == 1){
    	line ++;
    	SFont_WriteCenter(surface, font, line * font_height, "{"); // up arrow
    	line ++;
    } else {
    	line += 2;
    }
    if(numcodes < linelimit){ // few menu items, require centering
		int posoffset = floor((linelimit - numcodes) / 2);
		line += posoffset;
	}
	column = 0;
	for (i = (posbegin * collimit); i < (posend * collimit); i += collimit) {
		j = i;
		column = 0;
		totaltext = "";
		for (j = 0; j < collimit; j++){
			if (menu->entries[i + j]->is_shiftable) {
				sprintf(buffer, "%s", menu->entries[i + j]->entries[menu->entries[i + j]->selected_entry]);
				text = buffer;
			} else {
				text = menu->entries[i + j]->text;
			}
			totaltext += std::string(text);
		}
		h_offset = (surface->w - (font_width * totaltext.length())) / 2;
		for (j = 0; j < collimit; j++){
			if (menu->entries[i + j]->is_shiftable) {
				sprintf(buffer, "%s", menu->entries[i + j]->entries[menu->entries[i + j]->selected_entry]);
				text = buffer;
			} else {
				text = menu->entries[i + j]->text;
			}
			totaltext = std::string(text);
			if ((menu->selected_entry == i + j) && (menu->entries[i + j]->selectable != 0)){ // only highlight selected entry if it's selectable
				width = SFont_TextWidth(font, text);
				highlight.x = (column * font_width) + h_offset - highlight_margin;
				highlight.y = line * font_height;
				if ((editmode == 0) && (collimit == 10) && ((i + j ) % 10 == 0)){ //gameshark, checkmark selected
					highlight.w = width + (highlight_margin * 2);
				} else if ((editmode == 0) && (collimit == 10) && ((i + j ) % 10 != 0)){ //gameshark, code selected
					highlight.w = (width * 8) + (highlight_margin * 2);
				} else if ((editmode == 0) && (collimit == 11) && ((i + j ) % 11 == 0)){ //gamegenie, code selected
					highlight.w = (width * 11) + (highlight_margin * 2);
				} else {
					highlight.w = width + (highlight_margin * 2);
				}
				highlight.h = font_height;
			}
			SFont_Write(surface, font, (column * font_width) + h_offset, line * font_height, text);
			column += totaltext.length();
		}

		if ((editmode == 1) && (collimit == 10) && (blink >= floor(BLINK_SPEED * 3 / 4))) { // blink the whole code while in edit mode. - gameshark
			for (j = 0; j < collimit; j++){
				if ((menu->selected_entry == (i + j)) && (menu->entries[i + j]->selectable == 2)){
					column = 0;
					hlcolor = SDL_MapRGB(surface->format, 255, 255, 255);
					linehighlight.x = ((column + 4) * font_width) + h_offset - highlight_margin;
					linehighlight.y = line * font_height;
					linehighlight.w = (font_width * 8) + (highlight_margin * 2);
					linehighlight.h = font_height;
					SDL_FillRect(surface, &linehighlight, hlcolor);
					break;
				}
			}
		} else if ((editmode == 1) && (collimit == 11) && (blink >= floor(BLINK_SPEED * 3 / 4))) { // blink the whole code while in edit mode. - gamegenie
			for (j = 0; j < collimit; j++){
				if ((menu->selected_entry == (i + j)) && (menu->entries[i + j]->selectable == 2)){
					column = 0;
					hlcolor = SDL_MapRGB(surface->format, 255, 255, 255);
					linehighlight.x = (column * font_width) + h_offset - highlight_margin;
					linehighlight.y = line * font_height;
					linehighlight.w = (font_width * 11) + (highlight_margin * 2);
					linehighlight.h = font_height;
					SDL_FillRect(surface, &linehighlight, hlcolor);
					break;
				}
			}
		}
		line++;
	}
	if((editmode == 1) && (blink < floor(BLINK_SPEED * 3 / 4))){
		invert_rect(surface, &highlight); //draw invert rect on top of selected code/digit.
	} else if (editmode == 0){
		invert_rect(surface, &highlight); //draw invert rect on top of selected code/digit.
	}
	if(downarrow == 1){
	    SFont_WriteCenter(surface, font, line * font_height, "}"); // down arrow
	}
	if(editmode == 1){
		SFont_WriteCenter(surface, font, 17 * font_height, "B-Cancel    Accept-A"); // footer while in edit mode
	} else if ((collimit == 10) && (menu->selected_entry % 10 == 0)){
		SFont_WriteCenter(surface, font, 17 * font_height, "B-Back      Toggle-A"); // footer while highlighting a toggle option in gameshark menu
	} else {
		if(footer_alt < FOOTER_ALT_SPEED){
			SFont_WriteCenter(surface, font, 17 * font_height, "B-Back        Edit-A"); // footer while highlighting a cheat code
		} else {
			SFont_WriteCenter(surface, font, 17 * font_height, "Press Start to Apply"); // alternating footer for gamegenie
		}
	}
}

menu_t *new_menu() {
	menu_t *menu = (menu_t *)malloc(sizeof(menu_t));
	menu->header = NULL;
	menu->title = NULL;
	menu->entries = NULL;
	menu->n_entries = 0;
	menu->selected_entry = 0;
	menu->quit = 0;
	menu->back_callback = NULL;
	return menu;
}

void delete_menu(menu_t *menu) {
	int i;
	if (menu->header != NULL)
		free(menu->header);
	if (menu->title != NULL)
		free(menu->title);
	if (menu->entries != NULL) {
		for (i = 0; i < menu->n_entries; i++) {
			delete_menu_entry(menu->entries[i]);
		}
		free(menu->entries);
	}
	free(menu);
}

void menu_set_header(menu_t *menu, const char *header) {
	if (menu->header != NULL)
		free(menu->header);
	menu->header = (char *)malloc(strlen(header) + 1);
	strcpy(menu->header, header);
}

void menu_set_title(menu_t *menu, const char *title) {
	if (menu->title != NULL)
		free(menu->title);
	menu->title = (char *)malloc(strlen(title) + 1);
	strcpy(menu->title, title);
}

void menu_add_entry(menu_t *menu, menu_entry_t *entry) {
	++menu->n_entries;
	menu->entries = (menu_entry_t **)realloc(menu->entries, sizeof(menu_entry_t) * menu->n_entries);
        menu->entries[menu->n_entries - 1] = entry;
}

menu_entry_t *new_menu_entry(int is_shiftable) {
	menu_entry_t *entry = (menu_entry_t *)malloc(sizeof(menu_entry_t));
	entry->entries = NULL;
    entry->text = NULL;
	entry->is_shiftable = is_shiftable;
	entry->selectable = 1;
	entry->n_entries = 0;
	entry->selected_entry = 0;
    entry->callback = NULL;
	return entry;
}

void delete_menu_entry(menu_entry_t *entry) {
	int i;
	if (entry->entries != NULL){
		if(entry->n_entries > 0){
			for (i = 0; i < entry->n_entries; i++) {
				free(entry->entries[i]);
			}
		}
		free(entry->entries);
	}
	if (entry->text != NULL){
		free(entry->text);
	}
	free(entry);
}

void menu_entry_set_text(menu_entry_t *entry, const char *text) {
	if (entry->text != NULL) {
		free(entry->text);
	}
	entry->text = (char *)malloc(strlen(text) + 1);
	strcpy(entry->text, text);
}

void menu_entry_set_text_no_ext(menu_entry_t *entry, const char *text) { // remove extension from text
	if (entry->text != NULL) {
		free(entry->text);
	}
	const char *ext = strrchr(text,'.');
	if((!ext) || (ext == text)) {
        return;
    } else if(strcmp(ext, ".gb") == 0){
		entry->text = (char *)calloc((strlen(text)-2), sizeof(char));
		strncpy(entry->text, text, (strlen(text)-3));
    } else {
    	entry->text = (char *)calloc((strlen(text)-3), sizeof(char));
		strncpy(entry->text, text, (strlen(text)-4));
    }
}

void menu_entry_add_entry(menu_entry_t *entry, const char* text) {
	assert(entry->is_shiftable == 1);
	++entry->n_entries;
	entry->entries = (char **)realloc(entry->entries, sizeof(char *) * entry->n_entries);
	entry->entries[entry->n_entries - 1] = (char *)malloc(strlen(text) + 1);
	strcpy(entry->entries[entry->n_entries - 1], text);
}

void callback_menu_quit(menu_t *caller_menu) {
	menuout = 0;
	caller_menu->quit = 1;
}

void set_menu_palette(uint32_t valwhite, uint32_t vallight, uint32_t valdark, uint32_t valblack) {
	menupalwhite = valwhite;
	menupallight = vallight;
	menupaldark = valdark;
	menupalblack = valblack;
	if(gameiscgb == 0){
		convert_bw_surface_colors(textoverlay, textoverlaycolored, menupalblack, menupaldark, menupallight, menupalwhite, 0); //if game is DMG, then apply DMG palette to overlay
	}
}

void init_menusurfaces(){
	menuscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0);
	surface_menuinout = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0);
	statepreview = SDL_CreateRGBSurface(SDL_SWSURFACE, 80, 72, 32, 0, 0, 0, 0);
	textoverlay = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 8, 16, 0, 0, 0, 0);
	textoverlaycolored = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 8, 16, 0, 0, 0, 0);
}

void free_menusurfaces(){ //currently unused
	SDL_FreeSurface(menuscreen);
	SDL_FreeSurface(surface_menuinout);
	SDL_FreeSurface(statepreview);
	SDL_FreeSurface(textoverlay);
	SDL_FreeSurface(textoverlaycolored);
}

int currentEntryInList(menu_t *menu, std::string fname){
	if(fname == "NONE"){
		return 0;
	} else if (fname == "DEFAULT"){
		return 1;
	} else if (fname == "AUTO"){
		return 2;
	}
	fname = fname.substr(0, fname.length() - 4);
    int count = menu->n_entries;
    int i;
    for (i = 0; i < count; ++i) {
    	if(strcmp(fname.c_str(), menu->entries[i]->text) == 0){
    		return i;
    	}
    }
    return 0;
}

void paint_titlebar(SDL_Surface *surface){
	uint32_t hlcolor = SDL_MapRGB(surface->format, 255, 255, 255);
	SDL_FillRect(surface, NULL, hlcolor);
	if(gameiscgb == 1){
		hlcolor = SDL_MapRGB(surface->format, 64, 128, 255);
	} else {
		hlcolor = SDL_MapRGB(surface->format, 168, 168, 168);
	}
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = 160;
    rect.h = 16;
    SDL_FillRect(surface, &rect, hlcolor);
    rect.x = 0;
    rect.y = 136;
    rect.w = 160;
    rect.h = 8;
    SDL_FillRect(surface, &rect, hlcolor);
}

void createBorderSurface(){
	switch(selectedscaler) {
		case 5:		/* Ayla's fullscreen scaler */
		case 6:		/* Bilinear fullscreen scaler */
		case 11:	/* Hardware Fullscreen */
#ifdef VGA_SCREEN
		case 14:	/* CRT Fullscreen scaler */
			borderimg = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 16, 0, 0, 0, 0);
			break;
		case 12:	/* Dot Matrix 3x scaler */
		case 13:	/* CRT 3x scaler */
#endif
		case 1:		/* Ayla's 1.5x scaler */
		case 2:		/* Bilinear 1.5x scaler */
			borderimg = SDL_CreateRGBSurface(SDL_SWSURFACE, 212, 160, 16, 0, 0, 0, 0);
			break;
		case 3:		/* Fast 1.66x scaler */
		case 4:		/* Bilinear 1.66x scaler */
			borderimg = SDL_CreateRGBSurface(SDL_SWSURFACE, 192, 144, 16, 0, 0, 0, 0);
			break;
		case 0:		/* no scaler */
		case 7:		/* Hardware 1.25x */
		case 8:		/* Hardware 1.36x */
		case 9:		/* Hardware 1.5x */
		case 10:	/* Hardware 1.66x */
		default:
			borderimg = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0, 0, 0, 0);
			break;
	}
	if(gameiscgb == 0){
		if(dmgbordername != "NONE") {
			clear_surface(borderimg, convert_hexcolor(borderimg, menupalwhite));
		} else { //if border image is disabled
			clear_surface(borderimg, 0x000000);
		}
	} else if(gameiscgb == 1){
		clear_surface(borderimg, 0x000000);
	}
}	

void load_border(std::string borderfilename){ //load border from menu
	SDL_FreeSurface(borderimg);
	std::string fullimgpath = (homedir + "/.gambatte/borders/");

	if (borderfilename == "DEFAULT"){
		if(gameiscgb == 0){
			RWops = SDL_RWFromMem(border_default_dmg, sizeof(border_default_dmg));
    		temp = IMG_LoadPNG_RW(RWops);
    		SDL_FreeRW(RWops);
		} else {
			RWops = SDL_RWFromMem(border_default_gbc, sizeof(border_default_gbc));
    		temp = IMG_LoadPNG_RW(RWops);
    		SDL_FreeRW(RWops);
		}
	} else if (borderfilename == "AUTO"){
		fullimgpath += (currgamename + ".png");
		temp = IMG_Load(fullimgpath.c_str());
		if(!temp){
			printf("Border file %s not found. Searching for default.png...\n", fullimgpath.c_str());
			fullimgpath = (homedir + "/.gambatte/borders/default.png");
			temp = IMG_Load(fullimgpath.c_str());
			if(!temp){
				printf("Border file %s not found. Loading default border...\n", fullimgpath.c_str());
				if(gameiscgb == 0){
					RWops = SDL_RWFromMem(border_default_dmg, sizeof(border_default_dmg));
		    		temp = IMG_LoadPNG_RW(RWops);
		    		SDL_FreeRW(RWops);
				} else {
					RWops = SDL_RWFromMem(border_default_gbc, sizeof(border_default_gbc));
		    		temp = IMG_LoadPNG_RW(RWops);
		    		SDL_FreeRW(RWops);
				}
			}
		}	
	} else {
		fullimgpath += (borderfilename);
		temp = IMG_Load(fullimgpath.c_str());
	}

	if(!temp){
    	if(borderfilename != "NONE"){
    		printf("error loading %s\n", fullimgpath.c_str());
    	} else {
    		createBorderSurface();
    	}
    } else {
    	createBorderSurface();
    	SDL_Rect brect;
		brect.x = (borderimg->w - temp->w) / 2;
		brect.y = (borderimg->h - temp->h) / 2;
		brect.w = temp->w;
		brect.h = temp->h;
		SDL_BlitSurface(temp, NULL, borderimg, &brect);
		// Add black bars if the border image is smaller than the screen
		uint32_t barcolor = SDL_MapRGB(borderimg->format, 0, 0, 0);
		if(borderimg->h > temp->h){	
			SDL_Rect bara, barb;
			bara.x = 0;
	    	bara.y = 0;
	    	bara.w = borderimg->w;
	    	bara.h = (borderimg->h - temp->h) / 2;
	    	SDL_FillRect(borderimg, &bara, barcolor);
	    	barb.x = 0;
	    	barb.y = temp->h + ((borderimg->h - temp->h) / 2);
	    	barb.w = borderimg->w;
	    	barb.h = (borderimg->h - temp->h) / 2;
	    	SDL_FillRect(borderimg, &barb, barcolor);
		}
		if(borderimg->w > temp->w){
			SDL_Rect barc, bard;
			barc.x = 0;
	    	barc.y = 0;
	    	barc.w = (borderimg->w - temp->w) / 2;
	    	barc.h = borderimg->h;
	    	SDL_FillRect(borderimg, &barc, barcolor);
	    	bard.x = temp->w + ((borderimg->w - temp->w) / 2);
	    	bard.y = 0;
	    	bard.w = (borderimg->w - temp->w) / 2;
	    	bard.h = borderimg->h;
	    	SDL_FillRect(borderimg, &bard, barcolor);
		}		
		SDL_FreeSurface(temp);
    }	
}

void paint_border(SDL_Surface *surface){
	size_t offset;
	SDL_Rect rect, rectb;
	uint32_t barcolor = SDL_MapRGB(surface->format, 0, 0, 0);
	int bpp = surface->format->BytesPerPixel;
	switch(selectedscaler) {
		case 0:		/* no scaler */
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 320;
    		rect.h = 240;
			SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
		case 1:		/* Ayla's 1.5x scaler */
		case 2:		/* Bilinear 1.5x scaler */
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 1;
    		rect.h = 240;
			rectb.x = 319;
    		rectb.y = 0;
    		rectb.w = 1;
    		rectb.h = 240;
    		SDL_FillRect(surface, &rect, barcolor);
			SDL_FillRect(surface, &rectb, barcolor);
    		offset = 1;
			scaleborder15x((uint32_t*)((uint8_t *)surface->pixels + offset * bpp), (uint32_t*)borderimg->pixels);
			break;
		case 3:		/* Fast 1.66x scaler */
		case 4:		/* Bilinear 1.66x scaler */
			scaleborder166x((uint32_t*)((uint8_t *)surface->pixels), (uint32_t*)borderimg->pixels);
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 1;
    		rect.h = 240;
			rectb.x = 319;
    		rectb.y = 0;
    		rectb.w = 1;
    		rectb.h = 240;
    		SDL_FillRect(surface, &rect, barcolor);
			SDL_FillRect(surface, &rectb, barcolor);
			break;
		case 7:		/* Hardware 1.25x */
			rect.x = 32;
    		rect.y = 24;
    		rect.w = 256;
    		rect.h = 192;
    		SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
		case 8:		/* Hardware 1.36x */
			rect.x = 48;
    		rect.y = 32;
    		rect.w = 224;
    		rect.h = 176;
    		SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
		case 9:		/* Hardware 1.5x */
			rect.x = 56;
    		rect.y = 40;
    		rect.w = 208;
    		rect.h = 160;
    		SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
		case 10:	/* Hardware 1.66x*/
			rect.x = 64;
    		rect.y = 48;
    		rect.w = 192;
    		rect.h = 144;
    		SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
#ifdef VGA_SCREEN
		case 12:	/* Dot Matrix 3x scaler */
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 2;
    		rect.h = 480;
			rectb.x = 638;
    		rectb.y = 0;
    		rectb.w = 2;
    		rectb.h = 480;
    		SDL_FillRect(surface, &rect, barcolor);
			SDL_FillRect(surface, &rectb, barcolor);
    		offset = 2;
			scaleborder3x((uint32_t*)((uint8_t *)surface->pixels + offset * bpp), (uint32_t*)borderimg->pixels);
			break;
		case 13:	/* CRT 3x scaler */
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 2;
    		rect.h = 480;
			rectb.x = 638;
    		rectb.y = 0;
    		rectb.w = 2;
    		rectb.h = 480;
    		SDL_FillRect(surface, &rect, barcolor);
			SDL_FillRect(surface, &rectb, barcolor);
    		offset = 2;
			scaleborder3x_crt((uint32_t*)((uint8_t *)surface->pixels + offset * bpp), (uint32_t*)borderimg->pixels);
			break;
		case 14:	/* CRT Fullscreen */
#endif
		case 5:		/* Ayla's fullscreen scaler */
		case 6:		/* Bilinear fullscreen scaler */
		case 11:	/* Hardware Fullscreen */
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 0;
    		rect.h = 0;
    		SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
		default:
			rect.x = 0;
    		rect.y = 0;
    		rect.w = 320;
    		rect.h = 240;
    		SDL_BlitSurface(borderimg, &rect, surface, NULL);
			break;
	}
}

uint32_t convert_hexcolor(SDL_Surface *surface, const uint32_t color){
	uint8_t colorr = ((color >> 16) & 0xFF);
	uint8_t colorg = ((color >> 8) & 0xFF);
	uint8_t colorb = ((color) & 0xFF);
	uint32_t result = SDL_MapRGB(surface->format, colorr, colorg, colorb);
	return result;
}

/************************************ COLORIZE MENU SCREEN *****************************************/

uint32_t getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;
    switch (bpp)
    {
        case 1:
            return *p;
            break;
        case 2:
            return *(uint16_t *)p;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;
        case 4:
            return *(uint32_t *)p;
            break;
        default:
            return 0;
    }
}
 
void putpixel(SDL_Surface *surface, int x, int y, uint32_t pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;
    switch (bpp)
    {
        case 1:
            *p = pixel;
            break;
        case 2:
            *(uint16_t *)p = pixel;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;
        case 4:
            *(uint32_t *)p = pixel;
            break;
    }
}

uint32_t fixcolor(SDL_Surface *surface, uint32_t srccolor){
	uint8_t r, g, b;
    r = (srccolor >> 16) & 0xff;
    g = (srccolor >> 8) & 0xff;
    b = srccolor & 0xff;
    uint32_t dstcolor = SDL_MapRGB(surface->format, r, g, b);
    return dstcolor;
}

uint32_t mixfixcolor(SDL_Surface *surface, uint32_t srccolor1, uint32_t srccolor2){
	uint8_t r1, g1, b1, r2, g2, b2;
    r1 = (srccolor1 >> 16) & 0xff;
    g1 = (srccolor1 >> 8) & 0xff;
    b1 = srccolor1 & 0xff;
    r2 = (srccolor2 >> 16) & 0xff;
    g2 = (srccolor2 >> 8) & 0xff;
    b2 = srccolor2 & 0xff;
    uint32_t dstcolor = SDL_MapRGB(surface->format, (r1 + r2)/2, (g1 + g2)/2, (b1 + b2)/2);
    return dstcolor;
}
 
//----------------------------------------------------------------------------------------
// CALL THIS FUNCTION LIKE SO TO SWAP BLACK->WHITE
// uint8_t repl_black_r = 0xff,
//         repl_black_g = 0xff,
//         repl_black_b = 0xff;
// uint8_t repl_white_r = 0,
//         repl_white_g = 0,
//         repl_white_b = 0;
// uint32_t repl_col_black = SDL_MapRGB(my_surface->format, black_r, black_g, black_b);
// uint32_t repl_col_white = SDL_MapRGB(my_surface->format, white_r, white_g, white_b);
// convert_bw_surface_colors(my_surface, repl_col_black, repl_col_white);
// MODE parameter: 0 = converts to 4 color shades. 1 = converts to 7 color shades.
//----------------------------------------------------------------------------------------
void convert_bw_surface_colors(SDL_Surface *surface, SDL_Surface *surface2, const uint32_t repl_col_black, const uint32_t repl_col_dark, const uint32_t repl_col_light, const uint32_t repl_col_white, int mode)
{
	if(mode == 0){
	    uint32_t col_black = SDL_MapRGB(surface->format, 63, 63, 63);
	    uint32_t col_dark = SDL_MapRGB(surface->format, 127, 127, 127);
	    uint32_t col_light = SDL_MapRGB(surface->format, 191, 191, 191);
	    uint32_t col_white = SDL_MapRGB(surface->format, 255, 255, 255);
	    SDL_LockSurface(surface);
	    SDL_LockSurface(surface2);
	    int x,y;
	    for (y=0; y < surface->h; ++y)
	    {
	        for (x=0; x < surface->w; ++x)
	        {
	            const uint32_t pix = getpixel(surface, x, y);
	            uint32_t new_pix = pix;
	 
	            if (pix <= col_black)
	                new_pix = fixcolor(surface2, repl_col_black);
	            else if (pix <= col_dark)
	                new_pix = fixcolor(surface2, repl_col_dark);
	            else if (pix <= col_light)
	                new_pix = fixcolor(surface2, repl_col_light);
	            else if (pix <= col_white)
	                new_pix = fixcolor(surface2, repl_col_white);
	 
	            putpixel(surface2, x, y, new_pix);
	        }
	    }
	    SDL_UnlockSurface(surface);
	    SDL_UnlockSurface(surface2);
	} else if(mode == 1){
		uint32_t col_1 = SDL_MapRGB(surface->format, 36, 36, 36);
		uint32_t col_2 = SDL_MapRGB(surface->format, 72, 72, 72);
	    uint32_t col_3 = SDL_MapRGB(surface->format, 109, 109, 109);
	    uint32_t col_4 = SDL_MapRGB(surface->format, 145, 145, 145);
	    uint32_t col_5 = SDL_MapRGB(surface->format, 182, 182, 182);
	    uint32_t col_6 = SDL_MapRGB(surface->format, 218, 218, 218);
	    uint32_t col_7 = SDL_MapRGB(surface->format, 255, 255, 255);
	    SDL_LockSurface(surface);
	    SDL_LockSurface(surface2);
	    int x,y;
	    for (y=0; y < surface->h; ++y)
	    {
	        for (x=0; x < surface->w; ++x)
	        {
	            const uint32_t pix = getpixel(surface, x, y);
	            uint32_t new_pix = pix;
	            if (pix <= col_1)
	                new_pix = fixcolor(surface2, repl_col_black);
	            else if (pix <= col_2)
	                new_pix = mixfixcolor(surface2, repl_col_black, repl_col_dark);
	            else if (pix <= col_3)
	                new_pix = fixcolor(surface2, repl_col_dark);
	            else if (pix <= col_4)
	                new_pix = mixfixcolor(surface2, repl_col_dark, repl_col_light);
	            else if (pix <= col_5)
	                new_pix = fixcolor(surface2, repl_col_light);
	            else if (pix <= col_6)
	                new_pix = mixfixcolor(surface2, repl_col_light, repl_col_white);
	            else if (pix <= col_7)
	                new_pix = fixcolor(surface2, repl_col_white);
	            putpixel(surface2, x, y, new_pix);
	        }
	    }
	    SDL_UnlockSurface(surface);
	    SDL_UnlockSurface(surface2);
	}
}

/********************************END OF COLORIZE MENU***********************************/

static void invert_rect(SDL_Surface* surface, SDL_Rect *rect) {
	/* FIXME: with 32 bit color modes, alpha will be inverted */
	int x, y;
	Uint32 pixel;
	int max_x, max_y;
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			fprintf(stderr, "could not lock surface\n");
			return;
		}
	}	
	max_y = rect->y + rect->h;
	max_x = rect->x + rect->w;	
	if (max_x > surface->w)
		max_x = surface->w;
	if (max_y > surface->h)
		max_y = surface->h;		
	for (y = rect->y; y < max_y; y++) {
		for (x = rect->x; x < max_x; x++) {
			pixel = get_pixel(surface, x, y);
			pixel = ~pixel;
			put_pixel(surface, x, y, pixel);
		}
	}
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

static void redraw(menu_t *menu) {
	if(forcemenuexit == 0){
		clear_surface(menuscreen, 0xFFFFFF);
		if((!gambatte_p->isCgb()) && (dmgbordername != "NONE")) { // if system is DMG
			clear_surface(screen, convert_hexcolor(screen, menupalwhite));
			paint_border(screen);
		} else if((gambatte_p->isCgb()) && (gbcbordername != "NONE")) { // if system is GBC
			clear_surface(screen, 0x000000);
			paint_border(screen);
		} else { //if border image is disabled
			clear_surface(screen, 0x000000);
		}
		display_menu(menuscreen, menu);
		blitter_p->scaleMenu();
		SDL_Flip(screen);
	}	
}

static void redraw_blank(menu_t *menu) {
	if(forcemenuexit == 0){
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 16;
		rect.w = 160;
		rect.h = 120;
		clear_surface(menuscreen, 0xFFFFFF);
		if((!gambatte_p->isCgb()) && (dmgbordername != "NONE")) { // if system is DMG
			clear_surface(screen, convert_hexcolor(screen, menupalwhite));
			paint_border(screen);
			display_menu(menuscreen, menu);
			SDL_FillRect(menuscreen, &rect, convert_hexcolor(screen, 0xFFFFFF));
		} else if((gambatte_p->isCgb()) && (gbcbordername != "NONE")) { // if system is GBC
			clear_surface(screen, 0x000000);
			paint_border(screen);
			display_menu(menuscreen, menu);
			SDL_FillRect(menuscreen, &rect, convert_hexcolor(screen, 0xFFFFFF));
		} else { //if border image is disabled
			clear_surface(screen, 0x000000);
			display_menu(menuscreen, menu);
			SDL_FillRect(menuscreen, &rect, convert_hexcolor(screen, 0xFFFFFF));
		}
		blitter_p->scaleMenu();
		SDL_Flip(screen);
	}	
}

static void redraw_cheat(menu_t *menu) {
	if(forcemenuexit == 0){
		clear_surface(menuscreen, 0xFFFFFF);
		if((!gambatte_p->isCgb()) && (dmgbordername != "NONE")) { // if system is DMG
			clear_surface(screen, convert_hexcolor(screen, menupalwhite));
			paint_border(screen);
		} else if((gambatte_p->isCgb()) && (gbcbordername != "NONE")) { // if system is GBC
			clear_surface(screen, 0x000000);
			paint_border(screen);
		} else { //if border image is disabled
			clear_surface(screen, 0x000000);
		}
		display_menu_cheat(menuscreen, menu);
		blitter_p->scaleMenu();
		SDL_Flip(screen);
	}
}

void clear_surface(SDL_Surface *surface, Uint32 color) {
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = surface->w;
	rect.h = surface->h;
	SDL_FillRect(surface, &rect, color);
}

static void put_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	switch(bpp) {
		case 1:
		*p = (Uint8)pixel;
		break;
	case 2:
		*(Uint16 *)p = (Uint16)pixel;
		break;
	case 3:
		break;
	case 4:
		*(Uint32 *)p = pixel;
		break;
	}
}

static Uint32 get_pixel(SDL_Surface *surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	switch(bpp) {
		case 1:
		return *p;
		break;
	case 2:
		return *(Uint16 *)p;
		break;
	case 3:
		return 0;
		break;
	case 4:
		return *(Uint32 *)p;
		break;
	}
	return 0;
}

void loadPalette(std::string palettefile){
    if(palettefile == "NONE"){
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
	    set_menu_palette(0xF8FCF8, 0xA8A8A8, 0x505450, 0x000000);
    	return;
    } else if(palettefile == "DEFAULT"){
		Uint32 value;
#ifdef VGA_SCREEN
		if (selectedscaler == 12) {
			for (int i = 0; i < 3; ++i) {
		        for (int k = 0; k < 4; ++k) {
		            if(k == 0)
		                value = 0x5B8C07;
		            if(k == 1)
		                value = 0x187048;
		            if(k == 2)
		                value = 0x084448;
		            if(k == 3)
		                value = 0x002038;
		            gambatte_p->setDmgPaletteColor(i, k, value);
		        }
		    }
		    set_menu_palette(0x5B8C07, 0x187048, 0x084448, 0x002038);
		} else {
#endif
		    for (int i = 0; i < 3; ++i) {
		        for (int k = 0; k < 4; ++k) {
		            if(k == 0)
		                value = 0x64960a;
		            if(k == 1)
		                value = 0x1b7e3e;
		            if(k == 2)
		                value = 0x084e3c;
		            if(k == 3)
		                value = 0x003236;
		            gambatte_p->setDmgPaletteColor(i, k, value);
		        }
		    }
	    	set_menu_palette(0x64960a, 0x1b7e3e, 0x084e3c, 0x003236);
#ifdef VGA_SCREEN
		}
#endif
		return;
	} else {
		Uint32 values[12];
		std::string filepath = (homedir + "/.gambatte/palettes/");
		if(palettefile == "AUTO"){
			filepath += (currgamename + ".pal");
		} else {
			filepath.append(palettefile);
		}
		FILE *fpal = NULL;
		fpal = fopen(filepath.c_str(), "r");
		if (fpal == NULL) {
			if(palettefile == "AUTO"){
				printf("Palette file %s not found. Searching for default.pal...\n", filepath.c_str());
				filepath = (homedir + "/.gambatte/palettes/default.pal");
				fpal = fopen(filepath.c_str(), "r");
				if (fpal == NULL) {
					printf("Palette file %s not found. Loading default palette...\n", filepath.c_str());
					Uint32 value;
#ifdef VGA_SCREEN
					if (selectedscaler == 12) {
						for (int i = 0; i < 3; ++i) {
					        for (int k = 0; k < 4; ++k) {
					            if(k == 0)
					                value = 0x5B8C07;
					            if(k == 1)
					                value = 0x187048;
					            if(k == 2)
					                value = 0x084448;
					            if(k == 3)
					                value = 0x002038;
					            gambatte_p->setDmgPaletteColor(i, k, value);
					        }
					    }
					    set_menu_palette(0x5B8C07, 0x187048, 0x084448, 0x002038);
					} else {
#endif
					    for (int i = 0; i < 3; ++i) {
					        for (int k = 0; k < 4; ++k) {
					            if(k == 0)
					                value = 0x64960a;
					            if(k == 1)
					                value = 0x1b7e3e;
					            if(k == 2)
					                value = 0x084e3c;
					            if(k == 3)
					                value = 0x003236;
					            gambatte_p->setDmgPaletteColor(i, k, value);
					        }
					    }
				    	set_menu_palette(0x64960a, 0x1b7e3e, 0x084e3c, 0x003236);
#ifdef VGA_SCREEN
					}
#endif
					return;
				}
			} else {
				printf("Failed to open palette file %s\n", filepath.c_str());
				return;
			}
		}
		if(fpal){
		    int j = 0;
		    for (int i = 0; i < 12; ++i) { // TODO: Find a better way of parsing the palette values.
		        if(fscanf(fpal, "%x", &values[j]) == 1){
		            j++;
		        }
		    }
		    if (j == 12){ // all 12 palette values were successfully loaded
		        int m = 0;
		        for (int i = 0; i < 3; ++i) {
		            for (int k = 0; k < 4; ++k) {
		                gambatte_p->setDmgPaletteColor(i, k, values[m]);
		                m++;
		            }
		        }
		        set_menu_palette(values[0], values[1], values[2], values[3]);
		    } else {
		        printf("Error reading: %s:\n",filepath.c_str());
		        printf("Bad file format.\n");
		    }
		    fclose(fpal);
		}
		return;
	}
}

void loadFilter(std::string filterfile){
    if(filterfile == "NONE"){
    	colorfilter = 0;
    	gambatte_p->setColorFilter(0, filtervalue);
    	SDL_BlitSurface(textoverlay, NULL, textoverlaycolored, NULL);
    	return;
    } else if(filterfile == "DEFAULT"){
		filtervalue[0] = 135;
		filtervalue[1] = 20;
		filtervalue[2] = 0;
		filtervalue[3] = 25;
		filtervalue[4] = 0;
		filtervalue[5] = 125;
		filtervalue[6] = 20;
		filtervalue[7] = 25;
		filtervalue[8] = 0;
		filtervalue[9] = 20;
		filtervalue[10] = 105;
		filtervalue[11] = 30;
		colorfilter = 1;
		gambatte_p->setColorFilter(1, filtervalue);
		SDL_BlitSurface(textoverlay, NULL, textoverlaycolored, NULL);
		apply_cfilter(textoverlaycolored);
		return;
	} else {
		Uint32 values[12];
		std::string filepath = (homedir + "/.gambatte/filters/");
    	filepath.append(filterfile);
		FILE *ffil = NULL;
		ffil = fopen(filepath.c_str(), "r");
		if (ffil == NULL) {
			printf("Failed to open filter file %s\n", filepath.c_str());
			return;
		}
		if(ffil){
		    int j = 0;
		    for (int i = 0; i < 12; ++i) { // TODO: Find a better way of parsing the filter values.
		        if(fscanf(ffil, "%d", &values[j]) == 1){
		            j++;
		        }
		    }
		    if (j == 12){ // all 12 filter values were successfully loaded
		        for (int i = 0; i < 12; ++i) {
		            filtervalue[i] = values[i];
		        }
		        colorfilter = 1;
		        gambatte_p->setColorFilter(1, filtervalue);
		        SDL_BlitSurface(textoverlay, NULL, textoverlaycolored, NULL);
				apply_cfilter(textoverlaycolored);
		    } else {
		        printf("Error reading: %s:\n",filepath.c_str());
		        printf("Bad file format.\n");
		    }
		    fclose(ffil);
		}
		return;
	}
}

void saveConfig(int pergame){
	std::string configfile = (homedir + "/.gambatte/settings/");
	if (pergame == 0) {
		configfile += ("default.cfg");
	} else {
		configfile += (currgamename + ".cfg");
	}
	FILE * cfile;
    cfile = fopen(configfile.c_str(), "w");
    if (cfile == NULL) {
		printf("Failed to open config file for writing.\n");
		return;
	}
    if (fprintf(cfile,
		"SHOWFPS %d\n"
		"SELECTEDSCALER %d\n"
		"PALNAME %s\n"
		"FILTERNAME %s\n"
		"DMGBORDERNAME %s\n"
		"GBCBORDERNAME %s\n"
		"PREFERCGB %d\n"
		"BIOSENABLED %d\n"
		"GHOSTING %d\n"
		"BUTTONLAYOUT %d\n"
		"FFWHOTKEY %d\n"
		"STEREOSOUND %d\n",
		showfps,
		selectedscaler,
		palname.c_str(),
		filtername.c_str(),
		dmgbordername.c_str(),
		gbcbordername.c_str(),
		prefercgb,
		biosenabled,
		ghosting,
		buttonlayout,
		ffwhotkey,
		stereosound) < 0) {
    	printf("Failed to save config file.\n");
    } else {
    	printf("Config file successfully saved.\n");
    }
    fclose(cfile);
}

void deleteConfig(){
	std::string configfile = (homedir + "/.gambatte/settings/");
	configfile += (currgamename + ".cfg");
	if (currgamename != "default"){
		if (remove(configfile.c_str()) == 0){
			printf("Config file successfully deleted.\n");
		} else {
			printf("Failed deleting config file.\n");
		}
	}
}

void loadConfig(){
	std::string configfile = (homedir + "/.gambatte/settings/");
	configfile += (currgamename + ".cfg");
	FILE * cfile;
	char line[4096];
    cfile = fopen(configfile.c_str(), "r");
    if (cfile == NULL) {
    	printf("Per-Game config not found. Loading default config...\n");
    	configfile = (homedir + "/.gambatte/settings/default.cfg");
    	cfile = fopen(configfile.c_str(), "r");
    	if (cfile == NULL) {
    		printf("Failed to open config file for reading.\n");
			return;
    	}
	}
	while (fgets(line, sizeof(line), cfile)) {
		char *arg = strchr(line, ' ');
		int value;
		char charvalue[64];
		if (!arg) {
			continue;
		}
		*arg = '\0';
		arg++;
		if (!strcmp(line, "SHOWFPS")) {
			sscanf(arg, "%d", &value);
			showfps = value;
		} else if (!strcmp(line, "SELECTEDSCALER")) {
			sscanf(arg, "%d", &value);
			selectedscaler = value;
		} else if (!strcmp(line, "PALNAME")) {
			unsigned int len = strlen(arg);
			if (len == 0 || len > sizeof(charvalue) - 1) {
				continue;
			}
			if (arg[len-1] == '\n') {
				arg[len-1] = '\0';
			}
			strcpy(charvalue, arg);
			palname = std::string(charvalue);
		} else if (!strcmp(line, "FILTERNAME")) {
			unsigned int len = strlen(arg);
			if (len == 0 || len > sizeof(charvalue) - 1) {
				continue;
			}
			if (arg[len-1] == '\n') {
				arg[len-1] = '\0';
			}
			strcpy(charvalue, arg);
			filtername = std::string(charvalue);
		} else if (!strcmp(line, "DMGBORDERNAME")) {
			unsigned int len = strlen(arg);
			if (len == 0 || len > sizeof(charvalue) - 1) {
				continue;
			}
			if (arg[len-1] == '\n') {
				arg[len-1] = '\0';
			}
			strcpy(charvalue, arg);
			dmgbordername = std::string(charvalue);
		} else if (!strcmp(line, "GBCBORDERNAME")) {
			unsigned int len = strlen(arg);
			if (len == 0 || len > sizeof(charvalue) - 1) {
				continue;
			}
			if (arg[len-1] == '\n') {
				arg[len-1] = '\0';
			}
			strcpy(charvalue, arg);
			gbcbordername = std::string(charvalue);
		} else if (!strcmp(line, "PREFERCGB")) {
			sscanf(arg, "%d", &value);
			prefercgb = value;
		} else if (!strcmp(line, "BIOSENABLED")) {
			sscanf(arg, "%d", &value);
			biosenabled = value;
		} else if (!strcmp(line, "GHOSTING")) {
			sscanf(arg, "%d", &value);
			ghosting = value;
		} else if (!strcmp(line, "BUTTONLAYOUT")) {
			sscanf(arg, "%d", &value);
			buttonlayout = value;
		} else if (!strcmp(line, "FFWHOTKEY")) {
			sscanf(arg, "%d", &value);
			ffwhotkey = value;
		} else if (!strcmp(line, "STEREOSOUND")) {
			sscanf(arg, "%d", &value);
			stereosound = value;
		}
	}
	fclose(cfile);
}
