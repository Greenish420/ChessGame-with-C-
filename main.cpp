#include <iostream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
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

enum GameState
{
    Playing,
    Checkmate,
    Stalemate
};

struct Move
{
    int from[2];
    int to[2];

    int piece;

    int whatsOnDest;

    moveType moveType = MoveNormal;
};

const char *startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const Rectangle replayButton = {412, 580, 200, 70};
const int promoOptions[4] = {Queen, Rook, Bishop, Knight};


const string SPRITE_ROOT = "Resources/Sprites/";
const string UI_STYLE = "maple_burl";

struct BoardSprites
{
    Texture2D selected;
    Texture2D legalMove;
    Texture2D capture;
    Texture2D lastMove;
    Texture2D files[8]; // A..H
    Texture2D ranks[8]; // 1..8 (index 0 = rank 1)
};

BoardSprites loadBoardSprites()
{
    BoardSprites s;
    string hl = SPRITE_ROOT + "Board_Utilities/Highlight_Styles/" + UI_STYLE + "/";
    string co = SPRITE_ROOT + "Board_Utilities/Coordinate_Styles/" + UI_STYLE + "/";

    s.selected = LoadTexture((hl + "selected.png").c_str());
    s.legalMove = LoadTexture((hl + "legal_move.png").c_str());
    s.capture = LoadTexture((hl + "capture.png").c_str());
    s.lastMove = LoadTexture((hl + "last_move.png").c_str());

    for (int i = 0; i < 8; i++)
    {
        string file(1, (char)('A' + i));
        s.files[i] = LoadTexture((co + file + ".png").c_str());
        s.ranks[i] = LoadTexture((co + to_string(i + 1) + ".png").c_str());
    }
    return s;
}

void unloadBoardSprites(BoardSprites &s)
{
    UnloadTexture(s.selected);
    UnloadTexture(s.legalMove);
    UnloadTexture(s.capture);
    UnloadTexture(s.lastMove);
    for (int i = 0; i < 8; i++)
    {
        UnloadTexture(s.files[i]);
        UnloadTexture(s.ranks[i]);
    }
}

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

void computeLegalMoves(int Board[8][8], int piece, int from[2], int epNote[2], bool castleRights[4], bool legalDest[8][8])
{
    bool moverIsWhite = (piece & ColorMask) == White;

    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Move probe;
            probe.from[0] = from[0];
            probe.from[1] = from[1];
            probe.to[0] = r;
            probe.to[1] = c;
            probe.piece = piece;
            probe.whatsOnDest = Board[r][c];
            probe.moveType = MoveNormal;

            legalDest[r][c] = validateMove(probe, Board, epNote) && !leavesKingInCheck(Board, probe, moverIsWhite);
        }
    }

    int homeRow = moverIsWhite ? 7 : 0;
    if ((piece & TypeMask) == King && from[0] == homeRow && from[1] == 4)
    {
        if (canCastle(Board, moverIsWhite, 1, castleRights))
            legalDest[homeRow][6] = true;
        if (canCastle(Board, moverIsWhite, 0, castleRights))
            legalDest[homeRow][2] = true;
    }
}

void clearLegalMoves(bool legalDest[8][8])
{
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            legalDest[r][c] = false;
}

void checkGameEnd(int Board[8][8], bool whiteTurn, int epNote[2], GameState &gameState)
{
    if (!hasLegalMove(Board, whiteTurn, epNote))
    {
        int kingCords[2] = {-1, -1};
        findKing(Board, whiteTurn ? White : Black, kingCords[0], kingCords[1]);
        int noNote[2] = {-1, -1};
        if (isThisSquareAttacked(Board, !whiteTurn, kingCords, noNote))
            gameState = Checkmate;
        else
            gameState = Stalemate;

        
    }
}


void updateCheck(int Board[8][8], bool whiteTurn, int checkSq[2])
{
    checkSq[0] = -1;
    checkSq[1] = -1;

    int kr, kc;
    if (!findKing(Board, whiteTurn ? White : Black, kr, kc))
        return;

    int kingSquare[2] = {kr, kc};
    int noNote[2] = {-1, -1};
    if (isThisSquareAttacked(Board, !whiteTurn, kingSquare, noNote))
    {
        checkSq[0] = kr;
        checkSq[1] = kc;
    }
}

