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

#include "gambatte.h"
#include "cpu.h"
#include "initstate.h"
#include "savestate.h"
#include "state_osd_elements.h"
#include "statesaver.h"
#include "bootloader.h"
#include <cstring>
#include <sstream>
#include <stdio.h>

static std::string const itos(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

static std::string const statePath(std::string const &basePath, int stateNo) {
	return basePath + "_" + itos(stateNo) + ".gqs";
}

namespace gambatte {

struct GB::Priv {
	CPU cpu;
	int stateNo;
	unsigned loadflags;

	Priv() : stateNo(1), loadflags(0) {}

	void full_init();
};

GB::GB() : p_(new Priv) {}

GB::~GB() {
	if (p_->cpu.loaded()){
		p_->cpu.saveSavedata();
	}

	delete p_;
}

std::string GB::getSaveStatePath(int statenum){
	return statePath(p_->cpu.saveBasePath(), statenum);
}

std::ptrdiff_t GB::runFor(gambatte::uint_least32_t *const videoBuf, std::ptrdiff_t const pitch,
                          gambatte::uint_least32_t *const soundBuf, std::size_t &samples) {
	if (!p_->cpu.loaded()) {
		samples = 0;
		return -1;
	}

	p_->cpu.setVideoBuffer(videoBuf, pitch);
	p_->cpu.setSoundBuffer(soundBuf);

	long const cyclesSinceBlit = p_->cpu.runFor(samples * 2);
	samples = p_->cpu.fillSoundBuffer();
	return cyclesSinceBlit >= 0
	     ? static_cast<std::ptrdiff_t>(samples) - (cyclesSinceBlit >> 1)
	     : cyclesSinceBlit;
}

void GB::Priv::full_init() {

	SaveState state;
	cpu.setStatePtrs(state);
	setInitState(state, cpu.isCgb(), loadflags & GBA_CGB);
   
	cpu.mem_.bootloader.reset();
	cpu.mem_.bootloader.set_address_space_start((void*)cpu.rombank0_ptr());
	cpu.mem_.bootloader.load(cpu.isCgb(), loadflags & GBA_CGB);

	if (cpu.mem_.bootloader.using_bootloader) {
		uint8_t *ioamhram = (uint8_t*)state.mem.ioamhram.get();
		uint8_t serialctrl = (cpu.isCgb() || loadflags & GBA_CGB) ? 0x7C : 0x7E;
		state.cpu.pc = 0x0000;
		// the hw registers must be zeroed out to prevent the logo from being garbled
		std::memset((void*)(ioamhram + 0x100), 0x00, 0x100);
		//init values taken from SameBoy
		ioamhram[0x100] = 0xCF;//joypad initial value
		ioamhram[0x102] = serialctrl;//serialctrl
		ioamhram[0x148] = 0xFC;//object palette 0
		ioamhram[0x149] = 0xFC;//object palette 1
	}
   
	cpu.loadState(state);
	cpu.loadSavedata();
}

void GB::reset() {
	if (p_->cpu.loaded()) {
		p_->cpu.saveSavedata();

		SaveState state;
		p_->cpu.setStatePtrs(state);
		setInitState(state, p_->cpu.isCgb(), p_->loadflags & GBA_CGB);
		p_->cpu.loadState(state);
		p_->cpu.loadSavedata();
		//p_->full_init();
	}
}

void GB::setInputGetter(InputGetter *getInput) {
	p_->cpu.setInputGetter(getInput);
}

void GB::setBootloaderGetter(bool (*getter)(void* userdata, bool isgbc, uint8_t* data, uint32_t max_size)) {
   p_->cpu.mem_.bootloader.set_bootloader_getter(getter);
}

void GB::setSaveDir(std::string const &sdir) {
	p_->cpu.setSaveDir(sdir);
}

LoadRes GB::load(std::string const &romfile, unsigned const flags, int const preferCGB) {
	if (p_->cpu.loaded())
		p_->cpu.saveSavedata();

	LoadRes const loadres = p_->cpu.load(romfile,
	                                     flags & FORCE_DMG,
	                                     flags & MULTICART_COMPAT,
	                                     preferCGB);
	if (loadres == LOADRES_OK) {
		//SaveState state;
		//p_->cpu.setStatePtrs(state);
		//p_->loadflags = flags;
		//setInitState(state, p_->cpu.isCgb(), flags & GBA_CGB);
		//p_->cpu.loadState(state);
		//p_->cpu.loadSavedata();
		p_->full_init();

		p_->stateNo = 0;
		p_->cpu.setOsdElement(transfer_ptr<OsdElement>());
	}

	return loadres;
}

bool GB::isCgb() const {
	return p_->cpu.isCgb();
}

bool GB::isLoaded() const {
	return p_->cpu.loaded();
}

void GB::saveSavedata() {
	if (p_->cpu.loaded()){
		p_->cpu.saveSavedata();
		printf("Saving savedata...\n");
	}
}

void GB::setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
	p_->cpu.setDmgPaletteColor(palNum, colorNum, rgb32);
}

void GB::setColorFilter(int activated, int filtercolors[12]) {
	p_->cpu.setColorFilter(activated, filtercolors);
}

bool GB::loadState(std::string const &filepath) {
	if (p_->cpu.loaded()) {
		p_->cpu.saveSavedata();

		SaveState state;
		p_->cpu.setStatePtrs(state);

		if (StateSaver::loadState(state, filepath)) {
			p_->cpu.loadState(state);
			p_->cpu.mem_.bootloader.choosebank(state.mem.ioamhram.get()[0x150] != 0xFF);
			return true;
		}
	}

	return false;
}

bool GB::saveState(gambatte::uint_least32_t const *videoBuf, std::ptrdiff_t pitch) {
	if (saveState(videoBuf, pitch, statePath(p_->cpu.saveBasePath(), p_->stateNo))) {
		p_->cpu.setOsdElement(newStateSavedOsdElement(p_->stateNo));
		return true;
	}

	return false;
}

bool GB::saveState_NoOsd(gambatte::uint_least32_t const *videoBuf, std::ptrdiff_t pitch) {
	if (saveState(videoBuf, pitch, statePath(p_->cpu.saveBasePath(), p_->stateNo))) {
		return true;
	}

	return false;
}

bool GB::loadState() {
	if (loadState(statePath(p_->cpu.saveBasePath(), p_->stateNo))) {
		p_->cpu.setOsdElement(newStateLoadedOsdElement(p_->stateNo));
		return true;
	}

	return false;
}

bool GB::loadState_NoOsd() {
	if (loadState(statePath(p_->cpu.saveBasePath(), p_->stateNo))) {
		return true;
	}

	return false;
}

bool GB::saveState(gambatte::uint_least32_t const *videoBuf, std::ptrdiff_t pitch,
                   std::string const &filepath) {
	if (p_->cpu.loaded()) {
		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->cpu.saveState(state);
		return StateSaver::saveState(state, videoBuf, pitch, filepath);
	}

	return false;
}

void GB::selectState(int n) {
	n -= (n / 10) * 10;
	p_->stateNo = n < 0 ? n + 10 : n;

	if (p_->cpu.loaded()) {
		std::string const &path = statePath(p_->cpu.saveBasePath(), p_->stateNo);
		p_->cpu.setOsdElement(newSaveStateOsdElement(path, p_->stateNo));
	}
}

void GB::selectState_NoOsd(int n) {
	n -= (n / 10) * 10;
	p_->stateNo = n < 0 ? n + 10 : n;

	if (p_->cpu.loaded()) {
		std::string const &path = statePath(p_->cpu.saveBasePath(), p_->stateNo);
	}
}

int GB::currentState() const { return p_->stateNo; }

std::string const GB::romTitle() const {
	if (p_->cpu.loaded()) {
		char title[0x11];
		std::memcpy(title, p_->cpu.romTitle(), 0x10);
		title[title[0xF] & 0x80 ? 0xF : 0x10] = '\0';
		return std::string(title);
	}

	return std::string();
}

PakInfo const GB::pakInfo() const { return p_->cpu.pakInfo(p_->loadflags & MULTICART_COMPAT); }

void GB::setGameGenie(std::string const &codes) {
	p_->cpu.setGameGenie(codes);
}

void GB::setGameShark(std::string const &codes) {
	p_->cpu.setGameShark(codes);
}

}
