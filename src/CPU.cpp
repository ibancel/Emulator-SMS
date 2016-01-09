#include <fstream>
#include <string>

#include "CPU.h"

CPU::CPU(Memory *m, Graphics *g, Cartridge *c)
{
	_memory = m;
	_graphics = g;
	_cartridge = c;

	_pc = 0x0; // TODO changer à 0, 0x66 marche avec interrupt ?
	_sp = MEMORY_SIZE; // = 0

	for(int i = 0 ; i < REGISTER_SIZE ; i++)
		_register[i] = 0;

	for(int i = 0 ; i < MEMORY_SIZE ; i++)
		_stack[i] = 0;

	/// vv A ENLEVER vv
	_memory->init();

	ifstream fichier("ROMS/zexall.sms");
	//ifstream fichier("ROMS/Trans-Bot (UE).sms");

	if(!fichier) exit(EXIT_FAILURE);

	char h;
	uint8_t val;
	uint16_t compteur = 0x0;
	while(compteur < MEMORY_SIZE)
	{
		fichier.read(&h, sizeof(char));
		val = static_cast<uint8_t>(h);
		//cout << hex << (unsigned int)val << " ";

		if(!fichier.good())
		{
			break;
			fichier.close();
		}

		_memory->write(compteur++, val);
	}
	/*
	_memory[0] = 0b00111110;
	_memory[1] = 0x23;
	_memory[2] = 0b11000110;
	_memory[3] = 0x13;
	_memory[4] = 0b11000110;
	_memory[5] = 0x42;*/
	/// ^^ A ENLEVER ^^
}

void CPU::cycle()
{
	//while(true)
	{
		uint8_t prefix = _memory->read(_pc++);
		uint8_t opcode = prefix;

		if(prefix == 0xCB || prefix == 0xED || prefix == 0xED || prefix == 0xFD)
		{
			opcode = _memory->read(_pc++);
		}
		else
			prefix = 0;

		opcodeExecution(prefix, opcode);

		if(_pc > 0x8000) exit(8);
	}
}

resInstruction CPU::opcodeExecution(uint8_t prefix, uint8_t opcode)
{
	/// TODO: à prendre en compte les changements pour ALU
	/// TODO: gérer les interruptions

	slog << ldebug << hex <<  "#" << (uint16_t)(_pc-1) << " : " << (uint16_t) opcode;
	if(prefix != 0)
			slog << "(" << (uint16_t)prefix << ")";
	slog << endl;
	//cout << "regA : " << (uint16_t)_register[R_A] << endl;

	resInstruction res;

	uint8_t x = (opcode>>6) & 0b11;
	uint8_t y = (opcode>>3) & 0b111;
	uint8_t z = (opcode>>0) & 0b111;
	uint8_t p = (y >> 1);
	uint8_t q = y & 0b1;

	if(prefix == 0)
		opcode0(x,y,z,p,q);
	else if(prefix == 0xCB)
		opcodeCB(x,y,z,p,q);
	else if(prefix == 0xED)
		opcodeED(x,y,z,p,q);
}

void CPU::aluOperation(uint8_t index, uint8_t value)
{
	if(index == 0) // ADD A
	{
		uint8_t sum = (uint8_t)(_register[R_A] + value);
		setFlagBit(F_S, ((int8_t)(sum) < 0));
		setFlagBit(F_Z, (_register[R_A] == value));
		setFlagBit(F_H, ((_register[R_A]&0xF) + (value&0xF) < (_register[R_A]&0xF)));
		setFlagBit(F_P, ((_register[R_A]>>7)==(value>>7) && (value>>7)!=(sum>>7)));
		setFlagBit(F_N, 0);
		setFlagBit(F_C, (sum < _register[R_A]));
		setFlagBit(F_F3, (sum>>2)&1);
		setFlagBit(F_F5, (sum>>4)&1);

		_register[R_A] = sum;
	}
	//
	else if(index == 5) // XOR
	{
		_register[R_A] ^= value;
		setFlagBit(F_S, ((int8_t)(_register[R_A]) < 0));
		setFlagBit(F_Z, (_register[R_A] == 0));
		setFlagBit(F_H, 0);
		setFlagBit(F_P, (_register[R_A]>>7));
		setFlagBit(F_N, 0);
		setFlagBit(F_C, 0);
		setFlagBit(F_F3, (_register[R_A]>>2)&1);
		setFlagBit(F_F5, (_register[R_A]>>4)&1);
	}
	//
	else if(index == 7) // CP
	{
		/// TODO vérifier borrows & overflow
		uint8_t sum = _register[R_A] - value;
		setFlagBit(F_S, ((int8_t)(sum) < 0));
		setFlagBit(F_Z, (sum == 0));
		setFlagBit(F_H, ((_register[R_A]&0xF) - (value&0xF) < (_register[R_A]&0xF))); // ?
		setFlagBit(F_P, ((_register[R_A]>>7)==(value>>7) && (value>>7)!=(sum>>7))); // ?
		setFlagBit(F_N, 0);
		setFlagBit(F_C, (sum > _register[R_A])); // ?
		setFlagBit(F_F3, (value>>2)&1);
		setFlagBit(F_F5, (value>>4)&1);

		slog << ldebug << "CP(" << hex << (uint16_t)_register[R_A] << "," << (uint16_t)value << "): " << (uint16_t)(sum == 0) << endl;
	}
	//
	else
		slog << lwarning << "ALU " << (uint16_t)index << " is not implemented" << endl;
}

