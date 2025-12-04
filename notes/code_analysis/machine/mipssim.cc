// mipssim.cc -- 模拟 MIPS R2/3000 处理器
//
//   此代码已从 Ousterhout 的 MIPSSIM 包中改编。
//   字节序是小端序，因此我们可以与
//   DEC RISC 系统兼容。
//
//   不要更改 -- 机器模拟的一部分
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#include "copyright.h"

#include "machine.h"
#include "mipssim.h"
#include "system.h"

static void Mult(int a, int b, bool signedArith, int* hiPtr, int* loPtr);

//----------------------------------------------------------------------
// Machine::Run
// 	模拟在 Nachos 上执行的用户级程序。
//	由内核在程序启动时调用；永不返回。
//
//	此例程是可重入的，因为它可以被多次
//	并发调用 -- 每个执行用户代码的线程一次。
//----------------------------------------------------------------------

void
Machine::Run()
{
    Instruction *instr = new Instruction;  // 存储解码指令的空间

    if(DebugIsEnabled('m'))
        printf("Starting thread \"%s\" at time %d\n",
	       currentThread->getName(), stats->totalTicks);
    interrupt->setStatus(UserMode);
    for (;;) {
        OneInstruction(instr);
	interrupt->OneTick();
	if (singleStep && (runUntilTime <= stats->totalTicks))
	  Debugger();
    }
}


//----------------------------------------------------------------------
// TypeToReg
// 	检索指令中引用的寄存器号。
//----------------------------------------------------------------------

static int 
TypeToReg(RegType reg, Instruction *instr)
{
    switch (reg) {
      case RS:
	return instr->rs;
      case RT:
	return instr->rt;
      case RD:
	return instr->rd;
      case EXTRA:
	return instr->extra;
      default:
	return -1;
    }
}

//----------------------------------------------------------------------
// Machine::OneInstruction
// 	执行一条来自用户级程序的指令
//
// 	如果有任何类型的异常或中断，我们调用
//	异常处理程序，当它返回时，我们返回到 Run()，它
//	将在循环中重新调用我们。这允许我们
//	从头重新启动指令执行，在
//	任何状态已更改的情况下。在系统调用时，
// 	操作系统软件必须增加 PC 以便执行从
// 	系统调用之后的指令开始。
//
//	此例程是可重入的，因为它可以被多次
//	并发调用 -- 每个执行用户代码的线程一次。
//	我们通过从不缓存任何数据来获得可重入性 -- 我们总是重新启动
//	每次调用时的模拟，从头开始（或在陷阱
//	返回到 Nachos 内核的异常或中断之后），我们总是
//	将所有数据存储回机器寄存器和内存之前
//	离开。这允许 Nachos 内核控制我们的行为
//	通过控制内存内容、转换表和
//	寄存器集。
//----------------------------------------------------------------------

