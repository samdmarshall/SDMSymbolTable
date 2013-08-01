/*
 *  SDMPESymbolTable.h
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

#ifndef _SDMPESYMBOLTABLE_H_
#define _SDMPESYMBOLTABLE_H_

#pragma mark -
#pragma mark Includes
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#pragma mark -
#pragma mark Types

struct SDMPEOptionalHeader {
	bool hasHeader;
	uint16_t headerSize;
} __attribute__ ((packed)) SDMPEOptionalHeader;

struct SDMPELibraryTableInfo {
	uint16_t machineType;
	uintptr_t* header;
	uint16_t sectionCount;
	uintptr_t* symbolTable;
	uint32_t symbolTotal;
	struct SDMPEOptionalHeader *altHeader;
	uint16_t characteristics;
} SDMPELibraryTableInfo;

struct SDMPESymbol {
	uint32_t tableNumber;
	uint32_t symbolNumber;
	void* offset;
	char *name;
} __attribute__ ((packed)) SDMPESymbol;

struct SDMPELibrary {
	char *handle;
	uint32_t size;
} __attribute__ ((packed)) SDMPELibrary;

typedef struct SDMPELibrarySymbolTable {
	char *libraryPath;
	struct SDMPELibrary *library;
	struct SDMPELibraryTableInfo *libInfo;
	struct SDMPESymbol *table;
	uint32_t symbolCount;
} __attribute__ ((packed)) SDMPELibrarySymbolTable;

#pragma mark -
#pragma mark Declarations

struct SDMPELibrarySymbolTable* SDMSTLoadLibraryPE(char *path);

#endif