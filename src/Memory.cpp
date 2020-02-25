#include "Memory.h"

#include <iostream>
#include <cassert>

#include "definitions.h"
#include "Log.h"
#include "Cartridge.h"
#include "MemoryBank.h"

Memory::Memory()
{
	_cartridge = Cartridge::Instance();
	init();
}

void Memory::init()
{
	for (int i = 0; i < MEMORY_SIZE; i++) {
		if (i < 0x400) {
			_memory[i] = _cartridge->getBlock(i);
		} else {
			_memory[i] = 0;
		}
	}

	_memory[Paging_Reg::kBank0] = 0;
	switchBank(0);
	_memory[Paging_Reg::kBank1] = 1;
	switchBank(1);
	_memory[Paging_Reg::kBank2] = 2;
	switchBank(2);

	switchRamBank();
}

void Memory::write(uint16_t address, uint8_t value) {
	if (address < 0x400) {
		// Cannot write in banks 0 and 1
	} else if (address < 0xC000) {
		// TODO: check if RAM is mapped
	} else if (address < 0xE000) {
		_memory[address] = value;
	} else if (address < 0xFFFC) {
		_memory[address - 0x2000] = value;
	} else {
		if (address == Paging_Reg::kRamSelect) {
			_memory[address] = value;
			_memory[address - 0x2000] = value;
			if (getBit8(value, 3) == 1 || getBit8(value, 4) == 1) {
				// TODO !!
				NOT_IMPLEMENTED("RAM mapping");
			}
			if ((value & 0b11) != 0) {
				NOT_IMPLEMENTED("Bank shifting");
			}
		} else if (address == Paging_Reg::kBank0) {
			uint8_t nbCartridgeBlock = static_cast<uint16_t>(_cartridge->getSize()) / 0x4000;
			value = value & ((nbCartridgeBlock & 0xF0) | 0x0F);
			_memory[address] = value;
			_memory[address - 0x2000] = value;
			switchBank(0);
		} else if (address == Paging_Reg::kBank1) {
			uint8_t nbCartridgeBlock = static_cast<uint16_t>(_cartridge->getSize()) / 0x4000;
			value = value & ((nbCartridgeBlock & 0xF0) | 0x0F);
			_memory[address] = value;
			_memory[address - 0x2000] = value;
			switchBank(1);
		} else if (address == Paging_Reg::kBank2) {
			uint8_t nbCartridgeBlock = static_cast<uint16_t>(_cartridge->getSize()) / 0x4000;
			if (nbCartridgeBlock > 0x0F) {
				value &= ((nbCartridgeBlock & 0xF0) | 0x0F);
			} else {
				value &= nbCartridgeBlock;
			}
			_memory[address] = value;
			_memory[address - 0x2000] = value;
			switchBank(2);
		} else {
			NOT_IMPLEMENTED("Paging write");
		}
	}
}

uint8_t Memory::read(uint16_t address) {
	if (address < 0x400) {
		return _memory[address];
	} else if (address < 0x4000) {
		return getBank0().at(address - 0);
	} else if (address < 0x8000) {
		return getBank1().at(address - 0x4000);
	} else if (address < 0xC000) {
		return getBank2().at(address - 0x8000);
	} else if (address < 0xE000) {
		return _memory[address];
	} else if(address < 0xFFFC) {
		return _memory[address - 0x2000];
	} else {
		if (address == Paging_Reg::kRamSelect || address == Paging_Reg::kBank0 || address == Paging_Reg::kBank1 || address == Paging_Reg::kBank2) {
			return _memory[address - 0x2000];
		} else {
			NOT_IMPLEMENTED("Paging read");
			return 0;
		}
	}
}

MemoryBank Memory::getBank0() {
	return _memoryBank0;
}

MemoryBank Memory::getBank1() {
	return _memoryBank1;
}

MemoryBank Memory::getBank2() {
	return _memoryBank2;
}

void Memory::switchBank(const int iBankIndex)
{
	assert(iBankIndex >= 0 && iBankIndex < 3);

	if(iBankIndex == 0) {
		_memoryBank0 = _cartridge->getBank(_memory[Paging_Reg::kBank0] * 0x4000);
	} else if(iBankIndex == 1) {
		_memoryBank1 = _cartridge->getBank(_memory[Paging_Reg::kBank1] * 0x4000);
	} else if(iBankIndex == 2) {
		_memoryBank2 = _cartridge->getBank(_memory[Paging_Reg::kBank2] * 0x4000);
	}
}

void Memory::switchRamBank()
{
	_ramBank = _cartridge->getRamBank(getBit8(_memory[Paging_Reg::kRamSelect], 2));
}