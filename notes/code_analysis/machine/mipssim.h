// mipssim.h 
//	模拟 MIPS 指令集的内部数据结构。
//
//  不要更改 -- 机器模拟的一部分
//
// 版权所有 (c) 1992-1993 加州大学董事会。
// 保留所有权利。请参阅 copyright.h 以获得版权公告和责任限制 
// 以及保修声明条款。

#ifndef MIPSSIM_H
#define MIPSSIM_H

#include "copyright.h"

/*
 * OpCode 值。这些名称直接来自 MIPS
 * 手册，除了以下特殊值：
 *
 * OP_UNIMP -		表示此指令合法，但尚未
 *			在模拟器中实现。
 * OP_RES -		表示这是一个保留的操作码（它不
 *			被架构支持）。
 */

#define OP_ADD		1
#define OP_ADDI		2
#define OP_ADDIU	3
#define OP_ADDU		4
#define OP_AND		5
#define OP_ANDI		6
#define OP_BEQ		7
#define OP_BGEZ		8
#define OP_BGEZAL	9
#define OP_BGTZ		10
#define OP_BLEZ		11
#define OP_BLTZ		12
#define OP_BLTZAL	13
#define OP_BNE		14

#define OP_DIV		16
#define OP_DIVU		17
#define OP_J		18
#define OP_JAL		19
#define OP_JALR		20
#define OP_JR		21
#define OP_LB		22
#define OP_LBU		23
#define OP_LH		24
#define OP_LHU		25
#define OP_LUI		26
#define OP_LW		27
#define OP_LWL		28
#define OP_LWR		29

#define OP_MFHI		31
#define OP_MFLO		32

#define OP_MTHI		34
#define OP_MTLO		35
#define OP_MULT		36
#define OP_MULTU	37
#define OP_NOR		38
#define OP_OR		39
#define OP_ORI		40
#define OP_RFE		41
#define OP_SB		42
#define OP_SH		43
#define OP_SLL		44
#define OP_SLLV		45
#define OP_SLT		46
#define OP_SLTI		47
#define OP_SLTIU	48
#define OP_SLTU		49
#define OP_SRA		50
#define OP_SRAV		51
#define OP_SRL		52
#define OP_SRLV		53
#define OP_SUB		54
#define OP_SUBU		55
#define OP_SW		56
#define OP_SWL		57
#define OP_SWR		58
#define OP_XOR		59
#define OP_XORI		60
#define OP_SYSCALL	61
#define OP_UNIMP	62
#define OP_RES		63
#define MaxOpcode	63

/*
 * 杂项定义：
 */

#define IndexToAddr(x) ((x) << 2)

#define SIGN_BIT	0x80000000
#define R31		31

/*
 * 下表用于将指令的第31:26位
 * 转换为适合MemWord结构的"opCode"字段的值，
 * 或转换为特殊值以进行进一步解码。
 */

#define SPECIAL 100
#define BCOND	101

#define IFMT 1
#define JFMT 2
#define RFMT 3

struct OpInfo {
    int opCode;		/* 转换后的操作码。 */
    int format;		/* 格式类型 (IFMT 或 JFMT 或 RFMT) */
};

static OpInfo opTable[] = {
    {SPECIAL, RFMT}, {BCOND, IFMT}, {OP_J, JFMT}, {OP_JAL, JFMT},
    {OP_BEQ, IFMT}, {OP_BNE, IFMT}, {OP_BLEZ, IFMT}, {OP_BGTZ, IFMT},
    {OP_ADDI, IFMT}, {OP_ADDIU, IFMT}, {OP_SLTI, IFMT}, {OP_SLTIU, IFMT},
    {OP_ANDI, IFMT}, {OP_ORI, IFMT}, {OP_XORI, IFMT}, {OP_LUI, IFMT},
    {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_LB, IFMT}, {OP_LH, IFMT}, {OP_LWL, IFMT}, {OP_LW, IFMT},
    {OP_LBU, IFMT}, {OP_LHU, IFMT}, {OP_LWR, IFMT}, {OP_RES, IFMT},
    {OP_SB, IFMT}, {OP_SH, IFMT}, {OP_SWL, IFMT}, {OP_SW, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_SWR, IFMT}, {OP_RES, IFMT},
    {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT},
    {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT}, {OP_UNIMP, IFMT},
    {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}, {OP_RES, IFMT}
};

/*
 * 下表用于将SPECIAL指令的"funct"字段
 * 转换为MemWord的"opCode"字段。
 */