void handleMoving(int Board[8][8], bool &isHolding, int &whatsBeingHeld, int from[2], int to[2], Move &move,
                  Sound moveSFX, bool &whiteTurn, int epNote[2], bool castleRights[4],
                  bool &promoting, int &promoRow, int &promoCol, GameState &gameState, bool legalDest[8][8], int lastMove[4])
{
    if (promoting || gameState != Playing)
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

                bool pieceIsWhite = (whatsBeingHeld & ColorMask) == White;
                if (pieceIsWhite == whiteTurn)
                    computeLegalMoves(Board, whatsBeingHeld, from, epNote, castleRights, legalDest);
                else
                    clearLegalMoves(legalDest);
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

            clearLegalMoves(legalDest);

            if (legal)
            {
                whiteTurn = !whiteTurn;
                Board[to[0]][to[1]] = whatsBeingHeld;
                whatsBeingHeld = None;
                isHolding = false;

                
                PlaySound(moveSFX);

                lastMove[0] = move.from[0];
                lastMove[1] = move.from[1];
                lastMove[2] = move.to[0];
                lastMove[3] = move.to[1];

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
                {
                    checkGameEnd(Board, whiteTurn, epNote, gameState);

                    
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
    DrawTextureEx(Pieces[((whatsBeingHeld & ColorMask) / 8) - 1][(whatsBeingHeld & TypeMask)],
                  {(float)(GetMouseX() - 64), (float)(GetMouseY() - 64)}, 0, 4.0f, WHITE);
}

void handlePromotion(int Board[8][8], bool &promoting, int promoRow, int promoCol, bool whiteTurn, int epNote[2], GameState &gameState)
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
    checkGameEnd(Board, whiteTurn, epNote, gameState);

    
}

void drawPromotionMenu(int Board[8][8], int promoRow, int promoCol, Texture2D Pieces[2][7])
{
    int ci = ((Board[promoRow][promoCol] & ColorMask) / 8) - 1;

    DrawRectangle(2 * 128, 448, 4 * 128, 128, Fade(BLACK, 0.8f));
    for (int i = 0; i < 4; i++)
        DrawTextureEx(Pieces[ci][promoOptions[i]], {(float)((i + 2) * 128), 448.0f}, 0, 4.0f, WHITE);
}

void drawHighlights(bool isHolding, int from[2], bool legalDest[8][8], int Board[8][8],
                    int whatsBeingHeld, int lastMove[4], int checkSq[2], const BoardSprites &sprites)
{
    
    if (lastMove[0] >= 0)
    {
        DrawTextureEx(sprites.lastMove, {(float)(lastMove[1] * 128), (float)(lastMove[0] * 128)}, 0, 4.0f, WHITE);
        DrawTextureEx(sprites.lastMove, {(float)(lastMove[3] * 128), (float)(lastMove[2] * 128)}, 0, 4.0f, WHITE);
    }

    if (checkSq[0] >= 0)
    {
        float x = (float)(checkSq[1] * 128);
        float y = (float)(checkSq[0] * 128);
        float pulse = 0.65f + 0.35f * sinf((float)GetTime() * 6.0f);
        DrawTextureEx(sprites.selected, {x, y}, 0, 4.0f, Fade(RED, pulse)); // gold frame tinted red
        DrawTextureEx(sprites.capture, {x, y}, 0, 4.0f, Fade(WHITE, pulse));
    }

    if (!isHolding)
        return;

    DrawTextureEx(sprites.selected, {(float)(from[1] * 128), (float)(from[0] * 128)}, 0, 4.0f, WHITE);

    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            if (!legalDest[r][c])
                continue;

            // en passant lands on an empty square but is still a capture
            bool isCapture = Board[r][c] != None ||
                             ((whatsBeingHeld & TypeMask) == Pawn && c != from[1]);

            Vector2 pos = {(float)(c * 128), (float)(r * 128)};
            DrawTextureEx(isCapture ? sprites.capture : sprites.legalMove, pos, 0, 4.0f, WHITE);
        }
    }
}


