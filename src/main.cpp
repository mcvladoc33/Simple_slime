#include "slime_idle.h"
#include <U8g2lib.h>

#define UP 2
#define DOWN 3
#define LEFT 4
#define RIGHT 5
#define A 6
#define B 7

#define MAX_WALLS 15  // Максимальна кількість стінок

// 128x64 дисплей
U8G2_SSD1309_128X64_NONAME2_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

void slime_walk();
void game_update();
void random_move();
bool check_wall_collision(int new_x, int new_y);
void generate_grid_walls();
void generate_clustered_walls();
void generate_path_with_walls();
void init_random();
int true_random(int min, int max);

// Початкові координати слайма
int x = 10; // початкова позиція по x
int y = 64 - 25; // початкова позиція по y

// Змінні для анімації слайма
int slime_walk_state = 0;
int slime_jump_state = 0;

unsigned long previousMillis = 0;
unsigned long buttonMillis[4] = {0};
const long interval = 50;
const long buttonDelay = 50;
const long inactivityTimeout = 5000;
const long randomMoveInterval = 50;

const int num_idle_frames = idle_bitmap_allArray_LEN;

unsigned long lastInputMillis = 0;
unsigned long lastRandomMoveMillis = 0;
bool demoModeActive = false;


int wall_x[MAX_WALLS];  // Массив для зберігання координат X стінок
int wall_y[MAX_WALLS];  // Массив для зберігання координат Y стінок
int wall_width = 16;      // Ширина стінки
int wall_height = 16;     // Висота стінки
int num_walls = 0;       // Поточна кількість стінок

void setup() {
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(A, INPUT_PULLUP);
  pinMode(B, INPUT_PULLUP);
  u8g2.begin();

  init_random();  // Ініціалізуємо генератор випадкових чисел

  generate_grid_walls();  // Генерація стінок при запуску
}

void loop() {
  game_update();
}

void game_update() {
  unsigned long currentMillis = millis();

  // Перевірка кнопки "Left"
  if (digitalRead(LEFT) == LOW && currentMillis - buttonMillis[0] >= buttonDelay) {
    if (!check_wall_collision(x - 1, y)) {  // Перевірка зіткнення з стінкою
      x--;
    }
    buttonMillis[0] = currentMillis;
    lastInputMillis = currentMillis;
    demoModeActive = false;
  }

  // Перевірка кнопки "Right"
  if (digitalRead(RIGHT) == LOW && currentMillis - buttonMillis[1] >= buttonDelay) {
    if (!check_wall_collision(x + 1, y)) {  // Перевірка зіткнення з стінкою
      x++;
    }
    buttonMillis[1] = currentMillis;
    lastInputMillis = currentMillis;
    demoModeActive = false;
  }

  // Перевірка кнопки "Up"
  if (digitalRead(UP) == LOW && currentMillis - buttonMillis[2] >= buttonDelay) {
    if (!check_wall_collision(x, y - 1)) {  // Перевірка зіткнення з стінкою
      y--;
    }
    buttonMillis[2] = currentMillis;
    lastInputMillis = currentMillis;
    demoModeActive = false;
  }

  // Перевірка кнопки "Down"
  if (digitalRead(DOWN) == LOW && currentMillis - buttonMillis[3] >= buttonDelay) {
    if (!check_wall_collision(x, y + 1)) {  // Перевірка зіткнення з стінкою
      y++;
    }
    buttonMillis[3] = currentMillis;
    lastInputMillis = currentMillis;
    demoModeActive = false;
  }

  // Перевірка меж екрану
  if (x < 0) x = 0;
  if (x > 128 - SLIME_SMALL_WIDTH) x = 128 - SLIME_SMALL_WIDTH;
  if (y < 0) y = 0;
  if (y > 64 - SLIME_SMALL_HEIGHT) y = 64 - SLIME_SMALL_HEIGHT;

  // Якщо більше 5 секунд без вводу, активуємо демонстраційний режим
  if (!demoModeActive && currentMillis - lastInputMillis >= inactivityTimeout) {
    demoModeActive = true;
  }

  if (demoModeActive) {
    if (currentMillis - lastRandomMoveMillis >= randomMoveInterval) {
      random_move();
      lastRandomMoveMillis = currentMillis;
    }
  }

  slime_walk();
}

void slime_walk() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    u8g2.clearBuffer();

    u8g2.drawXBMP(x, y, SLIME_SMALL_WIDTH, SLIME_SMALL_HEIGHT, idle_bitmap_allArray[slime_walk_state]);

    // Малюємо стінки
    for (int i = 0; i < num_walls; i++) {
      u8g2.drawBox(wall_x[i], wall_y[i], wall_width, wall_height);  // Малюємо кожну стінку
    }

    u8g2.sendBuffer();

    slime_walk_state = (slime_walk_state + 1) % num_idle_frames;
  }
}

// Функція для перевірки на зіткнення з стінкою
bool check_wall_collision(int new_x, int new_y) {
  for (int i = 0; i < num_walls; i++) {
    if (new_x < wall_x[i] + wall_width && new_x + SLIME_SMALL_WIDTH > wall_x[i] &&
        new_y < wall_y[i] + wall_height && new_y + SLIME_SMALL_HEIGHT > wall_y[i]) {
      return true;  // Якщо є зіткнення
    }
  }
  return false;  // Якщо немає зіткнення
}

