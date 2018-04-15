/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
static SIM_coreState mainCoreState;
static int32_t EX_ans[2] = {0,0}, MEM_ans[2] = {0,0}, WB_ans[2] = {0,0};
static int stages_filled = 0;
static int32_t stage_pc[SIM_PIPELINE_DEPTH] = {0};

//0 - new command output
//1 - old command output

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
		// IF 
		SIM_MemInstRead(mainCoreState.pc, mainCoreState.pipeStageState[0].cmd);
		mainCoreState.pc += 4;
		stage_pc[stages_filled % SIM_PIPELINE_DEPTH] = mainCoreState.pc;
		// ID
		if(stages_filled >= 1) {
			memset(mainCoreState.pipeStageState[1].cmd, if_pipe, sizeof(SIM_cmd));
		}
		// EX
		if(stages_filled >= 2) {
			EX_ans[1] = EX_ans[0];
			memset(mainCoreState.pipeStageState[2].cmd, id_pipe, sizeof(SIM_cmd));
			SIM_cmd_opcode opc = id_pipe.opcode;
			if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI) {
				EX_ans[0] = makeArith(id_pipe.cmd.src1, id_pipe.cmd.src2, id_pipe.cmd.isSrc2Imm, opc);
			}
			else if(opc == CMD_LOAD) {
				EX_ans[0] = makeArith(id_pipe.cmd.src1, id_pipe.cmd.src2, id_pipe.cmd.isSrc2Imm, opc);
			}
			else if(opc == CMD_STORE) {
				EX_ans[0] = makeArith(id_pipe.cmd.dst, id_pipe.cmd.src2, id_pipe.cmd.isSrc2Imm, opc);
			}
			else if(opc == CMD_BR || opc == CMD_BREQ || opc == CMD_BRNEQ) {
				EX_ans[0] = (id_pipe.cmd.src1 == id_pipe.cmd.src2) ? 1 : 0;
			}
		}
		// MEM
		if(stages_filled >= 3) {
			MEM_ans[1] = MEM_ans[0];
			memset(mainCoreState.pipeStageState[3].cmd, ex_pipe, sizeof(SIM_cmd));\
			MEM_ans[0] = EX_ans[1];
			SIM_cmd_opcode opc = ex_pipe.opcode;
			if(opc == CMD_STORE) {
					SIM_MemDataWrite(MEM_ans[0], ex_pipe.cmd.src1);
			}
			else if(opc == CMD_LOAD) {
				if(!SIM_MemDataRead(MEM_ans[0], MEM_ans))
					//something!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					;
			}
			else if(opc == CMD_BR || (opc == CMD_BREQ && MEM_ans[0] == 1) || (opc == CMD_BRNEQ && MEM_ans[0] == 0)) {
				mainCoreState.pc = stage_pc[MEMORY]
			}
		}
		// WB
		if(stages_filled >= 4) {
			WB_ans[1] = WB_ans[0];
			SIM_cmd_opcode opc = wb_pipe.cmd.opcode;
			if(wb_pipe.cmd.dst != 0 && (opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD)) {
				curState.regFile[wb_pipe.cmd.dst] = WB_ans[1];
			}
			memset(mainCoreState.pipeStageState[4].cmd, mem_pipe, sizeof(SIM_cmd));
			WB_ans[0] = MEM_ans[1];
		}

		//
		++stages_filled;
	}
}

int makeArith(int32_t reg1, int32_t reg2, bool isReg2Imm, SIM_cmd_opcode opc) {
	int32_t num = (isReg2Imm) ? reg2 : mainCoreState.regFile[reg2];
	int32_t action = (opc == CMD_ADD || opc == CMD_ADDI || opc == CMD_LOAD || opc = CMD_STORE) ? 1 : -1;
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

