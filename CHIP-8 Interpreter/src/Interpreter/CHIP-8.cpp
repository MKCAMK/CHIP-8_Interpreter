#include <cstdlib>
#include <ctime>

#include "Interpreter\CHIP-8.h"

const uint8_t CHIP_8::Font[NUMBER_OF_FONT_SPRITES][SIZE_OF_FONT_SPRITES] =
{
	{ 0xF0, 0x90, 0x90, 0x90, 0xF0 },     /* "0" sprite*/
	{ 0x20, 0x60, 0x20, 0x20, 0x70 },     /* "1" sprite*/
	{ 0xF0, 0x10, 0xF0, 0x80, 0xF0 },     /* "2" sprite*/
	{ 0xF0, 0x10, 0xF0, 0x10, 0xF0 },     /* "3" sprite*/
	{ 0x90, 0x90, 0xF0, 0x10, 0x10 },     /* "4" sprite*/
	{ 0xF0, 0x80, 0xF0, 0x10, 0xF0 },     /* "5" sprite*/
	{ 0xF0, 0x80, 0xF0, 0x90, 0xF0 },     /* "6" sprite*/
	{ 0xF0, 0x10, 0x20, 0x40, 0x40 },     /* "7" sprite*/
	{ 0xF0, 0x90, 0xF0, 0x90, 0xF0 },     /* "8" sprite*/
	{ 0xF0, 0x90, 0xF0, 0x10, 0xF0 },     /* "9" sprite*/
	{ 0xF0, 0x90, 0xF0, 0x90, 0x90 },     /* "A" sprite*/
	{ 0xE0, 0x90, 0xE0, 0x90, 0xE0 },     /* "B" sprite*/
	{ 0xF0, 0x80, 0x80, 0x80, 0xF0 },     /* "C" sprite*/
	{ 0xE0, 0x90, 0x90, 0x90, 0xE0 },     /* "D" sprite*/
	{ 0xF0, 0x80, 0xF0, 0x80, 0xF0 },     /* "E" sprite*/
	{ 0xF0, 0x80, 0xF0, 0x80, 0x80}       /* "F" sprite*/
};

 CHIP_8::CHIP_8()
 {
	 Reset();
 }

void CHIP_8::Reset()
{
	for (unsigned int i = 0; i < MEMORY_SIZE; ++i)
	{
		Memory[i] = 0;
	}
	for (unsigned int i = 0; i < STACK_SIZE; ++i)
	{
		Stack[i] = 0;
	}

	LoadFonts();

	for (unsigned int i = 0; i < NUMBER_OF_GENERAL_REGISTERS; ++i)
	{
		Register_Vx[i] = 0;
	}

	Register_I = 0;

	Register_PC = 0;
	Register_SP = 0;

	Timer_DT = 0;
	Timer_ST = 0;
	SoundEmitted = false;

	for (unsigned int i = 0; i < NUMBER_OF_BUTTONS; ++i)
	{
		Keypad[i] = false;
	}
	ButtonHeld = false;
	HeldButton = 0;

	DrawingHappened = false;
	ClearDisplay();

	srand(static_cast<unsigned int>(time(0)));

	CurrentStatus = CHIP_8_ERROR_CODE__RESET;
}

void CHIP_8::LoadFonts()
{
	for (unsigned int i = 0; i < NUMBER_OF_FONT_SPRITES; ++i)
	{
		for (unsigned int j = 0; j < SIZE_OF_FONT_SPRITES; ++j)
		{
			Memory[FONT_AREA_START_ADDRESS + (i * SIZE_OF_FONT_SPRITES) + j] = Font[i][j];
		}
	}
}

void CHIP_8::AdvanceProgramCounter()
{
	if (Register_PC < (MEMORY_SIZE - 2))
	{
		Register_PC += 2;
	}
	else
		CurrentStatus = CHIP_8_ERROR_CODE__OUT_OF_BOUNDS_MEMORY_ACCESS;
}

void CHIP_8::PushStack()
{
	if (Register_SP < (STACK_SIZE))
	{
		Stack[Register_SP] = Register_PC;
		++Register_SP;
	}
	else
		CurrentStatus = CHIP_8_ERROR_CODE__STACK_OVERFLOW;
}

