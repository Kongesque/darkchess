#include <string.h>
#include <algorithm>
#include "libchess.h"
#include "MyAI.h"
using namespace std;

MyAI::MyAI() {
	InitBoard();
}

// Initial board
void MyAI::InitBoard() {
	const int cover[14] = {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5};
	color = UNKNOWN;
	time[RED] = 0;
	time[BLK] = 0;
	memcpy(coverPieceCount, cover, sizeof(int) * 14);
	allCoverCount = BOARD_SIZE;
	for (int i = 0, sq = 0; i < ROW_COUNT; i++) {
		for (int j = 0; j < COL_COUNT; j++, sq++) {
			board[sq] = FIN_COVER;
		}
	}
}

// Initial board by position
void MyAI::InitBoard(const char* data[]) {
	color = UNKNOWN;
	time[RED] = 0;
	time[BLK] = 0;
	allCoverCount = BOARD_SIZE;
	for (int r = ROW_COUNT - 1, i = 0; r >= 0; r--) {
		for (int c = 0; c < COL_COUNT; c++, i++) {
			board[r + c * 4] = char2fin(data[i][0]);
		}
	}
	for (int i = 0; i < FIN_COVER; i++) {
		coverPieceCount[i] = data[i + 32][0] - '0';
		allCoverCount += coverPieceCount[i];
	}
}

// Move a piece
void MyAI::Move(int from, int to) {
	color = (color == RED) ? BLK : RED;
	board[to] = board[from];
	board[from] = FIN_EMPTY;
}

// Flip a piece
void MyAI::Flip(int sq, FIN f) {
	color = (color == RED || color == BLK) ? (COLOR) !color : (COLOR) !color_of(f);
	board[sq] = f;
	coverPieceCount[f]--;
	allCoverCount--;
}

void MyAI::SetColor(COLOR c) {
	color = c;
}

void MyAI::SetTime(COLOR c, int t) {
	time[c] = t;
}

// Generate the best move using alpha-beta pruning
MOVE MyAI::GenerateMove() const {
    Node rootNode = createNode(board, numeric_limits<int>::min(), numeric_limits<int>::max(), 0);
    int bestScore = numeric_limits<int>::min();
    MOVE bestMove = MOVE_NULL;
    std::vector<MOVE> legalMoves = getAllMoves(board, (COLOR)color);
    int tempCoverPieceCount[14];
	int tempAllCoverCount;
    memcpy(tempCoverPieceCount, coverPieceCount, sizeof(int) * 14);
	tempAllCoverCount = allCoverCount;
    for (MOVE move : legalMoves) {
        FIN tempBoard[BOARD_SIZE];
        memcpy(tempBoard, board, sizeof(FIN) * BOARD_SIZE);
        int from = from_square(move);
        int to = to_square(move);
        if (from == to) { // Flip move
            FIN flippedPiece = (FIN)(rand() % FIN_COVER);
            if (tempCoverPieceCount[flippedPiece] == 0) { 
                for (int f = 0; f < FIN_COVER; f++) {
                    if (tempCoverPieceCount[f] > 0) {
                        flippedPiece = (FIN)f;
                        break;
                    }
                }
                if(tempCoverPieceCount[flippedPiece] == 0) {
                    continue;
                }
            }
           applyFlip(tempBoard, to, flippedPiece, tempCoverPieceCount, tempAllCoverCount);
             int score = alphaBeta(createNode(tempBoard, numeric_limits<int>::min(), numeric_limits<int>::max(), 1), maxDepth - 1 ,numeric_limits<int>::min(), numeric_limits<int>::max(), (COLOR) !color, tempCoverPieceCount, tempAllCoverCount);
               if (score > bestScore) {
                   bestScore = score;
                   bestMove = move;
               }
            memcpy(tempCoverPieceCount, coverPieceCount, sizeof(int) * 14);
			tempAllCoverCount = allCoverCount;
        } else { // Normal Move
                applyMove(tempBoard, from, to);
                int score = alphaBeta(createNode(tempBoard, numeric_limits<int>::min(), numeric_limits<int>::max(), 1), maxDepth - 1, numeric_limits<int>::min(), numeric_limits<int>::max(), (COLOR) !color, tempCoverPieceCount, tempAllCoverCount);
                  if (score > bestScore) {
                      bestScore = score;
                      bestMove = move;
                 }
             memcpy(tempCoverPieceCount, coverPieceCount, sizeof(int) * 14);
			 tempAllCoverCount = allCoverCount;
        }
    }
    printf("legal: ");
    for (int i = 0; i < legalMoves.size(); i++) {
        printf("%s, ", to_string(legalMoves[i]).c_str());
    }
    printf("\n");
	return bestMove;
}

