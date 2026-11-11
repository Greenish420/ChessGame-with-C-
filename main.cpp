#include <iostream>
#include <string>
#include <cctype>
#include "raylib.h"

using namespace std;

enum Piece
{
    None = 0,
    King = 1,
    Queen = 2,
    Rook = 3,
    Bishop = 4,
    Knight = 5,
    Pawn = 6,

    White = 8,
    Black = 16,

    TypeMask = 7,
    ColorMask = 24
};

enum moveType
{
    MoveNormal = 0,
    MoveCastling,
    MoveEnPassant,
    MovePromotion
};

struct Move
{
    int from[2];
    int to[2];

    int piece;

    int whatsOnDest;

    moveType moveType = MoveNormal;
};

int charToCode(char c)
{
    switch (c)
    {
    case 'k':
        return King;

    case 'q':
        return Queen;

    case 'n':
        return Knight;

    case 'b':
        return Bishop;

    case 'p':
        return Pawn;

    case 'r':
        return Rook;

    default:
        return None;
    }
}

char codeToChar(int c)
{
    switch (c & TypeMask)
    {
    case King:
        return 'k';

    case Queen:
        return 'q';

    case Knight:
        return 'n';

    case Bishop:
        return 'b';

    case Pawn:
        return 'p';

    case Rook:
        return 'r';

    default:
        return '.';
    }
}

void stringToBoard(string layout, int Board[8][8])
{

    int i = 0;
    int j = 0;

    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            Board[row][col] = None;

    for (char c : layout)
    {

        if (c == ' ')
            return; // temporary

        if (c == '/')
        {
            i++;
            j = 0;
            continue;
        }

        if (isdigit(c))
        {

            j += c - '0';

            continue;
        }

        if (isupper(c))
        {
            Board[i][j] = charToCode(tolower(c)) | White;
            j++;
            continue;
        }
        else
        {
            Board[i][j] = charToCode(tolower(c)) | Black;
            j++;
            continue;
        }
    }
}

void printBoard(int Board[8][8])
{
    cout << "[" << endl
         << "   ";

    for (int i = 0; i < 8; i++)
    {
        cout << (char)('a' + i) << ", ";
    }
    cout << endl;

    for (int i = 0; i < 8; i++)
    {
        cout << 8 - i << ", ";
        for (int j = 0; j < 8; j++)
        {
            cout << (((Board[i][j] & White) == 8) ? (char)toupper(codeToChar(Board[i][j])) : codeToChar(Board[i][j])) << ", ";
        }
        cout << endl;
    }
    cout << "]" << endl;
}

void movePiece(int Board[8][8], int from[2], int to[2])
{
    Board[to[0]][to[1]] = Board[from[0]][from[1]];

    Board[from[0]][from[1]] = None;
}

int *cordsToIndex(string cords)
{

    if (cords.size() > 2 || cords.size() < 2)
        return NULL;

    int i = (int)(tolower(cords[0]) - 'a');
    int j = (int)(8 - ((int)cords[1] - '0'));

    if (!(i >= 0 && i <= 7 && j >= 0 && j <= 7))
    {
        cout << "(cordsToIndex) Out of Range";
        return NULL;
    }

    int *outPut = new int[2];
    outPut[0] = j;
    outPut[1] = i;

    return outPut;
}

void drawPieces(int SQ)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Color color = (((i + j) % 2) == 0) ? BLACK : WHITE;
            DrawRectangle(j * SQ, i * SQ, SQ, SQ, color);
        }
    }
}

bool checkInBetween(int Board[8][8], int destenation[2], int origin[2])
{
    int xStep;
    if ((destenation[0] - origin[0]) > 0)
        xStep = 1;
    else if ((destenation[0] - origin[0]) < 0)
        xStep = -1;
    else
        xStep = 0;

    int yStep;

    if ((destenation[1] - origin[1]) > 0)
        yStep = 1;
    else if ((destenation[1] - origin[1]) < 0)
        yStep = -1;
    else
        yStep = 0;

    int x = origin[0], y = origin[1];

    while (x != destenation[0] || y != destenation[1])
    {

        x += xStep;
        y += yStep;

        if (Board[x][y] != None && (x != destenation[0] || y != destenation[1]))
            return false;
    }

    return true;
}

