#ifndef MARBLE_SOLITAIRE_BOARD_H
#define MARBLE_SOLITAIRE_BOARD_H

#include <vector>
#include <stack>
#include <ctime>

using namespace std;

// Cell states in the board
enum CellState
{
    EMPTY,
    MARBLE,
    INVALID // For cells outside the playable area
};

// Game move structure
struct Move
{
    int fromRow, fromCol;
    int toRow, toCol;
    int removedRow, removedCol; // The marble that was jumped over
};

// Game board class
class MarbleSolitaireBoard
{
private:
    vector<vector<CellState>> board;
    stack<Move> moveHistory;
    stack<Move> redoStack;
    time_t startTime;
    int remainingMarbles;
    bool gameOver;
    int selectedRow, selectedCol;

    void checkGameOver(); // Private function

public:
    MarbleSolitaireBoard();

    bool isValidMove(int fromRow, int fromCol, int toRow, int toCol);
    bool makeMove(int fromRow, int fromCol, int toRow, int toCol);
    bool undoMove();
    bool redoMove();

    bool isGameOver() const;
    int getRemainingMarbles() const;
    time_t getElapsedTime() const;
    CellState getCellState(int row, int col) const;

    void selectCell(int row, int col);
    void clearSelection();
    bool hasSelection() const;
    int getSelectedRow() const;
    int getSelectedCol() const;

    void reset();
};

#endif // MARBLE_SOLITAIRE_BOARD_H
