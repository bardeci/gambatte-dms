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
#include <dirent.h>

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
	DIR *ipu_dir = NULL;
	std::string ipu_OpenDinguxLegacy = ("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio");
	std::string ipu_RetroFW10 = ("/proc/jz/ipu_ratio");
	std::string ipu_RetroFW20 = ("/proc/jz/ipu");
	std::string ipu_OpenDingux = ("/sys/devices/platform/13080000.ipu");

	aspect_ratio_file = fopen(ipu_OpenDinguxLegacy.c_str(), "r+");
	if (aspect_ratio_file != NULL) {
		fclose(aspect_ratio_file);
		ipuscaling = ipu_OpenDinguxLegacy;
		printf("Detected IPU scaling - OpenDinguxLegacy\n");
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
	ipu_dir = opendir("/sys/devices/platform/13080000.ipu");
	if (ipu_dir != NULL) {
		closedir(ipu_dir);
		ipuscaling = "NEW_OD_IPU";
		printf("Detected IPU scaling - OpenDingux\n");
		return;
	}
	printf("Could not detect IPU scaling\n");
	return;
}

void SdlBlitter::SetVid(int w, int h, int bpp){	
#ifdef VERSION_GCW0
	screen = SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_TRIPLEBUF);
#elif VERSION_RETROFW
	screen = SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_TRIPLEBUF);
#elif defined VERSION_BITTBOY || defined VERSION_POCKETGO
	screen = SDL_SetVideoMode(w, h, bpp, SDL_SWSURFACE);
#else
	screen = SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_DOUBLEBUF);
#endif
}

void SdlBlitter::SetIPUAspectRatio(const char *ratiovalue){
	if (ipuscaling == "NEW_OD_IPU") {
		if(ratiovalue == "0"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_MODE=0");
		} else if(ratiovalue == "1"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_MODE=1");
		}
		return;
	}
	FILE *aspect_ratio_file = NULL;
	aspect_ratio_file = fopen(ipuscaling.c_str(), "r+");
	if (aspect_ratio_file != NULL) {
		fclose(aspect_ratio_file);
		aspect_ratio_file = fopen(ipuscaling.c_str(), "w");
		fwrite(ratiovalue, 1, 1, aspect_ratio_file);
		fclose(aspect_ratio_file);
	}
}

void SdlBlitter::SetIPUSharpness(const char *svalue){
	if (ipuscaling == "NEW_OD_IPU") {
		if(svalue == "0"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=0");
		} else if(svalue == "1"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=1");
		} else if(svalue == "2"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=2");
		} else if(svalue == "3"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=3");
		} else if(svalue == "4"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=4");
		} else if(svalue == "5"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=5");
		} else if(svalue == "6"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=6");
		} else if(svalue == "7"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=7");
		} else if(svalue == "8"){
			SDL_putenv("SDL_VIDEO_KMSDRM_SCALING_SHARPNESS=8");
		}
		return;
	}
	FILE *sharpness_file = NULL;
	sharpness_file = fopen("/sys/devices/platform/jz-lcd.0/sharpness_upscaling", "r+");
	if (sharpness_file != NULL) {
		fclose(sharpness_file);
		sharpness_file = fopen("/sys/devices/platform/jz-lcd.0/sharpness_upscaling", "w");
		fwrite(svalue, 1, 1, sharpness_file);
		fclose(sharpness_file);
	}
}

void SdlBlitter::setBufferDimensions() {
	SetIPUSharpness("1");
	if (selectedscaler == "No Scaling" ||
		selectedscaler == "1.5x Fast" ||
		selectedscaler == "1.5x Smooth" ||
		selectedscaler == "Aspect 1.66x Fast" ||
		selectedscaler == "Aspect 1.66x Smooth" ||
		selectedscaler == "FullScreen Fast" ||
		selectedscaler == "FullScreen Smooth")
	{
#ifdef VERSION_FUNKEYS
		SetVid(240, 240, 16);
#else
		SetVid(320, 240, 16);
#endif
	}
	else if (selectedscaler == "1.5x IPU")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		SetVid(208, 160, 16);
	}
	else if (selectedscaler == "Aspect IPU")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		SetVid(192, 144, 16);
	}
	else if (selectedscaler == "FullScreen IPU")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("0");
		SetVid(160, 144, 16);
	}
	else if (selectedscaler == "1.5x IPU-2x" ||
		selectedscaler == "1.5x DMG-2x" ||
		selectedscaler == "1.5x Scan-2x")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		SetVid(428, 320, 16);
	}
	else if (selectedscaler == "Aspect IPU-2x" ||
		selectedscaler == "Aspect DMG-2x" ||
		selectedscaler == "Aspect Scan-2x")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		SetVid(384, 288, 16);
	}
	else if (selectedscaler == "FullScreen IPU-2x" ||
		selectedscaler == "FullScreen DMG-2x" ||
		selectedscaler == "FullScreen Scan-2x")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("0");
		SetVid(320, 288, 16);
	}
	else if (selectedscaler == "1.5x DMG-3x" ||
		selectedscaler == "1.5x Scan-3x")
	{
		SetVid(640, 480, 16);
	}
	else if (selectedscaler == "Aspect DMG-3x" ||
		selectedscaler == "Aspect Scan-3x")
	{
		SetIPUSharpness("7");
		SetIPUAspectRatio("1");
		SetVid(576, 432, 16);
	}
	else if (selectedscaler == "FullScreen DMG-3x" ||
		selectedscaler == "FullScreen Scan-3x")
	{
		SetIPUSharpness("7");
		SetIPUAspectRatio("0");
		SetVid(480, 432, 16);
	}
	else
	{
		SetVid(320, 240, 16);
	}
	menu_set_screen(screen);
	init_ghostframes();
}

