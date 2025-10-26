#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <splash.h>
#include "Animations.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// I don't think debouncing is needed for these since there's not really any single-tap games
#define buttonA D7 // player inputs
#define buttonB D8
#define buttonX D9
#define buttonY D10

#define GROUND_Y 26

unsigned long lastFrameTime = 0;
int currentFrame = 0;
int choice = 0;
const unsigned long frameInterval = 500;
bool showSplash = true;

int breakout()
{
    // --- Game parameters ---
    const int paddleWidth = 20;
    const int paddleHeight = 3;
    int paddleX = (SCREEN_WIDTH - paddleWidth) / 2;
    const int paddleY = SCREEN_HEIGHT - paddleHeight - 1;

    float ballX = SCREEN_WIDTH / 2;
    float ballY = SCREEN_HEIGHT / 2;
    float ballVX = 2;
    float ballVY = -2;
    const int ballSize = 2;

    // Bricks
    const int brickRows = 3;
    const int brickCols = 8;
    const int brickWidth = SCREEN_WIDTH / brickCols;
    const int brickHeight = 5;
    bool bricks[brickRows][brickCols];
    for (int r = 0; r < brickRows; r++)
        for (int c = 0; c < brickCols; c++)
            bricks[r][c] = true;

    int score = 0;
    unsigned long lastFrame = 0;
    const int frameDelay = 33; // ~30 FPS

    // --- Countdown phase ---
    unsigned long startTime = millis();
    int lastSecondShown = -1;

    while (millis() - startTime < 3000)
    { // 3 seconds total
        // Paddle movement allowed
        if (digitalRead(buttonX) == LOW && paddleX > 0)
            paddleX -= 3;
        if (digitalRead(buttonY) == LOW && paddleX + paddleWidth < SCREEN_WIDTH)
            paddleX += 3;

        int secondsLeft = 3 - ((millis() - startTime) / 1000);
        if (secondsLeft != lastSecondShown)
        {
            lastSecondShown = secondsLeft;

            // Draw field
            display.clearDisplay();

            // Bricks
            for (int r = 0; r < brickRows; r++)
            {
                for (int c = 0; c < brickCols; c++)
                {
                    if (bricks[r][c])
                        display.fillRect(c * brickWidth, r * brickHeight + 5,
                                         brickWidth - 1, brickHeight - 1, SSD1306_WHITE);
                }
            }

            // Paddle
            display.fillRect(paddleX, paddleY, paddleWidth, paddleHeight, SSD1306_WHITE);

            // Ball (stationary)
            display.fillRect((int)ballX, (int)ballY, ballSize, ballSize, SSD1306_WHITE);

            // Countdown text (top-right corner)
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(SCREEN_WIDTH - 12, 0);
            display.print(secondsLeft);

            display.display();
        }

        delay(10); // Smooth paddle motion during countdown
    }

    // --- Main game loop ---
    while (true)
    {
        unsigned long now = millis();
        if (now - lastFrame < frameDelay)
            continue;
        lastFrame = now;

        // --- Input ---
        if (digitalRead(buttonX) == LOW && paddleX > 0)
            paddleX -= 3;
        if (digitalRead(buttonY) == LOW && paddleX + paddleWidth < SCREEN_WIDTH)
            paddleX += 3;

        // --- Ball physics ---
        ballX += ballVX;
        ballY += ballVY;

        // Wall collisions
        if (ballX <= 0 || ballX + ballSize >= SCREEN_WIDTH)
            ballVX = -ballVX;
        if (ballY <= 0)
            ballVY = -ballVY;

        // Paddle collision
        if (ballY + ballSize >= paddleY && ballX + ballSize >= paddleX && ballX <= paddleX + paddleWidth)
        {
            ballVY = -ballVY;
            float hitPos = (ballX + ballSize / 2.0 - (paddleX + paddleWidth / 2.0)) / (paddleWidth / 2.0);
            ballVX = hitPos * 2.5;
        }

        // Brick collision
        for (int r = 0; r < brickRows; r++)
        {
            for (int c = 0; c < brickCols; c++)
            {
                if (!bricks[r][c])
                    continue;
                int bx = c * brickWidth;
                int by = r * brickHeight + 5;
                if (ballX + ballSize > bx && ballX < bx + brickWidth && ballY + ballSize > by && ballY < by + brickHeight)
                {
                    bricks[r][c] = false;
                    score++;
                    ballVY = -ballVY;
                }
            }
        }

        // Game over
        if (ballY > SCREEN_HEIGHT)
            break;

        // --- Drawing ---
        display.clearDisplay();

        // Bricks
        for (int r = 0; r < brickRows; r++)
        {
            for (int c = 0; c < brickCols; c++)
            {
                if (bricks[r][c])
                    display.fillRect(c * brickWidth, r * brickHeight + 5,
                                     brickWidth - 1, brickHeight - 1, SSD1306_WHITE);
            }
        }

        // Paddle
        display.fillRect(paddleX, paddleY, paddleWidth, paddleHeight, SSD1306_WHITE);

        // Ball
        display.fillRect((int)ballX, (int)ballY, ballSize, ballSize, SSD1306_WHITE);

        // Score
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(SCREEN_WIDTH - 30, 0);
        display.print(score);

        display.display();
    }

    // --- Game Over ---
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 10);
    display.print("GAME OVER");
    display.setCursor(40, 20);
    display.print("Score:");
    display.print(score);
    display.display();
    delay(1000);

    return score;
}