void
Machine::OneInstruction(Instruction *instr)
{
    int raw;
    int nextLoadReg = 0; 	
    int nextLoadValue = 0; 	// 记录延迟加载操作，以应用
				// 在未来

    // 获取指令 
    if (!machine->ReadMem(registers[PCReg], 4, &raw))
	return;			// 异常发生
    instr->value = raw;
    instr->Decode();

    if (DebugIsEnabled('m')) {
       struct OpString *str = &opStrings[instr->opCode];

       ASSERT(instr->opCode <= MaxOpcode);
       printf("At PC = 0x%x: ", registers[PCReg]);
       printf(str->string, TypeToReg(str->args[0], instr), 
		TypeToReg(str->args[1], instr), TypeToReg(str->args[2], instr));
       printf("\n");
       }
    
    // 计算下一个 pc，但不安装，以防有错误或分支。
    int pcAfter = registers[NextPCReg] + 4;
    int sum, diff, tmp, value;
    unsigned int rs, rt, imm;

    // 执行指令 (参见 Kane 的书)
    switch (instr->opCode) {
	
      case OP_ADD:
	sum = registers[instr->rs] + registers[instr->rt];
	if (!((registers[instr->rs] ^ registers[instr->rt]) & SIGN_BIT) &&
	    ((registers[instr->rs] ^ sum) & SIGN_BIT)) {
	    RaiseException(OverflowException, 0);
	    return;
	}
	registers[instr->rd] = sum;
	break;
	
      case OP_ADDI:
	sum = registers[instr->rs] + instr->extra;
	if (!((registers[instr->rs] ^ instr->extra) & SIGN_BIT) &&
	    ((instr->extra ^ sum) & SIGN_BIT)) {
	    RaiseException(OverflowException, 0);
	    return;
	}
	registers[instr->rt] = sum;
	break;
	
      case OP_ADDIU:
	registers[instr->rt] = registers[instr->rs] + instr->extra;
	break;
	
      case OP_ADDU:
	registers[instr->rd] = registers[instr->rs] + registers[instr->rt];
	break;
	
      case OP_AND:
	registers[instr->rd] = registers[instr->rs] & registers[instr->rt];
	break;
	
      case OP_ANDI:
	registers[instr->rt] = registers[instr->rs] & (instr->extra & 0xffff);
	break;
	
      case OP_BEQ:
	if (registers[instr->rs] == registers[instr->rt])
	    pcAfter = registers[NextPCReg] + IndexToAddr(instr->extra);
	break;
	
      case OP_BGEZAL:
	registers[R31] = registers[NextPCReg] + 4;
      case OP_BGEZ:
	if (!(registers[instr->rs] & SIGN_BIT))
	    pcAfter = registers[NextPCReg] + IndexToAddr(instr->extra);
	break;
	
      case OP_BGTZ:
	if (registers[instr->rs] > 0)
	    pcAfter = registers[NextPCReg] + IndexToAddr(instr->extra);
	break;
	
      case OP_BLEZ:
	if (registers[instr->rs] <= 0)
	    pcAfter = registers[NextPCReg] + IndexToAddr(instr->extra);
	break;
	
      case OP_BLTZAL:
	registers[R31] = registers[NextPCReg] + 4;
      case OP_BLTZ:
	if (registers[instr->rs] & SIGN_BIT)
	    pcAfter = registers[NextPCReg] + IndexToAddr(instr->extra);
	break;
	
      case OP_BNE:
	if (registers[instr->rs] != registers[instr->rt])
	    pcAfter = registers[NextPCReg] + IndexToAddr(instr->extra);
	break;
	
      case OP_DIV:
	if (registers[instr->rt] == 0) {
	    registers[LoReg] = 0;
	    registers[HiReg] = 0;
	} else {
	    registers[LoReg] =  registers[instr->rs] / registers[instr->rt];
	    registers[HiReg] = registers[instr->rs] % registers[instr->rt];
	}
	break;
	
      case OP_DIVU:	  
	  rs = (unsigned int) registers[instr->rs];
	  rt = (unsigned int) registers[instr->rt];
	  if (rt == 0) {
	      registers[LoReg] = 0;
	      registers[HiReg] = 0;
	  } else {
	      tmp = rs / rt;
	      registers[LoReg] = (int) tmp;
	      tmp = rs % rt;
	      registers[HiReg] = (int) tmp;
	  }
	  break;
	
      case OP_JAL:
	registers[R31] = registers[NextPCReg] + 4;
      case OP_J:
	pcAfter = (pcAfter & 0xf0000000) | IndexToAddr(instr->extra);
	break;
	
      case OP_JALR:
	registers[instr->rd] = registers[NextPCReg] + 4;
      case OP_JR:
	pcAfter = registers[instr->rs];
	break;
	
      case OP_LB:
      case OP_LBU:
	tmp = registers[instr->rs] + instr->extra;
	if (!machine->ReadMem(tmp, 1, &value))
	    return;

	if ((value & 0x80) && (instr->opCode == OP_LB))
	    value |= 0xffffff00;
	else
	    value &= 0xff;
	nextLoadReg = instr->rt;
	nextLoadValue = value;
	break;
	
      case OP_LH:
      case OP_LHU:	  
	tmp = registers[instr->rs] + instr->extra;
	if (tmp & 0x1) {
	    RaiseException(AddressErrorException, tmp);
	    return;
	}
	if (!machine->ReadMem(tmp, 2, &value))
	    return;

	if ((value & 0x8000) && (instr->opCode == OP_LH))
	    value |= 0xffff0000;
	else
	    value &= 0xffff;
	nextLoadReg = instr->rt;
	nextLoadValue = value;
	break;
      	
      case OP_LUI:
	DEBUG('m', "Executing: LUI r%d,%d\n", instr->rt, instr->extra);
	registers[instr->rt] = instr->extra << 16;
	break;
	
      case OP_LW:
	tmp = registers[instr->rs] + instr->extra;
	if (tmp & 0x3) {
	    RaiseException(AddressErrorException, tmp);
	    return;
	}
	if (!machine->ReadMem(tmp, 4, &value))
	    return;
	nextLoadReg = instr->rt;
	nextLoadValue = value;
	break;
    	
      case OP_LWL:	  
	tmp = registers[instr->rs] + instr->extra;

	// ReadMem 假设所有 4 字节请求都对齐到偶数
	// 字边界。此外，小端/大端交换代码会
        // 失败（我认为）如果其他情况曾经被使用。
	ASSERT((tmp & 0x3) == 0);  

	if (!machine->ReadMem(tmp, 4, &value))
	    return;
	if (registers[LoadReg] == instr->rt)
	    nextLoadValue = registers[LoadValueReg];
	else
	    nextLoadValue = registers[instr->rt];
	switch (tmp & 0x3) {
	  case 0:
	    nextLoadValue = value;
	    break;
	  case 1:
	    nextLoadValue = (nextLoadValue & 0xff) | (value << 8);
	    break;
	  case 2:
	    nextLoadValue = (nextLoadValue & 0xffff) | (value << 16);
	    break;
	  case 3:
	    nextLoadValue = (nextLoadValue & 0xffffff) | (value << 24);
	    break;
	}
	nextLoadReg = instr->rt;
	break;
      	
      case OP_LWR:
	tmp = registers[instr->rs] + instr->extra;

	// ReadMem 假设所有 4 字节请求都对齐到偶数
	// 字边界。此外，小端/大端交换代码会
        // 失败（我认为）如果其他情况曾经被使用。
	ASSERT((tmp & 0x3) == 0);  

	if (!machine->ReadMem(tmp, 4, &value))
	    return;
	if (registers[LoadReg] == instr->rt)
	    nextLoadValue = registers[LoadValueReg];
	else
	    nextLoadValue = registers[instr->rt];
	switch (tmp & 0x3) {
	  case 0:
	    nextLoadValue = (nextLoadValue & 0xffffff00) |
		((value >> 24) & 0xff);
	    break;
	  case 1:
	    nextLoadValue = (nextLoadValue & 0xffff0000) |
		((value >> 16) & 0xffff);
	    break;
	  case 2:
	    nextLoadValue = (nextLoadValue & 0xff000000)
		| ((value >> 8) & 0xffffff);
	    break;
	  case 3:
	    nextLoadValue = value;
	    break;
	}
	nextLoadReg = instr->rt;
	break;
    	
      case OP_MFHI:
	registers[instr->rd] = registers[HiReg];
	break;
	
      case OP_MFLO:
	registers[instr->rd] = registers[LoReg];
	break;
	
      case OP_MTHI:
	registers[HiReg] = registers[instr->rs];
	break;
	
      case OP_MTLO:
	registers[LoReg] = registers[instr->rs];
	break;
	
      case OP_MULT:
	Mult(registers[instr->rs], registers[instr->rt], TRUE,
	     &registers[HiReg], &registers[LoReg]);
	break;
	
      case OP_MULTU:
	Mult(registers[instr->rs], registers[instr->rt], FALSE,
	     &registers[HiReg], &registers[LoReg]);
	break;
	
      case OP_NOR:
	registers[instr->rd] = ~(registers[instr->rs] | registers[instr->rt]);
	break;
	
      case OP_OR:
	registers[instr->rd] = registers[instr->rs] | registers[instr->rs];
	break;
	
      case OP_ORI:
	registers[instr->rt] = registers[instr->rs] | (instr->extra & 0xffff);
	break;
	
      case OP_SB:
	if (!machine->WriteMem((unsigned) 
		(registers[instr->rs] + instr->extra), 1, registers[instr->rt]))
	    return;
	break;
	
      case OP_SH:
	if (!machine->WriteMem((unsigned) 
		(registers[instr->rs] + instr->extra), 2, registers[instr->rt]))
	    return;
	break;
	
      case OP_SLL:
	registers[instr->rd] = registers[instr->rt] << instr->extra;
	break;
	
      case OP_SLLV:
	registers[instr->rd] = registers[instr->rt] <<
	    (registers[instr->rs] & 0x1f);
	break;
	
      case OP_SLT:
	if (registers[instr->rs] < registers[instr->rt])
	    registers[instr->rd] = 1;
	else
	    registers[instr->rd] = 0;
	break;
	
      case OP_SLTI:
	if (registers[instr->rs] < instr->extra)
	    registers[instr->rt] = 1;
	else
	    registers[instr->rt] = 0;
	break;
	
      case OP_SLTIU:	  
	rs = registers[instr->rs];
	imm = instr->extra;
	if (rs < imm)
	    registers[instr->rt] = 1;
	else
	    registers[instr->rt] = 0;
	break;
      	
      case OP_SLTU:	  
	rs = registers[instr->rs];
	rt = registers[instr->rt];
	if (rs < rt)
	    registers[instr->rd] = 1;
	else
	    registers[instr->rd] = 0;
	break;
      	
      case OP_SRA:
	registers[instr->rd] = registers[instr->rt] >> instr->extra;
	break;
	
      case OP_SRAV:
	registers[instr->rd] = registers[instr->rt] >>
	    (registers[instr->rs] & 0x1f);
	break;
	
      case OP_SRL:
	tmp = registers[instr->rt];
	tmp >>= instr->extra;
	registers[instr->rd] = tmp;
	break;
	
      case OP_SRLV:
	tmp = registers[instr->rt];
	tmp >>= (registers[instr->rs] & 0x1f);
	registers[instr->rd] = tmp;
	break;
	
      case OP_SUB:	  
	diff = registers[instr->rs] - registers[instr->rt];
	if (((registers[instr->rs] ^ registers[instr->rt]) & SIGN_BIT) &&
	    ((registers[instr->rs] ^ diff) & SIGN_BIT)) {
	    RaiseException(OverflowException, 0);
	    return;
	}
	registers[instr->rd] = diff;
	break;
      	
      case OP_SUBU:
	registers[instr->rd] = registers[instr->rs] - registers[instr->rt];
	break;
	
      case OP_SW:
	if (!machine->WriteMem((unsigned) 
		(registers[instr->rs] + instr->extra), 4, registers[instr->rt]))
	    return;
	break;
	
      case OP_SWL:	  
	tmp = registers[instr->rs] + instr->extra;

	// 小端/大端交换代码会
        // 失败（我认为）如果其他情况曾经被使用。
	ASSERT((tmp & 0x3) == 0);  

	if (!machine->ReadMem((tmp & ~0x3), 4, &value))
	    return;
	switch (tmp & 0x3) {
	  case 0:
	    value = registers[instr->rt];
	    break;
	  case 1:
	    value = (value & 0xff000000) | ((registers[instr->rt] >> 8) &
					    0xffffff);
	    break;
	  case 2:
	    value = (value & 0xffff0000) | ((registers[instr->rt] >> 16) &
					    0xffff);
	    break;
	  case 3:
	    value = (value & 0xffffff00) | ((registers[instr->rt] >> 24) &
					    0xff);
	    break;
	}
	if (!machine->WriteMem((tmp & ~0x3), 4, value))
	    return;
	break;
    	
      case OP_SWR:	  
	tmp = registers[instr->rs] + instr->extra;

	// 小端/大端交换代码会
        // 失败（我认为）如果其他情况曾经被使用。
	ASSERT((tmp & 0x3) == 0);  

	if (!machine->ReadMem((tmp & ~0x3), 4, &value))
	    return;
	switch (tmp & 0x3) {
	  case 0:
	    value = (value & 0xffffff) | (registers[instr->rt] << 24);
	    break;
	  case 1:
	    value = (value & 0xffff) | (registers[instr->rt] << 16);
	    break;
	  case 2:
	    value = (value & 0xff) | (registers[instr->rt] << 8);
	    break;
	  case 3:
	    value = registers[instr->rt];
	    break;
	}
	if (!machine->WriteMem((tmp & ~0x3), 4, value))
	    return;
	break;
    	
      case OP_SYSCALL:
	RaiseException(SyscallException, 0);
	return; 
	
      case OP_XOR:
	registers[instr->rd] = registers[instr->rs] ^ registers[instr->rt];
	break;
	
      case OP_XORI:
	registers[instr->rt] = registers[instr->rs] ^ (instr->extra & 0xffff);
	break;
	
      case OP_RES:
      case OP_UNIMP:
	RaiseException(IllegalInstrException, 0);
	return;
	
      default:
	ASSERT(FALSE);
    }
    
    // 现在我们已成功执行指令。
    
    // 执行任何延迟加载操作
    DelayedLoad(nextLoadReg, nextLoadValue);
    
    // 推进程序计数器。
    registers[PrevPCReg] = registers[PCReg];	// 用于调试，以防我们
						// 跳入未知领域
    registers[PCReg] = registers[NextPCReg];
    registers[NextPCReg] = pcAfter;
}

