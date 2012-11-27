
#include "AfUtf8.h"

#include <stdio.h>
#include <stdlib.h>

#define ERR_NONE 0
#define ERR_FILE 1
#define ERR_ENCODING 2

static int max(int a, int b) {
	return (a > b) ? a : b;
}

typedef int(*check_function)(const size_t length, const char* data);

static int process(const char* filename, check_function impl)
{
	int result = ERR_FILE;

	FILE* fp = fopen(filename, "rb");
	if(fp) {
		if(fseek(fp, 0, SEEK_END) == 0) {
			off_t len = ftello(fp);
			if(len >= 0) {
				char* data = malloc(len);
				if(data) {
					if(fseek(fp, 0, SEEK_SET) == 0) {
						if(fread(data, 1, len, fp) == len) {
							if(impl(len, data)) {
								result = ERR_NONE;
							} else {
								result = ERR_ENCODING;
							}
						}
					}
				}
				free(data);
			}
		}
		fclose(fp);
	}

	return result;
}


int main(int argc, char** argv) {
	int i, result;
	int maxerror = 0;

#ifdef AFUTF8_VERBOSE
	check_function impl = AfUtf8_check_prototype;
#else
	check_function impl = AfUtf8_check;
#endif

	for(i = 1; i < argc; ++i) {
		result = process(argv[i], impl);
		maxerror = max(result, maxerror);

		switch(result) {
			case ERR_NONE: printf("%s: valid UTF-8\n", argv[i]); break;
			case ERR_ENCODING: printf("%s: not valid UTF-8!\n", argv[i]); break;
			case ERR_FILE: printf("%s: could not read file into memory\n", argv[i]); break;
		}
	}

	return maxerror;
}

