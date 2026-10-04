/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Flappy Cube (Ultimate 60FPS Edition)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "st7789.h"
#include "XPT2046.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
    STATE_MENU,
    STATE_DIFFICULTY,
    STATE_PLAYING,
    STATE_DEATH_FALL,
    STATE_GAMEOVER
} GameState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Game Base Constants
#define BIRD_X           60
#define BIRD_SIZE        14
#define PIPE_WIDTH       40
#define FPS_TARGET_MS    16     // ~60 FPS

// Fixed-Point Math Constants (For sub-pixel smooth physics)
#define FLAP_STRENGTH_FP -1200
#define TERM_VEL_FP      1800

/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

touch_t touch;

GameState game_state = STATE_MENU;

// Dynamic Gameplay Variables
int pipe_speed = 3;
int gap_size = 85;
int gravity_fp = 70;
uint16_t bird_base_color = YELLOW;
int current_level = 1;

// Physics Variables
int32_t bird_y_fp = (120 << 8);
int32_t bird_vy_fp = 0;
int bird_y = 120; // Actual pixel Y

int pipe_x = 240;
int gap_y = 80;

// Score Tracking
int score = 0;
int last_score = -1;
int high_score = 0;
int score_color_timer = 0;

uint32_t frame_count = 0;
bool was_touched = false;
char text_buffer[20];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Helper to draw optimized delta rectangles with boundary clipping */
void Draw_Clipped_Rect(int x, int y, int w, int h, uint16_t color) {
    if (x >= ST7789_WIDTH || x + w <= 0 || y >= ST7789_HEIGHT || y + h <= 0) return;

    int cx = x < 0 ? 0 : x;
    int cy = y < 0 ? 0 : y;
    int cw = (x + w > ST7789_WIDTH) ? ST7789_WIDTH - cx : x + w - cx;
    int ch = (y + h > ST7789_HEIGHT) ? ST7789_HEIGHT - cy : y + h - cy;

    if (cw > 0 && ch > 0) {
        ST7789_DrawFilledRectangle(cx, cy, cw, ch, color);
    }
}

/* ZERO FLICKER BIRD RENDERING (With Dynamic Color Change Support) */
void Update_Bird_Display(int old_y, int new_y, uint16_t color) {
    static uint16_t last_color = BLACK;

    // If color changes (e.g. glowing while flapping), force full redraw to prevent artifacts
    if (color != last_color) {
        ST7789_DrawFilledRectangle(BIRD_X, old_y, BIRD_SIZE, BIRD_SIZE, BLACK);
        ST7789_DrawFilledRectangle(BIRD_X, new_y, BIRD_SIZE, BIRD_SIZE, color);
        last_color = color;
        return;
    }

    if (old_y == new_y) return;

    int dy = new_y - old_y;

    if (dy > 0) { // Moving DOWN
        if (dy < BIRD_SIZE) {
            ST7789_DrawFilledRectangle(BIRD_X, old_y, BIRD_SIZE, dy, BLACK);
            ST7789_DrawFilledRectangle(BIRD_X, old_y + BIRD_SIZE, BIRD_SIZE, dy, color);
        } else {
            ST7789_DrawFilledRectangle(BIRD_X, old_y, BIRD_SIZE, BIRD_SIZE, BLACK);
            ST7789_DrawFilledRectangle(BIRD_X, new_y, BIRD_SIZE, BIRD_SIZE, color);
        }
    } else { // Moving UP
        int abs_dy = -dy;
        if (abs_dy < BIRD_SIZE) {
            ST7789_DrawFilledRectangle(BIRD_X, new_y + BIRD_SIZE, BIRD_SIZE, abs_dy, BLACK);
            ST7789_DrawFilledRectangle(BIRD_X, new_y, BIRD_SIZE, abs_dy, color);
        } else {
            ST7789_DrawFilledRectangle(BIRD_X, old_y, BIRD_SIZE, BIRD_SIZE, BLACK);
            ST7789_DrawFilledRectangle(BIRD_X, new_y, BIRD_SIZE, BIRD_SIZE, color);
        }
    }
}

