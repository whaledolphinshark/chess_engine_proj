#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/board.h"
#include "chess_engine/search.h"

typedef struct{
    char *data;
    size_t size;
    int length;
    int move_first;
    char *game_id;
    int time;
}_response;

void get_json_data(char *start, char *end, char *name, char *value){
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

// this is a mess
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    const size_t total_length = size * nmemb;
    _response *response = (_response *)userp;

    const size_t new_length = response->length == 0 ? total_length + 1 : response->length + total_length;
    if (new_length > response->size){
        response->size = new_length;
        response->data = realloc(response->data, new_length);
        if (response->data == NULL){
            fprintf(stderr, "realloc() failed");
            exit(EXIT_FAILURE);
        }
    }

    memcpy(&(response->data[response->length]), contents, total_length);
    int i = response->length;
    response->length = new_length;
    response->data[response->length] = '\0';
    char *start = response->data;

    // check for responses
    while (i < response->length){
        if (response->data[i] == '\n'){
            // find type
            char *ptr_1 = strstr(start, "\"type\"");
            char *ptr_2 = strstr(start, "\"gameStart\"");
            const int type_index = ptr_1 - response->data;
            const int game_start_index = ptr_2 - response->data;
            if (ptr_1 != NULL && ptr_2 != NULL && type_index < i && game_start_index - type_index == 8){
                // new game has started
                // check if my turn first
                char *ptr_3 = strstr(start, "\"isMyTurn\"");
                const int is_my_turn_index = ptr_3 - response->data;
                if (ptr_3 != NULL && is_my_turn_index < i){
                    response->move_first = response->data[is_my_turn_index + 12] == 't' ? 1 : 0;
                }

                // get game id
                char *ptr_4 = strstr(start, "\"gameId\"");
                const int id_index = ptr_4 - response->data;
                if (ptr_4 != NULL && id_index < i){
                    const int id_value_index = id_index + 11;
                    int id_length = 0;
                    int j = id_value_index;
                    while (response->data[j] != '\"'){
                        id_length++;
                        j++;
                    }

                    response->game_id = malloc(sizeof(char) * (id_length + 1));
                    if (response->game_id == NULL){
                        fprintf(stderr, "malloc() failed");
                        exit(EXIT_FAILURE);
                    }
                    for (int k = 0; k < id_length; k++){
                        response->game_id[k] = response->data[id_value_index + k];
                    }
                }

                return CURL_WRITEFUNC_ERROR;
            }

            start = response->data + i + 1;
        }
        i++;
    }

    // resize data
    if (start != response->data){
        const int remaining_length = response->length - (start - response->data);
        memmove(response->data, start, sizeof(char) * remaining_length);
        response->length = remaining_length;
    }

    return total_length;
}

void play_game(int my_turn, char *id){
    _board *board = cb_create_board();
    _search_context context;
    _search_stats stats;
    se_init_search_context(&context);

    if (my_turn == 1){
        // se_search(board, 6, , &context, &stats);
    }
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
    response.length = 0;
    response.move_first = 0;
    response.game_id = NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    if (curl != NULL){
        struct curl_slist *headers = curl_slist_append(NULL, "Authorization: Bearer YOUR_API_TOKEN");

        curl_easy_setopt(curl, CURLOPT_URL, /*"https://lichess.org/api/stream/event"*/ "https://httpbin.org/stream/10");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        CURLcode result = curl_easy_perform(curl);

        // this feels kinda hacky
        if (result == CURLE_WRITE_ERROR){
            // start game i guess
            play_game(response.move_first, response.game_id);
        }
        else if (result != CURLE_OK) {
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