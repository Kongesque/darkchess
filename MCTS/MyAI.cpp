#include <string.h>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <limits>
#include <iostream>

#include "libchess.h"
#include "MyAI.h"
using namespace std;

// MCTS Node structure
struct Node {
    FIN board[BOARD_SIZE];
    int color;
    MOVE move;
    Node* parent;
    vector<Node*> children;
    int visitCount;
    float score;
    int pieceScore; 
    
    Node(const FIN b[BOARD_SIZE], int c, MOVE m, Node* p) :
        parent(p), visitCount(0), score(0.0f), pieceScore(0) {
         memcpy(board, b, sizeof(FIN) * BOARD_SIZE);
        color = c;
        move = m;
    }
    
    ~Node() {
       for(Node* child : children){
          delete child;
       }
    }

   float ucb1(float c = 1.414) const {
        if(visitCount == 0) return std::numeric_limits<float>::infinity();
        return score / visitCount + c * std::sqrt(std::log(parent->visitCount) / visitCount);
    }
};

// Calculate a score from the pieces on the board
int calculatePieceScore(const FIN board[BOARD_SIZE], int color) {
    int score = 0;
    int enemyPieceCount = 0;
    int myPieceCount = 0;
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i] != FIN_EMPTY && board[i] != FIN_COVER) {
           if (color_of(board[i]) == color)
           {
              myPieceCount++;
              switch (type_of(board[i])) {
                case FIN_K: score += 1000; break;
                case FIN_G: score += 700; break;
                case FIN_M: score += 600; break;
                case FIN_R: score += 500; break;
                case FIN_N: score += 400; break;
                case FIN_C: score += 300; break;
                case FIN_P: score += 200; break;
                default: break;
                }
           } else{
                enemyPieceCount++;
               switch (type_of(board[i])) {
                case FIN_K: score -= 1000; break;
                case FIN_G: score -= 700; break;
                case FIN_M: score -= 600; break;
                case FIN_R: score -= 500; break;
                case FIN_N: score -= 400; break;
                case FIN_C: score -= 300; break;
                case FIN_P: score -= 200; break;
                default: break;
                }
           }
        }
    }
   
   //Very high score if the enemy only has one piece left.
   if(enemyPieceCount == 1)
   {
       if (myPieceCount > 1)
       {
          return std::numeric_limits<int>::max();
       } else if (myPieceCount == 1) {
           //If my piece is king and enemy is king, then stalemate.
          for(int i = 0; i < BOARD_SIZE; i++)
          {
             if(type_of(board[i]) == FIN_K)
             {
                 if(color_of(board[i]) == color && myPieceCount == 1 && enemyPieceCount == 1) {
                    return 0;
                 }
             }
          }
          return -std::numeric_limits<int>::max();
       }
   }
    return score;
}

// Generate legal moves
std::vector<MOVE> generateLegalMoves(const FIN board[BOARD_SIZE], int color) {
	std::vector<MOVE> moveList;
	int to, cnt, row, col;

	for (int i = 0; i < BOARD_SIZE; i++) {
		if (board[i] == FIN_COVER) {
			moveList.push_back(make_move(i, i));
		} else if (board[i] != FIN_EMPTY && color_of(board[i]) == color) {
			row = i % ROW_COUNT;
			col = i / ROW_COUNT;
			if (board[i] == FIN_C || board[i] == FIN_c) {
				for (int delta : {-ROW_COUNT, +1, +ROW_COUNT, -1}) {
					to = i + delta;
					cnt = 0;
					while (to >= 0 && to < BOARD_SIZE && (to % ROW_COUNT == row || to / ROW_COUNT == col)) {
						cnt += (board[to] != FIN_EMPTY);
						if (cnt == 2 && color_of(board[to]) == !color) {
							moveList.push_back(make_move(i, to));
							break;
						}
						to += delta;
					}
				}
			}
			for (int to : {i-ROW_COUNT, i+1, i+ROW_COUNT, i-1}) {
				if (to >= 0 && to < BOARD_SIZE && (to % ROW_COUNT == row || to / ROW_COUNT == col)
				 && can_capture(board[i], board[to])) {
					moveList.push_back(make_move(i, to));
				}
			}
		}
	}
    return moveList;
}


// Check if there is a winning capture move
MOVE findWinningCapture(const FIN board[BOARD_SIZE], int color) {
    int enemyPieceCount = 0;
    
     for (int i = 0; i < BOARD_SIZE; i++) {
       if (board[i] != FIN_EMPTY && board[i] != FIN_COVER && color_of(board[i]) != color)
       {
          enemyPieceCount++;
       }
    }
    if (enemyPieceCount != 1)
    {
       return MOVE_NULL;
    }
    
    std::vector<MOVE> moves = generateLegalMoves(board, color);
    for(MOVE move : moves) {
        int from = from_square(move);
        int to = to_square(move);
        
         if(board[to] != FIN_EMPTY && board[to] != FIN_COVER && color_of(board[to]) != color) {
             return move;
         }
    }
    
    return MOVE_NULL;
}


