/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
static int ALUOut;
static SIM_coreState mainCoreState;

/*! SIM_CoreReset: Reset the processor core simulator machine to start new simulation
  Use this API to initialize the processor core simulator's data structures.
  The simulator machine must complete this call with these requirements met:
  - PC = 0  (entry point for a program is at address 0)
  - All the register file is cleared (all registers hold 0)
  - The value of IF is the instuction in address 0x0
  \returns 0 on success. <0 in case of initialization failure.
*/
int SIM_CoreReset(void) {
	if(memset(mainCoreState, 0, sizeof(mainCoreState)))	return 0;
	else return -1;
}

/*! SIM_CoreClkTick: Update the core simulator's state given one clock cycle.
  This function is expected to update the core pipeline given a clock cycle event.
*/
void SIM_CoreClkTick() {
	SIM_cmd if_pipe;
	SIM_cmd id_pipe;
	SIM_cmd ex_pipe;
	SIM_cmd mem_pipe;
	SIM_cmd wb_pipe;
	memset(if_pipe, mainCoreState.pipeStageState[0].cmd, sizeof(SIM_cmd));
	memset(id_pipe, mainCoreState.pipeStageState[1].cmd, sizeof(SIM_cmd));
	memset(ex_pipe, mainCoreState.pipeStageState[2].cmd, sizeof(SIM_cmd));
	memset(mem_pipe, mainCoreState.pipeStageState[3].cmd, sizeof(SIM_cmd));
	memset(wb_pipe, mainCoreState.pipeStageState[4].cmd, sizeof(SIM_cmd));
	if( !split_regfile && !forwarding ) {//when we start the machine nothing is initialized!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
										 // should also do something with src1Val and src2Val!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// IF -> ID
		SIM_MemInstRead(mainCoreState.pc, mainCoreState.pipeStageState[0].cmd);
		mainCoreState.pc += 4;
		// ID -> EX
		memset(mainCoreState.pipeStageState[1].cmd, if_pipe, sizeof(SIM_cmd));
		// EX -> MEM
		memset(mainCoreState.pipeStageState[2].cmd, id_pipe, sizeof(SIM_cmd));
		SIM_cmd EX_tmp = mainCoreState.pipeStageState[2].cmd;
		SIM_cmd_opcode opc = EX_tmp.opcode;
		int EX_ans = 0;
		if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI) {
			if(!EX_tmp.cmd.dst)
				EX_ans = makeArith(EX_tmp.cmd.src1, EX_tmp.cmd.src2, EX_tmp.cmd.isSrc2Imm, EX_tmp.cmd.dst, opc);
		}
		else if(opc == CMD_LOAD || opc == CMD_STORE) {
			if(!EX_tmp.cmd.dst)
				EX_ans = EX_tmp.cmd.src1 + EX_tmp.cmd.src2
		}
		else if(opc == CMD_BR || opc == CMD_BREQ || opc == CMD_BRNEQ) {
			EX_ans = (EX_tmp.cmd.src1 == EX_tmp.cmd.src2) ? 1 : 0;
		}
		// MEM -> WB
		memset(mainCoreState.pipeStageState[3].cmd, ex_pipe, sizeof(SIM_cmd));
		SIM_cmd MEM_tmp = mainCoreState.pipeStageState[3].cmd;
		SIM_cmd_opcode opc = MEM_tmp.opcode;
		if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI) {
			if(!MEM_tmp.cmd.dst)
				SIM_MemDataWrite(MEM_tmp.cmd.dst, EX_ans);
		}

		// WB
	}
}

int makeArith(int32_t reg1, int32_t reg2, bool isReg2Imm, int32_t dst, SIM_cmd_opcode opc) {
	int32_t num = (isReg2Imm) ? reg2 : mainCoreState.regFile[reg2];
	int32_t action = (opc == CMD_ADD || opc == CMD_ADDI) ? 1 : -1;
	return mainCoreState.regFile[reg1] + action * num;
}

void makeLoad(int32_t reg1, int32_t imm, int32_t dst) {
	if(dst == 0) return;

}

/*! SIM_CoreGetState: Return the current core (pipeline) internal state
    curState: The returned current pipeline state
    The function will return the state of the pipe at the end of a cycle
*/
void SIM_CoreGetState(SIM_coreState *curState) {
}