int dino()
{
    int score = 0;
    int dinoY = GROUND_Y;
    int velocity = 0;
    bool jumping = false;

    int obstacleX = SCREEN_WIDTH;
    int obstacleWidth = 6;
    int obstacleHeight = 8;

    unsigned long lastFrame = 0;
    const int frameDelay = 33; // ~30 FPS

    // pre-load screen
    display.clearDisplay();
    display.drawBitmap(0, 0, controls_dino_jump, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
    display.display();

    while (true)
    {
        if (digitalRead(buttonA) == LOW)
        {
            break;
        }
    }

    // --- Game Loop ---
    while (true)
    {
        unsigned long now = millis();
        if (now - lastFrame < frameDelay)
            continue;
        lastFrame = now;

        // --- Input ---
        if (digitalRead(buttonA) == LOW && !jumping)
        {
            jumping = true;
            velocity = -6; // jump impulse
        }

        // --- Physics ---
        if (jumping)
        {
            dinoY += velocity;
            velocity += 1; // gravity
            if (dinoY >= GROUND_Y)
            {
                dinoY = GROUND_Y;
                jumping = false;
            }
        }

        // --- Obstacle movement ---
        obstacleX -= 4;
        if (obstacleX + obstacleWidth < 0)
        {
            obstacleX = SCREEN_WIDTH + random(10, 40);
            score++;
        }

        // --- Collision detection ---
        if (obstacleX < 10 && obstacleX + obstacleWidth > 2 && dinoY > GROUND_Y - obstacleHeight)
        {
            break; // game over
        }

        // --- Drawing ---
        display.clearDisplay();

        // Ground line
        display.drawLine(0, GROUND_Y + 1, SCREEN_WIDTH, GROUND_Y + 1, SSD1306_WHITE);

        // Dino (8×8)
        display.fillRect(2, dinoY - 8, 8, 8, SSD1306_WHITE);

        // Obstacle
        display.fillRect(obstacleX, GROUND_Y - obstacleHeight, obstacleWidth, obstacleHeight, SSD1306_WHITE);

        // Score
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(SCREEN_WIDTH - 30, 0);
        display.print(score);

        display.display();
    }

    // --- Game Over screen ---
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 10);
    display.print("GAME OVER");
    display.setCursor(40, 20);
    display.print("Score:");
    display.print(score);
    display.display();

    delay(1500);
    return score;
}