// Perform a move on the board copy
void makeMove(FIN board[BOARD_SIZE], MOVE move) {
    int from = from_square(move);
    int to = to_square(move);

    if (from == to)
    {
         return; //Flip Move
    }
    
    board[to] = board[from];
    board[from] = FIN_EMPTY;
}


// Perform a simulation from a given node
float simulate(Node* node, int color, int simulation_depth) {
     FIN simBoard[BOARD_SIZE];
    memcpy(simBoard, node->board, sizeof(FIN) * BOARD_SIZE);

    int current_color = node->color;
    for(int depth = 0; depth < simulation_depth; depth++){
       std::vector<MOVE> possibleMoves = generateLegalMoves(simBoard, current_color);
        if(possibleMoves.empty()){
           break;
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, possibleMoves.size() - 1);

        MOVE selectedMove = possibleMoves[distrib(gen)];
         makeMove(simBoard, selectedMove);
         
        current_color = current_color == RED ? BLK : RED;
     }
     
    return calculatePieceScore(simBoard, color);
}

// MCTS selection
Node* select(Node* node) {
    while (!node->children.empty()) {
         Node* selectedChild = nullptr;
        float bestUcb1 = -std::numeric_limits<float>::infinity();
        for(Node* child : node->children) {
           float ucb1 = child->ucb1();
           if(ucb1 > bestUcb1)
           {
              bestUcb1 = ucb1;
              selectedChild = child;
           }
        }
        node = selectedChild;
    }
    return node;
}

// MCTS expansion
void expand(Node* node) {
    std::vector<MOVE> possibleMoves = generateLegalMoves(node->board, node->color);
    for (MOVE move : possibleMoves) {
        FIN childBoard[BOARD_SIZE];
        memcpy(childBoard, node->board, sizeof(FIN) * BOARD_SIZE);
         
        if (from_square(move) == to_square(move)) {
            childBoard[to_square(move)] =  char2fin(finEN[rand() % 14]);
        } else {
           makeMove(childBoard, move);
        }
         
        int nextColor = node->color == RED ? BLK : RED;
        Node* child = new Node(childBoard, nextColor, move, node);
        node->children.push_back(child);
    }
}


// MCTS backpropagation
void backpropagate(Node* node, float score, int pieceScore) {
    while (node != nullptr) {
        node->visitCount++;
        node->score += score;
        node->pieceScore += pieceScore;
        node = node->parent;
    }
}


MyAI::MyAI() {
	InitBoard();
}

//Initial board
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

//Initial board by giving position
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

// Move a piece and alternate move turn
void MyAI::Move(int from, int to) {
	if (color == RED) {
		color = BLK;
	} else if (color == BLK) {
		color = RED;
	}
	board[to] = board[from];
	board[from] = FIN_EMPTY;
}

//Flip a piece and alternate move turn
void MyAI::Flip(int sq, FIN f) {
	if (color == RED) {
		color = BLK;
	} else if (color == BLK) {
		color = RED;
	} else {
		color = !color_of(f);
	}
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


// Generate the best move using MCTS
MOVE MyAI::GenerateMove() const {

    // Check for winning capture move
    MOVE winningMove = findWinningCapture(board, color);
     if(winningMove != MOVE_NULL) {
        return winningMove;
    }
    
    // MCTS
    Node* root = new Node(board, color, MOVE_NULL, nullptr);
    
    int iteration_count = 1000;
    int simulation_depth = 10;
    
     // MCTS iterations
    for (int i = 0; i < iteration_count; ++i) {
        Node* selectedNode = select(root);
        if (generateLegalMoves(selectedNode->board, selectedNode->color).empty())
        {
           backpropagate(selectedNode, 0, calculatePieceScore(selectedNode->board, color));
           continue;
        }
        expand(selectedNode);
        
        for (Node* child : selectedNode->children) {
            float score = simulate(child, color, simulation_depth);
            backpropagate(child, score, calculatePieceScore(child->board, color));
        }
    }
    
    // Get best move
    MOVE bestMove = MOVE_NULL;
    float bestScore = -std::numeric_limits<float>::infinity();
    
    for(Node* child : root->children) {
        float score = (float)child->pieceScore + child->score / child->visitCount; //Total Score
        if (score > bestScore)
        {
           bestScore = score;
           bestMove = child->move;
        }
    }
    
    
    printf("legal: ");
	for (Node* child : root->children) {
		printf("%s(score: %.2f, visit: %d), ", to_string(child->move).c_str(), (float)child->score / child->visitCount, child->visitCount);
	}
	printf("\n");
    
   
   delete root;
   
   return bestMove;
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