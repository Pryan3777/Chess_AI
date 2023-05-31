#include "SDL.h"
#include <SDL_image.h>
#undef main
#include <iostream>
#include <list>
#include <string>
#include <random>
using namespace std;

// BY PEYTON RYAN
// LAST UPDATED 4/7/2022

// Declarations
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Surface* screen;
bool isRunning;
bool fullscreen;
void handleEvents();
void update();
void render();
int* generateTables();

// Constants
SDL_Color LIGHT_SQUARE = { 235, 200, 160 };
SDL_Color DARK_SQUARE = { 162, 120, 60 };
int BOARD_SIZE = 60;
// Values of Pieces for evaluation
int PIECE_IMAGE_SIZE = 60;
int PAWN = 100;
int KNIGHT = 300;
int BISHOP = 300;
int ROOK = 500;
int QUEEN = 900;
// Amount of moves to look ahead. Greater is better and slower
int ENGINE_DEPTH = 4; 

int* tables = NULL;
int num_evaluated = 0;

// Object Declerations
class Board;
class Move;

// Required Board Decleration first
void freeBoard(Board*);

// Used to store information about a given move
class Move {
public:
	int from_x;
	int from_y;
	int to_x;
	int to_y;
	// What piece to promote to, if pawn at end
	int promotion_piece;
	// What file en_passant is possible on, if any
	int en_passant;
	bool white_castling_l;
	bool white_castling_r;
	bool black_castling_l;
	bool black_castling_r;

	// Constructor for Move class
	Move(int from_xi, int from_yi, int to_xi, int to_yi, bool bcl, bool bcr, bool wcl, bool wcr, int piece = 0, int ep = -1) {
		from_x = from_xi;
		from_y = from_yi;
		to_x = to_xi;
		to_y = to_yi;
		promotion_piece = piece;
		en_passant = ep;
		white_castling_l = wcl;
		white_castling_r = wcr;
		black_castling_l = bcl;
		black_castling_r = bcr;
	}
};


// Object for a given position. Used to hold give position as well
// as test future positions
class Board {
public:
	list<Move*> *moves; // List of legal moves
	bool turn;
	bool white_check;
	bool black_check;
	int ** squares; // Holds piece values
	bool white_castling_l;
	bool white_castling_r;
	bool black_castling_l;
	bool black_castling_r;
	int move_num;
	int en_passant;
	int depth;
	int eval;