//----------------------------------------------------------------------
// Machine::DelayedLoad
// 	模拟延迟加载的效果。
//
// 	注意 -- RaiseException/CheckInterrupts 也必须调用 DelayedLoad，
//	因为在陷入内核之前必须应用任何延迟加载。
//----------------------------------------------------------------------

void
Machine::DelayedLoad(int nextReg, int nextValue)
{
    registers[registers[LoadReg]] = registers[LoadValueReg];
    registers[LoadReg] = nextReg;
    registers[LoadValueReg] = nextValue;
    registers[0] = 0; 	// 并始终确保 R0 保持为零。
}

//----------------------------------------------------------------------
// Instruction::Decode
// 	解码 MIPS 指令 
//----------------------------------------------------------------------

void
Instruction::Decode()
{
    OpInfo *opPtr;
    
    rs = (value >> 21) & 0x1f;
    rt = (value >> 16) & 0x1f;
    rd = (value >> 11) & 0x1f;
    opPtr = &opTable[(value >> 26) & 0x3f];
    opCode = opPtr->opCode;
    if (opPtr->format == IFMT) {
	extra = value & 0xffff;
	if (extra & 0x8000) {
    	   extra |= 0xffff0000;
	}
    } else if (opPtr->format == RFMT) {
	extra = (value >> 6) & 0x1f;
    } else {
	extra = value & 0x3ffffff;
    }
    if (opCode == SPECIAL) {
	opCode = specialTable[value & 0x3f];
    } else if (opCode == BCOND) {
	int i = value & 0x1f0000;

	if (i == 0) {
    	    opCode = OP_BLTZ;
	} else if (i == 0x10000) {
    	    opCode = OP_BGEZ;
	} else if (i == 0x100000) {
    	    opCode = OP_BLTZAL;
	} else if (i == 0x110000) {
    	    opCode = OP_BGEZAL;
	} else {
    	    opCode = OP_UNIMP;
	}
    }
}