int snake()
{
    const int cellSize = 4; // grid size
    const int cols = SCREEN_WIDTH / cellSize;
    const int rows = SCREEN_HEIGHT / cellSize;

    struct Point
    {
        int x, y;
    };
    Point snake[cols * rows]; // maximum possible length
    int length = 3;

    // initial snake position and direction
    snake[0] = {cols / 2, rows / 2};
    int dirX = 1, dirY = 0;

    // food
    Point food = {random(cols), random(rows)};
    int score = 0;

    unsigned long lastMove = 0;
    const int moveDelay = 120; // lower = faster

    while (true)
    {
        unsigned long now = millis();
        if (now - lastMove < moveDelay)
            continue;
        lastMove = now;

        // --- input ---
        if (digitalRead(buttonA) == LOW && dirY != 1)
        {
            dirX = 0;
            dirY = -1;
        } // up
        if (digitalRead(buttonB) == LOW && dirY != -1)
        {
            dirX = 0;
            dirY = 1;
        } // down
        if (digitalRead(buttonX) == LOW && dirX != 1)
        {
            dirX = -1;
            dirY = 0;
        } // left
        if (digitalRead(buttonY) == LOW && dirX != -1)
        {
            dirX = 1;
            dirY = 0;
        } // right

        // --- move snake ---
        for (int i = length; i > 0; i--)
            snake[i] = snake[i - 1];

        snake[0].x += dirX;
        snake[0].y += dirY;

        // --- wrap around edges (optional) ---
        if (snake[0].x < 0)
            snake[0].x = cols - 1;
        if (snake[0].x >= cols)
            snake[0].x = 0;
        if (snake[0].y < 0)
            snake[0].y = rows - 1;
        if (snake[0].y >= rows)
            snake[0].y = 0;

        // --- check food ---
        if (snake[0].x == food.x && snake[0].y == food.y)
        {
            length++;
            score++;
            food = {random(cols), random(rows)};
        }

        // --- check self collision ---
        for (int i = 1; i < length; i++)
        {
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
            {
                // game over
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(35, 10);
                display.print("GAME OVER");
                display.setCursor(35, 20);
                display.print("Score:");
                display.print(score);
                display.display();
                delay(1500);
                return score;
            }
        }

        // --- draw ---
        display.clearDisplay();

        // draw food
        display.fillRect(food.x * cellSize, food.y * cellSize, cellSize, cellSize, SSD1306_WHITE);

        // draw snake
        for (int i = 0; i < length; i++)
            display.fillRect(snake[i].x * cellSize, snake[i].y * cellSize, cellSize, cellSize, SSD1306_WHITE);

        // score
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(SCREEN_WIDTH - 30, 0);
        display.print(score);

        display.display();
    }
}

int flappyBird()
{
    const int GROUND_Y2 = 31;
    const int BIRD_X = 20;
    const int PIPE_WIDTH = 10;
    const int GAP_HEIGHT = 14;
    const float GRAVITY = 0.25;
    const float FLAP_STRENGTH = -2.8;

    float birdY = 16;
    float velocity = 0;
    int score = 0;

    // Pipe variables
    float pipeX = SCREEN_WIDTH;
    int gapY = random(8, 24);

    unsigned long lastUpdate = millis();
    bool gameOver = false;

    while (!gameOver)
    {
        unsigned long now = millis();
        float delta = (now - lastUpdate) / 16.0; // normalize timing
        lastUpdate = now;

        // --- Input ---
        if (digitalRead(buttonA) == LOW)
        {
            velocity = FLAP_STRENGTH;
            delay(120); // debounce
        }

        // --- Physics ---
        velocity += GRAVITY * delta;
        birdY += velocity * delta;
        if (birdY < 0)
            birdY = 0;

        // --- Move pipe ---
        pipeX -= 2 * delta;
        if (pipeX + PIPE_WIDTH < 0)
        {
            pipeX = SCREEN_WIDTH;
            gapY = random(6, 24);
            score++;
        }

        // --- Collision detection ---
        bool hitPipe = (BIRD_X + 3 >= pipeX && BIRD_X <= pipeX + PIPE_WIDTH &&
                        (birdY < gapY || birdY + 3 > gapY + GAP_HEIGHT));
        if (birdY + 3 >= GROUND_Y2 || hitPipe)
        {
            gameOver = true;
        }

        // --- Draw frame ---
        display.clearDisplay();

        // Bird
        display.fillRect(BIRD_X, (int)birdY, 4, 4, SSD1306_WHITE);

        // Pipes
        display.fillRect((int)pipeX, 0, PIPE_WIDTH, gapY, SSD1306_WHITE);                                            // top
        display.fillRect((int)pipeX, gapY + GAP_HEIGHT, PIPE_WIDTH, GROUND_Y2 - (gapY + GAP_HEIGHT), SSD1306_WHITE); // bottom

        // Ground
        display.drawLine(0, GROUND_Y2, SCREEN_WIDTH, GROUND_Y2, SSD1306_WHITE);

        // Score
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(2, 0);
        display.print("Score: ");
        display.print(score);

        display.display();
        delay(16);
    }

    // --- Game Over Screen ---
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 10);
    display.print("Game Over!");
    display.setCursor(25, 22);
    display.print("Score: ");
    display.print(score);
    display.display();
    delay(1000);

    return score;
}

