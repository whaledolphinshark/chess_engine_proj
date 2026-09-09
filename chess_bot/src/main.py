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
min_elo: int | None = None

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

def decline_draw(id: str):
    url = f"https://lichess.org/api/board/game/{id}/draw/no"
    headers = {"Authorization" : f"Bearer {api_token}"}
    requests.post(url=url, headers=headers).raise_for_status()

def decline_takeback(id: str):
    url = f"https://lichess.org/api/board/game/{id}/takeback/no"
    headers = {"Authorization" : f"Bearer {api_token}"}
    requests.post(url=url, headers=headers).raise_for_status()

def play_game(id: str, my_color: bool, event_queue: queue.Queue):
    bot_path = Path(__file__).resolve().parent.parent / "bin" / "bot"
    chess_game = subprocess.Popen(
        [bot_path, str(min_depth), str(max_seconds)], 
        stdin=subprocess.PIPE, 
        stdout=subprocess.PIPE, 
        text=True)
    assert chess_game.stdin is not None
    assert chess_game.stdout is not None

    game_stop: threading.Event = threading.Event()
    stream_started: threading.Event = threading.Event()
    def stream_reader():
        # white = True, black = False
        color: bool = True
        try:
            url: str = f"https://lichess.org/api/bot/game/stream/{id}"
            headers: dict = {"Authorization" : f"Bearer {api_token}"}
            with requests.get(url=url, headers=headers, stream=True) as r:
                r.raise_for_status()
                stream_started.set()
                for line in r.iter_lines():
                    if not line:
                        continue

                    event = json.loads(line)
                    event_type: str = event["type"]
                    if event_type == "gameFull":
                        # see which color i am
                        if event["black"]["name"] == my_name:
                            color = False
                        event = event["state"]
                    elif event_type != "gameState":
                        continue

                    # check if takeback or draw offer
                    draw_offer: str = "bdraw" if color else "wdraw"
                    takeback_offer: str = "btakeback" if color else "wtakeback"
                    if draw_offer in event:
                        # decline
                        decline_draw(id)
                    if takeback_offer in event:
                        # decline
                        decline_takeback(id)

                    event_queue.put(("stream", event))
        except Exception as e:
            event_queue.put(("stream error", e))
            game_stop.set()

    def engine():
        assert chess_game.stdout is not None
        try:
            while not game_stop.is_set():
                line = chess_game.stdout.readline()
                if not line:  # engine crashed
                    event_queue.put(("engine_error", RuntimeError("engine crashed")))
                    return
                event_queue.put(("engine", line))
        except Exception as e:
            event_queue.put(("engine_error", e))

    threading.Thread(target=stream_reader).start()
    threading.Thread(target=engine).start()

    can_abort: bool = True
    source: str = ""
    message = None
    try:
        while True:
            item = event_queue.get()
            if item is None:
                raise RuntimeError("Stop signal")
            
            source, message = item
            if source == "stream":
                event: dict = message
                if event["status"] != "started":
                    game_stop.set()
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
                post_move(id, response)
            else:
                error: Exception = message
                raise error
    except Exception as e:
        if stream_started.is_set():
            if can_abort:
                abort_game(id)
            else:
                resign_game(id)
        print(f"{source}: {e}")

    chess_game.stdin.close()
    chess_game.wait()

    print(f"finished game: {id}")

def read_env():
    global api_token, my_name, min_depth, max_seconds, max_games, min_elo
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
            elif key_value[0] == "min_elo":
                min_elo = int(key_value[1])

    if not api_token or not my_name or not min_depth or not max_seconds or not max_games or not min_elo:
        sys.exit("some environment variables were not found\nrequired: api_token, my_name, min_depth, max_seconds, max_games, min_elo")

