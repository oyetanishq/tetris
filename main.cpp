#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _WIN32
void clearScreen() { system("cls"); }
char getKey() { return _kbhit() ? _getch() : '\0'; }
#else
void clearScreen() { system("clear"); }

char getKey() {
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
        char ch;
        read(STDIN_FILENO, &ch, 1);
        return ch;
    }
    return '\0';
}

void enableRawMode() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disableRawMode() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}
#endif

typedef std::vector<std::vector<int>> Board;
typedef std::vector<std::vector<int>> Piece;

void pc(Board board, int score) {
    clearScreen();

    for (int i = 0; i < board.size(); ++i) {
        for (int j = 0; j < board[i].size(); ++j)
            std::cout << (!j ? "<!" : "") << (board[i][j] ? "[]" : " .") << (j == board[i].size() - 1 ? " !>" : "");

        std::cout << std::endl;
    }

    std::string bar(board[0].size() * 2 + 1, '=');

    std::cout << "<!" << bar << "!>" << std::endl;
    std::cout << "score: " << score << std::endl;
}

Piece pieces[] = {
    {
        {0,0,0},
        {1,1,0},
        {1,1,0}
    },
    {
        {1,1,1},
        {1,0,0},
        {0,0,0}
    },
    {
        {1,1,1},
        {0,0,0},
        {0,0,0}
    },
    {
        {1,1,1},
        {0,0,1},
        {0,0,0}
    },
    {
        {1,1,0},
        {0,1,1},
        {0,0,0}
    },
    {
        {0,1,1},
        {1,1,0},
        {0,0,0}
    },
    {
        {1,1,1},
        {0,1,0},
        {0,0,0}
    }
};

Board addPiece(Board board, Piece piece, std::pair<int, int> position) {
    for (int i = position.first, x = 0; i < position.first + 3; ++i, ++x)
        for (int j = position.second, y = 0; j < position.second + 3; ++j, ++y)
            if (i < board.size() && j < board[0].size())
                board[i][j] += piece[x][y];

    return board;
}

bool checkValidMove(Board board, Piece piece, std::pair<int, int> position) {
    if (position.first < 0 || position.second < 0) return false;

    try {
        for (int i = position.first, x = 0; i < position.first + 3; ++i, ++x)
            for (int j = position.second, y = 0; j < position.second + 3; ++j, ++y) {
                if (
                    ((position.first + x >= board.size()) && piece[x][y]) ||
                    ((position.second + y >= board[0].size()) && piece[x][y])
                    )
                    return false;

                if (i < board.size() && j < board[0].size()) {
                    board[i][j] += piece[x][y];

                    if (board[i][j] > 1)
                        return false;
                }
            }

        return true;
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return false;
    }
}

bool isMoveSet(Board board, Piece piece, std::pair<int, int> position) {
    try {
        for (int i = position.first, x = 0; i < position.first + 3; ++i, ++x)
            for (int j = position.second, y = 0; j < position.second + 3; ++j, ++y) {
                if ((position.first + x >= board.size()) && piece[x][y])
                    return true;

                if (i < board.size() && j < board[0].size()) {
                    board[i][j] += piece[x][y];

                    if (board[i][j] > 1)
                        return true;
                }
            }

        return false;
    }
    catch (std::exception& e) {
        return true;
    }
}

bool isOver(Board& board) {
    for (int i = 0; i < board[0].size(); ++i)
        if (board[0][i]) return true;

    return false;
}

Piece rotateClockwise(Piece matrix, int times = 1) {
    times %= 4;

    while (times--) {
        // Step 1: Transpose the matrix
        for (int i = 0; i < matrix.size(); ++i)
            for (int j = i + 1; j < matrix[0].size(); ++j)
                std::swap(matrix[i][j], matrix[j][i]);

        // Step 2: Reverse each row
        for (auto& row : matrix)
            reverse(row.begin(), row.end());
    }

    return matrix;
}

Board checkBarAndDelete(Board& board) {
    Board newBoard(board.size(), (std::vector<int>(board[0].size(), 0)));

    for (int i = board.size() - 1, x = board.size() - 1; i >= 0; --i) {
        int count = 0;

        for (int j = 0; j < board[0].size(); ++j)
            if (board[i][j]) count++;

        if (count != board[0].size()) {
            for (int j = 0; j < board[0].size(); ++j)
                newBoard[x][j] = board[i][j];

            x--;
        }
    }

    return newBoard;
}

signed main() {
    /**
     * Board Initialize
     */
    int horizontal = 10, vertical = 15;
    int score = 0;
    Board board(vertical, (std::vector<int>(horizontal, 0)));

    /**
     * Refresh every 2 seconds
     */

#ifndef _WIN32
    enableRawMode();
#endif

    std::pair<int, int> pos = { 0, 6 };
    int rotation = 0;
    int piece = -1;
    srand(time(0));

    while (true) {
        int ch = getKey();

        if (piece < 0) {
            piece = std::rand() % 7;
            pos = { 0, (rand() % (horizontal - 1)) };
            score++;
        }
        else pos.first++;

        std::pair<int, int> oldPos = pos;
        int oldRotation = rotation;

        switch (ch) {
        case 119: rotation++; break;
        case 97: pos.second--; break;
        case 100: pos.second++; break;
        case 115: pos.first++; break;
        }

        Piece pieceRotated = rotateClockwise(pieces[piece], rotation);

        if (checkValidMove(board, pieceRotated, pos)) pc(addPiece(board, pieceRotated, pos), score);
        else { pos = oldPos; rotation = oldRotation; }

        if (isMoveSet(board, pieceRotated, pos)) {
            piece = -1;
            pos.first--;
            board = addPiece(board, pieceRotated, pos);
            board = checkBarAndDelete(board);

            if (isOver(board))
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(600));
    }

#ifndef _WIN32
    disableRawMode();
#endif

    std::cout << std::endl;

    return 0;
}