void drawCoordinates(const BoardSprites &sprites)
{
    for (int i = 0; i < 8; i++)
    {
        DrawTextureEx(sprites.ranks[7 - i], {-19.0f, (float)(i * 128 - 15)}, 0, 2.0f, WHITE);
        DrawTextureEx(sprites.files[i], {(float)(i * 128 + 81), (float)(7 * 128 + 77)}, 0, 2.0f, WHITE);
    }
}

void drawTurnIndicator(bool whiteTurn)
{
    Vector2 center = {1008.0f, 16.0f};
    DrawCircleV(center, 10, whiteTurn ? BLACK : RAYWHITE); // thin contrasting rim so it reads on any tile
    DrawCircleV(center, 8, whiteTurn ? RAYWHITE : BLACK);
}


const Color markFill[4] = {{235, 64, 52, 255}, {88, 180, 70, 255}, {70, 130, 230, 255}, {240, 200, 50, 255}};
const Color markRim[4] = {{120, 20, 20, 255}, {30, 90, 30, 255}, {20, 50, 120, 255}, {130, 100, 10, 255}};

struct Mark
{
    int from[2];
    int to[2]; // same as from -> tile mark, otherwise an arrow
    int color;
};

struct Annotations
{
    vector<Mark> marks;
    bool drawing = false;
    int start[2] = {-1, -1};
};

struct Pt
{
    float x, y;
};

bool mouseSquare(int &row, int &col)
{
    int mx = GetMouseX();
    int my = GetMouseY();
    if (mx < 0 || my < 0 || mx >= 1024 || my >= 1024)
        return false;
    col = mx / 128;
    row = my / 128;
    return true;
}

int currentMarkColor()
{
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
        return 1;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        return 2;
    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
        return 3;
    return 0;
}

void toggleMark(Annotations &a, int fromRow, int fromCol, int toRow, int toCol, int color)
{
    for (size_t i = 0; i < a.marks.size(); i++)
    {
        Mark &m = a.marks[i];
        if (m.from[0] == fromRow && m.from[1] == fromCol && m.to[0] == toRow && m.to[1] == toCol)
        {
            if (m.color == color)
                a.marks.erase(a.marks.begin() + i); // same again -> remove
            else
                m.color = color; // other color -> recolor
            return;
        }
    }
    Mark m = {{fromRow, fromCol}, {toRow, toCol}, color};
    a.marks.push_back(m);
}

void clearAnnotations(Annotations &a)
{
    a.marks.clear();
    a.drawing = false;
}

void handleAnnotations(Annotations &a)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        clearAnnotations(a);

    int row, col;
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && mouseSquare(row, col))
    {
        a.drawing = true;
        a.start[0] = row;
        a.start[1] = col;
    }

    if (a.drawing && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
    {
        a.drawing = false;
        if (mouseSquare(row, col))
            toggleMark(a, a.start[0], a.start[1], row, col, currentMarkColor());
    }
}

void drawTileMark(int row, int col, int color)
{
    float x = (float)(col * 128);
    float y = (float)(row * 128);
    DrawRectangle((int)x, (int)y, 128, 128, Fade(markFill[color], 0.55f));
    DrawRectangleLinesEx({x, y, 128.0f, 128.0f}, 4, Fade(markRim[color], 0.8f));
}

void drawTileMarks(const Annotations &a)
{
    for (const Mark &m : a.marks)
        if (m.from[0] == m.to[0] && m.from[1] == m.to[1])
            drawTileMark(m.from[0], m.from[1], m.color);
}

float distToSegment(Pt p, Pt a, Pt b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len2 = dx * dx + dy * dy;
    float t = len2 > 0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2 : 0.0f;
    t = max(0.0f, min(1.0f, t));
    float cx = a.x + t * dx - p.x;
    float cy = a.y + t * dy - p.y;
    return sqrtf(cx * cx + cy * cy);
}