void CPU::portCommunication(bool rw, uint8_t address, uint16_t data)
{
	if(address == 0x7E || address == 0x7F || address == 0xBE || address == 0xBF)
	{
		/// TODO !
		/*if(rw)
			_graphics->write(address, data);
		else
			_graphics->read(address, data);*/
	}
}


/// PRIVATE :

bool CPU::isPrefixByte(uint8_t byte)
{
	return (byte == 0xCB || byte == 0xDD || byte == 0xED || byte == 0xFD);
}

void CPU::opcode0(uint8_t x, uint8_t y, uint8_t z, uint8_t p, uint8_t q)
{
	if(x == 0 && z == 0 && y == 0) // NOP
	{

	}
	//
	else if(x == 0 && z == 0 && y == 3) // JR d
	{
		_pc += (int8_t)(_memory->read(_pc)) + 1;
	}
	else if(x == 0 && z == 0 && y >= 4) // JR cc[y-4],d
	{
		if(condition(y-4))
			_pc += (int8_t)_memory->read(_pc) + 1;
	}
	//
	else if(x == 0 && z == 1 && q == 0) // LD rp[p],nn
	{
		uint16_t val = _memory->read(_pc++);
		val += (_memory->read(_pc++)<<8);
		setRegisterPair(p, val);
	}
	//
	else if(x == 0 && z == 2 && q == 0 && p == 3) // LD (nn), A
	{
		uint16_t addr = _memory->read(_pc++);
		addr += (_memory->read(_pc++)<<8);
		_memory->write(addr, _register[R_A]);
	}
	//
	else if(x == 0 && z == 2 && q == 1 && p == 3) // LD A, (nn)
	{
		uint16_t val = _memory->read(_pc++);
		val += (_memory->read(_pc++)<<8);
		_register[R_A] = _memory->read(val);

		slog << ldebug << "addr: " << hex << val << " val: " << (uint16_t)_register[R_A] << endl;
	}
	//
	else if(x == 0 && z == 3 && q == 0) // INC rp[p]
	{
		setRegisterPair(p, getRegisterPair(p)+1);
	}
	else if(x == 0 && z == 3 && q == 1) // DEC rp[p]
	{
		setRegisterPair(p, getRegisterPair(p)-1);
	}
	else if(x == 0 && z == 4) // INC r[y]
	{
		/// TODO flags
		_register[y]++;
	}
	else if(x == 0 && z == 5) // DEC r[y]
	{
		/// TODO flags
		_register[y]--;
	}
	else if(x == 0 && z == 6) // LD r[y],n
	{
		_register[y] = _memory->read(_pc++);
	}
	//
	else if(x == 1 && z != 7)
	{
		_register[y] = _register[z];
	}
	else if(x == 1 && z == 7) // HALT
	{
		/// TODO
	}
	else if(x == 2) // alu[y] r[z]
	{
		aluOperation(y, _register[z]);
	}
	//
	else if(x == 3 && z == 2) // JP cc[y],nn
	{
		uint16_t addr = _memory->read(_pc++);
		addr += (_memory->read(_pc++)<<8);
		if(condition(y))
			_pc = addr;
		slog << ldebug << "JP C,nn : " << (condition(y)?"true":"false") << " new addr: " << hex << _pc << endl;
	}
	//
	else if(x == 3 && z == 3 && y == 0) // JP nn
	{
		uint16_t val = _memory->read(_pc++);
		val += (_memory->read(_pc++) << 8);
		_pc = val;
		std::bitset<16> y(_pc);
		slog << ldebug << hex << "val: " << val << endl;
	}
	//
	else if(x == 3 && z == 3 && y == 2) // OUT (n),A
	{
		/// TODO voir les sorties
		uint8_t val = _memory->read(_pc++);
		portCommunication(true, val, _register[R_A]);
		slog << ldebug << "OUT ====> " << hex << (uint16_t)val << endl;
	}
	//
	else if(x == 3 && z == 3 && y == 6) // DI
	{
		/// TODO interrupts
		slog << ldebug << "-- TODO DI" << endl;
	}
	//
	else if(x == 3 && z == 5 && q == 0) // PUSH rp2[p]
	{
		/// TODO
		slog << ldebug << "-- TODO PUSH" << endl;
		uint16_t val = getRegisterPair2(p);
		_stack[_sp--] = (val >> 8);
		_stack[_sp--] = (val & 0xFF);
		//set
	}
	//
	else if(x == 3 && z == 5 && q == 1 && p == 0) // CALL nn
	{
		uint16_t newPC = _memory->read(_pc++);
		newPC += (_memory->read(_pc++)<<8);

		_stack[_sp--] = (_pc >> 8);
		_stack[_sp--] = (_pc & 0xFF);

		_pc = newPC;

		slog << ldebug << "CALL to " << hex << newPC << endl;
	}
	//
	else if(x == 3 && z == 6) // alu[y] n
	{
		aluOperation(y, _memory->read(_pc++));
	}
	//
	else
	{
		slog << lwarning << "Opcode : " << hex << (uint16_t)((x<<6)+(y<<3)+z) << " is not implemented" << endl;
		slog << ldebug << "# pc: " << hex << _pc << endl;
		//slog << ldebug << "x: " << (uint16_t)x << " | y: " << (uint16_t)y << " | z: " << (uint16_t)z << endl;
	}
}

