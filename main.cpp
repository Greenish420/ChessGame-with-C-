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
    cout << "[" << endl << "   ";
    
    for (int i = 0; i<8; i++){
        cout << (char)('a' + i) << ", ";
    }
    cout << endl;
    
    for (int i = 0; i < 8; i++)
    {
        cout << 8 - i <<", ";
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
    Board[to[0]][to[1]] =  Board[from[0]][from[1]];

    Board[from[0]][from[1]] = None;

}


int* cordsToIndex(string cords){

    if(cords.size() >  2 || cords.size() < 2)
        return NULL;

    int i = (int)(tolower(cords[0]) - 'a');
    int j = (int)(8 - ((int)cords[1] - '0'));

    if(!(i >= 0 && i <=7 && j >= 0 && j <= 7)){
        cout << "(cordsToIndex) Out of Range";
        return NULL;
    }

    int* outPut = new int[2];
    outPut[0] = j;
    outPut[1] = i;

    return outPut;
}



void drawPieces(int SQ){
    for(int i= 0; i<8; i++){
        for(int j = 0; j <8; j++){
            Color color = (((i + j) % 2) == 0) ?  BLACK :  WHITE;
            DrawRectangle(j*SQ, i*SQ, SQ, SQ, color);
        }
    }
}

int main()
{

    int Board[8][8];
    stringToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",Board); 

    InitWindow(1024, 1024, "Chess");

    Texture2D BoardTexture  = LoadTexture("Resources/Board.png");
    Texture2D Pieces[2][7];
    for(int i = 0; i<2; i++){

        for(int j = 1; j<7; j++){
            string path = ("Resources/Pieces/" + to_string(i) + "/" + to_string(j) + ".png");
            Pieces[i][j] = LoadTexture(path.c_str());
        }
        
    }
    SetTargetFPS(60);

    while(!WindowShouldClose()){

        BeginDrawing();
        ClearBackground(BEIGE);

        DrawTextureEx(BoardTexture, {0,0}, 0, 4.0f, WHITE);
        for(int i = 0; i<8; i++){
            for(int j = 0; j<8; j++){

                if(Board[i][j] == None)
                    continue;


                if((Board[i][j] & ColorMask) == 8){
                    DrawTextureEx(Pieces[(White/8)-1][(Board[i][j] & TypeMask)],{j * 128, i * 128},0,4.0f,WHITE);
                }
                else{
                    DrawTextureEx(Pieces[(Black/8)-1][(Board[i][j] & TypeMask)],{j * 128, i * 128},0,4.0f,WHITE);
                }
            }
        }
        EndDrawing();

    }

    UnloadTexture(BoardTexture);
    CloseWindow();
    
    return 0;
}