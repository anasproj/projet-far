# Chess Engine

Trying to play around with creating a chess engine and a small app to play
against it.

## Network tournament module

A new tournament manager has been added in `src/tournoi.c` and `src/tournoi.h`.
It supports elimination tournaments for 2, 4 or 8 players. The matchmaking and
salon routing are handled by another module.

To build the server + tournament support, run:

    make server

The existing `make` target still builds the chess application and AI libraries.

### Credits

- Chess Piece Icons <https://commons.wikimedia.org/wiki/Category:PNG_chess_pieces/Standard_transparent>