void SdlBlitter::setScreenRes() {
	SetIPUSharpness("1");
	if (selectedscaler == "No Scaling" ||
		selectedscaler == "1.5x Fast" ||
		selectedscaler == "1.5x Smooth" ||
		selectedscaler == "Aspect 1.66x Fast" ||
		selectedscaler == "Aspect 1.66x Smooth" ||
		selectedscaler == "FullScreen Fast" ||
		selectedscaler == "FullScreen Smooth")
	{
#ifdef VERSION_FUNKEYS
		if(screen->w != 240 || screen->h != 240) {
			SetVid(240, 240, 16);
		}
#else
		if(screen->w != 320 || screen->h != 240) {
			SetVid(320, 240, 16);
		}
#endif
	}
	else if (selectedscaler == "1.5x IPU")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		if(screen->w != 208 || screen->h != 160) {
			SetVid(208, 160, 16);
		}
	}
	else if (selectedscaler == "Aspect IPU")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		if(screen->w != 192 || screen->h != 144) {
			SetVid(192, 144, 16);
		}
	}
	else if (selectedscaler == "FullScreen IPU")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("0");
		if(screen->w != 160 || screen->h != 144) {
			SetVid(160, 144, 16);
		}
	}
	else if (selectedscaler == "1.5x IPU-2x" ||
		selectedscaler == "1.5x DMG-2x" ||
		selectedscaler == "1.5x Scan-2x")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		if(screen->w != 428 || screen->h != 320){
			SetVid(428, 320, 16);
		}
	}
	else if (selectedscaler == "Aspect IPU-2x" ||
		selectedscaler == "Aspect DMG-2x" ||
		selectedscaler == "Aspect Scan-2x")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("1");
		if(screen->w != 384 || screen->h != 288){
			SetVid(384, 288, 16);
		}
	}
	else if (selectedscaler == "FullScreen IPU-2x" ||
		selectedscaler == "FullScreen DMG-2x" ||
		selectedscaler == "FullScreen Scan-2x")
	{
		SetIPUSharpness("2");
		SetIPUAspectRatio("0");
		if(screen->w != 320 || screen->h != 288) {
			SetVid(320, 288, 16);
		}
	}
	else if (selectedscaler == "1.5x DMG-3x" ||
		selectedscaler == "1.5x Scan-3x")
	{
		if(screen->w != 640 || screen->h != 480){
			SetVid(640, 480, 16);
		}
	}
	else if (selectedscaler == "Aspect DMG-3x" ||
		selectedscaler == "Aspect Scan-3x")
	{
		SetIPUSharpness("7");
		SetIPUAspectRatio("1");
		if(screen->w != 576 || screen->h != 432){
			SetVid(576, 432, 16);
		}
	}
	else if (selectedscaler == "FullScreen DMG-3x" ||
		selectedscaler == "FullScreen Scan-3x")
	{
		SetIPUSharpness("7");
		SetIPUAspectRatio("0");
		if(screen->w != 480 || screen->h != 432) {
			SetVid(480, 432, 16);
		}
	}
	else
	{
		if(screen->w != 320 || screen->h != 240) {
			SetVid(320, 240, 16);
		}
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

void SdlBlitter::applyScalerToSurface(SDL_Surface *sourcesurface) {
	size_t offset;
	if (selectedscaler == "No Scaling")
	{
		SDL_Rect dst;
		dst.x = (screen->w - sourcesurface->w) / 2;
		dst.y = (screen->h - sourcesurface->h) / 2;
		dst.w = sourcesurface->w;
		dst.h = sourcesurface->h;
		SDL_BlitSurface(sourcesurface, NULL, screen, &dst);
	}
	else if (selectedscaler == "1.5x Fast")
	{
		offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
		scale15x((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "1.5x Smooth")
	{
#if defined VERSION_FUNKEYS
		offset = (2 * (240 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
		scale15x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, 240);
#else
		offset = (2 * (320 - 240) / 2) + ((240 - 216) / 2) * screen->pitch;
		scale15x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, 320);
#endif
	}
	else if (selectedscaler == "Aspect 1.66x Fast")
	{
		offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
		scale166x_fast((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "Aspect 1.66x Smooth")
	{
		offset = (2 * (320 - 266) / 2) + ((240 - 240) / 2) * screen->pitch;
		scale166x_pseudobilinear((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "FullScreen Fast")
	{
		fullscreen_upscale((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "FullScreen Smooth")
	{
		fullscreen_upscale_pseudobilinear((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "1.5x IPU-2x")
	{
		offset = (2 * (428 - 320) / 2) + ((320 - 288) / 2) * screen->pitch;
		scale15x_2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "1.5x DMG-2x")
	{
		offset = (2 * (428 - 320) / 2) + ((320 - 288) / 2) * screen->pitch;
		if (gameiscgb == 1){
			scale15x_dotmatrix2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalblack);
		} else {
			scale15x_dotmatrix2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalwhite);
		}
	}
	else if (selectedscaler == "1.5x Scan-2x")
	{
		offset = (2 * (428 - 320) / 2) + ((320 - 288) / 2) * screen->pitch;
		scale15x_crt2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "Aspect IPU-2x")
	{
		offset = (2 * (384 - 320) / 2) + ((288 - 288) / 2) * screen->pitch;
		scale166x_2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "Aspect DMG-2x")
	{
		offset = (2 * (384 - 320) / 2) + ((288 - 288) / 2) * screen->pitch;
		if (gameiscgb == 1){
			scale166x_dotmatrix2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalblack);
		} else {
			scale166x_dotmatrix2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalwhite);
		}
	}
	else if (selectedscaler == "Aspect Scan-2x")
	{
		offset = (2 * (384 - 320) / 2) + ((288 - 288) / 2) * screen->pitch;
		scale166x_crt2((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "1.5x DMG-3x")
	{
		offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
		if (gameiscgb == 1){
			scale15x_dotmatrix3((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalblack);
		} else {
			scale15x_dotmatrix3((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalwhite);
		}
	}
	else if (selectedscaler == "1.5x Scan-3x")
	{
		offset = (2 * (640 - 480) / 2) + ((480 - 432) / 2) * screen->pitch;
		scale15x_crt3((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "Aspect DMG-3x")
	{
		offset = (2 * (576 - 480) / 2) + ((432 - 432) / 2) * screen->pitch;
		if (gameiscgb == 1){
			scale166x_dotmatrix3((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalblack);
		} else {
			scale166x_dotmatrix3((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels, menupalwhite);
		}
	}
	else if (selectedscaler == "Aspect Scan-3x")
	{
		offset = (2 * (576 - 480) / 2) + ((432 - 432) / 2) * screen->pitch;
		scale166x_crt3((uint32_t*)((uint8_t *)screen->pixels + offset), (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "FullScreen IPU-2x")
	{
		fullscreen_2((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "FullScreen DMG-2x")
	{
		if (gameiscgb == 1){
			fullscreen_dotmatrix2((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels, menupalblack);
		} else {
			fullscreen_dotmatrix2((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels, menupalwhite);
		}
	}
	else if (selectedscaler == "FullScreen Scan-2x")
	{
		fullscreen_crt2((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "FullScreen DMG-3x")
	{
		if (gameiscgb == 1){
			fullscreen_dotmatrix3((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels, menupalblack);
		} else {
			fullscreen_dotmatrix3((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels, menupalwhite);
		}
	}
	else if (selectedscaler == "FullScreen Scan-3x")
	{
		fullscreen_crt3((uint32_t*)screen->pixels, (uint32_t*)sourcesurface->pixels);
	}
	else if (selectedscaler == "1.5x IPU" ||
		selectedscaler == "Aspect IPU" ||
		selectedscaler == "FullScreen IPU")
	{
		uint16_t *d = (uint16_t*)screen->pixels + (screen->w - sourcesurface->w) / 2 + (screen->h - sourcesurface->h) * screen->pitch / 4;
        uint16_t *s = (uint16_t*)sourcesurface->pixels;
        for (int y = 0; y < sourcesurface->h; y++)
        {
            memmove(d, s, sourcesurface->w * sizeof(uint16_t));
            s += sourcesurface->w;
            d += screen->w;
        }
	}
	else
	{
		uint16_t *d = (uint16_t*)screen->pixels + (screen->w - sourcesurface->w) / 2 + (screen->h - sourcesurface->h) * screen->pitch / 4;
        uint16_t *s = (uint16_t*)sourcesurface->pixels;
        for (int y = 0; y < sourcesurface->h; y++)
        {
            memmove(d, s, sourcesurface->w * sizeof(uint16_t));
            s += sourcesurface->w;
            d += screen->w;
        }
	}
}

static int frames = 0;
static clock_t old_time = 0;
static int fps = 0;

void SdlBlitter::draw() {

	clock_t cur_time;
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
		applyScalerToSurface(surface);
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
		applyScalerToSurface(currframe);
	}
	show_fps(screen, fps);
}

void SdlBlitter::scaleMenu() {

	if (!screen || !menuscreen)
		return;

	if(gambatte_p->isCgb()){
		if((colorfilter == 1) && (gameiscgb == 1)){
			apply_cfilter(menuscreen);
		}
	} else {
		convert_bw_surface_colors(menuscreen, menuscreen, menupalblack, menupaldark, menupallight, menupalwhite, 1); //if game is DMG, then menu matches DMG palette
	}
	applyScalerToSurface(menuscreen);
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
