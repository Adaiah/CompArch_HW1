/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
static SIM_coreState mainCoreState;
static int32_t stage_pc[SIM_PIPELINE_DEPTH] = {0}, dstVal[SIM_PIPELINE_DEPTH] = {0}; // pc of the current cmd in a stage + 4

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
	PipeStageState if_pipe;
	PipeStageState id_pipe;
	PipeStageState ex_pipe;
	PipeStageState mem_pipe;
	PipeStageState wb_pipe;
	memcpy(if_pipe, mainCoreState.pipeStageState[FETCH], sizeof(PipeStageState));
	memcpy(id_pipe, mainCoreState.pipeStageState[DECODE], sizeof(PipeStageState));
	memcpy(ex_pipe, mainCoreState.pipeStageState[EXECUTE], sizeof(PipeStageState));
	memcpy(mem_pipe, mainCoreState.pipeStageState[MEMORY], sizeof(PipeStageState));
	memcpy(wb_pipe, mainCoreState.pipeStageState[WRITEBACK], sizeof(PipeStageState));
	int32_t pc_tmp1, pc_tmp2;
	SIM_cmd_opcode opc;
	if( !split_regfile && !forwarding ) {//when we start the machine nothing is initialized!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
										 // should also do something with src1Val and src2Val!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		/*###################### IF ######################*/
		// On clock tick
		SIM_MemInstRead(mainCoreState.pc, mainCoreState.pipeStageState[FETCH]);
		mainCoreState.pc += 4;
		pc_tmp1 = stage_curr_pc[FETCH];
		stage_curr_pc[FETCH] = mainCoreState.pc; // pc + 4

		/*###################### ID ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[DECODE];
		//Action
		memcpy(mainCoreState.pipeStageState[DECODE], if_pipe, sizeof(PipeStageState));
		stage_curr_pc[DECODE] = pc_tmp1;
		pc_tmp1 = pc_tmp2;


		/*###################### EX ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[EXECUTE];
		MEM_IO[STAGE_IN] = EX_out;
		//Action
		memcpy(mainCoreState.pipeStageState[EXECUTE], id_pipe, sizeof(PipeStageState));
		stage_curr_pc[EXECUTE] = pc_tmp1;
		pc_tmp1 = pc_tmp2;
		opc = id_pipe.cmd.opcode;
		if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD) {
			if(id_pipe.cmd.isSrc2Imm) {
				EX_out = makeArith(id_pipe.src1Val, id_pipe.cmd.src2, opc);
			}
			else {
				EX_out = makeArith(id_pipe.src1Val, id_pipe.src2Val, opc);
			}
		}
		else if(opc == CMD_STORE) {
			if(id_pipe.cmd.isSrc2Imm) {
				EX_out = makeArith(dstVal[EXECUTE], id_pipe.cmd.src2, opc);
			}
			else {
				EX_out = makeArith(dstVal[EXECUTE], id_pipe.src2Val, opc);
			}
		}
		else if(opc == CMD_BR || opc == CMD_BREQ || opc == CMD_BRNEQ) {
			EX_out = (id_pipe.src1Val == id_pipe.src2Val) ? 1 : 0;
		}
		//// else nope - so will ignore


		/*###################### MEM ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[MEMORY];
		WB_IO[STAGE_IN] = MEM_IO[STAGE_OUT];
		//Action
		memcpy(mainCoreState.pipeStageState[MEMORY], ex_pipe, sizeof(PipeStageState));
		stage_curr_pc[MEMORY] = pc_tmp1;
		pc_tmp1 = pc_tmp2;
		opc = ex_pipe.cmd.opcode;
		if(opc == CMD_BR || (opc == CMD_BREQ && MEM_IO[STAGE_IN] == 1) || (opc == CMD_BRNEQ && MEM_IO[STAGE_IN] == 0)) {
			mainCoreState.pc = stage_curr_pc[MEMORY] + ex_pipe.cmd.dst; // the next command is the answer from the EX stage
			for (int i = FETCH; i <= EXECUTE; ++i)
			{
				memset(mainCoreState.pipeStageState[i], 0, sizeof(PipeStageState));
				stage_curr_pc[i] = 0;
			}
		}
		else if(opc == CMD_STORE) {
			SIM_MemDataWrite(MEM_IO[STAGE_IN], mainCoreState.regFile[ex_pipe.cmd.src1]);
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
		opc = wb_pipe.cmd.opcode;
		if(wb_pipe.dst != 0 && (opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD)) {
			mainCoreState.regFile[wb_pipe.dst] = WB_IO[STAGE_OUT];
		}
		//Action
		memcpy(mainCoreState.pipeStageState[WRITEBACK], mem_pipe, sizeof(PipeStageState));
		stage_curr_pc[WRITEBACK] = pc_tmp1;
		WB_IO[STAGE_OUT] = MEM_IO[STAGE_IN];

		//
	}
}

int32_t makeArith(int32_t num1, int32_t num2, SIM_cmd_opcode opc) {
	int action = (opc == CMD_ADD || opc == CMD_ADDI || opc == CMD_LOAD || opc = CMD_STORE) ? 1 : -1;
	return num1 + action * num2;
}

/*! SIM_CoreGetState: Return the current core (pipeline) internal state
    curState: The returned current pipeline state
    The function will return the state of the pipe at the end of a cycle
*/
void SIM_CoreGetState(SIM_coreState *curState) {
}

