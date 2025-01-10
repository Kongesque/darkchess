#ifndef MYAI_H
#define MYAI_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <time.h>
#include <vector>
#include <limits>

#include "libchess.h"

struct Node;

class MyAI {
public:
    MyAI();

    void InitBoard();
    void InitBoard(const char* data[]);
    void Move(int from, int to);
    void Flip(int sq, FIN f);
    void SetColor(COLOR c);
    void SetTime(COLOR c, int t);
    MOVE GenerateMove() const;

    std::vector<MOVE> getAllMoves(const FIN board[BOARD_SIZE], COLOR player) const;
    int evaluateBoard(const FIN board[BOARD_SIZE], COLOR player) const;
    int alphaBeta(Node node, int depth, int alpha, int beta, COLOR player, int coverPieceCount[14], int allCoverCount) const;
    Node createNode(const FIN board[BOARD_SIZE], int alpha, int beta, int depth) const;
    void applyMove(FIN board[BOARD_SIZE], int from, int to) const;
    void applyFlip(FIN board[BOARD_SIZE], int sq, FIN f, int coverPieceCount[14], int allCoverCount) const;

    std::string GetProtocolVersion() const;
    std::string GetAIName() const;
    std::string GetAIVersion() const;
    void Print() const;

private:
    int color;
    int time[2];
    FIN board[BOARD_SIZE];
    int coverPieceCount[14];
    int allCoverCount;

    int maxDepth = 5; // Max search depth
};

struct Node {
    FIN board[BOARD_SIZE];
    int alpha;
    int beta;
    int depth;
};

#endif