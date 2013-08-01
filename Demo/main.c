#include <stdio.h>
#include "SDMSymbolTable.h"

int main (int argc, const char * argv[]) {
	if (argc >= 2) {
		struct SDMMOLibrarySymbolTable *lib = SDMSTLoadLibrary((char*)argv[1]);
		for (uint32_t i = 0; i < lib->symbolCount; i++) {
			printf("%s\n",lib->table[i].name);
		}
	} else {
		struct SDMMOLibrarySymbolTable *lib = SDMSTLoadLibrary("/Applications/Cloud.app/Contents/MacOS/Cloud");
		printf("Found %i symbols...\n",lib->symbolCount);
		for (uint32_t i = 0; i < lib->symbolCount; i++) {
			printf("%s\n",lib->table[i].name);
		}
		
	}
    return 0;
}
