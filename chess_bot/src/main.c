#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
    char *data;
    size_t size;
}_response;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    _response *response = (_response *)userp;

    size_t new_size = response->size == 0 ? total_size + 1 : response->size + total_size;
    response->data = realloc(response->data, new_size);
    if (response->data == NULL){
        fprintf(stderr, "realloc() failed");
        return 0;
    }

    memcpy(&(response->data[response->size]), contents, total_size);
    response->size += total_size;
    response->data[response->size] = '\0';
    printf("debug: %s\n", response->data);

    return total_size;
}

int main(){
    // stuff here
    _response response;
    response.data = malloc(1);
    if (response.data == NULL){
        fprintf(stderr, "malloc() failed");
        return 1;
    }
    response.size = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    if (curl != NULL){
        struct curl_slist *headers = curl_slist_append(NULL, "Authorization: Bearer YOUR_API_TOKEN");

        curl_easy_setopt(curl, CURLOPT_URL, /*"https://lichess.org/api/stream/event"*/ "https://httpbin.org/stream/10");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        // curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        CURLcode result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(result));
        }

        printf("response:\n%s\n", response.data);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(response.data);
    curl_global_cleanup();
    return 0;
}