bool validateMove(Move &move, int Board[8][8], int epNote[2])
{

    int rowDiff = move.to[0] - move.from[0];
    int colDiff = move.to[1] - move.from[1];
    bool isRock = (((rowDiff) == 0) || ((colDiff) == 0));
    bool isBishop = (abs(rowDiff) == abs(colDiff));

    if (((rowDiff) == 0) && ((colDiff) == 0))
        return false;

    if ((move.whatsOnDest & ColorMask) == (move.piece & ColorMask))
        return false;

    switch ((move.piece & TypeMask))
    {
    case Rook:
        if (isRock && checkInBetween(Board, move.to, move.from))
        {
            return true;
        }
        else
        {
            return false;
        }

    case Bishop:
        if (isBishop && checkInBetween(Board, move.to, move.from))
        {
            return true;
        }
        else
        {
            return false;
        }
    case Queen:
        if ((isRock || isBishop) && checkInBetween(Board, move.to, move.from))
        {
            return true;
        }
        else
            return false;

    case King:
        if (abs(rowDiff) <= 1 && abs(colDiff) <= 1)
            return true;
        else
            return false;

    case Knight:

        if ((abs(rowDiff) == 1 && abs(colDiff) == 2) || (abs(rowDiff) == 2 && abs(colDiff) == 1))
            return true;
        else
            return false;

    case Pawn:
    {
        bool isWhite = ((move.piece & ColorMask) == 8);
        int dir = 0;
        if (isWhite)
            dir = -1;
        else
            dir = 1;

        bool isStartingRow = (isWhite && move.from[0] == 6) || (!isWhite && move.from[0] == 1);

        if (move.to[1] == move.from[1] && move.to[0] == move.from[0] + dir)
        {
            if (Board[move.to[0]][move.to[1]] == None)
            {
                return true;
            }
        }

        else if (move.to[1] == move.from[1] && move.to[0] == move.from[0] + (2 * dir))
        {
            if (isStartingRow &&
                Board[move.from[0] + dir][move.from[1]] == None && // Check square in between
                Board[move.to[0]][move.to[1]] == None)
            {
                return true;
            }
        }

        else if (abs(move.to[1] - move.from[1]) == 1 && move.to[0] == move.from[0] + dir)
        {
            if (Board[move.to[0]][move.to[1]] != None)
            {
                return true;
            }
            else if (move.to[0] == epNote[0] && move.to[1] == epNote[1])
            {
                move.moveType = MoveEnPassant;
                return true;
            }
        }

        return false;
    }

    default:
        return false;
    }
}

bool findKing(int Board[8][8], int color, int &row, int &col)
{
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            if ((Board[r][c] & TypeMask) == King && (Board[r][c] & ColorMask) == color)
            {
                row = r;
                col = c;
                return true;
            }
        }
    }
    row = -1;
    col = -1;
    return false;
}

bool isThisSquareAttacked(int Board[8][8], bool attackerIsWhite, int TargetSquare[2], int epNote[2])
{
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            int square = Board[row][col];
            if ((square != None) && ((attackerIsWhite && ((square & ColorMask) == 8)) || (!attackerIsWhite && ((square & ColorMask) == 16))))
            {
                Move move;
                move.from[0] = row;
                move.from[1] = col;

                move.to[0] = TargetSquare[0];
                move.to[1] = TargetSquare[1];

                move.piece = square;

                move.whatsOnDest = Board[TargetSquare[0]][TargetSquare[1]];

                move.moveType = MoveNormal;

                if (validateMove(move, Board, epNote))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool leavesKingInCheck(int Board[8][8], Move move, bool moverIsWhite)
{
    int copy[8][8];
    int kingRow = -1;
    int kingCol = -1;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            copy[i][j] = Board[i][j];

            if (((copy[i][j] & TypeMask) == King) && ((copy[i][j] & ColorMask) == (moverIsWhite ? White : Black)))
            {
                kingRow = i;
                kingCol = j;
            }
        }
    }

    copy[move.to[0]][move.to[1]] = move.piece;
    copy[move.from[0]][move.from[1]] = None;

    if (move.moveType == MoveEnPassant)
        copy[move.from[0]][move.to[1]] = None;

    if ((move.piece & TypeMask) == King)
    {
        kingRow = move.to[0];
        kingCol = move.to[1];
    }

    if (kingCol == -1 && kingRow == -1)
        return false;

    int kingSquare[2] = {kingRow, kingCol};
    int noNote[2] = {-1, -1};
    if(isThisSquareAttacked(copy, !moverIsWhite, kingSquare, noNote))
        cout << "Check";

    return isThisSquareAttacked(copy, !moverIsWhite, kingSquare, noNote);
}

