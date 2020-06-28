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
#include <cmath>

SDL_Surface *lastframe;
SDL_Surface *currframe;
SDL_Surface *borderimg;

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
: screen()
, surface(SDL_CreateRGBSurface(SDL_SWSURFACE, 160, 144, 16, 0, 0, 0, 0))
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

void init_border(SDL_Surface *dst){
	if (!dst){
		printf("init_border: screen is not initialized");
		return;
	}
	if(gameiscgb == 0){
		load_border(dmgbordername);
		if((borderimg) && (dmgbordername != "NONE")) {
			clear_surface(dst, convert_hexcolor(dst, menupalwhite));
			paint_border(dst);
		} else { //if border image is disabled
			clear_surface(dst, 0x000000);
		}
	} else if(gameiscgb == 1){
		load_border(gbcbordername);
		if((borderimg) && (gbcbordername != "NONE")) {
			clear_surface(dst, 0x000000);
			paint_border(dst);
		} else { //if border image is disabled
			clear_surface(dst, 0x000000);
		}
	}
}

void SdlBlitter::CheckIPU(){
	FILE *aspect_ratio_file = NULL;
	std::string ipu_OpenDingux = ("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio");
	std::string ipu_RetroFW10 = ("/proc/jz/ipu_ratio");
	std::string ipu_RetroFW20 = ("/proc/jz/ipu");

	aspect_ratio_file = fopen(ipu_OpenDingux.c_str(), "r+");
	if (aspect_ratio_file != NULL) {
		fclose(aspect_ratio_file);
		ipuscaling = ipu_OpenDingux;
		printf("Detected IPU scaling - OpenDingux\n");
		return;
	}
	aspect_ratio_file = fopen(ipu_RetroFW10.c_str(), "r+");
	if (aspect_ratio_file != NULL) {
		fclose(aspect_ratio_file);
		ipuscaling = ipu_RetroFW10;
		printf("Detected IPU scaling - RetroFW 1.X\n");
		return;
	}
	aspect_ratio_file = fopen("/proc/jz/gpio", "r+"); //workaround to check if the fw is retrofw2
	if (aspect_ratio_file != NULL) {
		fclose(aspect_ratio_file);
		ipuscaling = ipu_RetroFW20;
		printf("Detected IPU scaling - RetroFW 2.X\n");
		return;
	}
	printf("Could not detect IPU scaling\n");
	return;
}

void SdlBlitter::SetVid(int w, int h, int bpp){	
#ifdef VERSION_OPENDINGUX
	screen = SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_TRIPLEBUF);
#elif VERSION_RETROFW
	screen = SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_TRIPLEBUF);
#elif defined VERSION_BITTBOY || defined VERSION_POCKETGO
	screen = SDL_SetVideoMode(w, h, bpp, SDL_SWSURFACE);
#else
	screen = SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_DOUBLEBUF);
#endif
}

