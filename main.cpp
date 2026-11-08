#include <iostream>
#include <string>
#include <cctype>

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

char codeToChar(int c){
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
    cout << "[" << endl;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            cout << (((Board[i][j] & White) == 8) ? (char)toupper(codeToChar(Board[i][j])) : codeToChar(Board[i][j]))  << ", ";
        }
        cout << endl;
    }
    cout << "]";
}



int main()
{
    int Board[8][8];
   
    stringToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",Board);

    printBoard(Board);
    

    return 0;
}