int showMenu()
{
    const char *games[] = {"Breakout", "Dino", "Snake", "Flappy"};
    const int gameCount = sizeof(games) / sizeof(games[0]);
    int selected = 0;
    int scrollOffset = 0;       // topmost visible index
    const int visibleItems = 2; // how many can fit on screen at once

    while (true)
    {
        // --- Handle input ---
        if (digitalRead(buttonA) == LOW)
        { // up
            selected--;
            if (selected < 0)
                selected = gameCount - 1;
            if (selected < scrollOffset)
                scrollOffset = selected;
            delay(150);
        }
        if (digitalRead(buttonB) == LOW)
        { // down
            selected++;
            if (selected >= gameCount)
                selected = 0;
            if (selected >= scrollOffset + visibleItems)
                scrollOffset = selected - visibleItems + 1;
            delay(150);
        }
        if (digitalRead(buttonX) == LOW)
        { // go to splash
            showSplash = true;
            return -1;
        }
        if (digitalRead(buttonY) == LOW)
        { // confirm game
            showSplash = false;
            return selected;
        }

        // --- Draw menu ---
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("KEYCHAIN ARCADE");

        for (int i = 0; i < visibleItems; i++)
        {
            int gameIndex = scrollOffset + i;
            if (gameIndex >= gameCount)
                break;
            int y = 12 + i * 8;

            if (gameIndex == selected)
            {
                display.fillRect(0, y - 1, SCREEN_WIDTH, 8, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            }
            else
            {
                display.setTextColor(SSD1306_WHITE);
            }

            display.setCursor(4, y);
            display.print(games[gameIndex]);
        }

        // Optional scrollbar on right
        int barHeight = (visibleItems * 32) / gameCount;
        int barY = (scrollOffset * 32) / gameCount;
        display.drawRect(124, barY, 3, barHeight, SSD1306_WHITE);

        display.display();
        delay(20);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing!");
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.drawBitmap(0, 0, splash_introducing, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
    display.display();
    pinMode(buttonA, INPUT_PULLUP);
    pinMode(buttonB, INPUT_PULLUP);
    pinMode(buttonX, INPUT_PULLUP);
    pinMode(buttonY, INPUT_PULLUP);
    Serial.println(testVar);
    delay(3000);
    display.clearDisplay();
    display.display();
}

void loop()
{

    if (showSplash)
    {

        if (digitalRead(buttonA) == LOW || digitalRead(buttonB) == LOW || digitalRead(buttonX) == LOW || digitalRead(buttonY) == LOW)
        {
            showSplash = false;
        }
        unsigned long now = millis();

        // Check if it's time to switch frames
        if (now - lastFrameTime >= frameInterval)
        {
            lastFrameTime = now;

            // Toggle between the two frames
            currentFrame = !currentFrame;

            display.clearDisplay();
            if (currentFrame == 0)
            {
                display.drawBitmap(0, 0, splash_loadingf1, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
            }
            else
            {
                display.drawBitmap(0, 0, splash_loadingf2, CONTROLS_WIDTH, CONTROLS_HEIGHT, 1);
            }
            display.display();
        }
    }
    else
    {
        choice = showMenu();

        if (showSplash || choice == -1)
        {
            // Go show the splash instead of loading a game
            return;
        }
        // create menu to play games from
        // pick random game and queue into that
        switch (choice)
        {
        case 0:
            breakout();
            break;
        case 1:
            dino();
            break;
        case 2:
            snake();
            break;
        case 3:
            flappyBird();
            break;
        default:
            Serial.println("No game loaded.");
            break;
        }
        delay(1000);
    }
}