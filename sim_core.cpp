/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
static SIM_coreState mainCoreState;
static int32_t stage_pc[SIM_PIPELINE_DEPTH] = {0}; // pc of the current cmd in a stage + 4

static int32_t EX_out = 0, MEM_IO[2] = {0,0}, WB_IO[2] = {0,0};
//[0] - stage input
//[1] - stage output

typedef enum {
    STAGE_IN = 0, STAGE_OUT,
} stageIO;

/*! SIM_CoreReset: Reset the processor core simulator machine to start new simulation
  Use this API to initialize the processor core simulator's data structures.
  The simulator machine must complete this call with these requirements met:
  - PC = 0  (entry point for a program is at address 0)
  - All the register file is cleared (all registers hold 0)
  - The value of IF is the instuction in address 0x0
  \returns 0 on success. <0 in case of initialization failure.
*/
int SIM_CoreReset(void) {
	if(memset(mainCoreState, 0, sizeof(mainCoreState))) {
		return 0;
	}
	else {
		return -1;
	}
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
	memcpy(if_pipe, mainCoreState.pipeStageState[FETCH].cmd, sizeof(SIM_cmd));
	memcpy(id_pipe, mainCoreState.pipeStageState[DECODE].cmd, sizeof(SIM_cmd));
	memcpy(ex_pipe, mainCoreState.pipeStageState[EXECUTE].cmd, sizeof(SIM_cmd));
	memcpy(mem_pipe, mainCoreState.pipeStageState[MEMORY].cmd, sizeof(SIM_cmd));
	memcpy(wb_pipe, mainCoreState.pipeStageState[WRITEBACK].cmd, sizeof(SIM_cmd));
	int32_t pc_tmp1, pc_tmp2;
	SIM_cmd_opcode opc;
	if( !split_regfile && !forwarding ) {//when we start the machine nothing is initialized!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
										 // should also do something with src1Val and src2Val!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		/*###################### IF ######################*/
		// On clock tick
		SIM_MemInstRead(mainCoreState.pc, mainCoreState.pipeStageState[FETCH].cmd);
		mainCoreState.pc += 4;
		pc_tmp1 = stage_curr_pc[FETCH];
		stage_curr_pc[FETCH] = mainCoreState.pc; // pc + 4

		/*###################### ID ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[DECODE];
		//Action
		memcpy(mainCoreState.pipeStageState[DECODE].cmd, if_pipe, sizeof(SIM_cmd));
		stage_curr_pc[DECODE] = pc_tmp1;
		pc_tmp1 = pc_tmp2;


		/*###################### EX ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[EXECUTE];
		MEM_IO[STAGE_IN] = EX_out;
		//Action
		memcpy(mainCoreState.pipeStageState[EXECUTE].cmd, id_pipe, sizeof(SIM_cmd));
		stage_curr_pc[EXECUTE] = pc_tmp1;
		pc_tmp1 = pc_tmp2;
		opc = id_pipe.opcode;
		if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI) {
			EX_out = makeArith(id_pipe.src1, id_pipe.src2, id_pipe.isSrc2Imm, opc);
		}
		else if(opc == CMD_LOAD) {
			EX_out = makeArith(id_pipe.src1, id_pipe.src2, id_pipe.isSrc2Imm, opc);
		}
		else if(opc == CMD_STORE) {
			EX_out = makeArith(id_pipe.dst, id_pipe.src2, id_pipe.isSrc2Imm, opc);
		}
		else if(opc == CMD_BR || opc == CMD_BREQ || opc == CMD_BRNEQ) {
			EX_out = (id_pipe.src1 == id_pipe.src2) ? 1 : 0;
		}
		//// else nope - so will ignore


		/*###################### MEM ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[MEMORY];
		WB_IO[STAGE_IN] = MEM_IO[STAGE_OUT];
		//Action
		memcpy(mainCoreState.pipeStageState[MEMORY].cmd, ex_pipe, sizeof(SIM_cmd));
		stage_curr_pc[MEMORY] = pc_tmp1;
		pc_tmp1 = pc_tmp2;
		opc = ex_pipe.opcode;
		if(opc == CMD_BR || (opc == CMD_BREQ && MEM_IO[STAGE_IN] == 1) || (opc == CMD_BRNEQ && MEM_IO[STAGE_IN] == 0)) {
			mainCoreState.pc = stage_curr_pc[MEMORY] + mem_pipe.dst; // the next command is the answer from the EX stage
			for (int i = FETCH; i <= EXECUTE; ++i)
			{
				memset(mainCoreState.pipeStageState[i], 0, sizeof(PipeStageState));
				stage_curr_pc[i] = 0;
			}
		}
		else if(opc == CMD_STORE) {
			SIM_MemDataWrite(MEM_IO[STAGE_IN], mem_pipe.src1);
		}
		else if(opc == CMD_LOAD) {
			if(!SIM_MemDataRead(MEM_IO[STAGE_IN], &MEM_IO[STAGE_OUT]))
				//something!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				;
		}
		MEM_IO[STAGE_OUT] = MEM_IO[STAGE_IN];
		//// else nope - so will ignore

		/*###################### WB ######################*/
		// On clock tick
		opc = wb_pipe.opcode;
		if(wb_pipe.dst != 0 && (opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD)) {
			curState.regFile[wb_pipe.dst] = WB_IO[STAGE_OUT];
		}
		//Action
		memcpy(mainCoreState.pipeStageState[WRITEBACK].cmd, mem_pipe, sizeof(SIM_cmd));
		stage_curr_pc[WRITEBACK] = pc_tmp1;
		WB_IO[STAGE_OUT] = MEM_IO[STAGE_IN];

		//
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

