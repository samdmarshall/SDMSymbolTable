/*
 *  SDMSymbolCall.s
 *  SDMSymbolTable
 *
 *  Copyright (c) 2013, Sam Marshall
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software must display the following acknowledgement:
 *  	This product includes software developed by the Sam Marshall.
 *  4. Neither the name of the Sam Marshall nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY Sam Marshall ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Sam Marshall BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
 
.globl _makeDynamicCallWithIntList
_makeDynamicCallWithIntList: ;// int (int nargs, int *ints, int (*fptr)(int n, int args...))
#if __x86_64__
	pushq %rbp
	pushq %r10
	pushq %r11
	pushq %r12
	
	movq %rdi, %r10 ;// nargs
	movq %rsi, %r11 ;// ints
	movq %rdx, %r12 ;// fptr
	
	;// if there's an arg left, put it in rsi
	cmp $0, %r10
	cmovne (%r11), %rsi
	addq $4, %r11
	
	;// if there's another arg left, put it in rdx
	dec %r10
	cmp $0, %r10
	cmovne (%r11), %rdx
	addq $4, %r11
	
	;// and so on for rcx, r8, r9...
	
	;// did we run out of registers?
L1:
	cmp $0, %r8
	je L2
	pushq (%r9)
	addq $4, %r9
	dec %r8
	jmp L1

L2:
	call *%r12
	;// leave rax alone
	
	pop %r12
	pop %r11
	pop %r10
	pop %rbp
	ret
#elif __i386__
	;// same thing, but just go straight to the dump-to-stack loop
#elif __arm__
	;// same thing, but the sequence is r0, r1,r2, r3, stack
#endif