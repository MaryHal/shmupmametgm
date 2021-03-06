/*** m6809: Portable 6809 emulator ******************************************/

#pragma once

#ifndef __M6809_H__
#define __M6809_H__


enum
{
	M6809_PC=1, M6809_S, M6809_CC ,M6809_A, M6809_B, M6809_U, M6809_X, M6809_Y,
	M6809_DP
};

#define M6809_IRQ_LINE	0	/* IRQ line number */
#define M6809_FIRQ_LINE 1   /* FIRQ line number */

CPU_GET_INFO( m6809 );
#define CPU_M6809 CPU_GET_INFO_NAME( m6809 )

/* M6809e has LIC line to indicate opcode/data fetch */
CPU_GET_INFO( m6809e );
#define CPU_M6809E CPU_GET_INFO_NAME( m6809e )


CPU_DISASSEMBLE( m6809 );

typedef struct _m6809_config m6809_config;
struct _m6809_config
{
	UINT8	encrypt_only_first_byte;		/* encrypt only the first byte in 10 xx and 11 xx opcodes */
};

#endif /* __M6809_H__ */
