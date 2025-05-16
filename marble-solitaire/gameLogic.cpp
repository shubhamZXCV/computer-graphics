#include "gameLogic.hpp"
#include <iostream>

MarbleSolitaireBoard::MarbleSolitaireBoard()
{
    reset();
}

void MarbleSolitaireBoard::reset()
{
    board = vector<vector<CellState>>(7, vector<CellState>(7, INVALID));
    remainingMarbles = 0;

    // Set up the cross-shaped board
    for (int row = 0; row < 7; row++)
    {
        for (int col = 0; col < 7; col++)
        {
            if ((row >= 2 && row <= 4) || (col >= 2 && col <= 4))
            {
                board[row][col] = MARBLE;
                remainingMarbles++;
            }
        }
    }

    // Set the center to empty
    board[3][3] = EMPTY;
    remainingMarbles--;

    // Initialize other variables
    startTime = time(NULL);
    gameOver = false;
    selectedRow = selectedCol = -1;
    while (!moveHistory.empty())
        moveHistory.pop();
    while (!redoStack.empty())
        redoStack.pop();
}

bool MarbleSolitaireBoard::isValidMove(int fromRow, int fromCol, int toRow, int toCol)
{
    // Check if positions are within bounds
    if (fromRow < 0 || fromRow >= 7 || fromCol < 0 || fromCol >= 7 ||
        toRow < 0 || toRow >= 7 || toCol < 0 || toCol >= 7)
    {
        return false;
    }

    // Check if source has marble and destination is empty
    if (board[fromRow][fromCol] != MARBLE || board[toRow][toCol] != EMPTY)
    {
        return false;
    }

    // Check if the move is two spaces horizontally or vertically
    int rowDiff = abs(toRow - fromRow);
    int colDiff = abs(toCol - fromCol);
    if (!((rowDiff == 2 && colDiff == 0) || (rowDiff == 0 && colDiff == 2)))
    {
        return false;
    }

    // Check if there is a marble in between
    int middleRow = (fromRow + toRow) / 2;
    int middleCol = (fromCol + toCol) / 2;
    return board[middleRow][middleCol] == MARBLE;
}

bool MarbleSolitaireBoard::makeMove(int fromRow, int fromCol, int toRow, int toCol)
{
    if (!isValidMove(fromRow, fromCol, toRow, toCol))
    {
        return false;
    }

    int middleRow = (fromRow + toRow) / 2;
    int middleCol = (fromCol + toCol) / 2;

    // Execute the move
    board[fromRow][fromCol] = EMPTY;
    board[middleRow][middleCol] = EMPTY;
    board[toRow][toCol] = MARBLE;
    remainingMarbles--;

    // Record the move
    Move move = {fromRow, fromCol, toRow, toCol, middleRow, middleCol};
    moveHistory.push(move);
    while (!redoStack.empty())
        redoStack.pop();

    checkGameOver();
    return true;
}

bool MarbleSolitaireBoard::undoMove()
{
    printf(moveHistory.empty() ? "true" : "false");
    if (moveHistory.empty())
    {
        return false;
    }

    Move move = moveHistory.top();
    moveHistory.pop();

    // Restore the board state
    board[move.fromRow][move.fromCol] = MARBLE;
    board[move.removedRow][move.removedCol] = MARBLE;
    board[move.toRow][move.toCol] = EMPTY;
    remainingMarbles++;

    redoStack.push(move);
    gameOver = false;
    return true;
}

bool MarbleSolitaireBoard::redoMove()
{
    if (redoStack.empty())
    {
        return false;
    }

    Move move = redoStack.top();
    redoStack.pop();

    // Reapply the move
    board[move.fromRow][move.fromCol] = EMPTY;
    board[move.removedRow][move.removedCol] = EMPTY;
    board[move.toRow][move.toCol] = MARBLE;
    remainingMarbles--;

    moveHistory.push(move);
    checkGameOver();
    return true;
}

void MarbleSolitaireBoard::checkGameOver()
{
    for (int row = 0; row < 7; row++)
    {
        for (int col = 0; col < 7; col++)
        {
            if (board[row][col] == MARBLE)
            {
                if (isValidMove(row, col, row + 2, col) ||
                    isValidMove(row, col, row - 2, col) ||
                    isValidMove(row, col, row, col + 2) ||
                    isValidMove(row, col, row, col - 2))
                {
                    return;
                }
            }
        }
    }
    gameOver = true;
}

bool MarbleSolitaireBoard::isGameOver() const { return gameOver; }
int MarbleSolitaireBoard::getRemainingMarbles() const { return remainingMarbles; }
time_t MarbleSolitaireBoard::getElapsedTime() const { return time(NULL) - startTime; }
CellState MarbleSolitaireBoard::getCellState(int row, int col) const
{
    if (row < 0 || row >= 7 || col < 0 || col >= 7)
    {
        return INVALID;
    }
    return board[row][col];
}

void MarbleSolitaireBoard::selectCell(int row, int col)
{
    selectedRow = row;
    selectedCol = col;
}

void MarbleSolitaireBoard::clearSelection()
{
    selectedRow = selectedCol = -1;
}

bool MarbleSolitaireBoard::hasSelection() const
{
    return selectedRow != -1 && selectedCol != -1;
}

int MarbleSolitaireBoard::getSelectedRow() const { return selectedRow; }
int MarbleSolitaireBoard::getSelectedCol() const { return selectedCol; }
