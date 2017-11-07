/*
 * Exploring the types of json-c.
 *
 * clang -Wall -I/usr/include/json-c/ -o json_types json_types.c -ljson-c
 */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>

// https://translate.google.com/translate_a/t
// #define GTR_URL_PREFIX "http://translate.google.com/translate_a/t"
#define GTR_URL_PREFIX "https://translation.googleapis.com/language/translate/v2?key=AIzaSyDhmZhB5hbjjLgVCljrU-hM_suUAbLXtqM"


// TODO :these are default; add runtime equivalents to override
#define FORM_LEN     7
#define SL           "fr"
#define HL           "en"
#define VERSION      "1.0"
#define SRC          "is"
#define TK           "75953.488672"
#define TTSSPEED     1 // 0~1 : 0 = slow, 1 = normal
#define MAX_TOTAL    2
#define MAX_L        511
#define MAX_EMPL     32
#define STDIN        0

#define DEBUG

#define MAX_FILESIZE 4096
#define MAX_L        511
#define MAX_Q_LEN    (16*8)

// defs from cu.c
#define TTS_URL_PREFIX "http://translate.google.com/translate_tts"
#define SUG_URL_PREFIX "https://clients1.google.com/complete/search?ie=UTF-8&client=translate-web&ds=translate&hl=fr&jsonp=:&q="

// TODO :these are default; add runtime equivalents to override
#define FORM_LEN       7
#define TL             "fr"
#define IE             "UTF-8"
#define TTSSPEED       1 // 0~1 : 0 = slow, 1 = normal
#define MAX_LEN_Q      1021
#define MAX_TOTAL      2
#define MAX_L          511
#define MAX_EMPL       32
#define STDIN          0

// this limits the use of fst; let caller allocate mem so it's more portable.
struct fst {
	long nl;
	long nc;
	long nlemp;
	int  empln[MAX_EMPL];
	int  section_header[MAX_EMPL];
	char fmem[MAX_L][MAX_LEN_Q];
};
extern void debug(char *fmt, ...);
extern json_object * cu(int, char *[]);

// TODO: add contructor for fst
extern void f2mem(FILE   *fp, struct fst *fst_inst);
extern int  isempl(int    k, struct fst *fst_inst);
extern int  binsearch(int x, int v[], int n);

// functions and strcts in json_parser.c
struct MemoryStruct {
	char *memory;
	size_t size;
};

extern void analyse_obj(struct json_object *jobj);
static size_t write_callback(
		void *contents,
		size_t size,
		size_t nmemb,
		void *userp);
extern json_object *mem2jobj(struct MemoryStruct *chunk);
extern json_object *get_translatedText(json_object *goog_sez);

void debug(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
#ifdef DEBUG
	fprintf(stderr, "[%s] ", __func__);
	// fprintf(stderr, "%6s() %c ", func, ':'); 
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
#endif
	va_end(args);
	return ;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		debug("out of memory (realloc returned NULL)");
		return 0;
	}
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void analyse_obj(struct json_object *jobj) {
	int jobj_len;

	enum json_type jtype = json_object_get_type(jobj);
	struct json_object_iterator beg, end;
	// struct array_list* jerry;
	json_object *jerry_stk_mmb;
	debug("%s \n%s", __func__, json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));


	switch (jtype) {
	case json_type_null:
		debug("JLEN %s", "got NULL");
		break;
	case json_type_object:
		// struct json_object_iterator beg, end;
		beg = json_object_iter_begin(jobj);
		end = json_object_iter_end(jobj);

		while (!json_object_iter_equal(&beg, &end)) {
			// analyse_obj(json_object_iter_peek_value(&beg));
			debug("-- TOKENER --\t%s", json_object_iter_peek_name(&beg));
			json_object_iter_next(&beg);
		}
		break;
	case json_type_array:
		/* struct array_list* jerry;
		 * jerry = json_object_get_array(jobj);
		 * json_object jerry_stk_mmb;
		 */
		jobj_len = json_object_array_length(jobj);
		// debug("got %s with len %d", json_type_to_name(jtype), jobj_len);
		for (int i=0; i<jobj_len; i++) {
			jerry_stk_mmb = json_object_array_get_idx(jobj, i);
			if(json_type_array == json_object_get_type(jerry_stk_mmb))
				debug("--ARRAY--\t%s",
						json_object_to_json_string(json_object_array_get_idx(jobj, i)));
			// analyse_obj(jerry_stk_mmb);
		}
		break;
	case json_type_int:
		debug("--%s--\t%d",
				json_type_to_name(jtype), json_object_get_int64(jobj));
		break;
	case json_type_string:
		jobj_len = json_object_get_string_len(jobj);
		debug("--%s--\t%s (len %d)",
				json_type_to_name(jtype), json_object_get_string(jobj), jobj_len);
		break;
	case json_type_boolean:
	case json_type_double:
	default:
		break;
	}
}

json_object *mem2jobj(struct MemoryStruct *chunk)
{
	json_tokener *tok = json_tokener_new();
	json_object *ret_obj;
	enum json_tokener_error jerry;

	// debug("%s %s", __func__, chunk->memory);
	do {
		ret_obj = json_tokener_parse_ex(tok, chunk->memory, chunk->size);
	} while ((jerry = json_tokener_get_error(tok)) == json_tokener_continue);
	if (jerry != json_tokener_success) {
		debug("-- ERROR --: %s %s\n", __func__, json_tokener_error_desc(jerry));
		// Handle errors, as appropriate for your application.
	}
	json_tokener_free(tok);
	return ret_obj;
}