float triangleDepth(Pt p, Pt a, Pt b, Pt c)
{
    float orient = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    float sign = orient > 0 ? 1.0f : -1.0f;
    Pt v[3] = {a, b, c};
    float best = 1e9f;
    for (int i = 0; i < 3; i++)
    {
        Pt p0 = v[i];
        Pt p1 = v[(i + 1) % 3];
        float ex = p1.x - p0.x;
        float ey = p1.y - p0.y;
        float len = sqrtf(ex * ex + ey * ey);
        float d = sign * (ex * (p.y - p0.y) - ey * (p.x - p0.x)) / len;
        best = min(best, d);
    }
    return best;
}


void drawPixelArrow(int fromRow, int fromCol, int toRow, int toCol, int color, float alpha)
{
    const float halfW = 3.5f;   // shaft half-width (art pixels)
    const float headLen = 12.0f;
    const float headHalf = 9.0f;

    Pt s = {fromCol * 32 + 16.0f, fromRow * 32 + 16.0f};
    Pt e = {toCol * 32 + 16.0f, toRow * 32 + 16.0f};

    int dr = abs(toRow - fromRow);
    int dc = abs(toCol - fromCol);
    bool knight = (dr == 1 && dc == 2) || (dr == 2 && dc == 1);

    Pt corner = s;
    Pt prev = s;
    if (knight)
    {
        corner = (dc == 2) ? Pt{e.x, s.y} : Pt{s.x, e.y}; // long leg first, like chess.com
        prev = corner;
    }

    float dx = e.x - prev.x;
    float dy = e.y - prev.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f)
        return;
    dx /= len;
    dy /= len;

    Pt base = {e.x - dx * headLen, e.y - dy * headLen};
    Pt wing1 = {base.x - dy * headHalf, base.y + dx * headHalf};
    Pt wing2 = {base.x + dy * headHalf, base.y - dx * headHalf};

    Pt shaft[3] = {s, base, base};
    int shaftPoints = 2;
    if (knight)
    {
        shaft[1] = corner;
        shaft[2] = base;
        shaftPoints = 3;
    }

    auto inside = [&](Pt p, float margin) -> bool
    {
        for (int i = 0; i < shaftPoints - 1; i++)
            if (distToSegment(p, shaft[i], shaft[i + 1]) <= halfW + margin)
                return true;
        return triangleDepth(p, e, wing1, wing2) >= -margin;
    };

    float pad = headHalf + 2.0f;
    int x0 = max(0, (int)floorf(min(min(s.x, e.x), corner.x) - pad));
    int x1 = min(255, (int)ceilf(max(max(s.x, e.x), corner.x) + pad));
    int y0 = max(0, (int)floorf(min(min(s.y, e.y), corner.y) - pad));
    int y1 = min(255, (int)ceilf(max(max(s.y, e.y), corner.y) + pad));

    Color fill = Fade(markFill[color], alpha);
    Color rim = Fade(markRim[color], alpha);

    for (int py = y0; py <= y1; py++)
    {
        for (int px = x0; px <= x1; px++)
        {
            Pt p = {px + 0.5f, py + 0.5f};
            if (inside(p, 0.0f))
                DrawRectangle(px * 4, py * 4, 4, 4, fill);
            else if (inside(p, 1.0f))
                DrawRectangle(px * 4, py * 4, 4, 4, rim);
        }
    }
}

void drawArrows(const Annotations &a)
{
    for (const Mark &m : a.marks)
        if (m.from[0] != m.to[0] || m.from[1] != m.to[1])
            drawPixelArrow(m.from[0], m.from[1], m.to[0], m.to[1], m.color, 0.8f);

    
    int row, col;
    if (a.drawing && mouseSquare(row, col) && (row != a.start[0] || col != a.start[1]))
        drawPixelArrow(a.start[0], a.start[1], row, col, currentMarkColor(), 0.6f);
}