	// Constructor, takes in a fen string (notation for chess position) and sets up the board
	Board(string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", int mn = 1) {
		turn = true;
		white_check = false;
		black_check = false;
		move_num = mn;
		en_passant = -1;
		depth = 1;
		squares = new int*[8];
		moves = new list<Move*>;
		// Initialize board
		for (int x = 0; x < 8; x++)
		{
			squares[x] = new int[8];
			for (int y = 0; y < 8; y++)
			{
				squares[x][y] = NULL;
			}
		}


		// For each character in fen, add piece to board
		int y = 0;
		for (int x = 0; x < fen.length(); x++)
		{
			switch (fen[x]) {
			case '/':
				continue;
			case 'r':
				squares[y % 8][y / 8] = 4;
				y++;
				break;
			case 'n':
				squares[y % 8][y / 8] = 3;
				y++;
				break;
			case 'b':
				squares[y % 8][y / 8] = 2;
				y++;
				break;
			case 'q':
				squares[y % 8][y / 8] = 5;
				y++;
				break;
			case 'k':
				squares[y % 8][y / 8] = 6;
				y++;
				break;
			case 'p':
				squares[y % 8][y / 8] = 1;
				y++;
				break;
			case 'R':
				squares[y % 8][y / 8] = 14;
				y++;
				break;
			case 'N':
				squares[y % 8][y / 8] = 13;
				y++;
				break;
			case 'B':
				squares[y % 8][y / 8] = 12;
				y++;
				break;
			case 'Q':
				squares[y % 8][y / 8] = 15;
				y++;
				break;
			case 'K':
				squares[y % 8][y / 8] = 16;
				y++;
				break;
			case 'P':
				squares[y % 8][y / 8] = 11;
				y++;
				break;
			}
			if ((int)(fen[x] - '0') > 0 && (int)(fen[x] - '0') < 9)
			{
				for (int z = y; z < y + (int)(fen[x] - '0'); z++)
				{
					squares[y % 8][y / 8] = 0;
				}
				y += (int)(fen[x] - '0');
			}
		}

		// Assumes castling is possible if rooks and kings are in starting squares
		// (FEN strings do not provide this info inherently)
		black_castling_l = false;
		black_castling_r = false;
		if (squares[4][0] == 6)
		{
			if (squares[0][0] == 4)
			{
				black_castling_l = true;
			}
			if (squares[7][0] == 4)
			{
				black_castling_r = true;
			}
		}

		white_castling_l = false;
		white_castling_r = false;
		if (squares[4][7] == 16)
		{
			if (squares[0][7] == 14)
			{
				white_castling_l = true;
			}
			if (squares[7][7] == 14)
			{
				white_castling_r = true;
			}
		}
		evaluate(); // Evaluates given position by itself
		findMoves(); // Calculates all legal moves from position
	};
	// Constructor used to create a board from another, with a move played
	Board(Board * b, int from_x, int from_y, int to_x, int to_y, bool bcl, bool bcr, bool wcl, bool wcr, int piece = 0, int ep = -1, int d = 1)
	{
		turn = !(b->turn);
		move_num = b->move_num++;
		white_check = false;
		black_check = false;
		en_passant = ep;
		white_castling_l = wcl;
		white_castling_r = wcr;
		black_castling_l = bcl;
		black_castling_r = bcr;
		depth = d;
		squares = new int* [8];
		moves = new list<Move*>;
		for (int x = 0; x < 8; x++)
		{
			squares[x] = new int[8];
			for (int y = 0; y < 8; y++)
			{
				squares[x][y] = b->squares[x][y];
			}
		}
		if (from_x != to_x || from_y != to_y)
		{
			// Check for special moves
			if (squares[from_x][from_y] % 10 == 1 && from_x != to_x && squares[to_x][to_y] == 0) // en passant
			{
				squares[to_x][from_y] = 0;
			}
			if (squares[from_x][from_y] == 6 && from_x - to_x == 2 && bcl == false) // black castling left
			{
				squares[0][from_y] = 0;
				squares[to_x][to_y] = 6;
				squares[from_x - 1][from_y] = 4;
				squares[from_x][from_y] = 0;
			}
			else if (squares[from_x][from_y] == 6 && from_x - to_x == -2 && bcr == false) // black castling right
			{
				squares[7][from_y] = 0;
				squares[to_x][to_y] = 6;
				squares[from_x + 1][from_y] = 4;
				squares[from_x][from_y] = 0;
			}
			else if (squares[from_x][from_y] == 16 && from_x - to_x == 2 && wcl == false) // white castling left 
			{
				squares[0][from_y] = 0;
				squares[to_x][to_y] = 16;
				squares[from_x - 1][from_y] = 14;
				squares[from_x][from_y] = 0;
			}
			else if (squares[from_x][from_y] == 16 && from_x - to_x == -2 && wcr == false) // white castling right
			{
				squares[7][from_y] = 0;
				squares[to_x][to_y] = 16;
				squares[from_x + 1][from_y] = 14;
				squares[from_x][from_y] = 0;
			}
			else // otherwise make the move normally
			{
				squares[to_x][to_y] = squares[from_x][from_y];
				squares[from_x][from_y] = 0;
			}
		}
		if ((to_y == 0 || to_y == 7) && squares[to_x][to_y] % 10 == 1) // If Pawn made it to end
		{
			switch (piece)
			{
			case 0: // Queen
				squares[to_x][to_y] += 4;
			}
		}
		evaluate(); // Evaluate given position
		if(depth > 0) // Moves are not needed if position is only being used to determine check
			findMoves();
	}
	Board* doMove(int from_xi, int from_yi, int to_xi, int to_yi, int piece = 0)
	{
		// If a move is attempted to be played, check if it is in the list of calculated moves
		for (list<Move*>::iterator i = moves->begin(); i != moves->end(); i++)
		{
			if (from_xi == (*i)->from_x && from_yi == (*i)->from_y && to_xi == (*i)->to_x && to_yi == (*i)->to_y && piece == (*i)->promotion_piece)
			{
				// If it is a valid move, play it and return the new board
				return new Board(this, (*i)->from_x, (*i)->from_y, (*i)->to_x, (*i)->to_y, (*i)->black_castling_l, (*i)->black_castling_r, (*i)->white_castling_l, (*i)->white_castling_r, (*i)->promotion_piece, (*i)->en_passant);
			}
		}
		return this;
	}
	void findMoves()
	{
		// Get all legal moves from this position
		int l, n, a, b;
		bool left, right;
		for (int x = 0; x < 8; x++)
		{
			for (int y = 0; y < 8; y++)
			{
				int p = squares[x][y];
				// If there is a piece of the current players color, check moves
				if (p != 0 && (p > 9 == turn))
				{
					switch (p)
					{
					case 1: // Black Pawn
						if (squares[x][y + 1] == 0)
						{
							checkMove(x, y, x, y + 1, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
							if (y == 1 && squares[x][y + 2] == 0)
							{
								// First move double
								checkMove(x, y, x, y + 2, black_castling_l, black_castling_r, white_castling_l, white_castling_r, 0, x);
							}
						}
						// Take Piece
						if (x > 0 && (squares[x - 1][y + 1] > 10 || en_passant == x - 1))
						{
							checkMove(x, y, x - 1, y + 1, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
						}
						if (x < 7 && (squares[x + 1][y + 1] > 10 || en_passant == x + 1))
						{
							checkMove(x, y, x + 1, y + 1, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
						}
						break;
					case 11: // White Pawn
						if (squares[x][y - 1] == 0)
						{
							checkMove(x, y, x, y - 1, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
							if (y == 6 && squares[x][y - 2] == 0)
							{
								// First move double
								checkMove(x, y, x, y - 2, black_castling_l, black_castling_r, white_castling_l, white_castling_r, 0 , x);
							}
						}
						// Take Piece
						if (x > 0 && ((squares[x - 1][y - 1] > 0 && squares[x - 1][y - 1] < 10) || en_passant == x - 1))
						{
							checkMove(x, y, x - 1, y - 1, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
						}
						if (x < 7 && ((squares[x + 1][y - 1] > 0 && squares[x + 1][y - 1] < 10) || en_passant == x + 1))
						{
							checkMove(x, y, x + 1, y - 1, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
						}
						break;
					case 2: // Black Bishop
						for (l = 0; l < 4; l++)
						{
							// Iterates over 4 directions
							a = ((l % 2) * 2) - 1; // -1,  1, -1,  1
							b = ((l / 2) * 2) - 1; // -1, -1,  1,  1
							n = 1;
							// Keep going along diagonal until edge or friendly piece
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] > 10 || squares[x + (n * a)][y + (n * b)] == 0))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, white_castling_l, white_castling_r);
								// if there is an enemy piece, end here
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						break;
					case 12: // White Bishop
						for (l = 0; l < 4; l++)
						{
							// Iterates over 4 directions
							a = ((l % 2) * 2) - 1; // -1,  1, -1,  1
							b = ((l / 2) * 2) - 1; // -1, -1,  1,  1
							n = 1;
							// Keep going along diagonal until edge or friendly piece
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] < 10))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, white_castling_l, white_castling_r);
								// if there is an enemy piece, end here
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						break;
					case 3: // Black Knight
						for (n = 0; n < 8; n++)
						{
							// Iterates over 8 Knight moves
							a = (n % 4) + ((n % 4) / 2) - 2;								// -2, -1,  1,  2, -2, -1,  1,  2
							b = (((n % 4) + ((n % 4) / 2)) % 2 + 1) * (((n / 4) * 2) - 1);	// -1, -2, -2, -1,  1,  2,  2,  1
							if (x + a >= 0 && x + a < 8 && y + b >= 0 && y + b < 8 && (squares[x + a][y + b] > 10 || squares[x + a][y + b] == 0))
							{
								checkMove(x, y, x + a, y + b, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
							}
						}
						break;
					case 13: // White Knight
						for (n = 0; n < 8; n++)
						{
							// Iterates over 8 Knight moves
							a = (n % 4) + ((n % 4) / 2) - 2;								// -2, -1,  1,  2, -2, -1,  1,  2
							b = (((n % 4) + ((n % 4) / 2)) % 2 + 1) * (((n / 4) * 2) - 1);	// -1, -2, -2, -1,  1,  2,  2,  1
							if (x + a >= 0 && x + a < 8 && y + b >= 0 && y + b < 8 && (squares[x + a][y + b] < 10))
							{
								checkMove(x, y, x + a, y + b, black_castling_l, black_castling_r, white_castling_l, white_castling_r);
							}
						}
						break;
					case 4: // Black Rook
						for (l = 0; l < 4; l++)
						{
							// Iterate over 4 lateral directions
							a = (1 - (l / 2)) * (((l % 2) * 2) - 1); // -1, 1,  0, 0 
							b = (l / 2) * (((l % 2) * 2) - 1);       //  0, 0, -1, 1
							n = 1;
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] > 10 || squares[x + (n * a)][y + (n * b)] == 0))
							{
								// Remove castling rights as needed
								left = black_castling_l;
								right = black_castling_r;
								if (x == 0 && y == 0)
								{
									left = false;
								}
								else if (x == 7 && y == 0)
								{
									right = false;
								}
								checkMove(x, y, x + (n * a), y + (n * b), left, right, white_castling_l, white_castling_r);
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						break;
					case 14: // White Rook
						for (l = 0; l < 4; l++)
						{
							// Iterate over 4 lateral directions
							a = (1 - (l / 2)) * (((l % 2) * 2) - 1); // -1, 1,  0, 0 
							b = (l / 2) * (((l % 2) * 2) - 1);       //  0, 0, -1, 1
							n = 1;
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] < 10))
							{
								// Remove castling rights as needed
								left = white_castling_l;
								right = white_castling_r;
								if (x == 0 && y == 7)
								{
									left = false;
								}
								else if (x == 7 && y == 7)
								{
									right = false;
								}
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, left, right);
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						break;
					case 5: // Black Queen
						for (l = 0; l < 4; l++) // Bishop
						{
							a = ((l % 2) * 2) - 1;
							b = ((l / 2) * 2) - 1;
							n = 1;
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] > 10 || squares[x + (n * a)][y + (n * b)] == 0))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, white_castling_l, white_castling_r);
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						for (l = 0; l < 4; l++) // Rook
						{
							a = (1 - (l / 2)) * (((l % 2) * 2) - 1);
							b = (l / 2) * (((l % 2) * 2) - 1);
							n = 1;
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] > 10 || squares[x + (n * a)][y + (n * b)] == 0))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, white_castling_l, white_castling_r);
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						break;
					case 15: // White Queen
						for (l = 0; l < 4; l++) // Bishop
						{
							a = ((l % 2) * 2) - 1;
							b = ((l / 2) * 2) - 1;
							n = 1;
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] < 10))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, white_castling_l, white_castling_r);
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						for (l = 0; l < 4; l++) // Rook
						{
							a = (1 - (l / 2)) * (((l % 2) * 2) - 1);
							b = (l / 2) * (((l % 2) * 2) - 1);
							n = 1;
							while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] < 10))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, white_castling_l, white_castling_r);
								if (squares[x + (n * a)][y + (n * b)] != 0)
									break;
								n++;
							}
						}
						break;
					case 6: // Black King
						for (l = 0; l < 4; l++)
						{
							// Diagonal
							a = ((l % 2) * 2) - 1;
							b = ((l / 2) * 2) - 1;
							n = 1;
							if (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] > 10 || squares[x + (n * a)][y + (n * b)] == 0))
							{
								checkMove(x, y, x + (n * a), y + (n * b), false, false, white_castling_l, white_castling_r);
							}
						}
						for (l = 0; l < 4; l++)
						{
							// Lateral
							a = (1 - (l / 2)) * (((l % 2) * 2) - 1);
							b = (l / 2) * (((l % 2) * 2) - 1);
							n = 1;
							if (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] > 10 || squares[x + (n * a)][y + (n * b)] == 0))
							{
								checkMove(x, y, x + (n * a), y + (n * b), false, false, white_castling_l, white_castling_r);
							}
						}
						// Castle Left
						if (black_castling_l && !isCheck(turn) && squares[x - 1][y] == 0 && squares[x - 2][y] == 0)
						{
							Board* a = new Board(this, x, y, x - 1, y, true, true, white_castling_l, white_castling_r, 0, -1, 0);
							Board* b = new Board(this, x, y, x - 2, y, true, true, white_castling_l, white_castling_r, 0, -1, 0);
							if (!(a->isCheck(turn) || b->isCheck(turn)))
							{
								freeBoard(a);
								freeBoard(b);
								checkMove(x, y, x - 2, y, false, false, white_castling_l, white_castling_r);
							}
						}
						// Castle Right
						if (black_castling_r && !isCheck(turn) && squares[x + 1][y] == 0 && squares[x + 2][y] == 0)
						{
							Board* a = new Board(this, x, y, x + 1, y, true, true, white_castling_l, white_castling_r, 0, -1, 0);
							Board* b = new Board(this, x, y, x + 2, y, true, true, white_castling_l, white_castling_r,  0, -1, 0);
							if (!(a->isCheck(turn) || b->isCheck(turn)))
							{
								freeBoard(a);
								freeBoard(b);
								checkMove(x, y, x + 2, y, false, false, white_castling_l, white_castling_r);
							}
						}
						break;
					case 16: // White King
						for (l = 0; l < 4; l++)
						{
							// Diagonal
							a = ((l % 2) * 2) - 1;
							b = ((l / 2) * 2) - 1;
							n = 1;
							if (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] < 10))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, false, false);
							}
						}
						for (l = 0; l < 4; l++)
						{
							// Lateral
							a = (1 - (l / 2)) * (((l % 2) * 2) - 1);
							b = (l / 2) * (((l % 2) * 2) - 1);
							n = 1;
							if (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] < 10))
							{
								checkMove(x, y, x + (n * a), y + (n * b), black_castling_l, black_castling_r, false, false);
							}
						}
						// Castle Left
						if (white_castling_l && !isCheck(turn) && squares[x - 1][y] == 0 && squares[x - 2][y] == 0)
						{
							// Cannot Castle Through Check
							Board* a = new Board(this, x, y, x - 1, y, black_castling_l, black_castling_r, true, true, 0, -1, 0);
							Board* b = new Board(this, x, y, x - 2, y, black_castling_l, black_castling_r, true, true, 0, -1, 0);
							if (!(a->isCheck(turn) || b->isCheck(turn)))
							{
								freeBoard(a);
								freeBoard(b);
								checkMove(x, y, x - 2, y, black_castling_l, black_castling_r, false, false);
							}
						}
						// Castle Right
						if (white_castling_r && !isCheck(turn) && squares[x + 1][y] == 0 && squares[x + 2][y] == 0)
						{
							// Cannot Castle Through Check
							Board* a = new Board(this, x, y, x + 1, y, black_castling_l, black_castling_r, true, true, 0, -1, 0);
							Board* b = new Board(this, x, y, x + 2, y, black_castling_l, black_castling_r, true, true, 0, -1, 0);
							if (!(a->isCheck(turn) || b->isCheck(turn)))
							{
								freeBoard(a);
								freeBoard(b);
								checkMove(x, y, x + 2, y, black_castling_l, black_castling_r, false, false);
							}
						}
						break;
					}
				}
			}
		}
	}
	// Check if a given move results in check or other invalid posiiton
	void checkMove(int x, int y, int xi, int yi, bool bcl, bool bcr, bool wcl, bool wcr, int piece = 0, int ep = -1)
	{
		Move m = Move(x, y, xi, yi, bcl, bcr, wcl, wcr, piece, ep);
		Board* b = new Board(this, x, y, xi, yi, bcl, bcr, wcl, wcr, piece, ep, 0);
		if (!b->isCheck(turn))
		{
			freeBoard(b);
			moves->push_back(new Move(x, y, xi, yi, bcl, bcr, wcl, wcr, piece, ep));
		}
	}
	// Checks if a player is in check on this board
	bool isCheck(bool color)
	{
		int x, y, a, b, n, l;
		if (color) // White
		{
			x = -1;
			// Find King
			for (a = 0; a < 8; a++)
			{
				for (b = 0; b < 8; b++)
				{
					if (squares[a][b] == 16)
					{
						x = a;
						y = b;
						a = 8; // artificial break statement for outer loop
						break;
					}
				}
			}
			if (x == -1)
			{
				return true;
			}
			// Check for enemy Knights
			for (n = 0; n < 8; n++)
			{
				a = (n % 4) + ((n % 4) / 2) - 2;
				b = (((n % 4) + ((n % 4) / 2)) % 2 + 1) * (((n / 4) * 2) - 1);
				if (x + a >= 0 && x + a < 8 && y + b >= 0 && y + b < 8 && (squares[x + a][y + b] == 3))
				{
					return true;
				}
			}

			// Check for Diagonal Pieces
			for (l = 0; l < 4; l++)
			{
				a = ((l % 2) * 2) - 1;
				b = ((l / 2) * 2) - 1;
				n = 1;
				while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] ==  2 || squares[x + (n * a)][y + (n * b)] == 5 || (squares[x + (n * a)][y + (n * b)] == 6 && n == 1) || (squares[x + (n * a)][y + (n * b)] == 1 && n == 1 && a == 1) || squares[x + (n * a)][y + (n * b)] == 0))
				{
					if (squares[x + (n * a)][y + (n * b)] != 0)
					{
						return true;
					}
					n++;
				}
			}

			// Check for horizontal/vertical Pieces
			for (l = 0; l < 4; l++)
			{
				a = (1 - (l / 2)) * (((l % 2) * 2) - 1);
				b = (l / 2) * (((l % 2) * 2) - 1);
				n = 1;
				while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] == 4 || squares[x + (n * a)][y + (n * b)] == 5 || (squares[x + (n * a)][y + (n * b)] == 6 && n == 1) || squares[x + (n * a)][y + (n * b)] == 0))
				{
					if (squares[x + (n * a)][y + (n * b)] != 0)
					{
						return true;
					}
					n++;
				}
			}
		}
		else // Black
		{
			x = -1;
			// Find King
			for (a = 0; a < 8; a++)
			{
				for (b = 0; b < 8; b++)
				{
					if (squares[a][b] == 6)
					{
						x = a;
						y = b;
						a = 8; // artificial break statement for outer loop
						break;
					}
				}
			}
			if (x == -1)
			{
				return true;
			}
			// Check for enemy Knights
			for (n = 0; n < 8; n++)
			{
				a = (n % 4) + ((n % 4) / 2) - 2;
				b = (((n % 4) + ((n % 4) / 2)) % 2 + 1) * (((n / 4) * 2) - 1);
				if (x + a >= 0 && x + a < 8 && y + b >= 0 && y + b < 8 && (squares[x + a][y + b] == 13))
				{
					return true;
				}
			}

			// Check for Diagonal Pieces
			for (l = 0; l < 4; l++)
			{
				a = ((l % 2) * 2) - 1;
				b = ((l / 2) * 2) - 1;
				n = 1;
				while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] == 12 || squares[x + (n * a)][y + (n * b)] == 15 || (squares[x + (n * a)][y + (n * b)] == 16 && n == 1) || (squares[x + (n * a)][y + (n * b)] == 11 && n == 1 && a == -1) || squares[x + (n * a)][y + (n * b)] == 0))
				{
					if (squares[x + (n * a)][y + (n * b)] != 0)
					{
						return true;
					}
					n++;
				}
			}

			// Check for horizontal/vertical Pieces
			for (l = 0; l < 4; l++)
			{
				a = (1 - (l / 2)) * (((l % 2) * 2) - 1);
				b = (l / 2) * (((l % 2) * 2) - 1);
				n = 1;
				while (x + (n * a) >= 0 && x + (n * a) < 8 && y + (n * b) >= 0 && y + (n * b) < 8 && (squares[x + (n * a)][y + (n * b)] == 14 || squares[x + (n * a)][y + (n * b)] == 15 || (squares[x + (n * a)][y + (n * b)] == 16 && n == 1) || squares[x + (n * a)][y + (n * b)] == 0))
				{
					if (squares[x + (n * a)][y + (n * b)] != 0)
					{
						return true;
					}
					n++;
				}
			}
		}
		return false;
	}
	// Evalutates Given position based on piece counts, and positions
	void evaluate()
	{
		int x, y, e = 0;
		if (tables == NULL)
		{
			tables = generateTables();
		}
		// Count up given pieces as well as add points based on their piece tables
		for (x = 0; x < 8; x++)
		{
			for (y = 0; y < 8; y++)
			{
				switch (squares[x][y])
				{
				case 1:
					e -= PAWN;
					e -= tables[(0 * 64) + (8 * (7 - x)) + y];
					break;
				case 2:
					e -= BISHOP;
					e -= tables[(1 * 64) + (8 * (7 - x)) + y];
					break;
				case 3:
					e -= KNIGHT;
					e -= tables[(2 * 64) + (8 * (7 - x)) + y];
					break;
				case 4:
					e -= ROOK;
					e -= tables[(3 * 64) + (8 * (7 - x)) + y];
					break;
				case 5:
					e -= QUEEN;
					e -= tables[(4 * 64) + (8 * (7 - x)) + y];
					break;
				case 11:
					e += PAWN;
					e += tables[(0 * 64) + (8 * x) + y];
					break;
				case 12:
					e += BISHOP;
					e += tables[(1 * 64) + (8 * x) + y];
					break;
				case 13:
					e += KNIGHT;
					e += tables[(2 * 64) + (8 * x) + y];
					break;
				case 14:
					e += ROOK;
					e += tables[(3 * 64) + (8 * x) + y];
					break;
				case 15:
					e += QUEEN;
					e += tables[(4 * 64) + (8 * x) + y];
					break;
				}
			}
		}
		if (!turn)
		{
			e = e * -1;
		}
		eval = e;
	}
	// Main function for finding best moves
	// Recursively searches d moves ahead using minmax function to find best move
	// Also uses alpha beta pruning to avoid unnecessary calculations
	Board* getBest(int d, int alpha, int beta, bool surface = false)
	{
		int al = alpha;
		int bet = beta;
		list<Board*> bestMoves;
		Board* ret;
		num_evaluated++;
		if (d == 0)
		{
			return this;
		}
		eval = -10000000;
		int mult = 1;
		if (moves->empty())
		{
			if (!isCheck(turn))
			{
				eval = 0;
			}
			return this;
		}
		Board* b;
		for (list<Move*>::iterator i = moves->begin(); i != moves->end(); i++)
		{
			b = new Board(this, (*i)->from_x, (*i)->from_y, (*i)->to_x, (*i)->to_y, (*i)->black_castling_l, (*i)->black_castling_r, (*i)->white_castling_l, (*i)->white_castling_r, (*i)->promotion_piece, (*i)->en_passant);
			b->getBest(d - 1, bet, al);
			// Evaluate if this branch is prunable
			// It will not be relevant for the depths above it so ignore it
			if (b->eval < al)
			{
				eval = 1000000;
				for (auto& i : bestMoves)
				{
					freeBoard(i);
				}
				bestMoves.clear();
				return NULL;
			}
			// If this is the current best move, clear the list and add this to it
			if (eval < b->eval * -1)
			{
				eval = b->eval * -1;
				bet = eval;
				for (auto& i : bestMoves)
				{
					freeBoard(i);
				}
				bestMoves.clear();
				bestMoves.push_back(b);
			}
			// If this is equally best move, add to list
			else if (b->eval * -1 == eval)
			{
				bestMoves.push_back(b);
			}
			// Otherwise trash it
			else
			{
				freeBoard(b);
			}
		}
		if (moves->size() == 0)
		{
			eval = -1000000;
			return NULL;
		}
		// Currently returns first move found of highest eval
		ret = bestMoves.front();
		for (auto& i : bestMoves)
		{
			if (i != ret)
			{
				freeBoard(i);
			}
		}
		bestMoves.clear();
		return ret;
	}
};