// GOOG
json_object *get_translatedText(json_object *goog_sez)
{
	enum json_type jtype = json_object_get_type(goog_sez);
	struct json_object_iterator beg, end;
	json_object *jerry_stk_mmb;

	static json_object *ret;

	int jobj_len;

	switch (jtype) {
	case json_type_object:
		beg = json_object_iter_begin(goog_sez);
		end = json_object_iter_end(goog_sez);
		// if (strcmp(json_object_get_string(goog_sez), "translatedText"))
		while (!json_object_iter_equal(&beg, &end)) {
			get_translatedText(json_object_iter_peek_value(&beg));
			json_object_iter_next(&beg);
		}
		break;
	case json_type_array:
		jobj_len = json_object_array_length(goog_sez);
		for (int i=0; i<jobj_len; i++) {
			jerry_stk_mmb = json_object_array_get_idx(goog_sez, i);
			get_translatedText(jerry_stk_mmb);
		}
		break;
	case json_type_string:
		jobj_len = json_object_get_string_len(goog_sez);
		ret = goog_sez;
		break;
	default:
		break;
	}
	// debug("[%s] --%s--\t%s", __func__, json_type_to_name(json_object_get_type(ret)), json_object_get_string(ret));
	return ret;
}

void f2mem(FILE *fp, struct fst *fst_inst)
{
	fst_inst->nl       = 0;
	fst_inst->nc       = 0;
	int iscomment      = 0;
	int empl_marker    = 0;
	int comment_marker = 0;
	int cp             = 0;
	int c;
	while((c = fgetc(fp)) != EOF && (++fst_inst->nc)) {
		if (iscomment) {
			if (c != '/') {
				iscomment = 0;
				if (iscomment == 1) {
					fst_inst->section_header[comment_marker++] = fst_inst->nl;
				} else {
					iscomment++;
				}
			}
		}
		switch (c) {
		case '\n':
			// should also check for malformed comments
			if (iscomment)
				iscomment = 0;
			if (cp == 0) {
				debug("fst_inst->nl = %d", fst_inst->nl);
				fst_inst->empln[empl_marker++] = fst_inst->nl;
				debug("empln[%d] = %d", empl_marker-1, fst_inst->empln[empl_marker-1]);
				fst_inst->nlemp++;
			}
			fst_inst->fmem[fst_inst->nl][cp++] = c;
			fst_inst->fmem[fst_inst->nl++][cp] = '\0';
			cp ^= cp;
			break;
		case '/':
			if (cp)
				fst_inst->fmem[fst_inst->nl][cp++] = c;
			else
				iscomment++;
			break;
		case '\t' : case '\v' : // add more isspace here
			if (cp)
				fst_inst->fmem[fst_inst->nl][cp++] = ' ';
			break;
		default:
			fst_inst->fmem[fst_inst->nl][cp++] = c;
			break;
		}
	}
	fclose(fp);
	// fstat()
	debug("%s(%d) nl = %ld", __func__, __LINE__, fst_inst->nl);
	debug("%s(%d) nlemp = %ld", __func__, __LINE__, fst_inst->nlemp);
	debug("%s = %d", "empl_marker", empl_marker);
	while(--empl_marker>-1) {
		debug("%s(%d) empln[%d] = %d", __func__, __LINE__, empl_marker, fst_inst->empln[empl_marker]);
	}
	debug("non-empty lines:");
	int k = 0;
	int l = k + 1;
	while(k<fst_inst->nl)
		if (!isempl(k, fst_inst)) 
			fprintf(stderr, "[%03d] : %s", l++, fst_inst->fmem[k++]);
		else
			k++;
	return;
}

int isempl(int k, struct fst *fst_inst) {
	return (fst_inst->nlemp) ?
		binsearch(k, fst_inst->empln, fst_inst->nlemp) :
		fst_inst->nlemp;
}

int binsearch(int x, int v[], int n) {
	int low, high, mid;

	low = 0;
	high = n-1;
	if (x < v[low] || v[high] < x) {
		return 0;
	}
	while(low < high) {
		mid = (low+high)/2;
		if(x <= v[mid]) 
			high=mid;
		else 
			low = mid+1;
	}
	return (x == v[low]) ? 1 : 0;
}

json_object * cu (int arc, char *argv[])
{
	// build your url string
	char url[MAX_L+MAX_LEN_Q];
	sprintf(url, "%s", SUG_URL_PREFIX);
	int i = 1;
	while (i < arc)
		strcat(url, argv[i++]);
	debug("URL: %s", url);

	json_object *goog_sez = NULL;
	// get curl ready
	CURL *curl = curl_easy_init();
	if(!curl)
		return goog_sez;

	curl_easy_escape(curl, url, 0);

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	CURLcode res;
	curl_easy_setopt(curl, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

	struct curl_slist *list = NULL;
	list = curl_slist_append(list, "dnt: 1");
	list = curl_slist_append(list, "accept-encoding: gzip, deflate, br");
	list = curl_slist_append(list, "accept-language: en-GB,fr-FR;q=0.8,fr;q=0.6,fa-IR;q=0.4,fa;q=0.2,en-US;q=0.2,en;q=0.2");
	list = curl_slist_append(list, "accept */*");
	list = curl_slist_append(list, "referer: https://translate.google.co.uk/");
	list = curl_slist_append(list, "authority: clients1.google.com");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");

	struct MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	res = curl_easy_perform(curl);
	// Check for curl errors
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s",
				curl_easy_strerror(res));
	} else {
		chunk.memory += 2;
		// debug("%s %s", "chunk.mem", chunk.memory);
		goog_sez = mem2jobj(&chunk);
		// json_object_put(goog_sez);
	}

	// analyse_obj(goog_sez);
	curl_easy_cleanup(curl);
	free(chunk.memory -= 2);
	return goog_sez;
}
