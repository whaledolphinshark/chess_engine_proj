# Overview
A chess engine I made as a personal project. It plays online from time to time on lichess under the name somekindofbot. It was made with C and Python, and currently plays at around a 1750 elo on lichess. I don't expect anyone to use or care about this but here it is if you want it. It only runs on Mac/Linux. It is not guaranteed to run perfectly so please be mindful. I may update this repo from time to time.

# Features
A list of features the engine implements:
- Search: Principal variation search with iterative deepening, transposition table and quiescence search
- Evaluation: Handcrafted, accounting for material and mobility, with piece square tables
- Move ordering: MVV-LVA, history heuristic
- Move generation: Bitboards with magic bitboards for move generation

# Installing and Running
How to install and run:
1. Clone the repository and build the engine:
```bash
git clone git@github.com:whaledolphinshark/proj.git
cd proj/chess_bot
make
```

2. Create a virtual environment and install dependencies:
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

3. Set up the bot on lichess
  The engine requires an api token and its name in order to play on lichess
   1. Sign up for free on lichess at [https://lichess.org/signup](https://lichess.org/signup)
   2. Generate an API access token with challenge:read, challenge:write, board:play and bot:play permissions
   3. copy the api token and run the following command, replacing [yourTokenHere] with the api token:
```bash
curl -d '' https://lichess.org/api/bot/account/upgrade -H "Authorization: Bearer [yourTokenHere]"
```  

4. Configure .env
```bash
cp .env.example .env
```
Then edit .env with your own values
| Variable | Description |
|---|---|
| `api_token` | API token from Lichess (see step 3) |
| `name` | Username on Lichess |
| `min_depth` | The minimum depth the engine will always search to |
| `max_seconds` | Maximum amount of time the engine will search, min_depth overrides this, so it will always search to the minimum depth even if time is up |
| `max_games` | How many games the engine will play before stopping |
| `min_elo` | The minimum elo of the opponent the bot is willing to play |

5. Running the engine:
Make sure you are in the virtual environment first (See step 2, use the 2nd command)
```
python3 src/main.py
```

# Testing
You can make the tests by running:
```bash
make tests
```
The tests will be in the bin directory and can be run

# Acknowledgements
Thanks to Sebastian Lague's Chess-Coding-Adventure (https://github.com/SebLague/Chess-Coding-Adventure) for the magic bitboards and for inspiring this project in the first place
