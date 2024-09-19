#ifndef GAME_H
#define GAME_H

#define BOARD_SIZE 9

typedef struct {
  int isMine;
  int isRevealed;
  int isMarked;
  int totalNeighborMines;
} Cell;

typedef struct {
  int totalMines;
  int totalMarks;
  int usedMarks;
  int width;
  int height;
  Cell **board;
} Game;

void initBoard(Game *game);

void initGame(Game *game, int difficulty);

void printBoard(Game *game);

void setMarkedCell(Game *game, int x, int y);

void proccessMovement(Game *game, int x, int y);

char * serializeBoard(Game *game);

int isMovementAMine(Game *game, int x, int y);

int isMovementInsideBoard(Game *game, int x, int y);

int hasPlayerWon(Game *game);

int isWinMessage(char * msg);

int isLostMessage(char *msg);

void placeMinesOnBoard(Game *game);

void countNeighborMinesForEachCell(Game *game);

#endif
