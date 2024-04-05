#include <ewoksys/vfs.h>
#include <pthread.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include "fontd.h"

static void* load_thread(void* p) {
	var_t* var = (var_t*)p;
	uint32_t num = var_array_size(var);

	for(int i=0; i<num; i++) {
		var_t* v = var_array_get_var(var, i);
		const char* name = get_str(v, "name");
		const char* fname = get_str(v, "file");
		if(strcmp(name, DEFAULT_SYSTEM_FONT) != 0)
			font_open(name, fname, DEFAULT_SYSTEM_FONT_SIZE, -1);
	}

	var_unref(var);
	return NULL;
}

void load_config(void) {
	font_open(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_FILE, DEFAULT_SYSTEM_FONT_SIZE, -1);

	int sz = 0;
	var_t* var = NULL;
	char* str = (char*)vfs_readfile("/usr/system/fonts.json", &sz);
	if(str != NULL) {
		str[sz] = 0;
		var = json_parse(str);
		if(var != NULL) {
			pthread_create(NULL, NULL, &load_thread, var);
		}
		else {
			var_unref(var);
			var = NULL;
		}
	}
	return 0;
}
