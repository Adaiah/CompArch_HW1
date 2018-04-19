/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"
SIM_coreState mainCoreState;

int data_hazard_nop = 0, mem_didnt_read = 0, branch_taken = 0;
int32_t stage_curr_pc[SIM_PIPELINE_DEPTH] = {0}, dst_val[SIM_PIPELINE_DEPTH] = {0}; // pc of the current cmd in a stage + 4
int32_t stage_curr_pc_backup[SIM_PIPELINE_DEPTH] = {0}, dst_val_backup[SIM_PIPELINE_DEPTH] = {0}; // pc of the current cmd in a stage + 4

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
		stage_curr_pc[FETCH] = mainCoreState.pc;
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
		stage_curr_pc_backup[i] = stage_curr_pc[i];
		dst_val_backup[i] = dst_val[i];
	}
	// if(data_hazard_nop || mem_didnt_read) printf("data_hazard_nop %d ,mem_didnt_read %d\n", data_hazard_nop, mem_didnt_read);
	SIM_cmd_opcode opc;
	if( !split_regfile && !forwarding ) {
		/*###################### Registers ######################*/
		if(mem_didnt_read) {

			/*###################### MEM ######################*/
			//the opc has to be LOAD
			if(SIM_MemDataRead(MEM_IO[STAGE_IN], &MEM_IO[STAGE_OUT]) == 0) {

				mem_didnt_read = 0;
			}

			/*###################### WB ######################*/
			//WB - WB is nop untill load is over
			opc = pipeBackup[WRITEBACK].cmd.opcode;
			if(pipeBackup[WRITEBACK].cmd.dst != 0 && (opc >= CMD_ADD && opc <= CMD_LOAD)) {
				mainCoreState.regFile[pipeBackup[WRITEBACK].cmd.dst] = WB_IO[STAGE_OUT];
				if((data_hazard_nop == 1)) {
					--data_hazard_nop;
				}
			}
			memset((void*)(&mainCoreState.pipeStageState[WRITEBACK]), 0, sizeof(PipeStageState));
			stage_curr_pc[WRITEBACK] = 0;
			dst_val[WRITEBACK] = 0;

			/*###################### ID ######################*/
			//the problem is here. when i WRITEBACK i need to update the ID, because its sequential
			mainCoreState.pipeStageState[DECODE].src1Val = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src1];
			mainCoreState.pipeStageState[DECODE].src2Val = (mainCoreState.pipeStageState[DECODE].cmd.isSrc2Imm) ? 
														    mainCoreState.pipeStageState[DECODE].cmd.src2 : 
														    mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src2];
			dst_val[DECODE] = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.dst];
		}
		else {
			if(data_hazard_nop) {
				/*###################### ID ######################*/
				mainCoreState.pipeStageState[DECODE].src1Val = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src1];
				mainCoreState.pipeStageState[DECODE].src2Val = (mainCoreState.pipeStageState[DECODE].cmd.isSrc2Imm) ? 
															    mainCoreState.pipeStageState[DECODE].cmd.src2 : 
															    mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src2];
				dst_val[DECODE] = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.dst];


				/*###################### EX ######################*/
				MEM_IO[STAGE_IN] = EX_out;
				memset((void*)(&(mainCoreState.pipeStageState[EXECUTE])), 0, sizeof(PipeStageState));
				stage_curr_pc[EXECUTE] = 0;
				dst_val[EXECUTE] = 0;

				--data_hazard_nop;
			}
			else {
				/*###################### IF ######################*/
				if(branch_taken) {
					branch_taken = 0;
					mainCoreState.pc = stage_curr_pc[MEMORY] + dst_val[MEMORY]; // the next command is the answer from the EX stage
					for (int i = FETCH; i <= EXECUTE; ++i) {
						memset((void*)(&pipeBackup[i]), 0, sizeof(PipeStageState));
						stage_curr_pc[i] = 0;
						dst_val[i] = 0;
					}
				}
				mainCoreState.pc += 4;
				SIM_MemInstRead(mainCoreState.pc, &(mainCoreState.pipeStageState[FETCH].cmd));
				stage_curr_pc[FETCH] = mainCoreState.pc;

				/*###################### ID ######################*/
				memcpy((void*)(&mainCoreState.pipeStageState[DECODE]), (void*)(&pipeBackup[FETCH]), sizeof(PipeStageState));
				stage_curr_pc[DECODE] = stage_curr_pc_backup[FETCH];
				//
				opc = mainCoreState.pipeStageState[DECODE].cmd.opcode;
				int tmp_dst = mainCoreState.pipeStageState[DECODE].cmd.dst;
				bool tmp_isSrc2Imm = mainCoreState.pipeStageState[DECODE].cmd.isSrc2Imm;
				int tmp_src1 = mainCoreState.pipeStageState[DECODE].cmd.src1;
				int tmp_src2 = mainCoreState.pipeStageState[DECODE].cmd.src2;
				for (int i = DECODE; !data_hazard_nop && i <= MEMORY; ++i) {//problem because sequential
					if(tmp_dst != 0 && (pipeBackup[i].cmd.opcode >= CMD_ADD && pipeBackup[i].cmd.opcode <= CMD_LOAD) && (
					   ((opc >= CMD_ADD && opc <= CMD_LOAD) && (tmp_src1 == pipeBackup[i].cmd.dst || (!tmp_isSrc2Imm && tmp_src2 == pipeBackup[i].cmd.dst))) ||
					   (opc == CMD_STORE && (tmp_dst == pipeBackup[i].cmd.dst || (!tmp_isSrc2Imm && tmp_src2 == pipeBackup[i].cmd.dst))) ||
					   (opc == CMD_BR && tmp_dst == pipeBackup[i].cmd.dst) ||
					   ((opc == CMD_BREQ || opc == CMD_BRNEQ) && (tmp_src1 == pipeBackup[i].cmd.dst || tmp_src2 == pipeBackup[i].cmd.dst || tmp_dst == pipeBackup[i].cmd.dst)))) {

						data_hazard_nop = SIM_PIPELINE_DEPTH - i - 1;
					}
				}
				mainCoreState.pipeStageState[DECODE].src1Val = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src1];
				mainCoreState.pipeStageState[DECODE].src2Val = (mainCoreState.pipeStageState[DECODE].cmd.isSrc2Imm) ? 
															    mainCoreState.pipeStageState[DECODE].cmd.src2 : 
															    mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src2];
				dst_val[DECODE] = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.dst];

				/*###################### EX ######################*/
				MEM_IO[STAGE_IN] = EX_out;
				memcpy((void*)(&mainCoreState.pipeStageState[EXECUTE]), (void*)(&pipeBackup[DECODE]), sizeof(PipeStageState));
				stage_curr_pc[EXECUTE] = stage_curr_pc_backup[DECODE];
				dst_val[EXECUTE] = dst_val_backup[DECODE];
				//
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
						EX_out = makeArith(dst_val[EXECUTE], mainCoreState.pipeStageState[EXECUTE].cmd.src2, opc);
					}
					else {
						EX_out = makeArith(dst_val[EXECUTE], mainCoreState.pipeStageState[EXECUTE].src2Val, opc);
					}
				}
				else if(opc == CMD_BR || opc == CMD_BREQ || opc == CMD_BRNEQ) {
					EX_out = (mainCoreState.pipeStageState[EXECUTE].src1Val == mainCoreState.pipeStageState[EXECUTE].src2Val) ? 1 : 0;
				}
			}

			/*###################### MEM ######################*/
			WB_IO[STAGE_IN] = MEM_IO[STAGE_OUT];
			memcpy((void*)(&mainCoreState.pipeStageState[MEMORY]), (void*)(&pipeBackup[EXECUTE]), sizeof(PipeStageState));
			stage_curr_pc[MEMORY] = stage_curr_pc_backup[EXECUTE];
			dst_val[MEMORY] = dst_val_backup[EXECUTE];
			//
			opc = mainCoreState.pipeStageState[MEMORY].cmd.opcode;
			if(opc == CMD_BR || (opc == CMD_BREQ && MEM_IO[STAGE_IN] == 1) || (opc == CMD_BRNEQ && MEM_IO[STAGE_IN] == 0)) {
				branch_taken = 1;
				data_hazard_nop = 0;
			}
			else if(opc == CMD_STORE) {
				SIM_MemDataWrite(MEM_IO[STAGE_IN], mainCoreState.regFile[mainCoreState.pipeStageState[MEMORY].cmd.src1]);
			}
			else if(opc == CMD_LOAD) {
				if(SIM_MemDataRead(MEM_IO[STAGE_IN], &MEM_IO[STAGE_OUT])) {
					mem_didnt_read = 1;
				}
			}
			else if(opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI) {
				MEM_IO[STAGE_OUT] = MEM_IO[STAGE_IN];
			}

			/*###################### WB ######################*/
			opc = pipeBackup[WRITEBACK].cmd.opcode;
			if(pipeBackup[WRITEBACK].cmd.dst != 0 && (opc == CMD_ADD || opc == CMD_SUB || opc == CMD_ADDI || opc == CMD_SUBI || opc == CMD_LOAD)) {
				mainCoreState.regFile[pipeBackup[WRITEBACK].cmd.dst] = WB_IO[STAGE_OUT];
			}
			//
			mainCoreState.pipeStageState[DECODE].src1Val = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src1];
			mainCoreState.pipeStageState[DECODE].src2Val = (mainCoreState.pipeStageState[DECODE].cmd.isSrc2Imm) ? 
														    mainCoreState.pipeStageState[DECODE].cmd.src2 : 
														    mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.src2];
			dst_val[DECODE] = mainCoreState.regFile[mainCoreState.pipeStageState[DECODE].cmd.dst];
			//
			memcpy((void*)(&mainCoreState.pipeStageState[WRITEBACK]), (void*)(&pipeBackup[MEMORY]), sizeof(PipeStageState));
			stage_curr_pc[WRITEBACK] = stage_curr_pc_backup[MEMORY];
			dst_val[WRITEBACK] = dst_val_backup[MEMORY];
			//
			WB_IO[STAGE_OUT] = WB_IO[STAGE_IN];
		}
	}
}

/*! SIM_CoreGetState: Return the current core (pipeline) internal state
    curState: The returned current pipeline state
    The function will return the state of the pipe at the end of a cycle
*/
void SIM_CoreGetState(SIM_coreState *curState) {
	memcpy((void*)(curState), (void*)(&mainCoreState) , sizeof(SIM_coreState));
}