/* Instant Hardware Flash Effect */
void Screen_Hit_Flash(void) {
    ST7789_InvertColors(1);
    HAL_Delay(40);
    ST7789_InvertColors(0);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();

  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(TFT_LED_GPIO_Port, TFT_LED_Pin, GPIO_PIN_SET);

  ST7789_Init();
  XPT2046_init(&hspi1, XPT2046_LANDSCAPE, ST7789_WIDTH, ST7789_HEIGHT);

  ST7789_DrawFilledRectangle(0, 0, ST7789_WIDTH, ST7789_HEIGHT, BLACK);
  srand(HAL_GetTick());

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t frame_start = HAL_GetTick();
      frame_count++;

      /* --- INPUT HANDLING --- */
      bool is_touched = (HAL_GPIO_ReadPin(XPT2046_IRQ_GPIO_Port, XPT2046_IRQ_Pin) == GPIO_PIN_RESET);
      if (is_touched) touch = XPT2046_getTouch();
      bool touch_pressed = (is_touched && !was_touched);
      was_touched = is_touched;

      /* ====================================================================
         STATE: MENU
         ==================================================================== */
      if (game_state == STATE_MENU)
      {
          ST7789_WriteString(40, 50, "FLAPPY", Font_16x26, CYAN, BLACK);
          ST7789_WriteString(40, 80, "CUBE", Font_16x26, CYAN, BLACK);

          if ((frame_count % 60) == 0) ST7789_WriteString(24, 170, "TAP TO START", Font_16x26, WHITE, BLACK);
          if ((frame_count % 60) == 30) ST7789_DrawFilledRectangle(24, 170, 12*16, 26, BLACK);

          // Smooth Hover Animation
          int old_y = bird_y;
          bird_y = 120 + (abs(((int)frame_count % 40) - 20) - 10) / 2;
          Update_Bird_Display(old_y, bird_y, YELLOW);

          if (touch_pressed) {
              ST7789_DrawFilledRectangle(0, 0, 240, 240, BLACK); // Full clear for next menu
              game_state = STATE_DIFFICULTY;
          }
      }
      /* ====================================================================
         STATE: DIFFICULTY SELECTOR
         ==================================================================== */
      else if (game_state == STATE_DIFFICULTY)
      {
          ST7789_WriteString(40, 20, "DIFFICULTY", Font_16x26, CYAN, BLACK);

          ST7789_WriteString(80, 70, "EASY", Font_16x26, GREEN, BLACK);
          ST7789_DrawLine(20, 105, 220, 105, LGRAY);

          ST7789_WriteString(70, 120, "NORMAL", Font_16x26, YELLOW, BLACK);
          ST7789_DrawLine(20, 155, 220, 155, LGRAY);

          ST7789_WriteString(80, 170, "HARD", Font_16x26, RED, BLACK);

          if (touch_pressed) {
              // Map Touch Y to difficulty blocks
              if (touch.y < 105) {
                  pipe_speed = 2; gap_size = 100; gravity_fp = 50; bird_base_color = GREEN;
              } else if (touch.y < 155) {
                  pipe_speed = 3; gap_size = 85; gravity_fp = 70; bird_base_color = YELLOW;
              } else {
                  pipe_speed = 4; gap_size = 70; gravity_fp = 95; bird_base_color = RED;
              }

              ST7789_DrawFilledRectangle(0, 0, 240, 240, BLACK);

              // Reset Game Variables
              bird_y_fp = (120 << 8);
              bird_vy_fp = FLAP_STRENGTH_FP;
              pipe_x = ST7789_WIDTH;
              gap_y = 40 + (rand() % (ST7789_HEIGHT - 80 - gap_size));
              score = 0;
              last_score = -1;
              score_color_timer = 0;
              current_level = 1;

              game_state = STATE_PLAYING;
          }
      }
      /* ====================================================================
         STATE: PLAYING
         ==================================================================== */
      else if (game_state == STATE_PLAYING)
      {
          /* 1. Sub-Pixel Physics Update */
          if (touch_pressed) bird_vy_fp = FLAP_STRENGTH_FP;

          int old_bird_y = bird_y;

          bird_vy_fp += gravity_fp;
          if (bird_vy_fp > TERM_VEL_FP) bird_vy_fp = TERM_VEL_FP;
          bird_y_fp += bird_vy_fp;

          bird_y = bird_y_fp >> 8;

          /* 2. Pipe Update */
          int old_pipe_x = pipe_x;
          pipe_x -= pipe_speed;

          if (pipe_x + PIPE_WIDTH < 0) {
              // Ensure left edge is clean
              Draw_Clipped_Rect(0, 0, pipe_speed + 2, ST7789_HEIGHT, BLACK);
              pipe_x = ST7789_WIDTH;
              score++;
              score_color_timer = 20;

              // LEVEL UP MECHANICS
              if (score % 10 == 0) {
                  current_level++;
                  // Day/Night Dimension Shift
                  ST7789_InvertColors(current_level % 2 == 0 ? 1 : 0);

                  // Progressively increase difficulty
                  if (gap_size > 55) gap_size -= 5;
                  if (pipe_speed < 6) pipe_speed += 1;
              }

              gap_y = 30 + (rand() % (ST7789_HEIGHT - 60 - gap_size));
          }

          /* 3. Graphics Update */
          // Glow WHITE if flying upwards, otherwise normal color
          uint16_t render_color = (bird_vy_fp < 0) ? WHITE : bird_base_color;
          Update_Bird_Display(old_bird_y, bird_y, render_color);

          int trail_x = pipe_x + PIPE_WIDTH;
          Draw_Clipped_Rect(trail_x, 0, pipe_speed, gap_y, BLACK);
          Draw_Clipped_Rect(trail_x, gap_y + gap_size, pipe_speed, ST7789_HEIGHT - (gap_y + gap_size), BLACK);

          Draw_Clipped_Rect(pipe_x, 0, pipe_speed, gap_y, GREEN);
          Draw_Clipped_Rect(pipe_x, gap_y + gap_size, pipe_speed, ST7789_HEIGHT - (gap_y + gap_size), GREEN);

          // Score Effect: Turns Green briefly when passing a pipe
          bool pipe_under_score = (pipe_x < 70 && pipe_x + PIPE_WIDTH > 0);
          if (score != last_score || pipe_under_score || score_color_timer > 0) {
              uint16_t s_color = (score_color_timer > 0) ? GREEN : WHITE;
              if (score_color_timer > 0) score_color_timer--;

              sprintf(text_buffer, "%d ", score);
              ST7789_WriteString(10, 10, text_buffer, Font_16x26, s_color, BLACK);
              last_score = score;
          }

          /* 4. Collision Detection */
          bool hit_ground_ceil = (bird_y + BIRD_SIZE >= ST7789_HEIGHT) || (bird_y <= 0);
          bool hit_pipe_x = (BIRD_X + BIRD_SIZE >= pipe_x) && (BIRD_X <= pipe_x + PIPE_WIDTH);
          bool hit_pipe_y = (bird_y <= gap_y) || (bird_y + BIRD_SIZE >= gap_y + gap_size);

          if (hit_ground_ceil || (hit_pipe_x && hit_pipe_y)) {
              if (score > high_score) high_score = score;

              ST7789_InvertColors(0); // Reset Day/Night Cycle
              Screen_Hit_Flash();     // Epic visual hit feedback

              bird_vy_fp = 0;         // Stop flying up
              game_state = STATE_DEATH_FALL;
          }
      }
      /* ====================================================================
         STATE: DEATH FALL (Bird falls down red, pipes freeze)
         ==================================================================== */
      else if (game_state == STATE_DEATH_FALL)
      {
          int old_bird_y = bird_y;

          bird_vy_fp += (gravity_fp * 2); // Fall fast
          bird_y_fp += bird_vy_fp;
          bird_y = bird_y_fp >> 8;

          if (bird_y + BIRD_SIZE >= ST7789_HEIGHT) {
              bird_y = ST7789_HEIGHT - BIRD_SIZE; // Snap to floor
              Update_Bird_Display(old_bird_y, bird_y, RED);

              // Erase Pipe
              if (pipe_x < ST7789_WIDTH) {
                  Draw_Clipped_Rect(pipe_x, 0, PIPE_WIDTH, gap_y, BLACK);
                  Draw_Clipped_Rect(pipe_x, gap_y + gap_size, PIPE_WIDTH, ST7789_HEIGHT - (gap_y + gap_size), BLACK);
              }
              ST7789_DrawFilledRectangle(10, 10, 5*16, 26, BLACK); // Erase Score

              game_state = STATE_GAMEOVER;
          } else {
              Update_Bird_Display(old_bird_y, bird_y, RED); // Turn red while falling
          }
      }
      /* ====================================================================
         STATE: GAMEOVER
         ==================================================================== */
      else if (game_state == STATE_GAMEOVER)
      {
          ST7789_WriteString(50, 40, "GAME OVER", Font_16x26, RED, BLACK);

          sprintf(text_buffer, "SCORE: %d ", score);
          ST7789_WriteString(40, 100, text_buffer, Font_16x26, WHITE, BLACK);

          sprintf(text_buffer, "BEST : %d ", high_score);
          ST7789_WriteString(40, 130, text_buffer, Font_16x26, YELLOW, BLACK);

          if ((frame_count % 60) == 0) ST7789_WriteString(24, 190, "TAP TO RETRY", Font_16x26, WHITE, BLACK);
          if ((frame_count % 60) == 30) ST7789_DrawFilledRectangle(24, 190, 12*16, 26, BLACK);

          if (touch_pressed) {
              ST7789_DrawFilledRectangle(50, 40, 9*16, 26, BLACK);
              ST7789_DrawFilledRectangle(40, 100, 10*16, 26, BLACK);
              ST7789_DrawFilledRectangle(40, 130, 10*16, 26, BLACK);
              ST7789_DrawFilledRectangle(24, 190, 12*16, 26, BLACK);
              ST7789_DrawFilledRectangle(BIRD_X, bird_y, BIRD_SIZE, BIRD_SIZE, BLACK);

              game_state = STATE_MENU;
              HAL_Delay(300); // Debounce
              was_touched = false;
          }
      }

      /* --- 60 FPS LIMITER --- */
      uint32_t frame_time = HAL_GetTick() - frame_start;
      if (frame_time < FPS_TARGET_MS) {
          HAL_Delay(FPS_TARGET_MS - frame_time);
      }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
