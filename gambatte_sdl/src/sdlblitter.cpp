//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include <time.h>
#include "sdlblitter.h"
#include "scalebuffer.h"
#include "../menu.h"
#include "../scaler.h"

#include <string.h>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

SDL_Surface *lastframe;
SDL_Surface *currframe;
SDL_Surface *borderimg;
SDL_Surface *cfilter;

struct SdlBlitter::SurfaceDeleter {
	//static void del(SDL_Surface *s) { SDL_FreeSurface(s); }
	static void del(SDL_Overlay *o) {
		if (o) {
			SDL_UnlockYUVOverlay(o);
			SDL_FreeYUVOverlay(o);
		}
	}
};

SdlBlitter::SdlBlitter(unsigned inwidth, unsigned inheight,
                       int scale, bool yuv, bool startFull)
: screen(SDL_SetVideoMode(inwidth * scale, inheight * scale,
                           16,
                           SDL_HWSURFACE | SDL_DOUBLEBUF | (startFull ? SDL_FULLSCREEN : 0)))
, surface(screen && scale > 1 && !yuv
           ? SDL_CreateRGBSurface(SDL_SWSURFACE, inwidth, inheight,
                                  screen->format->BitsPerPixel, 0, 0, 0, 0)
           : 0)
, overlay_(screen && scale > 1 && yuv
           ? SDL_CreateYUVOverlay(inwidth * 2, inheight, SDL_UYVY_OVERLAY, screen)
           : 0)
{
	if (overlay_)
		SDL_LockYUVOverlay(overlay_.get());
}

SdlBlitter::~SdlBlitter() {
	if (surface != screen)
		SDL_FreeSurface(surface);
}

void init_ghostframes() {
	lastframe = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0);
	currframe = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0);
	SDL_SetAlpha(lastframe, SDL_SRCALPHA, 128);
}

void init_cfilter() {
	cfilter = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0);
}

void init_border_dmg(SDL_Surface *dst){ //load border on emulator start
	if (!dst){
		printf("init_border: screen is not initialized");
		return;
	}
	SDL_FreeSurface(borderimg);
	std::string fullimgpath;
    fullimgpath = (homedir + "/.gambatte/borders/");
	fullimgpath += (dmgbordername);
	if (dmgbordername == "DEFAULT"){
		fullimgpath = "./DefaultDMG.png";
	}
	borderimg = IMG_Load(fullimgpath.c_str());
	if((!borderimg) && (dmgbordername != "NONE")){
	    printf("error loading %s\n", fullimgpath.c_str());
	    return;
	}
	if(dmgbordername != "NONE") {
		clear_surface(dst, convert_hexcolor(dst, menupalwhite));
		paint_border(dst);
	}
}

void init_border_gbc(SDL_Surface *dst){ //load border on emulator start
	if (!dst){
		printf("init_border: screen is not initialized");
		return;
	}
	SDL_FreeSurface(borderimg);
	std::string fullimgpath;
    fullimgpath = (homedir + "/.gambatte/borders/");
	fullimgpath += (gbcbordername);
	if (gbcbordername == "DEFAULT"){
		fullimgpath = "./DefaultGBC.png";
	}
	borderimg = IMG_Load(fullimgpath.c_str());
	if((!borderimg) && (gbcbordername != "NONE")){
	    printf("error loading %s\n", fullimgpath.c_str());
	    return;
	}
	if(gbcbordername != "NONE") {
		clear_surface(dst, 0x000000);
		paint_border(dst);
	}
}

