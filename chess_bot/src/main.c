#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/init_chess_engine.h"
#include "chess_engine/board.h"
#include "chess_engine/search.h"

#define MAX_GAMES_PLAYED 5

typedef struct{
    char *data;
    size_t size;
    int length;
    int games_accepted;
    pthread_t games[MAX_GAMES_PLAYED];
}_event_stream_response;

typedef struct{
    char *id;
    int id_length;
    _color color;
}_game_args;

typedef struct{
    char *data;
    size_t size;
    int length;
    _board *board;
    _search_context *context;
    _search_stats *stats;
}_board_stream_response;

void get_json_data(const char *start, const char *end, const char *name, char *value){
    char *name_ptr = strstr(start, name);
    value = NULL;
    // check it was found and within range
    if (name_ptr == NULL || end - name_ptr <= 0){
        return;
    }

    // get to value
    while (*name_ptr != ' '){
        name_ptr++;
        if (end - name_ptr <= 0 || *name_ptr == '\0'){
            return;
        }
    }
    name_ptr++;
    if (end - name_ptr <= 0 || *name_ptr == '\0'){
        return;
    }

    // get size of value
    char *temp = name_ptr;
    int size = 1;
    while (temp != ','){
        temp++;
        size++;
        if (end - temp <= 0 || *temp == '\0'){
            return;
        }
    }

    // get value
    value = malloc(sizeof(char) * size);
    if (value == NULL){
        fprintf(stderr, "malloc() failed");
    }
    for (int i = 0; i < size - 1; i++){
        value[i] = name_ptr[i];
    }
    value[size - 1] = '\0';
}

void append_contents(char **data, char *contents, size_t content_len, size_t *data_len, size_t *data_size){
    const size_t new_len = *data_len == 0 ? content_len + 1 : *data_len + content_len;
    if (new_len > data_size){
        *data_size = new_len;
        *data = realloc(*data, new_len);
        if (*data == NULL){
            fprintf(stderr, "realloc() failed");
            exit(EXIT_FAILURE);
        }
    }

    memcpy(&(*data[*data_len]), contents, content_len);
    *data[new_len] = '\0';
    *data_len = new_len;
}

size_t board_event_callback(void *contents, size_t size, size_t nmemb, void *userp){
    const size_t content_len = size * nmemb;
    _board_stream_response *response = (_board_stream_response *)userp;

    append_contents(&response->data, contents, content_len, &response->length, &response->size);

    // check for responses  
}

void play_game(void *args){
    _game_args *arg = (_game_args *)args;
    const _color color = arg->color;
    const int id_length = arg->id_length;
    char *id = arg->id;
    _board_stream_response response;
    response.data = malloc(1);
    if (response.data == NULL){
        fprintf(stderr, "malloc() failed");
        return 1;
    }
    response.size = 0;
    response.length = 0;
    response.board = cb_create_board();
    _search_context context;
    _search_stats stats;
    se_init_search_context(&context);
    response.context = &context;
    response.stats = &stats;

    // listen to stream
    CURL *curl = curl_easy_init();
    if (curl != NULL){
        struct curl_slist *headers = curl_slist_append(NULL, "Authorization: Bearer YOUR_API_TOKEN");

        // make url
        char *base = "https://lichess.org/api/board/game/stream/";
        char *url = malloc(sizeof(char) * (42 + id_length));
        if (url == NULL){
            fprintf(stderr, "malloc() failed");
            exit(EXIT_FAILURE);
        }
        sprintf(url, "%s%s", base, id);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, board_event_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(id);
    cb_destroy_board(response.board);
    se_destroy_search_context(response.context);
}

size_t event_stream_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    const size_t content_len = size * nmemb;
    _event_stream_response *response = (_event_stream_response *)userp;

    int i = response->length;
    append_contents(&response->data, contents, content_len, &response->length, &response->size);
    char *start = response->data;

    // check for responses
    while (i < response->length){
        // found full response
        if (response->data[i] == '\n'){
            _game_args args;
            // find type
            char *val = NULL;
            const char *end = response + i;
            get_json_data(start, end, "\"type\"", val);
            if (val == NULL || strcmp(val, "\"gameStart\"") != 0){
                start = response->data + i + 1;
                i++;
                continue;
            }

            // check if my turn
            get_json_data(start, end, "\"color\"", val);
            if (val == NULL){
                start = response->data + i + 1;
                i++;
                continue;
            }
            args.color = strcmp(val, "\"white\"") == 0 ? WHITE : BLACK;
            free(val);
            val = NULL;

            // get game id
            get_json_data(start, end, "\"gameId\"", val);
            if (val == NULL){
                start = response->data + i + 1;
                i++;
                continue;
            }
            int j = 1;
            int id_length = 1;
            while (val[j] != '\"'){
                val[j - 1] = val[j];
                j++;
                id_length++;
            }
            val[j] = '\0';
            args.id = val;
            args.id_length = id_length;

            // start a thread for this game
            pthread_t thread_id;

            if (pthread_create(&thread_id, NULL, play_game, &args) != 0){
                fprintf(stderr, "failed to create thread");
                exit(EXIT_FAILURE);
            }

            response->games[response->games_accepted] = thread_id;
            response->games_accepted++;

            if (response->games_accepted >= MAX_GAMES_PLAYED){
                return 0;
            }
        }
        i++;
    }

    // move data
    if (start != response->data){
        const int remaining_length = response->length - (start - response->data);
        memmove(response->data, start, sizeof(char) * remaining_length);
        response->length = remaining_length;
    }

    return content_len;
}

int main(){
    _event_stream_response response;
    response.data = malloc(1);
    if (response.data == NULL){
        fprintf(stderr, "malloc() failed");
        return 1;
    }
    response.size = 0;
    response.length = 0;
    response.games_accepted = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    init_chess_engine();
    CURL *curl = curl_easy_init();

    if (curl != NULL){
        struct curl_slist *headers = curl_slist_append(NULL, "Authorization: Bearer YOUR_API_TOKEN");

        curl_easy_setopt(curl, CURLOPT_URL, "https://lichess.org/api/stream/event");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, event_stream_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        CURLcode result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(result));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(response.data);
    for (int i = 0; i < MAX_GAMES_PLAYED; i++){
        pthread_join(response.games[i], NULL);
    }
    curl_global_cleanup();
    return 0;
}