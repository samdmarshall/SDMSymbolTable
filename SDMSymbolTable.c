/*
 *  SDMSymbolTable.c
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

#ifndef _SDMSYMBOLTABLE_C_
#define _SDMSYMBOLTABLE_C_

#include "SDMSymbolTable.h"
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/ldsyms.h>

#include "udis86.h"

typedef struct SDMSTSymbolTableListEntry {
	union {
	   uint32_t n_strx;
	} n_un;
	uint8_t n_type;
	uint8_t n_sect;
	uint16_t n_desc;
} __attribute__ ((packed)) SDMSTSymbolTableListEntry;

typedef struct SDMSTSeg32Data {
	uint32_t vmaddr;
	uint32_t vmsize;
	uint32_t fileoff;
} __attribute__ ((packed)) SDMSTSeg32Data;

typedef struct SDMSTSeg64Data {
	uint64_t vmaddr;
	uint64_t vmsize;
	uint64_t fileoff;
} __attribute__ ((packed)) SDMSTSeg64Data;

struct SDMSTLibrarySymbolTable* SDMSTLoadLibrary(char *path) {
	struct SDMSTLibrarySymbolTable *table = (struct SDMSTLibrarySymbolTable *)calloc(0x1, sizeof(struct SDMSTLibrarySymbolTable));
	void* handle = dlopen(path, RTLD_LOCAL);
	if (handle) {
		table->libraryPath = path;
		table->libraryHandle = handle;
		table->libInfo = NULL;
		table->symbolCount = 0x0;
		table->table = (struct SDMSTMachOSymbol *)calloc(0x1, sizeof(struct SDMSTMachOSymbol));
		table->offsets = NULL;
		table->offsetCount = 0x0;
	}
	return table;
}

bool SMDSTSymbolDemangleAndCompare(char *symFromTable, char *symbolName) {
	bool matchesName = false;
	if (symFromTable) {
		uint32_t tabSymLength = strlen(symFromTable);
		uint32_t symLength = strlen(symbolName);
		if (symLength <= tabSymLength) {
			char *offset = strstr(symFromTable, symbolName);
			if (offset) {
				uint32_t originOffset = offset - symFromTable;
				if (tabSymLength-originOffset == symLength)
					matchesName = (strcmp(&symFromTable[originOffset], symbolName) == 0x0);
			} 
		}
	}
	return matchesName;
}

int CompareTableEntries(const void *entry1, const void *entry2) {
	if (((struct SDMSTOffsetTable *)entry1)->offset < ((struct SDMSTOffsetTable *)entry2)->offset) return -1;
	if (((struct SDMSTOffsetTable *)entry1)->offset == ((struct SDMSTOffsetTable *)entry2)->offset) return 0;
	if (((struct SDMSTOffsetTable *)entry1)->offset > ((struct SDMSTOffsetTable *)entry2)->offset) return 1;
}

uint32_t SDMSTGetFunctionLength(struct SDMSTLibrarySymbolTable *libTable, void* functionPointer) {
	uint32_t callLength = 0x0;
	if (libTable->offsets == NULL) {
		void* symbolAddress = 0x0;
		libTable->offsets = (struct SDMSTOffsetTable *)calloc(0x1, sizeof(struct SDMSTOffsetTable));
		libTable->offsetCount = 0x0;
		for (uint32_t i = 0x0; i < libTable->libInfo->symtabCount; i++) {
			struct symtab_command *cmd = &libTable->libInfo->symtabCommands[i];
			uint64_t fslide = 0x0, mslide = 0x0;
			if (libTable->libInfo->is64bit) {
				struct SDMSTSeg64Data *textData = (char*)libTable->libInfo->textSeg + sizeof(struct SDMSTSegmentEntry);
				struct SDMSTSeg64Data *linkData = (char*)libTable->libInfo->linkSeg + sizeof(struct SDMSTSegmentEntry);
				fslide = (linkData->vmaddr - textData->vmaddr) - linkData->fileoff;
				mslide = (char*)libTable->libInfo->mhOffset - textData->vmaddr;
			} else {
				struct SDMSTSeg32Data *textData = (char*)libTable->libInfo->textSeg + sizeof(struct SDMSTSegmentEntry);
				struct SDMSTSeg32Data *linkData = (char*)libTable->libInfo->linkSeg + sizeof(struct SDMSTSegmentEntry);
				fslide = (linkData->vmaddr - textData->vmaddr) - linkData->fileoff;
				mslide = (char*)libTable->libInfo->mhOffset - textData->vmaddr;
			}
			struct SDMSTSymbolTableListEntry *entry = (char*)libTable->libInfo->mhOffset + cmd->symoff + fslide;
			for (uint32_t j = 0x0; j < cmd->nsyms; j++) {
				if (!(entry->n_type & N_STAB) && ((entry->n_type & N_TYPE) == N_SECT)) {
					if (libTable->libInfo->is64bit) {
						uint64_t *n_value = ((char*)entry + sizeof(struct SDMSTSymbolTableListEntry));
						symbolAddress = *n_value;
					} else {
						uint32_t *n_value = ((char*)entry + sizeof(struct SDMSTSymbolTableListEntry));
						symbolAddress = *n_value;
					}
					libTable->offsets = realloc(libTable->offsets, libTable->offsetCount+0x1);
					struct SDMSTOffsetTable *aSymbol = (struct SDMSTOffsetTable *)calloc(0x1, sizeof(struct SDMSTOffsetTable));
					aSymbol->tableNumber = i;
					aSymbol->symbolNumber = j;
					aSymbol->offset = symbolAddress + mslide + _dyld_get_image_vmaddr_slide(libTable->libInfo->imageNumber);
					libTable->offsets[libTable->offsetCount] = *aSymbol;
					libTable->offsetCount++;
				}
				entry = (char*)entry + (sizeof(struct SDMSTSymbolTableListEntry) + (libTable->libInfo->is64bit ? sizeof(uint64_t) : sizeof(uint32_t)));
			}
		}
		qsort(libTable->offsets, libTable->offsetCount, sizeof(struct SDMSTOffsetTable), CompareTableEntries);
	}
	for (uint32_t i = 0x0; i < libTable->offsetCount; i++) {
		if (libTable->offsets[i].offset == functionPointer) {
			if (i+1 < libTable->offsetCount) {
				callLength = libTable->offsets[i+1].offset - libTable->offsets[i].offset;
				break;
			} else {
				
			}
		}
	}
	return callLength;
}

uint32_t SDMSTGetArgumentCount(struct SDMSTLibrarySymbolTable *libTable, void* functionPointer) {
	uint32_t functionLength = SDMSTGetFunctionLength(libTable, functionPointer);
	ud_t ud_obj;

	ud_init(&ud_obj);
	ud_set_mode(&ud_obj, (libTable->libInfo->is64bit? 64 : 32));
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);
	ud_set_input_buffer(&ud_obj, functionPointer, functionLength);
	while (ud_disassemble(&ud_obj)) {
		printf(" %-24s\n", ud_insn_asm(&ud_obj));
	}
	return 0x0;
}

void* SDMSTSymbolLookup(struct SDMSTLibrarySymbolTable *libTable, char *symbolName) {
	void* symbolAddress = 0x0;
	for (uint32_t i = 0x0; i < libTable->symbolCount; i++) {
		if (strcmp(libTable->table[i].symbolName, symbolName) == 0x0) {
			symbolAddress = libTable->table[i].functionPointer;
			break;
		}
	}
	if (symbolAddress == 0x0) {
		symbolAddress = dlsym(libTable->libraryHandle, symbolName);
		if (symbolAddress) {
			libTable->table = realloc(libTable->table, (libTable->symbolCount+0x1)*sizeof(struct SDMSTMachOSymbol));
			struct SDMSTMachOSymbol *newSymbol = (struct SDMSTMachOSymbol *)calloc(0x1, sizeof(struct SDMSTMachOSymbol));
			newSymbol->functionPointer = symbolAddress;
			newSymbol->symbolName = calloc(0x1, strlen(symbolName)+0x1);
			strcpy(newSymbol->symbolName, symbolName);
			libTable->table[libTable->symbolCount] = *newSymbol;
			libTable->symbolCount++;
		} else {
			if (libTable->libInfo == NULL) {
				libTable->libInfo = (struct SDMSTLibraryTableInfo *)calloc(0x1, sizeof(struct SDMSTLibraryTableInfo));
				uint32_t count = _dyld_image_count();
				for (uint32_t i = 0x0; i < count; i++) {
					if (strcmp(_dyld_get_image_name(i), libTable->libraryPath) == 0x0) {
						libTable->libInfo->imageNumber = i;
						break;
					}
				}
				const struct mach_header *imageHeader = _dyld_get_image_header(libTable->libInfo->imageNumber);
				libTable->libInfo->headerMagic = imageHeader->magic;
				libTable->libInfo->is64bit = ((imageHeader->cputype == CPU_TYPE_X86_64 || imageHeader->cputype == CPU_TYPE_POWERPC64) && (libTable->libInfo->headerMagic == MH_MAGIC_64 || libTable->libInfo->headerMagic == MH_CIGAM_64)  ? true : false);
				libTable->libInfo->mhOffset = (char*)imageHeader;
			}
			if (libTable->libInfo) {
				struct mach_header *libHeader = (char*)libTable->libInfo->mhOffset;
				if (libTable->libInfo->headerMagic == libHeader->magic) {
					if (libTable->libInfo->symtabCommands == NULL) {
						struct load_command *loadCmd = (char*)libTable->libInfo->mhOffset + (libTable->libInfo->is64bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header));
						libTable->libInfo->symtabCommands = (struct symtab_command *)calloc(0x1, sizeof(struct symtab_command));
						libTable->libInfo->symtabCount = 0x0;
						for (uint32_t i = 0x0; i < libHeader->ncmds; i++) {
							if (loadCmd->cmd == LC_SYMTAB) {
								libTable->libInfo->symtabCommands = realloc(libTable->libInfo->symtabCommands, (libTable->libInfo->symtabCount+1)*sizeof(struct symtab_command));
								libTable->libInfo->symtabCommands[libTable->libInfo->symtabCount] = *(struct symtab_command *)loadCmd;
								libTable->libInfo->symtabCount++;
							}
							if (loadCmd->cmd == (libTable->libInfo->is64bit ? LC_SEGMENT_64 : LC_SEGMENT)) {
								struct SDMSTSegmentEntry *seg = loadCmd;
								if ((libTable->libInfo->textSeg == NULL) && !strncmp(SEG_TEXT,seg->segname,sizeof(seg->segname))) {
									libTable->libInfo->textSeg = (char*)seg;
								} else if ((libTable->libInfo->linkSeg == NULL) && !strncmp(SEG_LINKEDIT,seg->segname,sizeof(seg->segname))) {
									libTable->libInfo->linkSeg = (char*)seg;
								}
							}
							loadCmd = (char*)loadCmd + loadCmd->cmdsize;
						}
					}
					bool foundSymbol = false;
					for (uint32_t i = 0x0; i < libTable->libInfo->symtabCount; i++) {
						struct symtab_command *cmd = &libTable->libInfo->symtabCommands[i];
						uint64_t fslide = 0x0, mslide = 0x0;
						if (libTable->libInfo->is64bit) {
							struct SDMSTSeg64Data *textData = (char*)libTable->libInfo->textSeg + sizeof(struct SDMSTSegmentEntry);
							struct SDMSTSeg64Data *linkData = (char*)libTable->libInfo->linkSeg + sizeof(struct SDMSTSegmentEntry);
							fslide = (linkData->vmaddr - textData->vmaddr) - linkData->fileoff;
							mslide = (char*)libTable->libInfo->mhOffset - textData->vmaddr;
						} else {
							struct SDMSTSeg32Data *textData = (char*)libTable->libInfo->textSeg + sizeof(struct SDMSTSegmentEntry);
							struct SDMSTSeg32Data *linkData = (char*)libTable->libInfo->linkSeg + sizeof(struct SDMSTSegmentEntry);
							fslide = (linkData->vmaddr - textData->vmaddr) - linkData->fileoff;
							mslide = (char*)libTable->libInfo->mhOffset - textData->vmaddr;
						}
						struct SDMSTSymbolTableListEntry *entry = (char*)libTable->libInfo->mhOffset + cmd->symoff + fslide;
						for (uint32_t j = 0x0; j < cmd->nsyms; j++) {
							if (!(entry->n_type & N_STAB) && ((entry->n_type & N_TYPE) == N_SECT)) {
								char *strTable = (char*)libTable->libInfo->mhOffset + cmd->stroff + fslide;	
								if (SMDSTSymbolDemangleAndCompare(((char *)strTable + entry->n_un.n_strx), symbolName)) {
									if (libTable->libInfo->is64bit) {
										uint64_t *n_value = (char*)entry + sizeof(struct SDMSTSymbolTableListEntry);
										symbolAddress = (void*)*n_value;
									} else {
										uint32_t *n_value = (char*)entry + sizeof(struct SDMSTSymbolTableListEntry);
										symbolAddress = (void*)*n_value;
									}
									libTable->table = realloc(libTable->table, libTable->symbolCount+0x1);
									struct SDMSTMachOSymbol *newSymbol = (struct SDMSTMachOSymbol *)calloc(0x1, sizeof(struct SDMSTMachOSymbol));
									newSymbol->functionPointer = symbolAddress + mslide + _dyld_get_image_vmaddr_slide(libTable->libInfo->imageNumber);
									newSymbol->symbolName = calloc(0x1, strlen(symbolName)+0x1);
									strcpy(newSymbol->symbolName, symbolName);
									libTable->table[libTable->symbolCount] = *newSymbol;
									libTable->symbolCount++;
									foundSymbol = true;
									break;
								}
							}
							entry = (char*)entry + (sizeof(struct SDMSTSymbolTableListEntry) + (libTable->libInfo->is64bit ? sizeof(uint64_t) : sizeof(uint32_t)));
						}
						if (foundSymbol)
							break;
					}
				}
			}
		}
	}
	return symbolAddress;
}

void SDMSTLibraryRelease(struct SDMSTLibrarySymbolTable *libTable) {
	free(libTable->libInfo);
	for (uint32_t i = 0x0; i < libTable->symbolCount; i++) {
		free(libTable->table[i].symbolName);
	}
	free(libTable->offsets);
	free(libTable->table);
	dlclose(libTable->libraryHandle);
	free(libTable);
}

#endif