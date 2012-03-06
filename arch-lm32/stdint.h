#ifndef __ARCH_LM32_STDINT_H__
#define __ARCH_LM32_STDINT_H__

/*
 * We miss a stdint.h in our compiler, so provide some types here,
 * knowing the CPU is 32-bits and uses LP32 model
 */

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
typedef signed char	int8_t;
typedef signed short	int16_t;
typedef signed long	int32_t;

#endif /* __ARCH_LM32_STDINT_H__ */
