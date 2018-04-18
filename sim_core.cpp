/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
SIM_coreState mainCoreState;

int data_hazard_nop = 0, mem_didnt_read = 0;
int32_t stage_curr_pc[SIM_PIPELINE_DEPTH] = {0}, dstVal[SIM_PIPELINE_DEPTH] = {0}; // pc of the current cmd in a stage + 4

int32_t EX_out = 0, MEM_IO[2] = {0,0}, WB_IO[2] = {0,0};
//[0] - stage input
//[1] - stage output

typedef enum {
    STAGE_IN = 0, STAGE_OUT,
} stageIO;

int32_t makeArith(int32_t num1, int32_t num2, SIM_cmd_opcode opc) {
	int action = (opc == CMD_ADD || opc == CMD_ADDI || opc == CMD_LOAD || opc == CMD_STORE) ? 1 : -1;
	return num1 + action * num2;
}

/*! SIM_CoreReset: Reset the processor core simulator machine to start new simulation
  Use this API to initialize the processor core simulator's data structures.
  The simulator machine must complete this call with these requirements met:
  - PC = 0  (entry point for a program is at address 0)
  - All the register file is cleared (all registers hold 0)
  - The value of IF is the instuction in address 0x0
  \returns 0 on success. <0 in case of initialization failure.
*/
int SIM_CoreReset(void) {
	if(memset((void*)(&mainCoreState), 0 , sizeof(SIM_coreState))) {
		SIM_MemInstRead(mainCoreState.pc, &(mainCoreState.pipeStageState[FETCH].cmd));
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
	PipeStageState pipeBackup[SIM_PIPELINE_DEPTH];
	for (int i = FETCH; i <= WRITEBACK; ++i) {
		memcpy((void*)(&pipeBackup[i]), (void*)(&mainCoreState.pipeStageState[i]), sizeof(PipeStageState));
	}
	int32_t pc_tmp1, pc_tmp2, dst_tmp1, dst_tmp2;
	SIM_cmd_opcode opc;
	if( !split_regfile && !forwarding ) {
		/*###################### Registers ######################*/
		if(mem_didnt_read) {
			//WB
			memset((void*)(&mainCoreState.pipeStageState[WRITEBACK]), 0, sizeof(PipeStageState));
			--mem_didnt_read;
		}
		else {
			if(data_hazard_nop) {
				memset((void*)(&mainCoreState.pipeStageState[EXECUTE]), 0, sizeof(PipeStageState));
				printf("Im here %d\n", data_hazard_nop);
				--data_hazard_nop;
			}
			else {
				//IF
				mainCoreState.pc += 4;
				SIM_MemInstRead(mainCoreState.pc, &(mainCoreState.pipeStageState[FETCH].cmd));

				//ID
				memcpy((void*)(&mainCoreState.pipeStageState[DECODE]), (void*)(&pipeBackup[FETCH]), sizeof(PipeStageState));

				//EX
				MEM_IO[STAGE_IN] = EX_out;
				memcpy((void*)(&mainCoreState.pipeStageState[EXECUTE]), (void*)(&pipeBackup[DECODE]), sizeof(PipeStageState));
			}

			//MEM
			WB_IO[STAGE_IN] = MEM_IO[STAGE_OUT];
			memcpy((void*)(&mainCoreState.pipeStageState[MEMORY]), (void*)(&pipeBackup[EXECUTE]), sizeof(PipeStageState));

			//WB
			opc = pipeBackup[WRITEBACK].cmd.opcode;
			if(pipeBackup[WRITEBACK].cmd.dst != 0 && (opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD)) {
				mainCoreState.regFile[pipeBackup[WRITEBACK].cmd.dst] = WB_IO[STAGE_OUT];
			}
			memcpy((void*)(&mainCoreState.pipeStageState[WRITEBACK]), (void*)(&pipeBackup[MEMORY]), sizeof(PipeStageState));
		}





		/*###################### Action ######################*/

		if(mem_didnt_read == 0 && data_hazard_nop == 0) {
			/*###################### IF ######################*/
			// Action
			pc_tmp1 = stage_curr_pc[FETCH];
			stage_curr_pc[FETCH] = mainCoreState.pc; // old_pc + 4

			/*###################### ID ######################*/
			// On clock tick
			pc_tmp2 = stage_curr_pc[DECODE];
			//Action
			stage_curr_pc[DECODE] = pc_tmp1;
			pc_tmp1 = pc_tmp2;
			dst_tmp1 = dstVal[DECODE];
			for (int i = EXECUTE; i <= WRITEBACK; ++i) {
				if(mainCoreState.pipeStageState[DECODE].cmd.dst != 0 && (mainCoreState.pipeStageState[DECODE].cmd.dst == pipeBackup[i].cmd.src1 ||
				   (!pipeBackup[i].cmd.isSrc2Imm && mainCoreState.pipeStageState[DECODE].cmd.dst == pipeBackup[i].cmd.src2))) {
					data_hazard_nop = SIM_PIPELINE_DEPTH-i;
				}
			}
			mainCoreState.pipeStageState[DECODE].src1Val = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src1];
			mainCoreState.pipeStageState[DECODE].src2Val = (mainCoreState.pipeStageState[DECODE].cmd.isSrc2Imm) ? 
														    mainCoreState.pipeStageState[DECODE].cmd.src2 : 
														    mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src2];
			dstVal[DECODE] = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.dst];
		}

		if(mem_didnt_read == 0) {
			/*###################### EX ######################*/
			// On clock tick
			pc_tmp2 = stage_curr_pc[EXECUTE];
			dst_tmp2 = dstVal[EXECUTE];
			//Action
			stage_curr_pc[EXECUTE] = pc_tmp1;
			pc_tmp1 = pc_tmp2;
			dstVal[EXECUTE] = dst_tmp1;
			dst_tmp1 = dst_tmp2;
			opc = mainCoreState.pipeStageState[EXECUTE].cmd.opcode;
			if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD) {
				if(mainCoreState.pipeStageState[EXECUTE].cmd.isSrc2Imm) {
					EX_out = makeArith(mainCoreState.pipeStageState[EXECUTE].src1Val, mainCoreState.pipeStageState[EXECUTE].cmd.src2, opc);
				}
				else {
					EX_out = makeArith(mainCoreState.pipeStageState[EXECUTE].src1Val, mainCoreState.pipeStageState[EXECUTE].src2Val, opc);
				}
			}
			else if(opc == CMD_STORE) {
				if(mainCoreState.pipeStageState[EXECUTE].cmd.isSrc2Imm) {
					EX_out = makeArith(dstVal[EXECUTE], mainCoreState.pipeStageState[EXECUTE].cmd.src2, opc);
				}
				else {
					EX_out = makeArith(dstVal[EXECUTE], mainCoreState.pipeStageState[EXECUTE].src2Val, opc);
				}
			}
			else if(opc == CMD_BR || opc == CMD_BREQ || opc == CMD_BRNEQ) {
				EX_out = (mainCoreState.pipeStageState[EXECUTE].src1Val == mainCoreState.pipeStageState[EXECUTE].src2Val) ? 1 : 0;
			}
			//// else 'nope' - so will ignore
		}

		/*###################### MEM ######################*/
		// On clock tick
		pc_tmp2 = stage_curr_pc[MEMORY];
		dst_tmp2 = dstVal[MEMORY];
		//Action
		stage_curr_pc[MEMORY] = pc_tmp1;
		pc_tmp1 = pc_tmp2;
		dstVal[MEMORY] = dst_tmp1;
		dst_tmp1 = dst_tmp2;
		opc = mainCoreState.pipeStageState[MEMORY].cmd.opcode;
		if(opc == CMD_BR || (opc == CMD_BREQ && MEM_IO[STAGE_IN] == 1) || (opc == CMD_BRNEQ && MEM_IO[STAGE_IN] == 0)) {
			mainCoreState.pc = stage_curr_pc[MEMORY] + dstVal[MEMORY]; // the next command is the answer from the EX stage
			for (int i = FETCH; i <= EXECUTE; ++i) {
				memset((void*)(&mainCoreState.pipeStageState[i]), 0, sizeof(PipeStageState));
				stage_curr_pc[i] = 0;
			}
		}
		else if(opc == CMD_STORE) {
			SIM_MemDataWrite(MEM_IO[STAGE_IN], mainCoreState.regFile[mainCoreState.pipeStageState[MEMORY].cmd.src1]);
		}
		else if(opc == CMD_LOAD) {
			if(!SIM_MemDataRead(MEM_IO[STAGE_IN], &MEM_IO[STAGE_OUT]))
				++mem_didnt_read;
		}
		else if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI) {
			MEM_IO[STAGE_OUT] = MEM_IO[STAGE_IN];
		}
		//// else 'nope' - so will ignore

		/*###################### WB ######################*/
		//Action
		stage_curr_pc[WRITEBACK] = pc_tmp1;
		WB_IO[STAGE_OUT] = WB_IO[STAGE_IN];
	}
}

/*! SIM_CoreGetState: Return the current core (pipeline) internal state
    curState: The returned current pipeline state
    The function will return the state of the pipe at the end of a cycle
*/
void SIM_CoreGetState(SIM_coreState *curState) {
	memcpy((void*)(curState), (void*)(&mainCoreState) , sizeof(SIM_coreState));
}