void CPU::opcodeCB(uint8_t x, uint8_t y, uint8_t z, uint8_t p, uint8_t q)
{

}

void CPU::opcodeED(uint8_t x, uint8_t y, uint8_t z, uint8_t p, uint8_t q)
{
	if(x == 0 || x == 3) // NOP
	{

	}
	//
	else if(x == 1 && z == 6) // IM im[y]
	{
		/// TODO
		slog << ldebug << "-- TODO IM" << endl;
	}
	//
	else
	{
		slog << lwarning << "Opcode : " << hex << (uint16_t)((x<<6)+(y<<3)+z) << " (ED) is not implemented" << endl;
	}
}

bool CPU::condition(uint8_t code)
{
	if(code == 0)
		return !getFlagBit(F_Z);
	else if(code == 1)
		return getFlagBit(F_Z);
	else if(code == 2)
		return !getFlagBit(F_C);
	else if(code == 3)
		return getFlagBit(F_C);

	return false;
}

void CPU::setRegisterPair(uint8_t code, uint16_t value)
{
	uint8_t first = (value >> 8);
	uint8_t second = (value & 0xFF);

	switch(code)
	{
		case 0:
			_register[R_B] = first;
			_register[R_C] = second;
			break;
		case 1:
			_register[R_D] = first;
			_register[R_E] = second;
			break;
		case 2:
			_register[R_H] = first;
			_register[R_L] = second;
			break;
		case 3:
			_sp = value;
			break;
		default:
			break;
	}
}

void CPU::setRegisterPair2(uint8_t code, uint16_t value)
{
	uint8_t first = (value >> 8);
	uint8_t second = (value & 0xFF);

	switch(code)
	{
		case 0:
			_register[R_B] = first;
			_register[R_C] = second;
			break;
		case 1:
			_register[R_D] = first;
			_register[R_E] = second;
			break;
		case 2:
			_register[R_H] = first;
			_register[R_L] = second;
			break;
		case 3:
			_register[R_A] = first;
			_registerFlag = second;
			break;
		default:
			break;
	}
}

void CPU::setFlagBit(F_NAME f, uint8_t value)
{
	if(value == 1)
		_registerFlag |= 1 << (uint8_t)f;
	else
		_registerFlag &= ~(1 << (uint8_t)f);
}


// get:

uint16_t CPU::getRegisterPair(uint8_t code)
{
	return 0;
	if(code < 3)
		return ((_register[code*2]<<8) + _register[code*2+1]);

	return _sp;
}

uint16_t CPU::getRegisterPair2(uint8_t code)
{
	return 0;
	if(code < 3)
		return ((_register[code*2]<<8) + _register[code*2+1]);

	return ((_register[R_A]<<8) + _registerFlag);
}

bool CPU::getFlagBit(F_NAME f)
{
	return (_registerFlag >> (uint8_t)f) & 1;
}