if __name__ == "__main__":
    read_env()
    assert max_games

    # get bots
    bots: set[str] = set()
    with requests.get(url="https://lichess.org/api/bot/online") as r:
        r.raise_for_status()

        for line in r.iter_lines():
            bot: dict = json.loads(line)
            if bot["perfs"]["blitz"]["rating"] >= min_elo:
                bots.add(bot["username"])

    num_games: int = 0
    playing_game: bool = False
    url: str = "https://lichess.org/api/stream/event"
    headers: dict = {"Authorization" : f"Bearer {api_token}"}
    event_queue: queue.Queue = None
    game: threading.Thread = None
    challenge_id: str = ""
    try:
        with requests.get(url=url, headers=headers, stream=True) as r:
            r.raise_for_status()

            for line in r.iter_lines():    
                if not line:
                    if not playing_game:    
                        if len(bots) == 0:
                            raise RuntimeError("No more bots to play")
                        
                        if not challenge_id:
                            # make challenge
                            data: dict = {"clock.limit": 300, "clock.increment": 3, "color": "random", "variant": "standard", "rated": "true"}
                            opponent: str = random.choice(tuple(bots))
                            with requests.post(url=f"https://lichess.org/api/challenge/{opponent}", headers=headers, data=data) as s:
                                response: dict = s.json()
                                challenge_id = response["id"] if "error" not in response else ""
                            bots.remove(opponent)
                        else:
                            with requests.get(url=f"https://lichess.org/api/challenge/{challenge_id}/show", headers=headers) as s:
                                s.raise_for_status()
                                challenge_status: dict = s.json()
                                status: str = challenge_status["status"]
                                if status == "offline":
                                    # cancel challenge
                                    requests.post(url=f"https://lichess.org/api/challenge/{challenge_id}/cancel", headers=headers).raise_for_status()
                                    print("canceling challenge") 
                                # check if previous challenge is still up
                                if (status == "declined" or status == "canceled" or status == "accepted") and not playing_game:
                                    # if not up and not playing game make new challenge
                                    print(f"previous challenge: {status}") 
                                    data: dict = {"clock.limit": 300, "clock.increment": 3, "color": "random", "variant": "standard", "rated": "true"}
                                    opponent: str = random.choice(tuple(bots))
                                    print("sending challenge")
                                    with requests.post(url=f"https://lichess.org/api/challenge/{opponent}", headers=headers, data=data) as t:
                                        response: dict = t.json()
                                        challenge_id = response["id"] if "error" not in response else ""
                                    bots.remove(opponent)
                    continue

                event: dict = json.loads(line)
                event_type: str = event["type"]
                if event_type == "gameStart":
                    print("game started") 
                    # reset the queue
                    event_queue = queue.Queue()

                    # start thread to handle game
                    args: tuple[str, bool, queue.Queue] = (event["game"]["gameId"], event["game"]["color"] == "white", event_queue)
                    game = threading.Thread(target=play_game, args=args)
                    game.start()
                    playing_game = True
                elif event_type == "challenge" and event["challenge"]["challenger"]["name"] != my_name:
                    incoming_challenge_id: str = event["challenge"]["id"]
                    print("received incoming challenge") 
                    if (not playing_game and
                        event["challenge"]["status"] == "created" and
                        event["challenge"]["destUser"]["name"] == my_name and
                        event["challenge"]["variant"]["key"] == "standard" and
                        event["challenge"]["speed"] == "blitz"):
                        
                        # see if any of my recently issued challenges are still up
                        with requests.get(url=f"https://lichess.org/api/challenge/{challenge_id}/show", headers=headers) as s:
                            s.raise_for_status()
                            challenge_status: dict = s.json()
                            status = challenge_status["status"]
                            if status == "created" or status == "offline":
                                # cancel my own challenge 
                                print("canceling challenge") 
                                requests.post(url=f"https://lichess.org/api/challenge/{challenge_id}/cancel", headers=headers).raise_for_status()
                        
                        print("accepting challenge")
                        # accept challange
                        requests.post(url=f"https://lichess.org/api/challenge/{incoming_challenge_id}/accept", headers=headers).raise_for_status()
                    else:
                        # decline challenge
                        print("declining challenge")
                        data: dict = {"reason": "generic"}
                        requests.post(url=f"https://lichess.org/api/challenge/{incoming_challenge_id}/decline", headers=headers, data=data).raise_for_status()
                elif event_type == "gameFinish":
                    game.join()
                    print("game finished")
                    playing_game = False
                    num_games += 1

                if num_games >= max_games:
                    break
    except Exception as e:
        if playing_game:
            event_queue.put(None)
        print(e)

    if game is not None:
        game.join()
    print("exiting program")