void CHIP_8::PopStack()
{
	if (Register_SP > 0)
	{
		--Register_SP;
		Register_PC = Stack[Register_SP];
	}
	else
		CurrentStatus = CHIP_8_ERROR_CODE__STACK_UNDERFLOW;
}

bool CHIP_8::IsMemoryAccessSafe()
{
	if (Register_I >= MEMORY_SIZE)
	{
		CurrentStatus = CHIP_8_ERROR_CODE__OUT_OF_BOUNDS_MEMORY_ACCESS;
		return false;
	}
	else
		return true;
}

void CHIP_8::ClearDisplay()
{
	for (unsigned int i = 0; i < RESOLUTION_X; ++i)
	{
		for (unsigned int j = 0; j < RESOLUTION_Y; ++j)
		{
			Display[i][j] = false;
		}
	}
	DrawingHappened = true;
}

bool CHIP_8::GetSound()
{
	return SoundEmitted;
}

void CHIP_8::PressButton(unsigned int Button)
{
	if (Button < NUMBER_OF_BUTTONS)
		Keypad[Button] = true;
}

void CHIP_8::UnpressButton(unsigned int Button)
{
	if (Button < NUMBER_OF_BUTTONS)
		Keypad[Button] = false;
}

bool CHIP_8::DidDrawingHappen()
{
	bool ReturnValue = DrawingHappened;
	DrawingHappened = false;
	return ReturnValue;
}

bool CHIP_8::GetDisplay(unsigned int PositionX, unsigned int PositionY)
{
	if (PositionX < RESOLUTION_X)
		if (PositionY < RESOLUTION_Y)
			return Display[PositionX][PositionY];
	return false;
}

CHIP_8_ERROR_CODE CHIP_8::LoadProgram(char* DataPointer, unsigned int DataSize)
{
	if (CurrentStatus != CHIP_8_ERROR_CODE__RESET)
		Reset();

	if (DataSize > (MEMORY_SIZE - PROGRAM_AREA_START_ADDRESS))
	{
		CurrentStatus = CHIP_8_ERROR_CODE__PROGRAM_TOO_BIG;
	}
	else
	{
		for (unsigned int i = 0; i < DataSize; ++i)
		{
			Memory[PROGRAM_AREA_START_ADDRESS + i] = DataPointer[i];
		}
		CurrentStatus = CHIP_8_ERROR_CODE__STATUS_OK;
	}
	Register_PC = PROGRAM_AREA_START_ADDRESS;
	return CurrentStatus;
}

CHIP_8_ERROR_CODE CHIP_8::Step(unsigned int NumberOf60thOfSecond)
{
	if (CurrentStatus)
		return CurrentStatus;

	if (Timer_DT)
	{
		if (NumberOf60thOfSecond > static_cast<unsigned int>(Timer_DT))
			Timer_DT = 0;
		else
			Timer_DT -= static_cast<uint8_t>(NumberOf60thOfSecond);
	}
	if (Timer_ST)
	{
		if (NumberOf60thOfSecond > static_cast<unsigned int>(Timer_ST))
		{
			Timer_ST = 0;
			SoundEmitted = false;
		}
		else
		{
			Timer_ST -= static_cast<uint8_t>(NumberOf60thOfSecond);
			if(Timer_ST == 0)
				SoundEmitted = false;
		}
	}
	else
		SoundEmitted = false;

	FetchInstruction();

	return CurrentStatus;
}

void CHIP_8::FetchInstruction()
{

	if (Register_PC < (MEMORY_SIZE - 1))
	{
		uint16_t FetchedInstruction = Memory[Register_PC];
		FetchedInstruction <<= 8;
		FetchedInstruction += Memory[Register_PC + 1];
		InstructionSwitch(FetchedInstruction);
	}
	else
	{
		CurrentStatus = CHIP_8_ERROR_CODE__OUT_OF_BOUNDS_MEMORY_ACCESS;
	}
}