void SdlBlitter::setBufferDimensions() {
	
	FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
	switch(selectedscaler) {
		case 0:		/* no scaler */
		case 1:		/* Ayla's 1.5x scaler */
		case 2:		/* Ayla's fullscreen scaler */
			surface = screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			break;
		case 3:		/* Hardware 1.25x */
			surface = screen = SDL_SetVideoMode(256, 192, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 4:		/* Hardware 1.36x */
			surface = screen = SDL_SetVideoMode(224, 176, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 5:		/* Hardware 1.5x */
			surface = screen = SDL_SetVideoMode(208, 160, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 6:		/* Hardware 1.66x */
			surface = screen = SDL_SetVideoMode(192, 144, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 7:		/* Hardware Fullscreen */
			surface = screen = SDL_SetVideoMode(160, 144, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("0", 1, 1, aspect_ratio_file);
			}
			break;
		default:
			surface = screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			break;
	}
	if(aspect_ratio_file){
		fclose(aspect_ratio_file);
	}

	menu_set_screen(screen);
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0);
	init_ghostframes();
	init_cfilter();
}

void SdlBlitter::setScreenRes() {
	FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");

	switch(selectedscaler) {
		case 0:		/* no scaler */
		case 1:		/* Ayla's 1.5x scaler */
		case 2:		/* Ayla's fullscreen scaler */
			if(screen->w != 320 || screen->h != 240)
				screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			break;
		case 3:		/* Hardware 1.25x */
			if(screen->w != 256 || screen->h != 192)
				screen = SDL_SetVideoMode(256, 192, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 4:		/* Hardware 1.36x */
			if(screen->w != 224 || screen->h != 176)
				screen = SDL_SetVideoMode(224, 176, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 5:		/* Hardware 1.5x */
			if(screen->w != 208 || screen->h != 160)
				screen = SDL_SetVideoMode(208, 160, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 6:		/* Hardware 1.66x */
			if(screen->w != 192 || screen->h != 144)
				screen = SDL_SetVideoMode(192, 144, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("1", 1, 1, aspect_ratio_file);
			}
			break;
		case 7:		/* Hardware Fullscreen */
			if(screen->w != 160 || screen->h != 144)
				screen = SDL_SetVideoMode(160, 144, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			if (aspect_ratio_file)
			{ 
				fwrite("0", 1, 1, aspect_ratio_file);
			}
			break;
		default:
			if(screen->w != 320 || screen->h != 240)
				screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
			break;
	}
	if(aspect_ratio_file){
		fclose(aspect_ratio_file);
	}
}

void SdlBlitter::force320x240() {
	printf("forcing 320x240...\n");
	screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
}

SdlBlitter::PixelBuffer SdlBlitter::inBuffer() const {
	PixelBuffer pb = { 0, 0, RGB32 };

	if (overlay_) {
		pb.pixels = overlay_->pixels[0];
		pb.format = UYVY;
		pb.pitch = overlay_->pitches[0] >> 2;
	} else if (SDL_Surface *s = surface ? surface : screen) {
		pb.pixels = (Uint8*)(s->pixels) + s->offset;
		pb.format = s->format->BitsPerPixel == 16 ? RGB16 : RGB32;
		pb.pitch = s->pitch / s->format->BytesPerPixel;
	}

	return pb;
}

template<typename T>
inline void SdlBlitter::swScale() {
	T const *src = reinterpret_cast<T *>(static_cast<char *>(surface->pixels) + surface->offset);
	T       *dst = reinterpret_cast<T *>(static_cast<char *>(screen->pixels) + screen->offset);
	scaleBuffer(src, dst, surface->w, surface->h,
	            screen->pitch / screen->format->BytesPerPixel, screen->h / surface->h);
}

void apply_cfilter(SDL_Surface *surface) {
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = 160;
	rect.h = 144;
	SDL_SetAlpha(cfilter, SDL_SRCALPHA, 96);
	uint32_t filtcolor = SDL_MapRGB(cfilter->format, 0, 0, 0);
	SDL_FillRect(cfilter, NULL, filtcolor);
	SDL_BlitSurface(cfilter, NULL, surface, &rect);
	SDL_SetAlpha(cfilter, SDL_SRCALPHA, 32);
	filtcolor = SDL_MapRGB(cfilter->format, 255, 255, 255);
	SDL_FillRect(cfilter, NULL, filtcolor);
	SDL_BlitSurface(cfilter, NULL, surface, &rect);
}

void blend_frames(SDL_Surface *surface) {
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = 160;
	rect.h = 144;
	SDL_BlitSurface(surface, NULL, currframe, &rect);
	SDL_BlitSurface(lastframe, NULL, currframe, &rect);
}

void store_lastframe(SDL_Surface *surface) {
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = 160;
	rect.h = 144;
	SDL_BlitSurface(surface, NULL, lastframe, &rect);
}

void store_lastframe2(SDL_Surface *surface) { // test function - currently not used - can delete
	SDL_SetAlpha(surface, SDL_SRCALPHA, 224); // 0-255 opacity
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = 160;
	rect.h = 144;
	SDL_BlitSurface(surface, NULL, lastframe, &rect);
	SDL_SetAlpha(surface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
}

void anim_menuin(SDL_Surface *surface) { // test function - surface = game surface - menuscreen = menu surface
	
	if(menuin >= 0){
		menuin += 16;
		SDL_Rect srcrect;
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = 160;
		srcrect.h = (0 + menuin); // ++
		SDL_Rect dstrect;
		dstrect.x = 0;
		dstrect.y = (144 - menuin); // --
		dstrect.w = 160;
		dstrect.h = 0;
		SDL_BlitSurface(surface, NULL, surface_menuinout, NULL);
		if((colorfilter == 1) && (gameiscgb == 1)){
			apply_cfilter(surface_menuinout);
		}
		SDL_BlitSurface(menuscreen, &srcrect, surface_menuinout, &dstrect);
		SDL_BlitSurface(surface_menuinout, NULL, menuscreen, NULL);
	}
	if(menuin >=144){
		menuin = -1;
	}
}

void anim_menuout(SDL_Surface *surface) { // test function - surface = game surface - menuscreen = menu surface

	if(menuout >= 0){
		menuout += 16;
		SDL_Rect srcrect;
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = 160;
		srcrect.h = (144 - menuout); // --
		SDL_Rect dstrect;
		dstrect.x = 0;
		dstrect.y = (0 + menuout); // --
		dstrect.w = 160;
		dstrect.h = 0;
		SDL_BlitSurface(surface_menuinout, &srcrect, surface, &dstrect);
	}
	if(menuout >=144){
		menuout = -1;
	}
}

static int frames = 0;
static clock_t old_time = 0;
static int fps = 0;
static int firstframe = 0;

void SdlBlitter::draw() {

	clock_t cur_time;
	size_t offset;
	++frames;
	cur_time = SDL_GetTicks();

	if (cur_time > old_time + 1000) {
		fps = frames;
		frames = 0;
		old_time = cur_time;
	}
	if (!screen || !surface)
		return;

	if(firstframe <= 1){ // paints border on frames 0 and 1 to avoid double-buffer flickering
		if(gameiscgb == 1)
			init_border_gbc(screen);
		else
			init_border_dmg(screen);
	}

	if(firstframe >= 8){ // Ensure firstframe variable only gets value 0 and 1 once.
		firstframe = 2;
	} else {
		firstframe++;
	}
	
	if(ghosting == 0){
		if((colorfilter == 1) && (gameiscgb == 1)){
			apply_cfilter(surface);
		}
		if(menuout >= 0){
			anim_menuout(surface);
		}
		switch(selectedscaler) {
			case 0:		/* no scaler */
				SDL_Rect dst;
				dst.x = (screen->w - surface->w) / 2;
				dst.y = ((screen->h - surface->h) / 2)-1;
				dst.w = surface->w;
				dst.h = surface->h;
				SDL_BlitSurface(surface, NULL, screen, &dst);
				break;
			case 1:		/* Ayla's 1.5x scaler */
				SDL_LockSurface(screen);
				SDL_LockSurface(surface);
				offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
				scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels);
				SDL_UnlockSurface(surface);
				SDL_UnlockSurface(screen);
				break;
			case 2:		/* Ayla's fullscreen scaler */
				SDL_LockSurface(screen);
				SDL_LockSurface(surface);
				fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)surface->pixels);
				SDL_UnlockSurface(surface);
				SDL_UnlockSurface(screen);
				break;
			case 3:		/* Hardware 1.25x */
			case 4:		/* Hardware 1.36x */
			case 5:		/* Hardware 1.5x */
			case 6:		/* Hardware 1.66x */
			case 7:		/* Hardware Fullscreen */
			default:
				SDL_Rect dst2;
				dst2.x = (screen->w - surface->w) / 2;
				dst2.y = (screen->h - surface->h) / 2;
				dst2.w = surface->w;
				dst2.h = surface->h;
				SDL_BlitSurface(surface, NULL, screen, &dst2);
				break;
		}
	} else if(ghosting == 1){
		blend_frames(surface);
		store_lastframe(surface);
		if((colorfilter == 1) && (gameiscgb == 1)){
			apply_cfilter(currframe);
		}
		if(menuout >= 0){
			anim_menuout(currframe);
		}
		switch(selectedscaler) {
			case 0:		/* no scaler */
				SDL_Rect dst;
				dst.x = (screen->w - currframe->w) / 2;
				dst.y = ((screen->h - currframe->h) / 2)-1;
				dst.w = currframe->w;
				dst.h = currframe->h;
				SDL_BlitSurface(currframe, NULL, screen, &dst);
				break;
			case 1:		/* Ayla's 1.5x scaler */
				SDL_LockSurface(screen);
				SDL_LockSurface(currframe);
				offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
				scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels);
				SDL_UnlockSurface(currframe);
				SDL_UnlockSurface(screen);
				break;
			case 2:		/* Ayla's fullscreen scaler */
				SDL_LockSurface(screen);
				SDL_LockSurface(currframe);
				fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)currframe->pixels);
				SDL_UnlockSurface(currframe);
				SDL_UnlockSurface(screen);
				break;
			case 3:		/* Hardware 1.25x */
			case 4:		/* Hardware 1.36x */
			case 5:		/* Hardware 1.5x */
			case 6:		/* Hardware 1.66x */
			case 7:		/* Hardware Fullscreen */
			default:
				SDL_Rect dst2;
				dst2.x = (screen->w - currframe->w) / 2;
				dst2.y = (screen->h - currframe->h) / 2;
				dst2.w = currframe->w;
				dst2.h = currframe->h;
				SDL_BlitSurface(currframe, NULL, screen, &dst2);
				break;
		}
	}

	/*if (surface_ && screen_) {
		if (surface_->format->BitsPerPixel == 16)
			swScale<Uint16>();
		else
			swScale<Uint32>();
	}*/

	show_fps(screen, fps);
}

void SdlBlitter::scaleMenu() {
	size_t offset;

	if (!screen || !menuscreen)
		return;

	if(gambatte_p->isCgb()){
		convert_bw_surface_colors(menuscreen, menuscreencolored, 0xFFFFFF, 0x6397FF, 0x083CA8, 0x000000); //if game is GBC, then menu has different colors
		if((colorfilter == 1) && (gameiscgb == 1)){
			apply_cfilter(menuscreen);
		}
	} else {
		convert_bw_surface_colors(menuscreen, menuscreencolored, menupalblack, menupaldark, menupallight, menupalwhite); //if game is DMG, then menu matches DMG palette
	}

	if(menuin >= 0){
		anim_menuin(surface);
	}

	switch(selectedscaler) {
		case 0:		/* no scaler */
			SDL_Rect dst;
			dst.x = (screen->w - menuscreen->w) / 2;
			dst.y = ((screen->h - menuscreen->h) / 2)-1;
			dst.w = menuscreen->w;
			dst.h = menuscreen->h;
			SDL_BlitSurface(menuscreen, NULL, screen, &dst);
			break;
		case 1:		/* Ayla's 1.5x scaler */
			SDL_LockSurface(screen);
			SDL_LockSurface(menuscreen);
			offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
			scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels);
			SDL_UnlockSurface(menuscreen);
			SDL_UnlockSurface(screen);
			break;
		case 2:		/* Ayla's fullscreen scaler */
			SDL_LockSurface(screen);
			SDL_LockSurface(menuscreen);
			fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)menuscreen->pixels);
			SDL_UnlockSurface(menuscreen);
			SDL_UnlockSurface(screen);
			break;
		case 3:		/* Hardware 1.25x */
		case 4:		/* Hardware 1.36x */
		case 5:		/* Hardware 1.5x */
		case 6:		/* Hardware 1.66x */
		case 7:		/* Hardware Fullscreen */
		default:
			SDL_Rect dst2;
			dst2.x = (screen->w - menuscreen->w) / 2;
			dst2.y = (screen->h - menuscreen->h) / 2;
			dst2.w = menuscreen->w;
			dst2.h = menuscreen->h;
			SDL_BlitSurface(menuscreen, NULL, screen, &dst2);
			break;
	}
}

void SdlBlitter::present() {
	if (!screen)
		return;

	if (overlay_) {
		SDL_Rect dstr = { 0, 0, Uint16(screen->w), Uint16(screen->h) };
		SDL_UnlockYUVOverlay(overlay_.get());
		SDL_DisplayYUVOverlay(overlay_.get(), &dstr);
		SDL_LockYUVOverlay(overlay_.get());
	} else {
		//SDL_UpdateRect(screen_, 0, 0, screen_->w, screen_->h);
		SDL_Flip(screen);
	}
}

void SdlBlitter::toggleFullScreen() {
	/*if (screen_) {
		screen_ = SDL_SetVideoMode(screen_->w, screen_->h, screen_->format->BitsPerPixel,
		                           screen_->flags ^ SDL_FULLSCREEN);
	}*/
}
