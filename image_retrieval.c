#include <regex.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct buffer {
	char* data;
	size_t size;
};

/* curl write callback, to fill tidy's input buffer...  */
unsigned int write_cb(char *in, unsigned int size, unsigned int nmemb, struct buffer* out)
{
	if (out->size != 0){
		// Plus 1 for null termination
		char* tmp = realloc(out->data, out->size+size*nmemb+1);
		if (!tmp){
			perror("Could not reallocate the data buffer to receive more data");
			return 0;
		}
		out->data = tmp;
	} else {
		out->data = calloc(nmemb, size*nmemb+1);
		if (!out){
			perror("Could not allocate a buffer to receive the response");
			return 0;
		}
	}
	memcpy(out->data+out->size, in, size*nmemb);
	out->size += size*nmemb;
	out->data[out->size] = '\0';
	return nmemb*size;
}

char* get_image_url(char* page_url){
	struct buffer data = {NULL, 0};
	CURL *curl = curl_easy_init();
	char* url=NULL;
	if(curl) {
		CURLcode res;
		char curl_errbuf[CURL_ERROR_SIZE];
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36");
		curl_easy_setopt(curl, CURLOPT_URL, page_url);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		res = curl_easy_perform(curl);

		if (data.data == NULL){
			goto cleanup;
		}

		if(!res) {
			int regerr;
			regex_t regex;
			if ((regerr = regcomp(&regex, "screenshot-image\" src=\"[^\"]*\"", REG_EXTENDED)) != 0){
				size_t errbuff_size = CURL_ERROR_SIZE;
				regerror(regerr, &regex, curl_errbuf, errbuff_size);
				printf("Failed regex compilation: %s\n", curl_errbuf);
				return NULL;
			}
			regmatch_t match[1] = {{0, 0}};
			if ((regexec(&regex, data.data, 1, match, 0)) != 0){
				size_t errbuff_size = CURL_ERROR_SIZE;
				regerror(regerr, &regex, curl_errbuf, errbuff_size);
				printf("Image not found: %s\n", curl_errbuf);
				return NULL;
			}
			int url_len = match[0].rm_eo - match[0].rm_so - 23 -1 ;// removes the start and the trailling quote
			url=calloc(url_len+1, sizeof(char)); // null termination
			strncpy(url, data.data+match[0].rm_so + 23, url_len); 
			regfree(&regex);
		}
		else {
			printf("%d -> %s\n", res, curl_easy_strerror(res));
		}
	}
cleanup:
	curl_easy_cleanup(curl);
	free(data.data);
	return url;
}

