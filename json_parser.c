# include "gtr.h"

int main(int arc, char *argv[])
{
	if (!arc)
		return 0;
	// json_object *jobj = json_object_from_file(argv[1]);
	json_object *jobj = cu(arc, argv);

	// GOOG
	if (!json_object_is_type(jobj, json_type_array))
		return 0;
	if (json_object_array_length(jobj) != 3 )
		return 0;

	json_object *sug_array = json_object_array_get_idx(jobj, 1);
	int nsug = json_object_array_length(sug_array); 
	if (!nsug)
		return 0;

	debug("got %d suggestions", nsug); // 8

	CURL *curl = curl_easy_init();
	if(!curl)
		return -4;


	curl_easy_escape(curl, GTR_URL_PREFIX, 0);

	int i = 0;
	// json_object *jobj_tmp0, *jobj_tmp1;
	json_object *call_back = json_object_new_object();

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, GTR_URL_PREFIX);
	CURLcode res;
	curl_easy_setopt(curl, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	// curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 1L);

	struct curl_slist *list = NULL;
	list = curl_slist_append(list, "dnt: 1");
	list = curl_slist_append(list, "accept-encoding: gzip, deflate, br");
	list = curl_slist_append(list, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");

	struct MemoryStruct chunk;

	json_object *sug;
	while(i < nsug) {
		sug = json_object_array_get_idx(json_object_array_get_idx(sug_array, i++), 0);
		json_object_object_add_ex(call_back, "q", sug,
				JSON_C_OBJECT_KEY_IS_CONSTANT+JSON_C_OBJECT_ADD_KEY_IS_NEW);
	};
	json_object_object_add(call_back, "source", json_object_new_string("fr"));
	json_object_object_add(call_back, "target", json_object_new_string("en"));
	json_object_object_add(call_back, "format", json_object_new_string("text"));

	debug("%s call_back \n%s", __func__, json_object_to_json_string_ext(call_back, JSON_C_TO_STRING_PRETTY));

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(call_back));

	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */ 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	res = curl_easy_perform(curl);
	// Check for errors
	json_object *complete_items = json_object_new_object();
	if(res != CURLE_OK)
		debug("curl_easy_perform() failed: %s", curl_easy_strerror(res));
	else {
		json_object *goog_sez = mem2jobj(&chunk);
		debug("%s \n:%s", "goog_sez", json_object_to_json_string_ext(goog_sez, JSON_C_TO_STRING_PRETTY));

		struct json_object_iterator it = json_object_iter_begin(goog_sez);
		struct json_object_iterator itEnd = json_object_iter_end(goog_sez);
		while (!json_object_iter_equal(&it, &itEnd) && (strcmp(json_object_iter_peek_name(&it), "data")))
			json_object_iter_next(&it);

		json_object *translations_obj;
		translations_obj = json_object_iter_peek_value(&it);
		json_object *translatedText_array;
		if (!json_object_object_get_ex(translations_obj, "translations", &translatedText_array)) {
			debug("%s", "no translations found; bailing out");
			return 0;
		}

		// analyse_obj(translatedText_array);
		// debug("%s \n%s", "translations array:", json_object_to_json_string_ext(json_object_iter_peek_value(&it), JSON_C_TO_STRING_PRETTY));

		i = 0;
		while (i < nsug) {
			sug = json_object_array_get_idx(json_object_array_get_idx(sug_array, i), 0);
			json_object *trans_pair = json_object_array_get_idx(translatedText_array, i++);
			struct json_object_iterator transit = json_object_iter_begin(trans_pair);
			struct json_object_iterator transitEnd = json_object_iter_end(trans_pair);
			while (strcmp(json_object_iter_peek_name(&transit), "translatedText"))
				if (!json_object_iter_equal(&transit, &transitEnd))
					json_object_iter_next(&transit);

			json_object *sug_transed = json_object_iter_peek_value(&transit);

			json_object_object_add(complete_items, "word", sug);
			json_object_object_add(complete_items, "menu", sug_transed);
			fprintf(stdout, "%s \n", json_object_to_json_string_ext(complete_items, JSON_C_TO_STRING_PRETTY));
			// fprintf(stdout, "%s:%s\n", json_object_get_string(sug), json_object_get_string(sug_transed));
			// json_object_iter_next(&it);
		}
		// json_object_put(goog_sez);
	}
	// TODO: write to vim's socket here
	// fprintf(stdout, "%s\n", json_object_to_json_string_ext(complete_items, JSON_C_TO_STRING_PRETTY));
	free(chunk.memory);

	curl_slist_free_all(list);
	// curl_easy_cleanup(curl);
	curl_global_cleanup();
	// debug("curl cleaned!");

	json_object_put(call_back);

	json_object_put(complete_items);

	return 0;
}
