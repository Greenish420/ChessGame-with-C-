#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
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
            return;

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
            Board[i][j] = charToCode(tolower(c)) | White;
        else
            Board[i][j] = charToCode(tolower(c)) | Black;
        j++;
    }
}

bool checkInBetween(int Board[8][8], int destenation[2], int origin[2])
{
    int xStep = 0;
    if ((destenation[0] - origin[0]) > 0)
        xStep = 1;
    else if ((destenation[0] - origin[0]) < 0)
        xStep = -1;

    int yStep = 0;
    if ((destenation[1] - origin[1]) > 0)
        yStep = 1;
    else if ((destenation[1] - origin[1]) < 0)
        yStep = -1;

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
    bool isRock = (rowDiff == 0) || (colDiff == 0);
    bool isBishop = (abs(rowDiff) == abs(colDiff));

    if (rowDiff == 0 && colDiff == 0)
        return false;

    if ((move.whatsOnDest & ColorMask) == (move.piece & ColorMask))
        return false;

    switch (move.piece & TypeMask)
    {
    case Rook:
        return isRock && checkInBetween(Board, move.to, move.from);

    case Bishop:
        return isBishop && checkInBetween(Board, move.to, move.from);

    case Queen:
        return (isRock || isBishop) && checkInBetween(Board, move.to, move.from);

    case King:
        return abs(rowDiff) <= 1 && abs(colDiff) <= 1;

    case Knight:
        return (abs(rowDiff) == 1 && abs(colDiff) == 2) || (abs(rowDiff) == 2 && abs(colDiff) == 1);

    case Pawn:
    {
        bool isWhite = ((move.piece & ColorMask) == White);
        int dir = isWhite ? -1 : 1;
        bool isStartingRow = (isWhite && move.from[0] == 6) || (!isWhite && move.from[0] == 1);

        if (move.to[1] == move.from[1] && move.to[0] == move.from[0] + dir)
        {
            if (Board[move.to[0]][move.to[1]] == None)
                return true;
        }
        else if (move.to[1] == move.from[1] && move.to[0] == move.from[0] + (2 * dir))
        {
            if (isStartingRow &&
                Board[move.from[0] + dir][move.from[1]] == None &&
                Board[move.to[0]][move.to[1]] == None)
                return true;
        }
        else if (abs(move.to[1] - move.from[1]) == 1 && move.to[0] == move.from[0] + dir)
        {
            if (Board[move.to[0]][move.to[1]] != None)
                return true;
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
    int attackerColor = attackerIsWhite ? White : Black;

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            int square = Board[row][col];
            if (square != None && (square & ColorMask) == attackerColor)
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
                    return true;
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

    return isThisSquareAttacked(copy, !moverIsWhite, kingSquare, noNote);
}

bool hasLegalMove(int Board[8][8], bool sideIsWhite, int epNote[2])
{
    int myColor = sideIsWhite ? White : Black;

    for (int fromRow = 0; fromRow < 8; fromRow++)
    {
        for (int fromCol = 0; fromCol < 8; fromCol++)
        {
            int piece = Board[fromRow][fromCol];
            if (piece == None || (piece & ColorMask) != myColor)
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
                    move.piece = piece;
                    move.whatsOnDest = Board[toRow][toCol];
                    move.moveType = MoveNormal;

                    if (validateMove(move, Board, epNote) && !leavesKingInCheck(Board, move, sideIsWhite))
                        return true;
                }
            }
        }
    }

    return false;
}


bool pawnAttacks(int Board[8][8], int row, int col, bool attackerIsWhite)
{
    int pawn = (attackerIsWhite ? White : Black) | Pawn;
    int pawnRow = attackerIsWhite ? row + 1 : row - 1;
    if (pawnRow < 0 || pawnRow > 7)
        return false;

    for (int dc = -1; dc <= 1; dc += 2)
    {
        int c = col + dc;
        if (c >= 0 && c <= 7 && Board[pawnRow][c] == pawn)
            return true;
    }
    return false;
}

bool squareUnsafe(int Board[8][8], int row, int col, bool attackerIsWhite)
{
    int target[2] = {row, col};
    int noNote[2] = {-1, -1};
    return isThisSquareAttacked(Board, attackerIsWhite, target, noNote) ||
           pawnAttacks(Board, row, col, attackerIsWhite);
}

// side: 0 = queenside, 1 = kingside
bool canCastle(int Board[8][8], bool whiteKing, int side, bool rights[4])
{
    int index = (whiteKing ? 0 : 2) + side;
    int row = whiteKing ? 7 : 0;
    int myColor = whiteKing ? White : Black;

    if (!rights[index])
        return false;

    int rookCol = side == 1 ? 7 : 0;
    if (Board[row][rookCol] != (myColor | Rook))
        return false;

    int firstEmpty = side == 1 ? 5 : 1;
    int lastEmpty = side == 1 ? 6 : 3;
    for (int c = firstEmpty; c <= lastEmpty; c++)
        if (Board[row][c] != None)
            return false;

    int safeCols[3] = {4, side == 1 ? 5 : 3, side == 1 ? 6 : 2};
    for (int i = 0; i < 3; i++)
        if (squareUnsafe(Board, row, safeCols[i], !whiteKing))
            return false;

    return true;
}

void clearCornerRight(int row, int col, bool rights[4])
{
    if (row == 7 && col == 0)
        rights[0] = false;
    if (row == 7 && col == 7)
        rights[1] = false;
    if (row == 0 && col == 0)
        rights[2] = false;
    if (row == 0 && col == 7)
        rights[3] = false;
}

void checkGameEnd(int Board[8][8], bool whiteTurn, int epNote[2])
{
    if (!hasLegalMove(Board, whiteTurn, epNote))
    {
        int kingCords[2] = {-1, -1};
        findKing(Board, whiteTurn ? White : Black, kingCords[0], kingCords[1]);
        int noNote[2] = {-1, -1};
        if (isThisSquareAttacked(Board, !whiteTurn, kingCords, noNote))
            cout << "checkMate" << endl;
        else
            cout << "staleMate" << endl;
    }
}

void handleMoving(int Board[8][8], bool &isHolding, int &whatsBeingHeld, int from[2], int to[2], Move &move,
                  Sound moveSFX, bool &whiteTurn, int epNote[2], bool castleRights[4],
                  bool &promoting, int &promoRow, int &promoCol)
{
    if (promoting)
        return;

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
            bool rightTurn = (moverIsWhite && whiteTurn) || (!moverIsWhite && !whiteTurn);

            bool isCastle = (move.piece & TypeMask) == King &&
                            from[0] == (moverIsWhite ? 7 : 0) && from[1] == 4 &&
                            to[0] == from[0] && abs(to[1] - from[1]) == 2;

            bool legal;
            if (isCastle)
            {
                legal = rightTurn && canCastle(Board, moverIsWhite, to[1] == 6 ? 1 : 0, castleRights);
                if (legal)
                    move.moveType = MoveCastling;
            }
            else
            {
                legal = rightTurn && validateMove(move, Board, epNote) && !leavesKingInCheck(Board, move, moverIsWhite);
            }

            if (legal)
            {
                whiteTurn = !whiteTurn;
                Board[to[0]][to[1]] = whatsBeingHeld;
                whatsBeingHeld = None;
                isHolding = false;
                PlaySound(moveSFX);

                if (move.moveType == MoveEnPassant)
                    Board[move.from[0]][move.to[1]] = None;

                if (move.moveType == MoveCastling)
                {
                    int r = move.to[0];
                    if (move.to[1] == 6)
                    {
                        Board[r][5] = Board[r][7];
                        Board[r][7] = None;
                    }
                    else
                    {
                        Board[r][3] = Board[r][0];
                        Board[r][0] = None;
                    }
                }

                if ((move.piece & TypeMask) == King)
                {
                    int base = moverIsWhite ? 0 : 2;
                    castleRights[base] = false;
                    castleRights[base + 1] = false;
                }
                clearCornerRight(move.from[0], move.from[1], castleRights);
                clearCornerRight(move.to[0], move.to[1], castleRights);

                epNote[0] = -1;
                epNote[1] = -1;

                if ((move.piece & TypeMask) == Pawn && abs(move.to[0] - move.from[0]) == 2)
                {
                    epNote[0] = (move.from[0] + move.to[0]) / 2;
                    epNote[1] = from[1];
                }

                if ((move.piece & TypeMask) == Pawn && (to[0] == 0 || to[0] == 7))
                {
                    promoting = true;
                    promoRow = to[0];
                    promoCol = to[1];
                }
                else
                    checkGameEnd(Board, whiteTurn, epNote);
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
    DrawTextureEx(Pieces[((whatsBeingHeld & ColorMask) / 8) - 1][(whatsBeingHeld & TypeMask)],
                  {(float)(GetMouseX() - 64), (float)(GetMouseY() - 64)}, 0, 4.0f, WHITE);
}

const int promoOptions[4] = {Queen, Rook, Bishop, Knight};

void handlePromotion(int Board[8][8], bool &promoting, int promoRow, int promoCol, bool whiteTurn, int epNote[2])
{
    if (!promoting || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return;

    int col = GetMouseX() / 128;
    int my = GetMouseY();

    
    if (my < 448 || my >= 576 || col < 2 || col > 5)
        return;

    int color = Board[promoRow][promoCol] & ColorMask;
    Board[promoRow][promoCol] = color | promoOptions[col - 2];
    promoting = false;
    checkGameEnd(Board, whiteTurn, epNote);
}

void drawPromotionMenu(int Board[8][8], int promoRow, int promoCol, Texture2D Pieces[2][7])
{
    int ci = ((Board[promoRow][promoCol] & ColorMask) / 8) - 1;

    DrawRectangle(2 * 128, 448, 4 * 128, 128, Fade(BLACK, 0.8f));
    for (int i = 0; i < 4; i++)
        DrawTextureEx(Pieces[ci][promoOptions[i]], {(float)((i + 2) * 128), 448.0f}, 0, 4.0f, WHITE);
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
    bool castleRights[4] = {true, true, true, true}; // wQ, wK, bQ, bK
    bool promoting = false;
    int promoRow = -1, promoCol = -1;

    while (!WindowShouldClose())
    {
       
        handlePromotion(Board, promoting, promoRow, promoCol, whiteTurn, epNote);
        handleMoving(Board, isHolding, whatsBeingHeld, from, to, move, moveSFX,
                     whiteTurn, epNote, castleRights, promoting, promoRow, promoCol);

        BeginDrawing();
        ClearBackground(BEIGE);

        DrawTextureEx(BoardTexture, {0, 0}, 0, 4.0f, WHITE);
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (Board[i][j] == None)
                    continue;

                int ci = ((Board[i][j] & ColorMask) / 8) - 1;
                DrawTextureEx(Pieces[ci][Board[i][j] & TypeMask],
                              {(float)(j * 128), (float)(i * 128)}, 0, 4.0f, WHITE);
            }
        }

        if (isHolding)
            handlePieceFollow(whatsBeingHeld, Pieces);

        if (promoting)
            drawPromotionMenu(Board, promoRow, promoCol, Pieces);

        EndDrawing();
    }

    UnloadTexture(BoardTexture);
    for (int i = 0; i < 2; i++)
        for (int j = 1; j < 7; j++)
            UnloadTexture(Pieces[i][j]);
    UnloadSound(moveSFX);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}