/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2008 coresystems GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARCH_IO_H
#define _ARCH_IO_H

#define readb(_a) (*(volatile const unsigned char *) (_a))
#define readw(_a) (*(volatile const unsigned short *) (_a))
#define readl(_a) (*(volatile const unsigned int *) (_a))

#define writeb(_v, _a) (*(volatile unsigned char *) (_a) = (_v))
#define writew(_v, _a) (*(volatile unsigned short *) (_a) = (_v))
#define writel(_v, _a) (*(volatile unsigned int *) (_a) = (_v))

#include "../../../port.h"

static inline unsigned int inl(int port)
{
//	unsigned long val;
//	__asm__ __volatile__("inl %w1, %0" : "=a"(val) : "Nd"(port));
//	return val;
	return port_in32(port);
}

static inline unsigned short inw(int port)
{
//	unsigned short val;
//	__asm__ __volatile__("inw %w1, %w0" : "=a"(val) : "Nd"(port));
//	return val;
	return port_in16(port);
}

static inline unsigned char inb(int port)
{
//	unsigned char val;
//	__asm__ __volatile__("inb %w1, %b0" : "=a"(val) : "Nd"(port));
//	return val;
	return port_in8(port);
}

static inline void outl(unsigned int val, int port)
{
	//__asm__ __volatile__("outl %0, %w1" : : "a"(val), "Nd"(port));
	port_out32(port, val);
}

static inline void outw(unsigned short val, int port)
{
	//__asm__ __volatile__("outw %w0, %w1" : : "a"(val), "Nd"(port));
	port_out16(port, val);
}

static inline void outb(unsigned char val, int port)
{
	//__asm__ __volatile__("outb %b0, %w1" : : "a"(val), "Nd"(port));
	port_out8(port, val);
}

static inline void outsl(int port, const void *addr, unsigned long count)
{
	//__asm__ __volatile__("rep; outsl" : "+S"(addr), "+c"(count) : "d"(port));
}

static inline void outsw(int port, const void *addr, unsigned long count)
{
	//__asm__ __volatile__("rep; outsw" : "+S"(addr), "+c"(count) : "d"(port));
}

static inline void outsb(int port, const void *addr, unsigned long count)
{
	//__asm__ __volatile__("rep; outsb" : "+S"(addr), "+c"(count) : "d"(port));
}

static inline void insl(int port, void *addr, unsigned long count)
{
	//__asm__ __volatile__("rep; insl" : "+D"(addr), "+c"(count) : "d"(port)
	//		     : "memory");
}

static inline void insw(int port, void *addr, unsigned long count)
{
	//__asm__ __volatile__("rep; insw" : "+D"(addr), "+c"(count) : "d"(port)
	//		     : "memory");
}

static inline void insb(int port, void *addr, unsigned long count)
{
	//__asm__ __volatile__("rep; insb" : "+D"(addr), "+c"(count) : "d"(port)
			     //: "memory");
}

#endif
