import requests
import json
import threading
from pathlib import Path
import sys
import random
import subprocess
import queue

api_token: str | None = None
my_name: str | None = None
min_depth: int | None = None
max_seconds: int | None = None
max_games: int | None = None

def resign_game(id: str):
    url = f"https://lichess.org/api/bot/game/{id}/resign"
    headers = {"Authorization" : f"Bearer {api_token}"}
    requests.post(url=url, headers=headers).raise_for_status()

def abort_game(id: str):
    url = f"https://lichess.org/api/bot/game/{id}/abort"
    headers = {"Authorization" : f"Bearer {api_token}"}
    requests.post(url=url, headers=headers).raise_for_status()

def post_move(id: str, move_uci: str):
    url = f"https://lichess.org/api/bot/game/{id}/move/{move_uci}"
    headers = {"Authorization" : f"Bearer {api_token}"}
    requests.post(url=url, headers=headers).raise_for_status()

def play_game(id: str, my_color: bool):
    bot_path = Path(__file__).resolve().parent.parent / "bin" / "bot"
    chess_game = subprocess.Popen(
        [bot_path, str(min_depth), str(max_seconds)], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        text=True)
    assert chess_game.stdin is not None
    assert chess_game.stdout is not None

    event_queue = queue.Queue()
    stream_stop = threading.Event()

    url = f"https://lichess.org/api/bot/game/stream/{id}"
    headers = {"Authorization" : f"Bearer {api_token}"}
    def stream_reader():
        try:
            with requests.get(url=url, headers=headers, stream=True) as r:
                r.raise_for_status()
                for line in r.iter_lines():
                    if not line:
                        continue
                    event = json.loads(line)
                    if event["type"] == "gameFull":
                        event = event["state"]
                    event_queue.put(("stream", event))
        except Exception as e:
            event_queue.put(("stream error", e))
            stream_stop.set()

    def engine():
        assert chess_game.stdout is not None
        try:
            while not stream_stop.is_set():
                line = chess_game.stdout.readline()
                if not line:  # engine crashed
                    event_queue.put(("engine_error", RuntimeError("engine crashed")))
                    return
                event_queue.put(("engine", line))
        except Exception as e:
            event_queue.put(("engine_error", e))

    stream_thread = threading.Thread(target=stream_reader, daemon=True)
    engine_thread = threading.Thread(target=engine, daemon=True)
    stream_thread.start()
    engine_thread.start()

    can_abort: bool = True
    try:
        while True:
            source, message = event_queue.get()

            if source == "stream":
                event: dict = message
                if event["status"] != "started":
                    stream_stop.set()
                    break
                
                moves_str: str = event["moves"]
                moves: list[str] = moves_str.split(" ") if moves_str else []
                num_moves: int = len(moves)
                if num_moves > 2:
                    can_abort = False
                last_move_color: bool = num_moves % 2 == 1
                if last_move_color != my_color:
                    seconds: str = str(event["wtime" if my_color else "btime"] // 1000)
                    move: str = moves[-1] if num_moves > 0 else "null"

                    chess_game.stdin.write(f"{move},{seconds}\n")
                    chess_game.stdin.flush()
            elif source == "engine":
                response: str = message
                print(f"response: {response}")
                post_move(id, response)
            else:
                error: Exception = message
                raise error
    except Exception as e:
        if can_abort:
            abort_game(id)
        else:
            resign_game(id)
        print(e)

    chess_game.stdin.close()
    chess_game.wait()

    print(f"finished game: {id}")

def read_env():
    global api_token, my_name, min_depth, max_seconds, max_games
    env_path = Path(__file__).resolve().parent.parent / ".env"
    with open(env_path, 'r') as file:
        lines = file.read().splitlines()
        for line in lines:
            key_value = line.split('=')
            if key_value[0] == "api_token":
                api_token = key_value[1]
            elif key_value[0] == "name":
                my_name = key_value[1]
            elif key_value[0] == "min_depth":
                min_depth = int(key_value[1])
            elif key_value[0] == "max_seconds":
                max_seconds = int(key_value[1])
            elif key_value[0] == "max_games":
                max_games = int(key_value[1])

    if not api_token or not my_name or not min_depth or not max_seconds or not max_games:
        sys.exit("some environment variables were not found")

if __name__ == "__main__":
    read_env()
    assert max_games

    url: str = "https://lichess.org/api/bot/online"
    bots: list[str] = []
    with requests.get(url=url) as r:
        r.raise_for_status()

        for line in r.iter_lines():
            bots.append(json.loads(line)["username"])

    games: list[threading.Thread] = []
    url = "https://lichess.org/api/stream/event"
    headers = {"Authorization" : f"Bearer {api_token}"}
    # challenge ai to test
    data = {"level": 7, "clock.limit": 300, "clock.increment": 3, "color": "random", "variant": "standard"}
    requests.post(url=f"https://lichess.org/api/challenge/ai", headers=headers, data=data).raise_for_status()

    with requests.get(url=url, headers=headers, stream=True) as r:
        r.raise_for_status()

        for line in r.iter_lines():
            if not line:
                continue

            event = json.loads(line)
            if event["type"] == "gameStart":
                # start thread to handle game
                game_thread: threading.Thread = threading.Thread(target=play_game, args=(event["game"]["gameId"], 
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