void SdlBlitter::setBufferDimensions() {
	
	FILE* aspect_ratio_file = fopen(ipuscaling.c_str(), "w");
	switch(selectedscaler) {
		case 0:		/* no scaler */
		case 1:		/* Ayla's 1.5x scaler */
		case 2:		/* Bilinear 1.5x scaler */
		case 3:		/* Fast 1.66x scaler */
		case 4:		/* Bilinear 1.66x scaler */
		case 5:		/* Ayla's fullscreen scaler */
		case 6:		/* Bilinear fullscreen scaler */
			SetVid(320, 240, 16);
			break;
		case 7:		/* Hardware 1.25x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SetVid(256, 192, 16);
			break;
		case 8:		/* Hardware 1.36x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SetVid(224, 176, 16);
			break;
		case 9:		/* Hardware 1.5x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SetVid(208, 160, 16);
			break;
		case 10:		/* Hardware 1.66x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SetVid(192, 144, 16);
			break;
		case 11:		/* Hardware Fullscreen */
			if (aspect_ratio_file) {
				fwrite("0", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SetVid(160, 144, 16);
			break;
#ifdef VGA_SCREEN
		case 12:		/* Dot Matrix 3x */
		case 13:		/* CRT 3x scaler */
			SetVid(640, 480, 16);
			break;
		case 14:		/* CRT Fullscreen */
			if (aspect_ratio_file) {
				fwrite("0", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			SetVid(480, 432, 16);
			break;
#endif
		default:
			SetVid(320, 240, 16);
			break;
	}

	menu_set_screen(screen);

	init_ghostframes();
}

void SdlBlitter::setScreenRes() {
	FILE* aspect_ratio_file = fopen(ipuscaling.c_str(), "w");

	switch(selectedscaler) {
		case 0:		/* no scaler */
		case 1:		/* Ayla's 1.5x scaler */
		case 2:		/* Bilinear 1.5x scaler */
		case 3:		/* Fast 1.66x scaler */
		case 4:		/* Bilinear 1.66x scaler */
		case 5:		/* Ayla's fullscreen scaler */
		case 6:		/* Bilinear fullscreen scaler */
			if(screen->w != 320 || screen->h != 240)
				SetVid(320, 240, 16);
			break;
		case 7:		/* Hardware 1.25x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			if(screen->w != 256 || screen->h != 192) {
				SetVid(256, 192, 16);
			}
			break;
		case 8:		/* Hardware 1.36x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			if(screen->w != 224 || screen->h != 176) {
				SetVid(224, 176, 16);
			}
			break;
		case 9:		/* Hardware 1.5x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			if(screen->w != 208 || screen->h != 160) {
				SetVid(208, 160, 16);
			}
			break;
		case 10:		/* Hardware 1.66x */
			if (aspect_ratio_file) {
				fwrite("1", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			if(screen->w != 192 || screen->h != 144) {
				SetVid(192, 144, 16);
			}
			break;
		case 11:		/* Hardware Fullscreen */
			if (aspect_ratio_file) {
				fwrite("0", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			if(screen->w != 160 || screen->h != 144) {
				SetVid(160, 144, 16);
			}
			break;
#ifdef VGA_SCREEN
		case 12:		/* Dot Matrix 3x */
		case 13:		/* CRT 3x scaler */
			if(screen->w != 640 || screen->h != 480)
				SetVid(640, 480, 16);
			break;
		case 14:		/* CRT Fullscreen */
			if (aspect_ratio_file) {
				fwrite("0", 1, 1, aspect_ratio_file);
				fclose(aspect_ratio_file);
			}
			if(screen->w != 480 || screen->h != 432) {
				SetVid(480, 432, 16);
			}
			break;
#endif
		default:
			if(screen->w != 320 || screen->h != 240)
				SetVid(320, 240, 16);
			break;
	}
}

void SdlBlitter::force320x240() {
	printf("forcing 320x240...\n");
	SetVid(320, 240, 16);
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

void blend_frames(SDL_Surface *surface) {
	SDL_BlitSurface(surface, NULL, currframe, NULL);
	SDL_BlitSurface(lastframe, NULL, currframe, NULL);
}

void store_lastframe(SDL_Surface *surface) {
	SDL_BlitSurface(surface, NULL, lastframe, NULL);
}

void anim_menuin(SDL_Surface *surface) { 
	
	if(menuin == 0){
		if(gambatte_p->isCgb()){
			if(colorfilter == 1){
				apply_cfilter(surface_menuinout);
			}
		} else {
			convert_bw_surface_colors(surface_menuinout, surface_menuinout, menupalblack, menupaldark, menupallight, menupalwhite, 1); //if game is DMG, then menu matches DMG palette
		}
	}
	if(menuin >= 0){
		menuin += 16; //16
		SDL_Rect srcrect;
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = 160;
		srcrect.h = (0 + menuin); // ++
		SDL_Rect dstrect;
		dstrect.x = 0;
		dstrect.y = (144 - menuin); // --
		dstrect.w = 160;
		dstrect.h = (0 + menuin);
		if(menuin > 0){
			SDL_BlitSurface(surface_menuinout, &srcrect, surface, &dstrect);
		}
	}
	if(menuin >= 144){
		menuin = -2;
	}
}

void anim_menuout(SDL_Surface *surface) { 

	if(menuout >= 0){
		if((firstframe >= 0) && (firstframe <= 15)){ // when new game has just been loaded, delay for 15 frames
			// do nothing
		} else {
			menuout += 16;
		}
		SDL_Rect srcrect;
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = 160;
		srcrect.h = (144 - menuout); // --
		SDL_Rect dstrect;
		dstrect.x = 0;
		dstrect.y = (0 + menuout); // --
		dstrect.w = 160;
		dstrect.h = (144 - menuout);
		if(menuout < 144){
			SDL_BlitSurface(surface_menuinout, &srcrect, surface, &dstrect);
		}	
	}
	if(menuout >= 144){
		menuout = -1;
	}
}

void anim_textoverlay(SDL_Surface *surface) { 

	if((firstframe >= 0) && (firstframe <= 15)){ // when new game has just been loaded, delay for 15 frames
		// do nothing
	} else {
		if(showoverlay >= 0){
			SDL_Rect dstrect;
			dstrect.x = 0;
			if(showoverlay - 8 > 0){
				dstrect.y = 0;
			} else {
				dstrect.y = showoverlay - 8;
			}
			dstrect.w = 160;
			dstrect.h = 8;
			if(showoverlay > 0){
				SDL_BlitSurface(textoverlaycolored, NULL, surface, &dstrect);
			}
		}
		if(showoverlay >= 64){
			overlay_inout = 1;
		}
		if(overlay_inout == 0){
			showoverlay += 1;
		} else if (overlay_inout == 1){
			showoverlay -= 1;
			if(showoverlay == -1){ //animation ended
				overlay_inout = 0;
			}
		}
	}	
}

static int frames = 0;
static clock_t old_time = 0;
static int fps = 0;

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

	if((firstframe >= 0) && (firstframe <= 2)){ // paints border on frames 0,1 and 2 to avoid triple-buffer flickering
		init_border(screen);
	}

	if((firstframe == 0) && (is_using_bios == 0)){ //if game is booting without bios, allow the user to reset game at any point.
		can_reset = 1;
	} else if((firstframe == 0) && (is_using_bios == 1)){ //if game is booting with bios, dont let the user reset until the bios animation has ended. Resetting while on boot screen causes unexpected behaviour.
		can_reset = 0;
	}
	if((is_using_bios == 1) && (gameiscgb == 0) && (firstframe == 336)){ //on DMG games the boot animation ends around frame 336 (might need finetuning here)
		can_reset = 1;
	} else if((is_using_bios == 1) && (gameiscgb == 1) && (firstframe == 180)){ //on GBC games the boot animation ends around frame 180 (might need finetunung here)
		can_reset = 1;
	}

	if(firstframe >= 0){ // Keep firstframe counting.
		firstframe++;
	}
	if(firstframe >= 400){ //Ensure firstframe value stops counting at some point.
		firstframe = -1;
	}
	
	if((ghosting == 0) || ((ghosting == 1) && (gameiscgb == 1)) || ((ghosting == 2) && (gameiscgb == 0))){ //Ghosting disabled for current system
		if(showoverlay >= 0){
			anim_textoverlay(surface);
		}
		if(menuout >= 0){
			anim_menuout(surface);
		}else if(menuin >= 0){
			anim_menuin(surface);
		}
		switch(selectedscaler) {
			case 0:		/* no scaler */
				SDL_Rect dst;
				dst.x = (screen->w - surface->w) / 2;
				dst.y = (screen->h - surface->h) / 2;
				dst.w = surface->w;
				dst.h = surface->h;
				SDL_BlitSurface(surface, NULL, screen, &dst);
				break;
			case 1:		/* Ayla's 1.5x scaler */
				offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
				scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels);
				break;
			case 2:		/* Bilinear 1.5x scaler */
				offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
				scale15x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels);
				break;
			case 3:		/* Fast 1.66x scaler */
				offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
				scale166x_fast((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels);
				break;
			case 4:		/* Bilinear 1.66x scaler */
				offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
				scale166x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels);
				break;
			case 5:		/* Ayla's fullscreen scaler */
				fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)surface->pixels);
				break;
			case 6:		/* Bilinear fullscreen scaler */
				fullscreen_upscale_pseudobilinear((uint32_t*)screen->pixels, (uint32_t*)surface->pixels);
				break;
#ifdef VGA_SCREEN
			case 12:		/* Dot Matrix 3x scaler */
				offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
				if (gameiscgb == 1){
					scale3x_dotmatrix((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels, menupalblack);
				} else {
					scale3x_dotmatrix((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels, menupalwhite);
				}
				break;
			case 13:		/* CRT 3x scaler */
				offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
				scale3x_crt((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)surface->pixels);
				break;
			case 14:	/* CRT Fullscreen */
				fullscreen_crt((uint32_t*)screen->pixels, (uint32_t*)surface->pixels);
				break;
#endif
			case 7:		/* Hardware 1.25x */
			case 8:		/* Hardware 1.36x */
			case 9:		/* Hardware 1.5x */
			case 10:		/* Hardware 1.66x */
			case 11:		/* Hardware Fullscreen */
			default:
				/*SDL_Rect dst2;
				dst2.x = (screen->w - surface->w) / 2;
				dst2.y = (screen->h - surface->h) / 2;
				dst2.w = surface->w;
				dst2.h = surface->h;
				SDL_BlitSurface(surface, NULL, screen, &dst2);*/
				uint16_t *d = (uint16_t*)screen->pixels + (screen->w - surface->w) / 2 + (screen->h - surface->h) * screen->pitch / 4;
	            uint16_t *s = (uint16_t*)surface->pixels;
	            for (int y = 0; y < surface->h; y++)
	            {
	                memmove(d, s, surface->w * sizeof(uint16_t));
	                s += surface->w;
	                d += screen->w;
	            }
				break;
		}
	} else if((ghosting == 3) || ((ghosting == 1) && (gameiscgb == 0)) || ((ghosting == 2) && (gameiscgb == 1))){ //Ghosting enabled for current system
		if(showoverlay >= 0){
			anim_textoverlay(surface);
		}
		blend_frames(surface);
		store_lastframe(surface);
		if(menuout >= 0){
			anim_menuout(currframe);
		}else if(menuin >= 0){
			anim_menuin(currframe);
		}
		switch(selectedscaler) {
			case 0:		/* no scaler */
				SDL_Rect dst;
				dst.x = (screen->w - currframe->w) / 2;
				dst.y = (screen->h - currframe->h) / 2;
				dst.w = currframe->w;
				dst.h = currframe->h;
				SDL_BlitSurface(currframe, NULL, screen, &dst);
				break;
			case 1:		/* Ayla's 1.5x scaler */
				offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
				scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels);
				break;
			case 2:		/* Bilinear 1.5x scaler */
				offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
				scale15x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels);
				break;
			case 3:		/* Fast 1.66x scaler */
				offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
				scale166x_fast((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels);
				break;
			case 4:		/* Bilinear 1.66x scaler */
				offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
				scale166x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels);
				break;
			case 5:		/* Ayla's fullscreen scaler */
				fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)currframe->pixels);
				break;
			case 6:		/* Bilinear fullscreen scaler */
				fullscreen_upscale_pseudobilinear((uint32_t*)screen->pixels, (uint32_t*)currframe->pixels);
				break;
#ifdef VGA_SCREEN
			case 12:		/* Dot Matrix 3x scaler */
				offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
				if (gameiscgb == 1){
					scale3x_dotmatrix((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels, menupalblack);
				} else {
					scale3x_dotmatrix((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels, menupalwhite);
				}
				break;
			case 13:		/* CRT 3x scaler */
				offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
				scale3x_crt((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)currframe->pixels);
				break;
			case 14:		/* CRT Fullscreen */
				fullscreen_crt((uint32_t*)screen->pixels, (uint32_t*)currframe->pixels);
				break;
#endif
			case 7:		/* Hardware 1.25x */
			case 8:		/* Hardware 1.36x */
			case 9:		/* Hardware 1.5x */
			case 10:		/* Hardware 1.66x */
			case 11:		/* Hardware Fullscreen */
			default:
				/*SDL_Rect dst2;
				dst2.x = (screen->w - currframe->w) / 2;
				dst2.y = (screen->h - currframe->h) / 2;
				dst2.w = currframe->w;
				dst2.h = currframe->h;
				SDL_BlitSurface(currframe, NULL, screen, &dst2);*/
				uint16_t *d = (uint16_t*)screen->pixels + (screen->w - currframe->w) / 2 + (screen->h - currframe->h) * screen->pitch / 4;
	            uint16_t *s = (uint16_t*)currframe->pixels;
	            for (int y = 0; y < currframe->h; y++)
	            {
	                memmove(d, s, currframe->w * sizeof(uint16_t));
	                s += currframe->w;
	                d += screen->w;
	            }
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
		if((colorfilter == 1) && (gameiscgb == 1)){
			apply_cfilter(menuscreen);
		}
	} else {
		convert_bw_surface_colors(menuscreen, menuscreen, menupalblack, menupaldark, menupallight, menupalwhite, 1); //if game is DMG, then menu matches DMG palette
	}

	switch(selectedscaler) {
		case 0:		/* no scaler */
			SDL_Rect dst;
			dst.x = (screen->w - menuscreen->w) / 2;
			dst.y = (screen->h - menuscreen->h) / 2;
			dst.w = menuscreen->w;
			dst.h = menuscreen->h;
			SDL_BlitSurface(menuscreen, NULL, screen, &dst);
			break;
		case 1:		/* Ayla's 1.5x scaler */
			offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
			scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels);
			break;
		case 2:		/* Bilinear 1.5x scaler */
			offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
			scale15x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels);
			break;
		case 3:		/* Fast 1.66x scaler */
			offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
			scale166x_fast((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels);
			break;
		case 4:		/* Bilinear 1.66x scaler */
			offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
			scale166x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels);
			break;
		case 5:		/* Ayla's fullscreen scaler */
			fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)menuscreen->pixels);
			break;
		case 6:		/* Bilinear fullscreen scaler */
			fullscreen_upscale_pseudobilinear((uint32_t*)screen->pixels, (uint32_t*)menuscreen->pixels);
			break;
#ifdef VGA_SCREEN
		case 12:		/* Dot Matrix 3x scaler */
			offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
			if (gameiscgb == 1){
				scale3x_dotmatrix((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels, menupalblack);
			} else {
				scale3x_dotmatrix((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels, menupalwhite);
			}
			break;
		case 13:		/* CRT 3x scaler */
			offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
			scale3x_crt((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)menuscreen->pixels);
			break;
		case 14:		/* CRT Fullscreen */
			fullscreen_crt((uint32_t*)screen->pixels, (uint32_t*)menuscreen->pixels);
			break;
#endif
		case 7:		/* Hardware 1.25x */
		case 8:		/* Hardware 1.36x */
		case 9:		/* Hardware 1.5x */
		case 10:		/* Hardware 1.66x */
		case 11:		/* Hardware Fullscreen */
		default:
			/*SDL_Rect dst2;
			dst2.x = (screen->w - menuscreen->w) / 2;
			dst2.y = (screen->h - menuscreen->h) / 2;
			dst2.w = menuscreen->w;
			dst2.h = menuscreen->h;
			SDL_BlitSurface(menuscreen, NULL, screen, &dst2);*/
			uint16_t *d = (uint16_t*)screen->pixels + (screen->w - menuscreen->w) / 2 + (screen->h - menuscreen->h) * screen->pitch / 4;
	        uint16_t *s = (uint16_t*)menuscreen->pixels;
	        for (int y = 0; y < menuscreen->h; y++)
	        {
	            memmove(d, s, menuscreen->w * sizeof(uint16_t));
	            s += menuscreen->w;
	            d += screen->w;
	        }
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