// Gets file paths of piece images
string  getImgPath(int piece, int team)
{
	string buffer;
	char a = (team + '0');
	char b = (piece + '0');
	buffer = "pieces/";
	buffer = buffer + a;
	buffer = buffer + b;
	buffer = buffer + ".png";
	return buffer;
}

// Other rendering variables for board
Board* current_board = new Board();
Board* next_board = NULL;
SDL_Surface* images[12];
SDL_Texture* textures[12];
int selected[2]; // Player first click
int to[2]; // Player second click
int mouse_pos[2];
bool mouse_down = false;

int main() {
	fullscreen = false;
	int flags = 0;
	flags = SDL_WINDOW_RESIZABLE;
	selected[0] = -1;
	selected[1] = -1;

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
	{
		cout << SDL_GetError() << endl;
	}

	if (fullscreen) {
		flags = flags | SDL_WINDOW_FULLSCREEN;
	}
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {

		window = SDL_CreateWindow("Chess - Peyton Ryan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 8 * BOARD_SIZE, 8 * BOARD_SIZE, flags);
		if (window) {
			SDL_SetWindowMinimumSize(window, BOARD_SIZE, BOARD_SIZE);
		}
		renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer) {
			SDL_SetRenderDrawColor(renderer, 121, 121, 121, 255);
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			isRunning = true;
		}

	}

	// Load in piece images
	for (int x = 0; x < 12; x++)
	{
		string path = getImgPath(((x + 6) % 6) + 1, x / 6);
		char* buffer = const_cast<char*>(path.c_str());
		images[x] = IMG_Load(buffer);
		if (images[x] == NULL)
		{
			cout << "COULD NOT OPEN IMAGE" << endl;
		}
		else
		{
			textures[x] = SDL_CreateTextureFromSurface(renderer, images[x]);
		}
	}

	// Start Loop
	while (isRunning) {
		handleEvents();
		update();
		render();
	}

	// Frees memory associated with renderer and window
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();


	return 0;
}