void CHIP_8::InstructionSwitch(uint16_t FetchedInstruction)
{
	uint16_t part = FetchedInstruction & 0xF000;
	switch (part)
	{
		case 0x0000:
		{
			part = FetchedInstruction & 0x0F00;
			switch (part)
			{
				case 0x0000:
				{
					part = FetchedInstruction & 0x00FF;
					switch (part)
					{
						case 0x00E0:
						{
							Instruction_00E0__CLS();
							break;
						}
						case 0x00EE:
						{
							Instruction_00EE__RET();
							break;
						}
						default:
							CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED;
					}
					break;
				}
				default:
					Instruction_0nnn__SYS_addr(FetchedInstruction);
			}
			break;
		}
		case 0x1000:
		{
			Instruction_1nnn__JP_addr(FetchedInstruction);
			break;
		}
		case 0x2000:
		{
			Instruction_2nnn__CALL_addr(FetchedInstruction);
			break;
		}
		case 0x3000:
		{
			Instruction_3xnn__SE_Vx_byte(FetchedInstruction);
			break;
		}
		case 0x4000:
		{
			Instruction_4xnn__SNE_Vx_byte(FetchedInstruction);
			break;
		}
		case 0x5000:
		{
			part = FetchedInstruction & 0x000F;
			switch (part)
			{
				case 0x0000:
				{
					Instruction_5xy0__SE_Vx_Vy(FetchedInstruction);
					break;
				}
				default:
					CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED;
			}
			break;
		}
		case 0x6000:
		{
			Instruction_6xnn__LD_Vx_byte(FetchedInstruction);
			break;
		}
		case 0x7000:
		{
			Instruction_7xnn__ADD_Vx_byte(FetchedInstruction);
			break;
		}
		case 0x8000:
		{
			part = FetchedInstruction & 0x000F;
			switch (part)
			{
				case 0x0000:
				{
					Instruction_8xy0__LD_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0001:
				{
					Instruction_8xy1__OR_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0002:
				{
					Instruction_8xy2__AND_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0003:
				{
					Instruction_8xy3__XOR_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0004:
				{
					Instruction_8xy4__ADD_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0005:
				{
					Instruction_8xy5__SUB_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0006:
				{
					Instruction_8xy6__SHR_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x0007:
				{
					Instruction_8xy7__SUBN_Vx_Vy(FetchedInstruction);
					break;
				}
				case 0x000E:
				{
					Instruction_8xyE__SHL_Vx_Vy(FetchedInstruction);
					break;
				}
				default:
					CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED;
			}
			break;
		}
		case 0x9000:
		{
			part = FetchedInstruction & 0x000F;
			switch (part)
			{
				case 0x0000:
				{
					Instruction_9xy0__SNE_Vx_Vy(FetchedInstruction);
					break;
				}
				default:
					CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED;
			}
			break;
		}
		case 0xA000:
		{
			Instruction_Annn__LD_I_addr(FetchedInstruction);
			break;
		}
		case 0xB000:
		{
			Instruction_Bnnn__JP_V0_addr(FetchedInstruction);
			break;
		}
		case 0xC000:
		{
			Instruction_Cxnn__RND_Vx_byte(FetchedInstruction);
			break;
		}
		case 0xD000:
		{
			Instruction_Dxyn__DRW_Vx_Vy_nibble(FetchedInstruction);
			DrawingHappened = true;
			break;
		}
		case 0xE000:
		{
			part = FetchedInstruction & 0x00FF;
			switch (part)
			{
				case 0x009E:
				{
					Instruction_Ex9E__SKP_Vx(FetchedInstruction);
					break;
				}
				case 0x00A1:
				{
					Instruction_ExA1__SKNP_Vx(FetchedInstruction);
					break;
				}
				default:
					CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED;
			}
			break;
		}
		case 0xF000:
		{
			part = FetchedInstruction & 0x00FF;
			switch (part)
			{
				case 0x0007:
				{
					Instruction_Fx07__LD_Vx_DT(FetchedInstruction);
					break;
				}
				case 0x000A:
				{
					Instruction_Fx0A__LD_Vx_K(FetchedInstruction);
					break;
				}
				case 0x0015:
				{
					Instruction_Fx15__LD_DT_Vx(FetchedInstruction);
					break;
				}
				case 0x0018:
				{
					Instruction_Fx18__LD_ST_Vx(FetchedInstruction);
					break;
				}
				case 0x001E:
				{
					Instruction_Fx1E__ADD_I_Vx(FetchedInstruction);
					break;
				}
				case 0x0029:
				{
					Instruction_Fx29__LD_F_Vx(FetchedInstruction);
					break;
				}
				case 0x0033:
				{
					Instruction_Fx33__LD_B_Vx(FetchedInstruction);
					break;
				}
				case 0x0055:
				{
					Instruction_Fx55__LD_I_Vx(FetchedInstruction);
					break;
				}
				case 0x0065:
				{
					Instruction_Fx65__LD_Vx_I(FetchedInstruction);
					break;
				}
				default:
					CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED;
			}
			break;
		}
	}
}

void CHIP_8::Instruction_0nnn__SYS_addr(uint16_t FetchedInstruction)
{
	CurrentStatus = CHIP_8_ERROR_CODE__INSTRUCTION_0NNN_NOT_IMPLEMENTED;
}

void CHIP_8::Instruction_00E0__CLS()
{
	ClearDisplay();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_00EE__RET()
{
	PopStack();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_1nnn__JP_addr(uint16_t FetchedInstruction)
{
	Register_PC = FetchedInstruction & 0x0FFF;
}

void CHIP_8::Instruction_2nnn__CALL_addr(uint16_t FetchedInstruction)
{
	PushStack();
	Register_PC = FetchedInstruction & 0x0FFF;
}

void CHIP_8::Instruction_3xnn__SE_Vx_byte(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	if(Register_Vx[Vx] == (FetchedInstruction & 0x00FF))
		AdvanceProgramCounter();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_4xnn__SNE_Vx_byte(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	if (Register_Vx[Vx] != (FetchedInstruction & 0x00FF))
		AdvanceProgramCounter();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_5xy0__SE_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	if(Register_Vx[Vx] == Register_Vx[Vy])
		AdvanceProgramCounter();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_6xnn__LD_Vx_byte(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	Register_Vx[Vx] = FetchedInstruction & 0x00FF;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_7xnn__ADD_Vx_byte(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	Register_Vx[Vx] += FetchedInstruction & 0x00FF;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy0__LD_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	Register_Vx[Vx] = Register_Vx[Vy];
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy1__OR_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	Register_Vx[Vx] |= Register_Vx[Vy];
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy2__AND_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	Register_Vx[Vx] &= Register_Vx[Vy];
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy3__XOR_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	Register_Vx[Vx] ^= Register_Vx[Vy];
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy4__ADD_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	unsigned int Result = Register_Vx[Vx] + Register_Vx[Vy];
	Register_Vx[Vx] = Result;
	Register_Vx[0xF] = (Result > 0xFF) ? 1 : 0;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy5__SUB_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	unsigned int Flag = (Register_Vx[Vy] > Register_Vx[Vx]) ? 0 : 1;
	Register_Vx[Vx] -= Register_Vx[Vy];
	Register_Vx[0xF] = Flag;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xy6__SHR_Vx_Vy(uint16_t FetchedInstruction)
{
//"Unfortunately an erroneous assumption about 8XY6 and 8XYE, the two shift instructions, seems to have crept into Chip-8 lore at some point. Most recent documentation on Chip-8 suggests these instructions shift VX rather than VY." - "Chip-8 on the COSMAC VIP: Arithmetic and Logic Instructions" by Laurence Scotford "https://laurencescotford.com/chip-8-on-the-cosmac-vip-arithmetic-and-logic-instructions/"
#if INCORRECT_SHIFT_INSTRUCTIONS_VERSION == true
	//corrupted version
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Flag = (Register_Vx[Vx] & 0b00000001) ? 1 : 0;
	Register_Vx[Vx] >>= 1;
	Register_Vx[0xF] = Flag;
	AdvanceProgramCounter();
#else
	//correct version
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	unsigned int Flag = (Register_Vx[Vy] & 0b00000001) ? 1 : 0;
	Register_Vx[Vx] = Register_Vx[Vy] >> 1;
	Register_Vx[0xF] = Flag;
	AdvanceProgramCounter();
#endif
}

void CHIP_8::Instruction_8xy7__SUBN_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	unsigned int Flag = (Register_Vx[Vx] > Register_Vx[Vy]) ? 0 : 1;
	Register_Vx[Vx] = Register_Vx[Vy] - Register_Vx[Vx];
	Register_Vx[0xF] = Flag;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_8xyE__SHL_Vx_Vy(uint16_t FetchedInstruction)
{
//"Unfortunately an erroneous assumption about 8XY6 and 8XYE, the two shift instructions, seems to have crept into Chip-8 lore at some point. Most recent documentation on Chip-8 suggests these instructions shift VX rather than VY." - "Chip-8 on the COSMAC VIP: Arithmetic and Logic Instructions" by Laurence Scotford "https://laurencescotford.com/chip-8-on-the-cosmac-vip-arithmetic-and-logic-instructions/"
#if INCORRECT_SHIFT_INSTRUCTIONS_VERSION == true
	//corrupted version
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Flag = (Register_Vx[Vx] & 0b10000000) ? 1 : 0;
	Register_Vx[Vx] <<= 1;
	Register_Vx[0xF] = Flag;
	AdvanceProgramCounter();
#else
	//correct version
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	unsigned int Flag = (Register_Vx[Vy] & 0b10000000) ? 1 : 0;
	Register_Vx[Vx] = Register_Vx[Vy] << 1;
	Register_Vx[0xF] = Flag;
	AdvanceProgramCounter();
#endif
}

void CHIP_8::Instruction_9xy0__SNE_Vx_Vy(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	if (Register_Vx[Vx] != Register_Vx[Vy])
		AdvanceProgramCounter();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Annn__LD_I_addr(uint16_t FetchedInstruction)
{
	Register_I = (FetchedInstruction & 0x0FFF);
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Bnnn__JP_V0_addr(uint16_t FetchedInstruction)
{
	Register_PC = (FetchedInstruction & 0x0FFF) + Register_Vx[0x0];
}

void CHIP_8::Instruction_Cxnn__RND_Vx_byte(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Mask = FetchedInstruction & 0x00FF;
	unsigned int Result = rand();
	Result &= Mask;
	Register_Vx[Vx] = Result;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Dxyn__DRW_Vx_Vy_nibble(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Vy = FetchedInstruction & 0x00F0;
	Vy >>= 4;
	unsigned int OrginX = Register_Vx[Vx] % RESOLUTION_X;
	unsigned int OrginY = Register_Vx[Vy] % RESOLUTION_Y;
	unsigned int Size = FetchedInstruction & 0x000F;
	unsigned int Start = Register_I;
	unsigned int Erase = 0;
	for (unsigned int y = 0; y < Size; ++y)
	{
		if ((OrginY + y) >= RESOLUTION_Y)
			break;
		Register_I = Start + y;
		if (!IsMemoryAccessSafe())
			return;
		unsigned int Byte = Memory[Register_I];
		for (unsigned int x = 0; x < 8; ++x)
		{
			if ((OrginX + x) >= RESOLUTION_X)
				break;
			if (Byte & 0b10000000)
			{
				if (!(Display[OrginX + x][OrginY + y] = !Display[OrginX + x][OrginY + y]))
					Erase = 1;
			}
			Byte <<= 1;
		}
	}
	Register_Vx[0xf] = Erase;
	Register_I = Start;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Ex9E__SKP_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	if (Keypad[Register_Vx[Vx]])
		AdvanceProgramCounter();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_ExA1__SKNP_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	if (!Keypad[Register_Vx[Vx]])
		AdvanceProgramCounter();
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx07__LD_Vx_DT(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	Register_Vx[Vx] = Timer_DT;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx0A__LD_Vx_K(uint16_t FetchedInstruction)
{
	if (ButtonHeld)
	{
		if (!Keypad[HeldButton])
		{
			unsigned int Vx = FetchedInstruction & 0x0F00;
			Vx >>= 8;
			Register_Vx[Vx] = HeldButton;
			ButtonHeld = false;
			Timer_ST = 0;
			SoundEmitted = false;
			AdvanceProgramCounter();
		}
		Timer_ST = 255;
	}
	else
	{
		for (unsigned int i = 0; i < NUMBER_OF_BUTTONS; ++i)
		{
			if (Keypad[i])
			{
				HeldButton = i;
				ButtonHeld = true;
				Timer_ST = 255;
				SoundEmitted = true;
				break;
			}
		}
	}
}

void CHIP_8::Instruction_Fx15__LD_DT_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	Timer_DT = Register_Vx[Vx];
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx18__LD_ST_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	Timer_ST = Register_Vx[Vx];
	if (Timer_ST > 1)		//"It should be noted that in the COSMAC VIP manual, it was made clear that the minimum value that the timer will respond to is 02. Thus, setting the timer to a value of 01 would have no audible effect." - "Mastering CHIP‐8" by Matthew Mikolay "https://github.com/mattmikolay/chip-8/wiki/Mastering-CHIP%E2%80%908"
		SoundEmitted = true;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx1E__ADD_I_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	Register_I += Register_Vx[Vx];
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx29__LD_F_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	if (Register_Vx[Vx] < NUMBER_OF_FONT_SPRITES)
	{
		Register_I = FONT_AREA_START_ADDRESS + (SIZE_OF_FONT_SPRITES * Register_Vx[Vx]);
	}
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx33__LD_B_Vx(uint16_t FetchedInstruction)
{
	unsigned int Vx = FetchedInstruction & 0x0F00;
	Vx >>= 8;
	unsigned int Start = Register_I;
	unsigned int Value = Register_Vx[Vx];
	if (!IsMemoryAccessSafe())
		return;
	Memory[Register_I] = Value / 100;
	Value %= 100;
	++Register_I;
	if (!IsMemoryAccessSafe())
		return;
	Memory[Register_I] = Value / 10;
	Value %= 10;
	++Register_I;
	if (!IsMemoryAccessSafe())
		return;
	Memory[Register_I] = Value;
	Register_I = Start;
	AdvanceProgramCounter();
}

void CHIP_8::Instruction_Fx55__LD_I_Vx(uint16_t FetchedInstruction)
{
//"Erik Bryntse’s S-CHIP documentation incorrectly implies this instruction does not modify the I register. Certain S-CHIP-compatible emulators may implement this instruction in this manner." - "CHIP‐8 Instruction Set" by Matthew Mikolay "https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set"
#if INCORRECT_MEMORY_INSTRUCTIONS_VERSION == true
	//corrupted version
	unsigned int Range = FetchedInstruction & 0x0F00;
	Range >>= 8;
	unsigned int Start = Register_I;
	for (unsigned int Vx = 0; Vx <= Range; ++Vx)
	{
		if (!IsMemoryAccessSafe())
			return;
		Memory[Register_I] = Register_Vx[Vx];
		++Register_I;
	}
	Register_I = Start;
	AdvanceProgramCounter();
#else
	//correct version
	unsigned int Range = FetchedInstruction & 0x0F00;
	Range >>= 8;
	for (unsigned int Vx = 0; Vx <= Range; ++Vx)
	{
		if (!IsMemoryAccessSafe())
			return;
		Memory[Register_I] = Register_Vx[Vx];
		++Register_I;
	}
	AdvanceProgramCounter();
#endif
}

void CHIP_8::Instruction_Fx65__LD_Vx_I(uint16_t FetchedInstruction)
{
//"Erik Bryntse’s S-CHIP documentation incorrectly implies this instruction does not modify the I register. Certain S-CHIP-compatible emulators may implement this instruction in this manner." - "CHIP‐8 Instruction Set" by Matthew Mikolay "https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set"
#if INCORRECT_MEMORY_INSTRUCTIONS_VERSION == true
	//corrupted version
	unsigned int Range = FetchedInstruction & 0x0F00;
	Range >>= 8;
	unsigned int Start = Register_I;
	for (unsigned int Vx = 0; Vx <= Range; ++Vx)
	{
		if (!IsMemoryAccessSafe())
			return;
		Register_Vx[Vx] = Memory[Register_I];
		++Register_I;
	}
	Register_I = Start;
	AdvanceProgramCounter();
#else
	//correct version
	unsigned int Range = FetchedInstruction & 0x0F00;
	Range >>= 8;
	for (unsigned int Vx = 0; Vx <= Range; ++Vx)
	{
		if (!IsMemoryAccessSafe())
			return;
		Register_Vx[Vx] = Memory[Register_I];
		++Register_I;
	}
	AdvanceProgramCounter();
#endif
}