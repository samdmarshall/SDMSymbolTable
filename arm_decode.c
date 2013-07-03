/*
 *  arm_decode.c
 *  SDMSymbolTable
 *
 *  Copyright (c) 2013, Sam Marshall
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software must display the following acknowledgement:
 *  	This product includes software developed by the Sam Marshall.
 *  4. Neither the name of the Sam Marshall nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY Sam Marshall ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Sam Marshall BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef _SDMARMDECODE_C_
#define _SDMARMDECODE_C_

#include "arm_decode.h"
#include <math.h>

#pragma mark -
#pragma mark ARM
#pragma mark -
#pragma mark Condition Code
#define ARM_CONDITION_CODE(data) ((data & 0xF0000000) >> 28)

#pragma mark OP Instrunction
#define ARM_OP(data) ((data & 0x0F000000) >> 24)
#define ARM_OP_ALT(data) (((data & 0x000000F0) >> 4) & 1)
#define ARM_OP_0(data) (data >> 3)
#define ARM_OP_1(data) ((data >> 2) & 1)
#define ARM_OP_2(data) ((data >> 1) & 1)
#define ARM_OP_3(data) (data & 1)

#pragma mark Data-Processing
#define ARM_DP_OP0(data) (((data & 0x0F000000) >> 25) & 1)
#define ARM_DP_OP1(data) (((data & 0x0FF00000) >> 20) & 0x1F)
#define ARM_DP_OP2(data) ((data & 0x000000F0) >> 4)

#pragma mark Load/Store
#define ARM_LS_A(data) (((data & 0x0F000000) >> 25) & 1)
#define ARM_LS_OP1(data) (((data & 0x0FF00000) >> 20) & 0x1F)
#define ARM_LS_B(data) (((data & 0x000000F0) >> 4) & 1)
#define ARM_LS_Rn(data) ((data & 0x000F0000) >> 16)

#pragma mark Media
#define ARM_MEDIA_OP1(data) (((data & 0x0FF00000) >> 20) & 0x1F)
#define ARM_MEDIA_OP2(data) (((data & 0x000000F0) >> 4) & 0x7)
#define ARM_MEDIA_Rd(data) ((data & 0x0000F000) >> 12)
#define ARM_MEDIA_Rn(data) ((data & 0x0000000F) >> 20)

#pragma mark Branch
#define ARM_BRANCH_OP1(data) (((data & 0x0FF00000) >> 20) & 0x3F)
#define ARM_BRANCH_Rn(data) ((data & 0x000F0000) >> 16)
#define ARM_BRANCH_R(data) (((data & 0x0000F000) >> 15) & 1)

#pragma mark Coprocessor
#define ARM_CO_OP1(data) (((data & 0x0FF00000) >> 20) & 0x3F)
#define ARM_CO_Rn(data) ((data & 0x000F0000) >> 16)
#define ARM_CO_CoProc(data) ((data & 0x00000F00) >> 8)
#define ARM_CO_OP2(data) (((data & 0x0000000F) >> 3) & 1)

#pragma mark Unconditional
#define UC_OP1(data) ((data & 0x0FF00000) >> 20)
#define UC_Rn(data) ((data & 0x000F0000) >> 16)
#define CO_OP2(data) (((data & 0x0000000F) >> 3) & 1)

#pragma mark Instruction Classes
#define ARM_IsConditional(data) (ARM_CONDITION_CODE(data) != 0xF)
#define ARM_IsDataProcessing(data) (ARM_OP_0(ARM_OP(data)) == 0 && ARM_OP_1(ARM_OP(data)) == 0)
#define ARM_IsLoadStore(data) (ARM_OP_0(ARM_OP(data)) == 0 && ARM_OP_1(ARM_OP(data)) == 1 && (ARM_OP_2(ARM_OP(data)) == 0 || (ARM_OP_2(ARM_OP(data)) == 1 && ARM_OP_ALT(data) == 0)))
#define ARM_IsMedia(data) (ARM_OP_0(ARM_OP(data)) == 0 && ARM_OP_1(ARM_OP(data)) == 1 && ARM_OP_2(ARM_OP(data)) == 1 && ARM_OP_ALT(data) == 1)
#define ARM_IsBranch(data) (ARM_OP_0(ARM_OP(data)) == 1 && ARM_OP_1(ARM_OP(data)) == 0)
#define ARM_IsCoProc(data) (ARM_OP_0(ARM_OP(data)) == 1 && ARM_OP_1(ARM_OP(data)) == 1)

#define ARM_IsSIMDDataProcessing(data) (data)

#define ARM_IsFloatingDataProcessing(data) (data)


#pragma mark -
#pragma mark THUMB
#pragma mark -

#pragma mark Op Code
#define THUMB_OP(data) ((data & 0xFF00) >> 10)
#define THUMB_OP0(data) (THUMB_OP(data) >> 5)
#define THUMB_OP1(data) ((THUMB_OP(data) >> 4) & 1)
#define THUMB_OP2(data) ((THUMB_OP(data) >> 3) & 1)
#define THUMB_OP3(data) ((THUMB_OP(data) >> 2) & 1)
#define THUMB_OP4(data) ((THUMB_OP(data) >> 1) & 1)
#define THUMB_OP5(data) ((THUMB_OP(data)) & 1)

#define THUMB_16_BL_OP(data) ((data) & 0xFF00) >> 9)
#define THUMB_16_BL_OP0(data) (THUMB_16_BL_OP(data) >> 4)
#define THUMB_16_BL_OP1(data) ((THUMB_16_BL_OP(data) >> 3) & 1)
#define THUMB_16_BL_OP2(data) ((THUMB_16_BL_OP(data) >> 2) & 1)
#define THUMB_16_BL_OP3(data) ((THUMB_16_BL_OP(data) >> 1) & 1)
#define THUMB_16_BL_OP4(data) ((THUMB_16_BL_OP(data)) & 1)

#define THUMB_16_BL_MOVCHECK(data) (((data & 0x00F0) >> 6) & 0x7)

#pragma mark Type
#define THUMB_Is32Bit(data) (THUMB_OP(data) == 0x1F || THUMB_OP(data) == 0x1E || THUMB_OP(data) == 0x1D)
#define THUMB_Is16Bit(data) !THUMB_Is32Bit(data)

#pragma mark 16Bit Classes
#define THUMB_16_BasicLogic(data) (THUMB_OP0(data) == 0 && THUMB_OP1(data) == 0)
#define THUMB_16_DataProcessing(data) (THUMB_OP0(data) == 0 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 0 && THUMB_OP5(data) == 0)
#define THUMB_16_SpecialData(data) (THUMB_OP0(data) == 0 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 0 && THUMB_OP5(data) == 1)
#define THUMB_16_LoadLiteral(data) (THUMB_OP0(data) == 0 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 1)
#define THUMB_16_LoadStore(data) ((THUMB_OP0(data) == 0 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 1) || (THUMB_OP0(data) == 0 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 1) || (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 0 && THUMB_OP2(data) == 0))
#define THUMB_16_PCAddress(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 0 && THUMB_OP2(data) == 1 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 0)
#define THUMB_16_SPAddress(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 0 && THUMB_OP2(data) == 1 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 1)
#define THUMB_16_MiscInstructions(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 0 && THUMB_OP2(data) == 1 && THUMB_OP3(data) == 1)
#define THUMB_16_StoreMulti(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 0)
#define THUMB_16_LoadMulti(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 1)
#define THUMB_16_ConditionalBranch(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 0 && THUMB_OP3(data) == 1)
#define THUMB_16_UnconditionalBranch(data) (THUMB_OP0(data) == 1 && THUMB_OP1(data) == 1 && THUMB_OP2(data) == 1 && THUMB_OP3(data) == 0 && THUMB_OP4(data) == 0)

#define THUMB_16_TableResolve(data) (THUMB_16_UnconditionalBranch(data) << 11) + (THUMB_16_ConditionalBranch(data) << 10) + (THUMB_16_LoadMulti(data) << 9) + (THUMB_16_StoreMulti(data) << 8) + (THUMB_16_MiscInstructions(data) << 7) + (THUMB_16_SPAddress(data) << 6) + (THUMB_16_PCAddress(data) << 5) + (THUMB_16_LoadStore(data) << 4) + (THUMB_16_LoadLiteral(data) << 3) + (THUMB_16_SpecialData(data) << 2) + (THUMB_16_DataProcessing(data) << 1) + THUMB_16_BasicLogic(data)
#pragma mark -
#define THUMB_16_LShiftLeft(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 0 && THUMB_16_BL_OP2(data) == 0))
#define THUMB_16_LShiftRight(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 0 && THUMB_16_BL_OP2(data) == 1))
#define THUMB_16_AShiftRight(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 0))
#define THUMB_16_Add(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 1 && THUMB_16_BL_OP3(data) == 0 && THUMB_16_BL_OP4(data) == 0))
#define THUMB_16_Sub(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 1 && THUMB_16_BL_OP3(data) == 0 && THUMB_16_BL_OP4(data) == 1))
#define THUMB_16_Add3Bit(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 1 && THUMB_16_BL_OP3(data) == 1 && THUMB_16_BL_OP4(data) == 0))
#define THUMB_16_Sub3Bit(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 1 && THUMB_16_BL_OP3(data) == 1 && THUMB_16_BL_OP4(data) == 1))
#define THUMB_16_Mov(data) (THUMB_16_BasicLogic(data) && ((THUMB_16_BL_OP0(data) == 1 && THUMB_16_BL_OP1(data) == 0 && THUMB_16_BL_OP2(data) == 0) || ((THUMB_16_BL_OP0(data) == 0 && THUMB_16_BL_OP1(data) == 0 && THUMB_16_BL_OP2(data) == 0 && THUMB_16_BL_OP3(data) == 0 && THUMB_16_BL_OP4(data) == 0) && THUMB_16_BL_MOVCHECK(data) == 0)))
#define THUMB_16_Cmp(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 1 && THUMB_16_BL_OP1(data) == 0 && THUMB_16_BL_OP2(data) == 1))
#define THUMB_16_Add8Bit(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 1 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 0))
#define THUMB_16_Sub8Bit(data) (THUMB_16_BasicLogic(data) && (THUMB_16_BL_OP0(data) == 1 && THUMB_16_BL_OP1(data) == 1 && THUMB_16_BL_OP2(data) == 1))

#define THUMB_16_BL_Resolve(data) (data)
#pragma mark -
#define THUMB_16_DP_OPCode(data) (((data & 0x0FF0) >> 5) & 0xF)
#define THUMB_16_DP_OP0(data) ((THUMB_16_DP_OPCode(data) >> 3) & 1)
#define THUMB_16_DP_OP1(data) ((THUMB_16_DP_OPCode(data) >> 2) & 1)
#define THUMB_16_DP_OP2(data) ((THUMB_16_DP_OPCode(data) >> 1) & 1)
#define THUMB_16_DP_OP3(data) ((THUMB_16_DP_OPCode(data)) & 1)


#define THUMB_16_OpCodeResolve(table,data) (table)

#pragma mark -
#pragma mark Translated Instrunctions
#pragma mark -

typedef enum THUMB_16_Table {
	BasicLogic = 1,
	DataProcessing = 2,
	SpecialData = 4,
	LoadLiteral = 8,
	LoadStore = 16,
	PCAddress = 32,
	SPAddress = 64,
	Misc = 128,
	StoreMulti = 256,
	LoadMulti = 512,
	ConditionalBranch = 1024,
	UnconditionalBranch = 2048
} THUMB_16_Table;

typedef struct THUMB_16_OpCode {
	uint32_t opcode;
	char *instruction;
} __attribute__ ((packed)) THUMB_16_OpCode;

static THUMB_16_OpCode THUMB_16_BasicLogicTable[11] = {
	{0,"lsl"},
	{4,"lsr"},
	{8,"asr"},
	{12,"add"},
	{13,"sub"},
	{14,"add"},
	{15,"sub"},
	{16,"mov"},
	{20,"cmp"},
	{24,"add"},
	{28,"sub"}
};

static THUMB_16_OpCode THUMB_16_DataProcessingTable[16] = {
	{0,"and"},
	{1,"eor"},
	{2,"lsl"},
	{3,"lsr"},
	{4,"asr"},
	{5,"adc"},
	{6,"sbc"},
	{7,"ror"},
	{8,"tst"},
	{9,"rsb"},
	{10,"cmp"},
	{11,"cmn"},
	{12,"orr"},
	{13,"mul"},
	{14,"bic"},
	{15,"mvn"}
};

static THUMB_16_OpCode *THUMB_16_MasterTable[] = {
	THUMB_16_BasicLogicTable,
	THUMB_16_DataProcessingTable
};

THUMB_16_OpCode arm_decode_opcode(uint32_t data) {
	if (THUMB_Is16Bit(data)) {
		THUMB_16_Table tableNum = (uint32_t)log2l(THUMB_16_TableResolve(data));
		return THUMB_16_MasterTable[tableNum][THUMB_16_OpCodeResolve(tableNum,data)];
	} else if (THUMB_Is32Bit(data)) {
		return;
	}
}

#endif