// Генерація стінок за допомогою сітки (Maze-like)
void generate_grid_walls() {
  const int gridWidth = 8;  // Number of columns in the grid (128px / 8px)
  const int gridHeight = 4;  // Number of rows in the grid (64px / 8px)
  const int cellWidth = 16;   // Width of each grid cell (wall size)
  const int cellHeight = 16;  // Height of each grid cell (wall size)

  num_walls = 0;  // Reset the wall counter

  // Define the area around the slime's spawn that should remain free of walls
  const int spawnBufferX = 20; // Area buffer around slime's X spawn point
  const int spawnBufferY = 20; // Area buffer around slime's Y spawn point
  const int slime_spawn_x_min = x - spawnBufferX;
  const int slime_spawn_x_max = x + SLIME_SMALL_WIDTH + spawnBufferX;
  const int slime_spawn_y_min = y - spawnBufferY;
  const int slime_spawn_y_max = y + SLIME_SMALL_HEIGHT + spawnBufferY;

  // Loop through each grid cell and randomly decide whether to place a wall
  for (int row = 0; row < gridHeight; row++) {
    for (int col = 0; col < gridWidth; col++) {
      int wallX = col * cellWidth;
      int wallY = row * cellHeight;

      // Skip placing a wall if it's within the spawn area of the slime
      if (wallX >= slime_spawn_x_min && wallX < slime_spawn_x_max &&
          wallY >= slime_spawn_y_min && wallY < slime_spawn_y_max) {
        continue; // Skip this grid cell
      }

      // Only place walls in random grid cells if the number of walls is less than MAX_WALLS
      if (random(0, 100) < 50 && num_walls < MAX_WALLS) {
        wall_x[num_walls] = wallX;
        wall_y[num_walls] = wallY;
        num_walls++;
      }
    }
  }
}

// Генерація кластеризованих стінок
void generate_clustered_walls() {
  num_walls = 0;  // Reset the wall counter

  for (int i = 0; i < 10; i++) {  // Place 10 clusters
    int clusterCenterX = random(0, 128);
    int clusterCenterY = random(0, 64);
    int clusterSize = random(3, 6);  // Number of walls in the cluster

    for (int j = 0; j < clusterSize; j++) {
      // Randomly spread walls from the center point
      int offsetX = random(-5, 5);
      int offsetY = random(-5, 5);

      int wallX = clusterCenterX + offsetX;
      int wallY = clusterCenterY + offsetY;

      if (wallX >= 0 && wallX <= 128 - wall_width && wallY >= 0 && wallY <= 64 - wall_height) {
        if (num_walls < MAX_WALLS) {
          wall_x[num_walls] = wallX;
          wall_y[num_walls] = wallY;
          num_walls++;
        }
      }
    }
  }
}

// Генерація стінок з карбуванням шляхів
void generate_path_with_walls() {
  generate_clustered_walls();  // First, generate clustered walls

  // After generating walls, create paths by removing some of the walls
  for (int i = 0; i < num_walls; i++) {
    // Example: Remove walls in a certain pattern to create a path
    if (random(0, 100) < 30) {  // 30% chance to remove a wall
      wall_x[i] = -1;  // Mark this wall as removed (invalid position)
      wall_y[i] = -1;
    }
  }
}

// Ініціалізація генератора випадкових чисел через аналоговий шум
void init_random() {
  randomSeed(analogRead(0));  // Зчитуємо значення з аналогового піну 0
}

// Функція для отримання випадкового числа
int true_random(int min, int max) {
  return min + (random() % (max - min + 1));
}

void random_move() {
  static int moveDirection = true_random(0, 7);  // Початковий випадковий напрямок

  // Рух слайма в обраному напрямку
  switch (moveDirection) {
    case 0:  // Ліворуч
      if (!check_wall_collision(x - 1, y) && x - 1 >= 0) {  // Перевірка на межі екрану
        x--;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 1:  // Праворуч
      if (!check_wall_collision(x + 1, y) && x + 1 <= 128 - SLIME_SMALL_WIDTH) {  // Перевірка на межі екрану
        x++;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 2:  // Вгору
      if (!check_wall_collision(x, y - 1) && y - 1 >= 0) {  // Перевірка на межі екрану
        y--;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 3:  // Вниз
      if (!check_wall_collision(x, y + 1) && y + 1 <= 64 - SLIME_SMALL_HEIGHT) {  // Перевірка на межі екрану
        y++;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 4:  // Вверх ліворуч (діагональ)
      if (!check_wall_collision(x - 1, y - 1) && x - 1 >= 0 && y - 1 >= 0) {  // Перевірка на межі екрану
        x--;
        y--;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 5:  // Вверх праворуч (діагональ)
      if (!check_wall_collision(x + 1, y - 1) && x + 1 <= 128 - SLIME_SMALL_WIDTH && y - 1 >= 0) {  // Перевірка на межі екрану
        x++;
        y--;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 6:  // Вниз ліворуч (діагональ)
      if (!check_wall_collision(x - 1, y + 1) && x - 1 >= 0 && y + 1 <= 64 - SLIME_SMALL_HEIGHT) {  // Перевірка на межі екрану
        x--;
        y++;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;

    case 7:  // Вниз праворуч (діагональ)
      if (!check_wall_collision(x + 1, y + 1) && x + 1 <= 128 - SLIME_SMALL_WIDTH && y + 1 <= 64 - SLIME_SMALL_HEIGHT) {  // Перевірка на межі екрану
        x++;
        y++;
      } else {
        moveDirection = true_random(0, 7);  // Якщо зіткнення з перешкодою або межею, вибираємо новий напрямок
      }
      break;
  }
}