//----------------------------------------------------------------------
// Mult
// 	模拟 R2000 乘法。
// 	*hiPtr 和 *loPtr 处的字被乘法的
// 	双长度结果覆盖。
//----------------------------------------------------------------------

static void
Mult(int a, int b, bool signedArith, int* hiPtr, int* loPtr)
{
    if ((a == 0) || (b == 0)) {
	*hiPtr = *loPtr = 0;
	return;
    }

    // 计算结果的符号，然后使所有内容为正
    // 以便在主循环中进行无符号计算。
    bool negative = FALSE;
    if (signedArith) {
	if (a < 0) {
	    negative = (bool)!negative;
	    a = -a;
	}
	if (b < 0) {
	    negative = (bool)!negative;
	    b = -b;
	}
    }

    // 在无符号算术中计算结果（检查 a 的位，一次一个，
    // 并添加一个移位的 b 值）。
    unsigned int bLo = b;
    unsigned int bHi = 0;
    unsigned int lo = 0;
    unsigned int hi = 0;
    for (int i = 0; i < 32; i++) {
	if (a & 1) {
	    lo += bLo;
	    if (lo < bLo)  // 低位的进位？
		hi += 1;
	    hi += bHi;
	    if ((a & 0xfffffffe) == 0)
		break;
	}
	bHi <<= 1;
	if (bLo & 0x80000000)
	    bHi |= 1;
	
	bLo <<= 1;
	a >>= 1;
    }

    // 如果结果应该是负数，计算双字结果的二进制补码。
    if (negative) {
	hi = ~hi;
	lo = ~lo;
	lo++;
	if (lo == 0)
	    hi++;
    }
    
    *hiPtr = (int) hi;
    *loPtr = (int) lo;
}