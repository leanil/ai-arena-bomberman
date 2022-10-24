# Bomberman - AI Arena

## Build

### Debug

- If the project structure (CMakeLists.txt) changed: `cmake -DCMAKE_BUILD_TYPE=Debug -BDebug .`
- If only some source files changed: `cmake --build Debug -j 8`

### Release

```
cmake -DCMAKE_BUILD_TYPE=Release -BRelease .
cmake --build Release -j 8
```

## Run with server

1. `rm -rf player*.in player*.out`
2. `mkfifo player1.in player1.out [player2.in player2.out ...]`
3. `[Debug/Release]/server/server <player_count> <level_file>`
4. `[Debug/Release]/bot/bot console <player1.in >player1.out`
5. repeat step 4. for the other players

## Formatting

We use clang-format. Plugin available for most IDEs. Feel free to edit the config: `.clang-format`.

Set up [pre-commit](https://pre-commit.com/#installation). It automatically formats the code before commit.
TL;DR:
```
pip install pre-commit
pre-commit install
```
