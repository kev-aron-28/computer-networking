#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void initGame(Game *game, int difficulty)
{
  switch (difficulty)
  {
  case 1:
    game->width = 9;
    game->height = 9;
    game->totalMines = 10;
    break;
  case 2:
    game->width = 16;
    game->height = 16;
    game->totalMines = 40;
    break;
  default:
    game->width = 16;
    game->height = 30;
    game->totalMines = 99;
    break;
  }

  game->totalMarks = game->totalMines;
  game->usedMarks = 0;

  game->board = (Cell **)malloc(game->height * sizeof(Cell *));
  for (int i = 0; i < game->height; i++)
  {
    game->board[i] = (Cell *)malloc(game->width * sizeof(Cell));
  }

  initBoard(game);
}

void initBoard(Game *game)
{
  for (int i = 0; i < game->height; i++)
  {
    for (int j = 0; j < game->width; j++)
    {
      game->board[i][j].isMine = 0;
      game->board[i][j].isRevealed = 0;
      game->board[i][j].totalNeighborMines = 0;
      game->board[i][j].isMarked = 0;
    }
  }

  placeMinesOnBoard(game);

  countNeighborMinesForEachCell(game);
}

void countNeighborMinesForEachCell(Game *game) {
  for (int i = 0; i < game->height; i++)
    {
      for (int j = 0; j < game->width; j++)
      {
        if (game->board[i][j].isMine)
        {
          for (int x = -1; x <= 1; x++)
          {
            for (int y = -1; y <= 1; y++)
            {
              int row = i + x;

              int col = j + y;
              
              if (row >= 0 && row < game->height && col >= 0 && col < game->width && !(x == 0 && y == 0))
              {
                game->board[row][col].totalNeighborMines += 1;
              }
            }
          }
        }
      }
    }
}

void placeMinesOnBoard(Game *game) {
  int minesPlaced = 0;

  srand(time(NULL));

  while (minesPlaced < game->totalMines) {
    int i = rand() % game->height;

    int j = rand() % game->width;

    if (!game->board[i][j].isMine)
    {
      game->board[i][j].isMine = 1;

      minesPlaced++;
    }
  }
}

void printBoard(Game *game)
{
  printf("  ");

  for (int col = 0; col < game->width; col++)
  {
    printf("%d\t", col);
  }

  printf("\n");

  for (int i = 0; i < game->height; i++)
  {
    printf("%d ", i);
    for (int j = 0; j < game->width; j++)
    {
      if (game->board[i][j].isRevealed)
      {
        printf("%d\t", game->board[i][j].totalNeighborMines);
      }
      else if (game->board[i][j].isMarked)
      {
        printf("<|>\t");
      }
      else if(game->board[i][j].isMine) {
        printf("M\t");
      }
      else
      {
        printf(".\t");
      }
    }
    printf("\n");
  }
}

char *serializeBoard(Game *game)
{
  static char boardState[4096] = {0};

  memset(boardState, 0, sizeof(boardState));

  strncat(boardState, "  ", 3);

  for (int col = 0; col < game->width; col++)
  {
    char colIndex[4];

    sprintf(colIndex, "%d\t", col);

    strncat(boardState, colIndex, sizeof(colIndex));
  }
  strncat(boardState, "\n", 2);

  for (int i = 0; i < game->height; i++)
  {
    char rowIndex[4];

    sprintf(rowIndex, "%d ", i);

    strncat(boardState, rowIndex, sizeof(rowIndex));

    for (int j = 0; j < game->width; j++)
    {
      Cell currentCell = game->board[i][j];

      char cell[4];

      if (currentCell.isRevealed)
      {
        sprintf(cell, "%d", currentCell.totalNeighborMines);

        strncat(boardState, cell, sizeof(cell));

        strncat(boardState, "\t", 2);
      }
      else if (currentCell.isMarked)
      {
        strncat(boardState, "<|>\t", 5);
      }
      else
      {
        strncat(boardState, ".\t", 3);
      }
    }
    strncat(boardState, "\n", 2);
  }

  return boardState;
}

void proccessMovement(Game *game, int x, int y)
{
  if (x < 0 || x >= game->height || y < 0 || y >= game->width || game->board[x][y].isRevealed)
  {
    return;
  }

  game->board[x][y].isRevealed = 1;

  if (game->board[x][y].totalNeighborMines == 0)
  {
    for (int i = -1; i <= 1; i++)
    {
      for (int j = -1; j <= 1; j++)
      {
        int newX = x + i;

        int newY = y + j;
        
        if (newX >= 0 && newX < game->height && newY >= 0 && newY < game->width)
        {
          proccessMovement(game, newX, newY);
        }
      }
    }
  }
}

void setMarkedCell(Game *game, int x, int y)
{
  if (!isMovementInsideBoard(game, x, y))
    return;

  Cell *currentCell = &game->board[x][y];

  if (currentCell->isRevealed)
    return;

  if (currentCell->isMarked)
  {
    currentCell->isMarked = 0;
    game->usedMarks--;
  }
  else if (game->usedMarks < game->totalMarks)
  {
    currentCell->isMarked = 1;
    game->usedMarks++;
  }
}

int isMovementAMine(Game *game, int x, int y)
{
  if (!isMovementInsideBoard(game, x, y))
  {
    return -1;
  }

  return game->board[x][y].isMine;
}

int isMovementInsideBoard(Game *game, int x, int y)
{
  return x >= 0 && x < game->height && y >= 0 && y < game->width;
}

int hasPlayerWon(Game *game)
{
  for (int i = 0; i < game->height; i++)
  {
    for (int j = 0; j < game->width; j++)
    {
      if (!game->board[i][j].isMine && !game->board[i][j].isRevealed)
      {
        return 0;
      }
    }
  }
  return 1;
}

int hasPlayerMarkedAllMines(Game *game) {
  int minesMarkedSoFar = 0;

  for (int i = 0; i < game->height; i++)
  {
    for (int j = 0; j < game->width; j++)
    {
      if (game->board[i][j].isMine && game->board[i][j].isMarked)
      {
        minesMarkedSoFar++;
      }
    }
  }

  if(minesMarkedSoFar == game->totalMines) return 1;

  return 0;
}

int isWinMessage(char *msg) {
  int isEqual = strcmp(msg, "You won the game");

  return isEqual == 0;
}

int isLostMessage(char *msg) {
  int isEqual = strcmp(msg, "GAME OVER");

  return isEqual == 0;
}