static int specialTable[] = {
    OP_SLL, OP_RES, OP_SRL, OP_SRA, OP_SLLV, OP_RES, OP_SRLV, OP_SRAV,
    OP_JR, OP_JALR, OP_RES, OP_RES, OP_SYSCALL, OP_UNIMP, OP_RES, OP_RES,
    OP_MFHI, OP_MTHI, OP_MFLO, OP_MTLO, OP_RES, OP_RES, OP_RES, OP_RES,
    OP_MULT, OP_MULTU, OP_DIV, OP_DIVU, OP_RES, OP_RES, OP_RES, OP_RES,
    OP_ADD, OP_ADDU, OP_SUB, OP_SUBU, OP_AND, OP_OR, OP_XOR, OP_NOR,
    OP_RES, OP_RES, OP_SLT, OP_SLTU, OP_RES, OP_RES, OP_RES, OP_RES,
    OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES,
    OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES
};


// 用于帮助打印每条指令，用于调试

enum RegType { NONE, RS, RT, RD, EXTRA }; 

struct OpString {
    const char *string;	// 指令的打印版本
    RegType args[3];
};

static struct OpString opStrings[] = {
	{"不应该发生", {NONE, NONE, NONE}},
	{"ADD r%d,r%d,r%d", {RD, RS, RT}},
	{"ADDI r%d,r%d,%d", {RT, RS, EXTRA}},
	{"ADDIU r%d,r%d,%d", {RT, RS, EXTRA}},
	{"ADDU r%d,r%d,r%d", {RD, RS, RT}},
	{"AND r%d,r%d,r%d", {RD, RS, RT}},
	{"ANDI r%d,r%d,%d", {RT, RS, EXTRA}},
	{"BEQ r%d,r%d,%d", {RS, RT, EXTRA}},
	{"BGEZ r%d,%d", {RS, EXTRA, NONE}},
	{"BGEZAL r%d,%d", {RS, EXTRA, NONE}},
	{"BGTZ r%d,%d", {RS, EXTRA, NONE}},
	{"BLEZ r%d,%d", {RS, EXTRA, NONE}},
	{"BLTZ r%d,%d", {RS, EXTRA, NONE}},
	{"BLTZAL r%d,%d", {RS, EXTRA, NONE}},
	{"BNE r%d,r%d,%d", {RS, RT, EXTRA}},
	{"不应该发生", {NONE, NONE, NONE}},
	{"DIV r%d,r%d", {RS, RT, NONE}},
	{"DIVU r%d,r%d", {RS, RT, NONE}},
	{"J %d", {EXTRA, NONE, NONE}},
	{"JAL %d", {EXTRA, NONE, NONE}},
	{"JALR r%d,r%d", {RD, RS, NONE}},
	{"JR r%d,r%d", {RD, RS, NONE}},
	{"LB r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"LBU r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"LH r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"LHU r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"LUI r%d,%d", {RT, EXTRA, NONE}},
	{"LW r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"LWL r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"LWR r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"不应该发生", {NONE, NONE, NONE}},
	{"MFHI r%d", {RD, NONE, NONE}},
	{"MFLO r%d", {RD, NONE, NONE}},
	{"不应该发生", {NONE, NONE, NONE}},
	{"MTHI r%d", {RS, NONE, NONE}},
	{"MTLO r%d", {RS, NONE, NONE}},
	{"MULT r%d,r%d", {RS, RT, NONE}},
	{"MULTU r%d,r%d", {RS, RT, NONE}},
	{"NOR r%d,r%d,r%d", {RD, RS, RT}},
	{"OR r%d,r%d,r%d", {RD, RS, RT}},
	{"ORI r%d,r%d,%d", {RT, RS, EXTRA}},
	{"RFE", {NONE, NONE, NONE}},
	{"SB r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"SH r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"SLL r%d,r%d,%d", {RD, RT, EXTRA}},
	{"SLLV r%d,r%d,r%d", {RD, RT, RS}},
	{"SLT r%d,r%d,r%d", {RD, RS, RT}},
	{"SLTI r%d,r%d,%d", {RT, RS, EXTRA}},
	{"SLTIU r%d,r%d,%d", {RT, RS, EXTRA}},
	{"SLTU r%d,r%d,r%d", {RD, RS, RT}},
	{"SRA r%d,r%d,%d", {RD, RT, EXTRA}},
	{"SRAV r%d,r%d,r%d", {RD, RT, RS}},
	{"SRL r%d,r%d,%d", {RD, RT, EXTRA}},
	{"SRLV r%d,r%d,r%d", {RD, RT, RS}},
	{"SUB r%d,r%d,r%d", {RD, RS, RT}},
	{"SUBU r%d,r%d,r%d", {RD, RS, RT}},
	{"SW r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"SWL r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"SWR r%d,%d(r%d)", {RT, EXTRA, RS}},
	{"XOR r%d,r%d,r%d", {RD, RS, RT}},
	{"XORI r%d,r%d,%d", {RT, RS, EXTRA}},
	{"SYSCALL", {NONE, NONE, NONE}},
	{"未实现", {NONE, NONE, NONE}},
	{"保留", {NONE, NONE, NONE}}
      };

#endif // MIPSSIM_H