# 👾 Alien Tower Defense

In this tower defense game, aliens use A* pathfinding to navigate custom or pre-set paths, while you strategically place towers to shoot and eliminate them before they reach the exit.

---

## 🔨 How To Run the Game

1. Download & Extract
  * Download the project .zip file.
  * Extract it to your desired folder.

2. Open in Your IDE
  * Launch your preferred C++ compatible editor.
  * Open the project folder.

3. Build with CMake
  * Ensure CMake is installed.
  * Configure and generate build files using CMake.
  * Build the project.

4. Run the Game
  * Launch the executable from your build directory.
  * You're ready to play!

## 🕹️ Features

### 🖊️ FreeType Font Integration

* The font initialization (font-init) and text rendering (renderText) routines were successfully merged into the core OpenGL rendering pipeline.
* Fonts are used to display game information such as the intro screen title, in-game stats, and messages during gameplay.
* Font resources (.ttf files) are stored in the data/fonts/ directory.

### 🖥️ Introduction Screen

- Displays the game title, author name, and a dynamic menu.
- Menu options include:
  - Game name.
  - Selected game level that updates selection with keys 0-9.
  - ```Space``` to select the level.
  - ```P``` to play the game.
  - ```O``` to show the Alien path (only works during gameplay).
  - ```R``` to restart the current level (only works during gameplay).
  - ```Esc``` to exit the game or return to the intro screen.
 
### 🖥️ Game Over Screens

- The victory screen appears when all invaders are destroyed.
- Defeat screen appears when any invader reaches the exit.
- Both screens fade in and display respective messages.
- Pressing ```Esc``` returns the user to the intro screen.

### 💯 Game Score Display

- Displays score in top-left uing FreeType.
- Score updates in real-time as invaders are destroyed.

### 🏰 Placing Towers

- Towers can be placed using the **left mouse button**.
- Towers can be removed using the **right mouse button**.
- Towers cannot be placed on path tiles or the top row.

### 🎯 Rotating Towers
- Towers rotate to shoot at the closest invader within a 5-tile radius.
- Towers only fire when aligned and cooldowns are complete.

### ❌ Invalid Level Handling
- Invalid level files show an error message on the intro screen.

### 👽 Invader Spawning
- Each level spawns  ```10 * (current level + 1)``` invaders.
- Each type of invader has a distinct health and speed.
- ```Unspawned``` counter at the top displays remaining invaders.

### 🏆 Invader Points
- Each invader is assigned a points value.
- Points are displayed above each invader's head using FreeType fonts.

---

## ⚖️ License 

Based on UBC CPSC 427 assignment structure. Unauthorized copying, redistribution, or reuse of any portion of this code for other CPSC 427 submissions is strictly prohibited. All code and assets are the intellectual property of ther respective authors and are intended solely for educational and personal demonstration purposes. Please respect academic integrity policies.

---

## 🦗 Known Bugs
- Sometimes I have to double press 'O' for colored tiles to show up. Specifically, this happens when pressing 'O' on the drawing screen, then pressing 'P' to play, and then pressing 'P' to go back to the drawing screen. Then you have to double press 'O' in order to see the colored tiles again.
- Repeatedly pressing R will result in the points to continuously decrease until an eventual victory screen.