// Get all legal moves
std::vector<MOVE> MyAI::getAllMoves(const FIN board[BOARD_SIZE], COLOR player) const {
    std::vector<MOVE> moveList;	
    int to, cnt, row, col;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] == FIN_COVER) {
            moveList.push_back(make_move(i, i)); // Flip move
        } else if (board[i] != FIN_EMPTY && color_of(board[i]) == player) {
            row = i % ROW_COUNT;
            col = i / ROW_COUNT;
            if (board[i] == FIN_C || board[i] == FIN_c) {
                for (int delta : {-ROW_COUNT, +1, +ROW_COUNT, -1}) {
                    to = i + delta;
                    cnt = 0;
                    while (to >= 0 && to < BOARD_SIZE && (to % ROW_COUNT == row || to / ROW_COUNT == col)) {
                        cnt += (board[to] != FIN_EMPTY);
                        if (cnt == 2 && color_of(board[to]) == !player) {
                            moveList.push_back(make_move(i, to)); // Cannon capture
                            break;
                        }
                        to += delta;
                    }
                }
            }
            for (int to : {i-ROW_COUNT, i+1, i+ROW_COUNT, i-1}) {
                if (to >= 0 && to < BOARD_SIZE && (to % ROW_COUNT == row || to / ROW_COUNT == col)
                && can_capture(board[i], board[to])) {
                    moveList.push_back(make_move(i, to)); // Normal move
                }
            }
        }
    }
    return moveList;
}

// Evaluation function
int MyAI::evaluateBoard(const FIN board[BOARD_SIZE], COLOR player) const{
    int score = 0;
    int pieceValues[] = {1000, 1000, 500, 500, 400, 400, 300, 300, 200, 200, 200, 200, 100, 100};
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] != FIN_COVER && board[i] != FIN_EMPTY) {
            int pieceValue = pieceValues[board[i]];
            if(color_of(board[i]) == player) {
                score += pieceValue;
            } else {
                score -= pieceValue;
            }
        }
    }
    return score;
}

