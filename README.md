




# ♟️ Chess

A complete two-player chess game written in **C++17** with **raylib**. It supports every standard rule of movement and a mouse-driven GUI with pixel-art graphics. The rules engine was written from scratch, with no chess libraries.

<!-- ![Screenshot](docs/Chess.gif) -->
https://github.com/user-attachments/assets/2b92c632-2480-4a63-8ee3-053dfd267792


---

## Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [How to Play](#how-to-play)
- [Project Structure](#project-structure)
- [How It Works](#how-it-works)
- [How It Was Built](#how-it-was-built)
- [Known Limitations](#known-limitations)
- [Roadmap](#roadmap)
- [Credits](#credits)
- [License](#license)

---

## Features

**Rules**
- All six piece types with correct movement, including sliding pieces blocked by other pieces
- Check, checkmate, and stalemate detection
- Pins and illegal king moves are prevented automatically
- Castling (both sides, both colors) with all conditions enforced:
  - neither the king nor the rook has moved
  - no pieces between them
  - the king is not in check, and does not pass through or land on an attacked square
- En passant
- Pawn promotion with a piece-selection menu (queen, rook, bishop, knight)
- Turn enforcement

**Interface**
- Click-to-pick-up and click-to-drop piece movement, with the piece following the cursor
- Highlight on the selected piece's square
- Legal-move dots, with rings on capturable pieces
- Turn indicator in the top-right corner
- Checkmate and stalemate screens with a **Replay** button (or press `R`)
- Pixel-art board and pieces, scaled 4x with crisp nearest-neighbor rendering
- Move sound effect

---

## Getting Started

### Requirements

- A C++17 compiler (developed with GCC 16 from MSYS2 UCRT64 on Windows)
- [raylib](https://www.raylib.com/)

### Windows (MSYS2 UCRT64)

1. Install [MSYS2](https://www.msys2.org/) and open the **UCRT64** terminal.
2. Install the toolchain and raylib:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
   ```

3. Make sure `C:\msys64\ucrt64\bin` is on your `PATH`.
4. From the project folder, build:

   ```bash
   g++ -std=c++17 -Wall -Wextra main.cpp -o main.exe -lraylib -lopengl32 -lgdi32 -lwinmm
   ```

5. Run it **from the project folder**, so the asset paths resolve:

   ```bash
   ./main.exe
   ```

### Linux / macOS

Install raylib with your package manager, then link it with the platform's libraries. See the [raylib build instructions](https://github.com/raysan5/raylib/wiki) for your system. The code itself uses no Windows-specific APIs.

### VS Code

A `tasks.json` that runs the g++ command above lets you build with `Ctrl+Shift+B`. If you use the Code Runner extension, add the raylib `-l` flags to its C++ executor command.

---

## How to Play

| Action | Control |
|---|---|
| Pick up a piece | Left-click it |
| Drop the piece | Left-click the destination square |
| Cancel / illegal move | Drop it anywhere illegal and it snaps back |
| Promote a pawn | Click one of the four pieces in the menu |
| Restart after the game ends | Click **Replay** or press `R` |
| Quit | `Esc` or close the window |

White moves first. Castle by moving the king two squares toward the rook.

---

## Project Structure

```
Chess Game/
├── main.cpp              # All game code
├── Resources/
│   ├── Board.png         # 256x256 pixel-art board
│   └── Pieces/
│       ├── 0/            # White pieces: 1.png (King) ... 6.png (Pawn)
│       └── 1/            # Black pieces: 1.png (King) ... 6.png (Pawn)
├── SFX/
│   └── sound.wav         # Move sound
├── .gitignore
└── README.md
```

The piece image names match the numbers used in the code (1 = King, 2 = Queen, 3 = Rook, 4 = Bishop, 5 = Knight, 6 = Pawn), so a piece's texture is found with simple indexing instead of a lookup table.

---

## How It Works

### Board representation

The board is an `int Board[8][8]`, where `Board[0][0]` is a8 (top-left) and `Board[7][7]` is h1. That matches how FEN strings are written and how raylib draws, so nothing needs to be flipped.

### Piece encoding

Each piece is one integer that packs its type and color into separate bits:

```
type  = piece & TypeMask    (bits 0-2: 1 to 6)
color = piece & ColorMask   (White = 8, Black = 16)
```

For example, a white knight is `8 | 5 = 13` and a black pawn is `16 | 6 = 22`. An empty square is `0`.

### FEN loading

Positions are loaded from the first field of a [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) string. This made it easy to set up small test positions (a lone rook, a pinned bishop, a castling setup) instead of replaying whole games.

### Move validation, in layers

Move legality is split into separate stages, each answering one question:

1. **`validateMove`** is pure piece-movement logic. It reads the board and never changes it. It works from the row and column difference between the squares, and uses a `checkInBetween` walk for rooks, bishops, and queens.
2. **`leavesKingInCheck`** copies the board, plays the move on the copy, finds the king, and asks whether it is attacked. This one check handles check, pins, and king safety.
3. **`canCastle`** is a dedicated function with the castling conditions. It is kept out of `validateMove` on purpose, because attack detection itself calls `validateMove`, and a castling rule inside it would recurse forever.
4. **`hasLegalMove`** tries every piece against every destination. If the side to move has no legal move, the game is checkmate (king attacked) or stalemate (king not attacked).

Attack detection (`isThisSquareAttacked`) reuses `validateMove` by asking whether any enemy piece could move onto the target square. Pawns need a dedicated helper (`pawnAttacks`) for castling, because a pawn can't *move* diagonally onto an empty square but still attacks it.

### The `Move` struct

Each attempted move is described by a struct: origin, destination, the moving piece, the piece on the destination, and a move type (`MoveNormal`, `MoveCastling`, `MoveEnPassant`, `MovePromotion`). The validator tags the move, and the code that applies it branches on that tag to do the extra work (moving the rook, removing the en passant pawn).

### Special-move state

- **En passant:** a single "note" stores the square a pawn just skipped over (or `-1, -1`). It is reset after every move, so it only lasts one turn.
- **Castling rights:** four booleans (white queenside/kingside, black queenside/kingside). A king move clears both of its color's rights, and a rook leaving or being captured on its home corner clears that corner's right.
- **Promotion:** reaching the last row pauses the game and shows a menu. The checkmate test runs only after a piece is chosen.

### Game loop

Every frame does three things in order: handle input, update state, draw. The loop keeps running during game-over and promotion states (so the window stays responsive), and input is simply ignored.

---

## How It Was Built

The project was developed incrementally, with a commit after each working step:

1. **Toolchain setup:** verified g++, set up a git repository, and configured the build for raylib.
2. **Board logic first:** piece encoding, a FEN parser, and a console board printer to test it.
3. **GUI:** an empty raylib window, then the board, then the pieces, then mouse picking.
4. **Movement rules:** a `Move` struct and a validator, built piece by piece from easiest to hardest (knight, king, rook, bishop, queen, pawn), each tested on a tiny FEN position.
5. **Special moves:** en passant, then check detection and the legality filter, then checkmate and stalemate.
6. **Castling and promotion:** the last and fiddliest rules.
7. **Quality of life:** legal-move highlights, a turn indicator, a game-over screen, and replay.

Most bugs along the way came from a few recurring causes: operator precedence in bit-mask expressions (`a & b == c`), character versus numeric values (`'7'` is 55, not 7), and mixing up row/column order against x/y order.

---

## Credits

- Built with [raylib](https://www.raylib.com/)
- Pixel-art board and pieces: [Free Ultimate Pixel Art Chess Asset Pack](https://itch.io/t/7143117/free-ultimate-pixel-art-chess-asset-pack-100-piece-styles-100-chessboards) by [author name]. Please check the pack's license before redistributing the assets.

[Your name] — [link to your GitHub]