// Handles events from input and quit
void handleEvents() {
	SDL_Event event;
	SDL_PollEvent(&event);


	switch (event.type) {
	case SDL_QUIT:
		isRunning = false;
		break;
	case SDL_MOUSEBUTTONDOWN:
		switch (event.button.button)
		{
		case SDL_BUTTON_LEFT:
			selected[0] = mouse_pos[0] / BOARD_SIZE;
			selected[1] = mouse_pos[1] / BOARD_SIZE;
			if (current_board->squares[selected[0]][selected[1]] != 0)
			{
				mouse_down = true;
			}
			break;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		switch (event.button.button)
		{
		case SDL_BUTTON_LEFT:
			if (mouse_down)
			{
				mouse_down = false;
				next_board = current_board->doMove(selected[0], selected[1], mouse_pos[0] / BOARD_SIZE, mouse_pos[1] / BOARD_SIZE);
				// If players selected move is valid
				if (next_board != current_board)
				{
					freeBoard(current_board);
					// Make the move
					current_board = next_board;
					if (current_board->moves->size() == 0)
					{
						render();
						cout << "GAME OVER" << endl;
						while (true)
						{

						}
					}
					render();
					num_evaluated = 0;
					// Computer makes next move
					next_board = current_board->getBest(ENGINE_DEPTH, -100000, -100000, true);
					freeBoard(current_board);
					current_board = next_board;
					if (current_board->moves->size() == 0)
					{
						render();
						cout << "GAME OVER" << endl;
						while (true)
						{

						}
					}
				}
			}
			selected[0] = -1;
			selected[1] = -1;
			break;
		}
		break;
	case SDL_MOUSEMOTION:
		// Track Mouse position changes
		mouse_pos[0] = event.motion.x;
		mouse_pos[1] = event.motion.y;
		break;
	default:
		break;
	}
}

// Render graphics
void render() {
	SDL_SetRenderDrawColor(renderer, 121, 121, 121, 255);
	SDL_RenderClear(renderer);

	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			// Draw board
			SDL_Rect square = { x * BOARD_SIZE, y * BOARD_SIZE, BOARD_SIZE, BOARD_SIZE };
			SDL_Color c = (x + y) % 2 == 0 ? LIGHT_SQUARE : DARK_SQUARE;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
			SDL_RenderFillRect(renderer, &square);
			// Draw non-selected pieces
			if (current_board->squares[x][y] != 0)
			{
				if (x != selected[0] || y != selected[1] || mouse_down == false)
				{
					SDL_RenderCopy(renderer, textures[current_board->squares[x][y]%10 + (6 * (current_board->squares[x][y] / 10)) - 1], NULL, &square);
				}
			}
		}
	}
	// If piece is selected, draw it last so it is on top
	if (mouse_down && current_board->squares[selected[0]][selected[1]] != NULL)
	{
		SDL_Rect moving = { mouse_pos[0] - (BOARD_SIZE / 2), mouse_pos[1] - (BOARD_SIZE / 2), BOARD_SIZE, BOARD_SIZE };
		SDL_RenderCopy(renderer, textures[current_board->squares[selected[0]][selected[1]] % 10 + (6 * (current_board->squares[selected[0]][selected[1]] / 10)) - 1], NULL, &moving);
	}



	SDL_RenderPresent(renderer);
}

