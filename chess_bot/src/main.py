import requests
import json
from threading import Thread
from typing import NewType
from ctypes import CDLL, Structure, POINTER, c_int, c_char, byref
from pathlib import Path
import sys
import random

lib_path = Path(__file__).resolve().parent.parent / "bin" / "main.so"
lib = CDLL(str(lib_path))

class board(Structure):
    pass

class move(Structure):
    pass

class search_context(Structure):
    pass

class search_stats(Structure):
    pass

board_ptr = NewType("board_ptr", POINTER(board))
context_ptr = NewType("context_ptr", POINTER(search_context))

lib.init_chess_engine.restype = None
lib.init_chess_engine.argtypes = []
lib.cb_create_board.restype = POINTER(board)
lib.cb_create_board.argtypes = []
lib.cb_make_move.restype = None
lib.cb_make_move.argtypes = [POINTER(board), move]
lib.cb_destroy_board.restype = None
lib.cb_destroy_board.argtypes = [POINTER(board)]
lib.mv_move_to_uci.restype = None
lib.mv_move_to_uci.argtypes = [move, POINTER(c_char)]
lib.mv_uci_to_move.restype = move
lib.mv_uci_to_move.argtypes = [POINTER(c_char), POINTER(board)]
lib.se_search.restype = move
lib.se_search.argtypes = [POINTER(board), c_int, c_int, POINTER(search_context), POINTER(search_stats)]
lib.se_init_search_context.restype = POINTER(search_context)
lib.se_init_search_context.argtypes = []
lib.se_destroy_search_context.restype = None
lib.se_destroy_search_context.argtypes = [POINTER(search_context)]

api_token: str = ""
my_name: str = ""

def post_move(id: str, move: move):
    move_uci = (c_char * 6)()
    lib.mv_move_to_uci(move, move_uci)

    url = f"https://lichess.org/api/bot/game/{id}/{move_uci.value.decode()}"
    headers = {"Authorization" : f"Bearer {api_token}"}
    response = requests.post(url=url, headers=headers)
    response.raise_for_status()

def play_game(id: str, my_color: bool):
    # white = true, black = false
    game_board: board_ptr = lib.cb_create_board()
    context: search_context = lib.se_init_search_context()
    stats: search_stats = search_stats()

    url = f"https://lichess.org/api/board/game/stream/{id}"
    headers = {"Authorization" : f"Bearer {api_token}"}
    with requests.get(url=url, headers=headers, stream=True) as r:
        r.raise_for_status()

        for line in r.iter_lines():
            if not line:
                continue

            event = json.loads(line)
            if event["type"] == "gameFull":
                event = event["state"]

            # check status
            if event["status"] != "started":
                break

            moves: list[str] = event["moves"].split(" ")
            print(moves)
            last_move_color: bool = len(moves) % 2 == 1
            if last_move_color != my_color:
                # make last move
                if len(moves) > 0:
                    last_move: move = lib.mv_uci_to_move(moves[-1].encode('utf-8'), game_board)
                    lib.cb_make_move(game_board, last_move)

                # get time
                seconds: int = event["wtime" if my_color else "btime"] // 1000

                # make move
                next_move: move = lib.se_search(game_board, 6 if seconds > 6 else seconds, byref(context), byref(stats))
                lib.cb_make_move(game_board, next_move)
                post_move(id, next_move)

    lib.destroy_board(game_board)
    lib.destroy_search_context(byref(context))

if __name__ == "__main__":
    env_path = Path(__file__).resolve().parent.parent / "variables.env"
    with open(env_path, 'r') as file:
        lines = file.read().splitlines()
        for line in lines:
            key_value = line.split('=')
            if key_value[0] == "api_token":
                api_token = key_value[1]
            elif key_value[0] == "name":
                my_name = key_value[1]

    if not api_token or not my_name:
        sys.exit("api token or name not found")

    lib.init_chess_engine()

    url: str = "https://lichess.org/api/bot/online"
    bots: list[str] = []
    with requests.get(url=url) as r:
        r.raise_for_status()

        for line in r.iter_lines():
            bots.append(json.loads(line)["username"])

    max_games: int = 5
    games: list[Thread] = []
    url = "https://lichess.org/api/stream/event"
    headers = {"Authorization" : f"Bearer {api_token}"}
    # challenge ai to test
    data = {"level": 1, "clock.limit": 300, "clock.increment": 3, "color": "random", "variant": "standard"}
    requests.post(url=f"https://lichess.org/api/challenge/ai", headers=headers, data=data).raise_for_status()

    with requests.get(url=url, headers=headers, stream=True) as r:
        r.raise_for_status()

        for line in r.iter_lines():
            if not line:
                continue

            event = json.loads(line)
            if event["type"] == "gameStart":
                # start thread to handle game
                game_thread: Thread = Thread(target=play_game, args=(event["game"]["gameId"], 
                                            event["game"]["color"] == "white"))
                game_thread.start()
                games.append(game_thread)
            elif (event["type"] == "challenge" and 
                  event["challenge"]["status"] == "created" and
                  event["challenge"]["destUser"]["name"] == my_name and
                  event["challenge"]["variant"]["key"] == "standard" and
                  event["challenge"]["speed"] == "blitz"):
                # accept challenge
                requests.post(url=f"https://lichess.org/api/challenge/{event["challenge"]["id"]}/accept", headers=headers).raise_for_status()


            if len(games) >= max_games:
                break
            
            # # make challenges if we can
            # with requests.get(url="https://lichess.org/api/challenge", headers=headers) as s:
            #     challenges = s.json()
            #     if len(challenges["in"]) == 0 and len(challenges["out"]) == 0:
            #         # make challenge
            #         url = f"https://lichess.org/api/challenge/{random.choice(bots)}?clock.limit=300&clock.increment=3&color=random"
            #         requests.post(url=url, headers=headers).raise_for_status()

    for game in games:
        game.join()       