bool hasLegalMove(int Board[8][8], bool sideIsWhite, int epNote[2])
{

    for (int fromRow = 0; fromRow < 8; fromRow++)
    {
        for (int fromCol = 0; fromCol < 8; fromCol++)
        {
            if ((Board[fromRow][fromCol] == None) || (Board[fromRow][fromCol] & ColorMask) == (!sideIsWhite ? White : Black))
                continue;

            for (int toRow = 0; toRow < 8; toRow++)
            {
                for (int toCol = 0; toCol < 8; toCol++)
                {

                    Move move;
                    move.from[0] = fromRow;
                    move.from[1] = fromCol;
                    move.to[0] = toRow;
                    move.to[1] = toCol;
                    move.piece = Board[fromRow][fromCol];
                    move.whatsOnDest = Board[toRow][toCol];
                    move.moveType = MoveNormal;

                    if (validateMove(move, Board, epNote) && !leavesKingInCheck(Board, move, sideIsWhite))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void handleMoving(int Board[8][8], bool &isHolding, int &whatsBeingHeld, int from[2], int to[2], Move &move, Sound moveSFX, bool &whiteTurn, int epNote[2])
{

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {

        int x = GetMouseX() / 128;
        int y = GetMouseY() / 128;

        if (x > 7 || x < 0 || y > 7 || y < 0)
            return;

        if (!isHolding)
        {

            if (Board[y][x] != None)
            {

                from[0] = y;
                from[1] = x;
                isHolding = true;
                whatsBeingHeld = Board[y][x];
                Board[y][x] = None;
            }
        }
        else
        {

            to[0] = y;
            to[1] = x;

            move.from[0] = from[0];
            move.from[1] = from[1];

            move.to[0] = to[0];
            move.to[1] = to[1];

            move.piece = whatsBeingHeld;
            move.whatsOnDest = Board[y][x];

            move.moveType = MoveNormal;

            bool moverIsWhite = (move.piece & ColorMask) == White;
            bool rightTurn = ((((move.piece & ColorMask) == 8 && whiteTurn) || ((move.piece & ColorMask) == 16 && !whiteTurn)));

            if (rightTurn && validateMove(move, Board, epNote) && !leavesKingInCheck(Board, move, moverIsWhite))
            {
                whiteTurn = !whiteTurn;
                Board[to[0]][to[1]] = whatsBeingHeld;
                whatsBeingHeld = None;
                isHolding = false;
                PlaySound(moveSFX);

                if (move.moveType == MoveEnPassant)
                {
                    Board[move.from[0]][move.to[1]] = None;
                }

                epNote[0] = -1;
                epNote[1] = -1;

                if ((move.piece & TypeMask) == Pawn && abs(move.to[0] - move.from[0]) == 2)
                {
                    epNote[0] = (move.from[0] + move.to[0]) / 2;
                    epNote[1] = from[1];
                }

                int kingCords[2] = {-1, -1};
                findKing(Board, whiteTurn ? White : Black, kingCords[0], kingCords[1]);

                if (!hasLegalMove(Board, whiteTurn, epNote))
                {
                    int noNote[2] = {-1, -1};
                    if (isThisSquareAttacked(Board, !whiteTurn, kingCords, noNote))
                        cout << "checkMate";

                    else
                        cout << "staleMate";
                }
            }
            else
            {
                Board[from[0]][from[1]] = whatsBeingHeld;
                whatsBeingHeld = None;
                isHolding = false;
            }
        }
    }
}

void handlePieceFollow(int whatsBeingHeld, Texture2D Pieces[2][7])
{
    DrawTextureEx(Pieces[((whatsBeingHeld & ColorMask) / 8) - 1][(whatsBeingHeld & TypeMask)], {GetMouseX() - 64, GetMouseY() - 64}, 0, 4.0f, WHITE);
}

int main()
{

    int Board[8][8];
    stringToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", Board);

    InitWindow(1024, 1024, "Chess");
    InitAudioDevice();

    Sound moveSFX = LoadSound("SFX/sound.wav");

    Texture2D BoardTexture = LoadTexture("Resources/Board.png");
    Texture2D Pieces[2][7];
    for (int i = 0; i < 2; i++)
    {

        for (int j = 1; j < 7; j++)
        {
            string path = ("Resources/Pieces/" + to_string(i) + "/" + to_string(j) + ".png");
            Pieces[i][j] = LoadTexture(path.c_str());
        }
    }
    SetTargetFPS(60);

    bool isHolding = false;
    int whatsBeingHeld = None;
    int from[2];
    int to[2];
    Move move;
    bool whiteTurn = true;
    int epNote[2] = {-1, -1};
    
    while (!WindowShouldClose())
    {

        BeginDrawing();

        handleMoving(Board, isHolding, whatsBeingHeld, from, to, move, moveSFX, whiteTurn, epNote);

        ClearBackground(BEIGE);

        DrawTextureEx(BoardTexture, {0, 0}, 0, 4.0f, WHITE);
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {

                if (Board[i][j] == None)
                    continue;

                if ((Board[i][j] & ColorMask) == 8)
                {
                    DrawTextureEx(Pieces[(White / 8) - 1][(Board[i][j] & TypeMask)], {j * 128, i * 128}, 0, 4.0f, WHITE);
                }
                else
                {
                    DrawTextureEx(Pieces[(Black / 8) - 1][(Board[i][j] & TypeMask)], {j * 128, i * 128}, 0, 4.0f, WHITE);
                }
            }
        }
        if (isHolding)
            handlePieceFollow(whatsBeingHeld, Pieces);
        EndDrawing();
    }

    UnloadTexture(BoardTexture);
    CloseWindow();

    return 0;
}