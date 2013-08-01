/*
 *  SDMPESymbolTable.c
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

#ifndef _SDMPESYMBOLTABLE_C_
#define _SDMPESYMBOLTABLE_C_

#include "SDMPESymbolTable.h"

struct SDMPELibrarySymbolTable* SDMSTLoadLibraryPE(char *path) {
	struct SDMPELibrarySymbolTable *table = (struct SDMPELibrarySymbolTable *)calloc(0x1, sizeof(struct SDMPELibrarySymbolTable));
	if (table && path) {
		table->libraryPath = path;
		table->library = (struct SDMPELibrary *)calloc(0x1, sizeof(struct SDMPELibrary));
		FILE* dll = fopen(table->libraryPath, "r");
		if (dll) {
			fseek(dll, 0x0, SEEK_END);
			table->library->size = ftell(dll);
			fseek(dll, 0x0, SEEK_SET);
			table->library->handle = calloc(table->library->size, 0x1);
			fread(table->library->handle, table->library->size, 0x1, dll);
		}
		fclose(dll);
		
	}
	return table;
}

#endif