//simple update function
void update() {
	//if things could update the code would go in here.
}

// Tables used to weight eval so that pieces are give higher or lower
// values based on their position on the board. Positive is better, flipped per side
int * generateTables()
{
	static int ret[6 * 64] =
	{
			// Pawn
			0,  0,  0,  0,  0,  0,  0,  0,
			50, 50, 50, 50, 50, 50, 50, 50,
			10, 10, 20, 30, 30, 20, 10, 10,
			5,  5, 10, 25, 25, 10,  5,  5,
			0,  0,  0, 20, 20,  0,  0,  0,
			5, -5,-10,  0,  0,-10, -5,  5,
			5, 10, 10,-20,-20, 10, 10,  5,
			0,  0,  0,  0,  0,  0,  0,  0,
			// Bishop
			-20,-10,-10,-10,-10,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5, 10, 10,  5,  0,-10,
			-10,  5,  5, 10, 10,  5,  5,-10,
			-10,  0, 10, 10, 10, 10,  0,-10,
			-10, 10, 10, 10, 10, 10, 10,-10,
			-10,  5,  0,  0,  0,  0,  5,-10,
			-20,-10,-10,-10,-10,-10,-10,-20,
			// Knight
			-50,-40,-30,-30,-30,-30,-40,-50,
			-40,-20,  0,  0,  0,  0,-20,-40,
			-30,  0, 10, 15, 15, 10,  0,-30,
			-30,  5, 15, 20, 20, 15,  5,-30,
			-30,  0, 15, 20, 20, 15,  0,-30,
			-30,  5, 10, 15, 15, 10,  5,-30,
			-40,-20,  0,  5,  5,  0,-20,-40,
			-50,-40,-30,-30,-30,-30,-40,-50,
			// Rook
			  0,  0,  0,  0,  0,  0,  0,  0,
			  5, 10, 10, 10, 10, 10, 10,  5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			 -5,  0,  0,  0,  0,  0,  0, -5,
			  0,  0,  0,  5,  5,  0,  0,  0,
			// Queen
			-20,-10,-10, -5, -5,-10,-10,-20,
			-10,  0,  0,  0,  0,  0,  0,-10,
			-10,  0,  5,  5,  5,  5,  0,-10,
			 -5,  0,  5,  5,  5,  5,  0, -5,
			  0,  0,  5,  5,  5,  5,  0, -5,
			-10,  5,  5,  5,  5,  5,  0,-10,
			-10,  0,  5,  0,  0,  0,  0,-10,
			-20,-10,-10, -5, -5,-10,-10,-20,
			// King
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-30,-40,-40,-50,-50,-40,-40,-30,
			-20,-30,-30,-40,-40,-30,-30,-20,
			-10,-20,-20,-20,-20,-20,-20,-10,
			 20, 20,  0,  0,  0,  0, 20, 20,
			 20, 30, 10,  0,  0, 10, 30, 20
	};
	return ret;
}

// Clean up board and child objects
void freeBoard(Board* b)
{
	int x;
	for (x = 0; x < 8; x++)
	{
		delete[] b->squares[x];
	}
	delete [] b->squares;
	for (auto& i : *b->moves)
	{
		delete i;
	}
	delete b->moves;
	delete b;
}