// Alpha-beta pruning function
int MyAI::alphaBeta(Node node, int depth, int alpha, int beta, COLOR player, int coverPieceCount[14], int allCoverCount) const {
     if (depth == 0) {
       return evaluateBoard(node.board, (COLOR)color);
    }
    std::vector<MOVE> legalMoves = getAllMoves(node.board, player);
     if (legalMoves.empty()) {
           return evaluateBoard(node.board, (COLOR)color);
     }
     if (player == (COLOR)color) { // Maximizing player
        int maxEval = numeric_limits<int>::min();
        for (MOVE move : legalMoves) {
            FIN tempBoard[BOARD_SIZE];
            int tempCoverPieceCount[14];
			int tempAllCoverCount;
             memcpy(tempCoverPieceCount, coverPieceCount, sizeof(int) * 14);
			 tempAllCoverCount = allCoverCount;
            memcpy(tempBoard, node.board, sizeof(FIN) * BOARD_SIZE);
            int from = from_square(move);
            int to = to_square(move);
            if(from == to) {
                FIN flippedPiece = (FIN)(rand() % FIN_COVER);
                    if (tempCoverPieceCount[flippedPiece] == 0) {
                    for (int f = 0; f < FIN_COVER; f++) {
                        if (tempCoverPieceCount[f] > 0) {
                            flippedPiece = (FIN)f;
                            break;
                        }
                    }
                    if(tempCoverPieceCount[flippedPiece] == 0) {
                        continue;
                    }
                }
                applyFlip(tempBoard, to, flippedPiece, tempCoverPieceCount, tempAllCoverCount);
                  int eval = alphaBeta(createNode(tempBoard, alpha, beta, depth - 1), depth - 1, alpha, beta, (COLOR) !player, tempCoverPieceCount, tempAllCoverCount);
               maxEval = std::max(maxEval, eval);
               alpha = std::max(alpha, eval);
               if (beta <= alpha) {
                    break;
                 }
            } else {
                  applyMove(tempBoard, from, to);
                  int eval = alphaBeta(createNode(tempBoard, alpha, beta, depth - 1), depth - 1, alpha, beta, (COLOR) !player, tempCoverPieceCount, tempAllCoverCount);
                  maxEval = std::max(maxEval, eval);
                   alpha = std::max(alpha, eval);
               if (beta <= alpha) {
                  break;
                 }
            }
        }
        return maxEval;
    } else { // Minimizing player
        int minEval = numeric_limits<int>::max();
           for (MOVE move : legalMoves) {
            FIN tempBoard[BOARD_SIZE];
			int tempCoverPieceCount[14];
			int tempAllCoverCount;
             memcpy(tempCoverPieceCount, coverPieceCount, sizeof(int) * 14);
			 tempAllCoverCount = allCoverCount;
            memcpy(tempBoard, node.board, sizeof(FIN) * BOARD_SIZE);
            int from = from_square(move);
            int to = to_square(move);
            if(from == to) {
                 FIN flippedPiece = (FIN)(rand() % FIN_COVER);
                    if (tempCoverPieceCount[flippedPiece] == 0) {
                    for (int f = 0; f < FIN_COVER; f++) {
                        if (tempCoverPieceCount[f] > 0) {
                            flippedPiece = (FIN)f;
                            break;
                        }
                    }
                    if(tempCoverPieceCount[flippedPiece] == 0) {
                        continue;
                    }
                }
               applyFlip(tempBoard, to, flippedPiece, tempCoverPieceCount, tempAllCoverCount);
                int eval = alphaBeta(createNode(tempBoard, alpha, beta, depth - 1), depth - 1, alpha, beta, (COLOR) !player, tempCoverPieceCount, tempAllCoverCount);
                 minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha) {
                    break;
                }
            } else {
                  applyMove(tempBoard, from, to);
                   int eval = alphaBeta(createNode(tempBoard, alpha, beta, depth - 1), depth - 1, alpha, beta, (COLOR) !player, tempCoverPieceCount, tempAllCoverCount);
                 minEval = std::min(minEval, eval);
                   beta = std::min(beta, eval);
                if (beta <= alpha) {
                    break;
                }
            }
        }
        return minEval;
    }
}

// Create a new node
Node MyAI::createNode(const FIN board[BOARD_SIZE], int alpha, int beta, int depth) const {
    Node newNode;
    memcpy(newNode.board, board, sizeof(FIN) * BOARD_SIZE);
    newNode.alpha = alpha;
    newNode.beta = beta;
    newNode.depth = depth;
    return newNode;
}

// Apply a move to a board
void MyAI::applyMove(FIN board[BOARD_SIZE], int from, int to) const {
    board[to] = board[from];
    board[from] = FIN_EMPTY;
}

// Apply a flip to a board
void MyAI::applyFlip(FIN board[BOARD_SIZE], int sq, FIN f, int coverPieceCount[14], int allCoverCount) const {
	board[sq] = f;
	coverPieceCount[f]--;
	allCoverCount--;
}

string MyAI::GetProtocolVersion() const {
	return "1.1.0";
}

string MyAI::GetAIName() const {
	return "MyAI";
}

string MyAI::GetAIVersion() const {
	return "1.0.0";
}

// Print current position state
void MyAI::Print() const {	
	if (color == RED) {
		printf("[RED] ");
	} else if (color == BLK) {
		printf("[BLK] ");
	} else {
		printf("[UNKNOWN] ");
	}
	for (int i = 0; i < FIN_COVER; i++) {
		printf("%d ", coverPieceCount[i]);
	}
	printf("\n");
	for (int i = ROW_COUNT - 1; i >= 0; i--) {
		printf("%d ", i+1);
		for (int j = 0; j < BOARD_SIZE; j += ROW_COUNT) {
			printf("%c ", finEN[board[i + j]]);
		}
		printf("\n");
	}
	printf("  a b c d\n");
}