void drawGameOver(GameState gameState, bool whiteTurn)
{
    if (gameState == Playing)
        return;

    DrawRectangle(0, 0, 1024, 1024, Fade(BLACK, 0.65f));

    const char *title = gameState == Checkmate ? "Checkmate" : "Stalemate";
    const char *subtitle;
    if (gameState == Checkmate)
        subtitle = whiteTurn ? "Black wins" : "White wins";
    else
        subtitle = "Draw";

    int titleWidth = MeasureText(title, 90);
    DrawText(title, (1024 - titleWidth) / 2, 380, 90, WHITE);

    int subWidth = MeasureText(subtitle, 50);
    DrawText(subtitle, (1024 - subWidth) / 2, 490, 50, LIGHTGRAY);

    bool hover = CheckCollisionPointRec(GetMousePosition(), replayButton);
    DrawRectangleRec(replayButton, hover ? LIGHTGRAY : RAYWHITE);
    int btnWidth = MeasureText("Replay", 36);
    DrawText("Replay", (int)(replayButton.x + (replayButton.width - btnWidth) / 2), (int)(replayButton.y + 17), 36, BLACK);
}

void resetGame(int Board[8][8], bool &whiteTurn, int epNote[2], bool castleRights[4], bool &promoting,
               bool &isHolding, int &whatsBeingHeld, GameState &gameState, bool legalDest[8][8],
               int lastMove[4], int checkSq[2])
{
    stringToBoard(startFEN, Board);
    whiteTurn = true;
    epNote[0] = -1;
    epNote[1] = -1;
    for (int i = 0; i < 4; i++)
        castleRights[i] = true;
    promoting = false;
    isHolding = false;
    whatsBeingHeld = None;
    gameState = Playing;
    clearLegalMoves(legalDest);
    for (int i = 0; i < 4; i++)
        lastMove[i] = -1;
    checkSq[0] = -1;
    checkSq[1] = -1;
}

int main()
{
    int Board[8][8];
    stringToBoard(startFEN, Board);

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
    BoardSprites sprites = loadBoardSprites();
    SetTargetFPS(60);

    bool isHolding = false;
    int whatsBeingHeld = None;
    int from[2] = {-1, -1};
    int to[2] = {-1, -1};
    Move move;
    bool whiteTurn = true;
    int epNote[2] = {-1, -1};
    bool castleRights[4] = {true, true, true, true};
    bool promoting = false;
    int promoRow = -1, promoCol = -1;
    GameState gameState = Playing;
    bool legalDest[8][8];
    clearLegalMoves(legalDest);
    int lastMove[4] = {-1, -1, -1, -1};
    int checkSq[2] = {-1, -1};
    Annotations notes;

    while (!WindowShouldClose())
    {
        if (gameState != Playing)
        {
            bool clickedReplay = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), replayButton);
            if (clickedReplay || IsKeyPressed(KEY_R))
            {
                resetGame(Board, whiteTurn, epNote, castleRights, promoting, isHolding, whatsBeingHeld, gameState, legalDest, lastMove, checkSq);
                clearAnnotations(notes);
            }
        }

        handleAnnotations(notes);
        handlePromotion(Board, promoting, promoRow, promoCol, whiteTurn, epNote, gameState);
        handleMoving(Board, isHolding, whatsBeingHeld, from, to, move, moveSFX,
                     whiteTurn, epNote, castleRights, promoting, promoRow, promoCol, gameState, legalDest, lastMove);

        if (promoting)
        {
            checkSq[0] = -1;
            checkSq[1] = -1;
        }
        else if (!isHolding)
            updateCheck(Board, whiteTurn, checkSq);

        BeginDrawing();
        ClearBackground(BEIGE);

        DrawTextureEx(BoardTexture, {0, 0}, 0, 4.0f, WHITE);

        drawTileMarks(notes);
        drawHighlights(isHolding, from, legalDest, Board, whatsBeingHeld, lastMove, checkSq, sprites);

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

        drawCoordinates(sprites);
        drawArrows(notes);

        if (isHolding)
            handlePieceFollow(whatsBeingHeld, Pieces);

        if (promoting)
            drawPromotionMenu(Board, promoRow, promoCol, Pieces);

        drawTurnIndicator(whiteTurn);
        drawGameOver(gameState, whiteTurn);

        EndDrawing();
    }

    UnloadTexture(BoardTexture);
    for (int i = 0; i < 2; i++)
        for (int j = 1; j < 7; j++)
            UnloadTexture(Pieces[i][j]);
    unloadBoardSprites(sprites);
    UnloadSound(moveSFX);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}