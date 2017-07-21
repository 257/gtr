/*
 * Exploring the types of json-c.
 *
 * clang -Wall -I/usr/include/json-c/ -o json_types json_types.c -ljson-c
 */
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#define MAX_FILESIZE 4096

void debug(char *, ...);

int main(int arc, char *argv[])
{
	json_object *jobj = NULL;
	json_tokener *tok = json_tokener_new();
	enum json_tokener_error jerr;
	char f2m[MAX_FILESIZE];
	long nc = 0;
	FILE *fp;
	int c;
	do {
		if (arc > 1) {
			fp = fopen(argv[1], "r");
		} else {
			fp = fdopen(STDIN_FILENO, "r");
		}
		if (!fp) {
			debug("%s", strerror(errno));
			return errno;
		}
		while((c = fgetc(fp)) != EOF)
			f2m[nc++] = c;
		fclose(fp);
		debug("[DEBUG] file operations: %s", strerror(errno));
		debug("[DEBUG] %s = %d", "nc", nc);
		debug("[DEBUG] %s : %s", "f2m", f2m);
		jobj = json_tokener_parse_ex(tok, f2m, nc);
	} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerr != json_tokener_success) {
		debug("[ERROR]: %s\n", json_tokener_error_desc(jerr));
		// Handle errors, as appropriate for your application.
	}
	debug("[JSON] %s", json_object_get_string(jobj));
	debug("[LEN] %s %d", "jobj", json_object_array_length(jobj)); // 2
	json_object *jobj_1 = json_object_array_get_idx(jobj, 1);
	int len = json_object_array_length(jobj_1); 
	debug("[LEN] %s %d", "jobj_1", json_object_array_length(jobj_1)); // 8
	int i = 0;
	json_object *jobj_tmp0, *jobj_tmp1;
	while(i < len) {
		jobj_tmp0 = json_object_array_get_idx(jobj_1, i++);
		jobj_tmp1 = json_object_array_get_idx(jobj_tmp0, 0);
		fprintf(stdout, "%s\n", json_object_get_string(jobj_tmp1));
	}
	json_object_put(jobj_tmp0);
	json_object_put(jobj_tmp1);

	// json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
	/*
	if (strcmp(type, "string") == 0) {
		debug("[TYPE] %s", type);
		debug("[STR] %s", json_object_get_string(jobj_1_0_0));
	}
	if (strcmp(type, "array") == 0)
		debug("[ARR] %s", type);
	if (strcmp(type, "int") == 0)
		debug("[INT] %s", type);
	*/
	json_tokener_free(tok);
	json_object_put(jobj_1);
	json_object_put(jobj);

	return 0;
}
void debug(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
#ifdef DEBUG
	fprintf(stderr, "[debug] ");
	// fprintf(stderr, "%6s() %c ", func, ':'); 
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
#endif
	va_end(args);
	return ;
}
