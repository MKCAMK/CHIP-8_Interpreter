#pragma once
#define INCORRECT_SHIFT_INSTRUCTIONS_VERSION false
#define INCORRECT_MEMORY_INSTRUCTIONS_VERSION false
#include <cstdint>

enum CHIP_8_ERROR_CODE { CHIP_8_ERROR_CODE__STATUS_OK, CHIP_8_ERROR_CODE__RESET, CHIP_8_ERROR_CODE__PROGRAM_TOO_BIG, CHIP_8_ERROR_CODE__OUT_OF_BOUNDS_MEMORY_ACCESS, CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED, CHIP_8_ERROR_CODE__INSTRUCTION_0NNN_NOT_IMPLEMENTED, CHIP_8_ERROR_CODE__STACK_OVERFLOW, CHIP_8_ERROR_CODE__STACK_UNDERFLOW };

class CHIP_8
{
	private:
		CHIP_8_ERROR_CODE CurrentStatus;

		static const unsigned int MEMORY_SIZE = 0x1000;
		uint8_t Memory[MEMORY_SIZE];

		static const unsigned int STACK_SIZE = 16;
		uint16_t Stack[STACK_SIZE];

		static const uint16_t FONT_AREA_START_ADDRESS = 0;
		static const unsigned int NUMBER_OF_FONT_SPRITES = 16;
		static const unsigned int SIZE_OF_FONT_SPRITES = 5;
		static const uint8_t Font[NUMBER_OF_FONT_SPRITES][SIZE_OF_FONT_SPRITES];

		static const uint16_t PROGRAM_AREA_START_ADDRESS = 0x200;

		static const unsigned int NUMBER_OF_GENERAL_REGISTERS = 16;
		uint8_t Register_Vx[NUMBER_OF_GENERAL_REGISTERS];

		uint16_t Register_I;

		uint16_t Register_PC;
		uint8_t Register_SP;

		uint8_t Timer_DT;
		uint8_t Timer_ST;
		bool SoundEmitted;

		static const unsigned int NUMBER_OF_BUTTONS = 16;
		bool Keypad[NUMBER_OF_BUTTONS];
		bool ButtonHeld;
		unsigned int HeldButton;

	public:
		static const unsigned int RESOLUTION_X = 0x40;
		static const unsigned int RESOLUTION_Y = 0x20;
	private:
		bool Display[RESOLUTION_X][RESOLUTION_Y];
		bool DrawingHappened;

		void Reset();
		void LoadFonts();
		void AdvanceProgramCounter();
		void PushStack();
		void PopStack();
		bool IsMemoryAccessSafe();
		void ClearDisplay();
		void FetchInstruction();
		void InstructionSwitch(uint16_t);
		void Instruction_0nnn__SYS_addr(uint16_t);
		void Instruction_00E0__CLS();
		void Instruction_00EE__RET();
		void Instruction_1nnn__JP_addr(uint16_t);
		void Instruction_2nnn__CALL_addr(uint16_t);
		void Instruction_3xnn__SE_Vx_byte(uint16_t);
		void Instruction_4xnn__SNE_Vx_byte(uint16_t);
		void Instruction_5xy0__SE_Vx_Vy(uint16_t);
		void Instruction_6xnn__LD_Vx_byte(uint16_t);
		void Instruction_7xnn__ADD_Vx_byte(uint16_t);
		void Instruction_8xy0__LD_Vx_Vy(uint16_t);
		void Instruction_8xy1__OR_Vx_Vy(uint16_t);
		void Instruction_8xy2__AND_Vx_Vy(uint16_t);
		void Instruction_8xy3__XOR_Vx_Vy(uint16_t);
		void Instruction_8xy4__ADD_Vx_Vy(uint16_t);
		void Instruction_8xy5__SUB_Vx_Vy(uint16_t);
		void Instruction_8xy6__SHR_Vx_Vy(uint16_t);
		void Instruction_8xy7__SUBN_Vx_Vy(uint16_t);
		void Instruction_8xyE__SHL_Vx_Vy(uint16_t);
		void Instruction_9xy0__SNE_Vx_Vy(uint16_t);
		void Instruction_Annn__LD_I_addr(uint16_t);
		void Instruction_Bnnn__JP_V0_addr(uint16_t);
		void Instruction_Cxnn__RND_Vx_byte(uint16_t);
		void Instruction_Dxyn__DRW_Vx_Vy_nibble(uint16_t);
		void Instruction_Ex9E__SKP_Vx(uint16_t);
		void Instruction_ExA1__SKNP_Vx(uint16_t);
		void Instruction_Fx07__LD_Vx_DT(uint16_t);
		void Instruction_Fx0A__LD_Vx_K(uint16_t);
		void Instruction_Fx15__LD_DT_Vx(uint16_t);
		void Instruction_Fx18__LD_ST_Vx(uint16_t);
		void Instruction_Fx1E__ADD_I_Vx(uint16_t);
		void Instruction_Fx29__LD_F_Vx(uint16_t);
		void Instruction_Fx33__LD_B_Vx(uint16_t);
		void Instruction_Fx55__LD_I_Vx(uint16_t);
		void Instruction_Fx65__LD_Vx_I(uint16_t);
	public:
		CHIP_8();
		bool GetSound();
		void PressButton(unsigned int);
		void UnpressButton(unsigned int);
		bool DidDrawingHappen();
		bool GetDisplay(unsigned int, unsigned int);
		CHIP_8_ERROR_CODE LoadProgram(char*, unsigned int);
		CHIP_8_ERROR_CODE Step(unsigned int);
};