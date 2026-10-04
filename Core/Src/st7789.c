#include "st7789.h"

#ifdef USE_DMA
#include <string.h>
uint16_t DMA_MIN_SIZE = 16;
/*
 * 5 lines of 240 pixels = 1200 pixels (2400 bytes).
 * Easily fits in the Blue Pill's 20KB RAM and acts as our super-fast DMA buffer.
 */
#define HOR_LEN     5
uint16_t disp_buf[ST7789_WIDTH * HOR_LEN];
#endif

/**
  * @brief Write command to ST7789 controller
  */
static void ST7789_WriteCommand(uint8_t cmd)
{
    ST7789_Select();
    ST7789_DC_Clr();
    HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
    ST7789_UnSelect();
}

/**
  * @brief Write data to ST7789 controller
  */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
    ST7789_Select();
    ST7789_DC_Set();

    while (buff_size > 0) {
        uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
        #ifdef USE_DMA
            if (DMA_MIN_SIZE <= buff_size)
            {
                HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);
                // Universally safer to check SPI state to ensure DMA and SPI are both finished
                while (HAL_SPI_GetState(&ST7789_SPI_PORT) != HAL_SPI_STATE_READY) {}
            }
            else
                HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        #else
            HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        #endif
        buff += chunk_size;
        buff_size -= chunk_size;
    }

    ST7789_UnSelect();
}

/**
  * @brief Write data to ST7789 controller, simplify for 8bit data.
  */
static void ST7789_WriteSmallData(uint8_t data)
{
    ST7789_Select();
    ST7789_DC_Set();
    HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
    ST7789_UnSelect();
}

/**
  * @brief Set the rotation direction of the display
  */
void ST7789_SetRotation(uint8_t m)
{
    ST7789_WriteCommand(ST7789_MADCTL);
    switch (m) {
    case 0:
        ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
        break;
    case 1:
        ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
        break;
    case 2:
        ST7789_WriteSmallData(ST7789_MADCTL_RGB);
        break;
    case 3:
        ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
        break;
    default:
        break;
    }
}

/**
  * @brief Set address of DisplayWindow
  */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ST7789_Select();
    uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
    uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

    ST7789_WriteCommand(ST7789_CASET);
    {
        uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
        ST7789_WriteData(data, sizeof(data));
    }

    ST7789_WriteCommand(ST7789_RASET);
    {
        uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
        ST7789_WriteData(data, sizeof(data));
    }
    ST7789_WriteCommand(ST7789_RAMWR);
    ST7789_UnSelect();
}

/**
  * @brief Initialize ST7789 controller
  */
void ST7789_Init(void)
{
    #ifdef USE_DMA
        memset(disp_buf, 0, sizeof(disp_buf));
    #endif
    HAL_Delay(10);
    ST7789_RST_Clr();
    HAL_Delay(10);
    ST7789_RST_Set();
    HAL_Delay(20);

    ST7789_WriteCommand(ST7789_COLMOD);
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
    ST7789_WriteCommand(0xB2);
    {
        uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        ST7789_WriteData(data, sizeof(data));
    }
    ST7789_SetRotation(ST7789_ROTATION);

    ST7789_WriteCommand(0XB7);
    ST7789_WriteSmallData(0x35);
    ST7789_WriteCommand(0xBB);
    ST7789_WriteSmallData(0x19);
    ST7789_WriteCommand(0xC0);
    ST7789_WriteSmallData (0x2C);
    ST7789_WriteCommand (0xC2);
    ST7789_WriteSmallData (0x01);
    ST7789_WriteCommand (0xC3);
    ST7789_WriteSmallData (0x12);
    ST7789_WriteCommand (0xC4);
    ST7789_WriteSmallData (0x20);
    ST7789_WriteCommand (0xC6);
    ST7789_WriteSmallData (0x0F);
    ST7789_WriteCommand (0xD0);
    ST7789_WriteSmallData (0xA4);
    ST7789_WriteSmallData (0xA1);

    ST7789_WriteCommand(0xE0);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
        ST7789_WriteData(data, sizeof(data));
    }

    ST7789_WriteCommand(0xE1);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
        ST7789_WriteData(data, sizeof(data));
    }
    ST7789_WriteCommand (ST7789_INVOFF);
    ST7789_WriteCommand (ST7789_SLPOUT);
    ST7789_WriteCommand (ST7789_NORON);
    ST7789_WriteCommand (ST7789_DISPON);

    HAL_Delay(50);
    ST7789_Fill_Color(BLACK);
}

/**
  * @brief Fill the DisplayWindow with single color (DMA Optimized)
  */
void ST7789_Fill_Color(uint16_t color)
{
    ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_Select();

    #ifdef USE_DMA
        // Swap bytes because STM32 is Little Endian but Display expects Big Endian
        uint16_t swapped_color = (color >> 8) | (color << 8);

        uint32_t pixels_per_chunk = ST7789_WIDTH * HOR_LEN;
        for (uint32_t i = 0; i < pixels_per_chunk; i++) {
            disp_buf[i] = swapped_color;
        }

        uint32_t total_pixels = ST7789_WIDTH * ST7789_HEIGHT;
        uint32_t sent = 0;

        ST7789_DC_Set();
        while (sent < total_pixels) {
            uint32_t to_send = ((total_pixels - sent) > pixels_per_chunk) ? pixels_per_chunk : (total_pixels - sent);
            HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)disp_buf, to_send * 2);
            while (HAL_SPI_GetState(&ST7789_SPI_PORT) != HAL_SPI_STATE_READY) {}
            sent += to_send;
        }
    #else
        uint16_t i, j;
        for (i = 0; i < ST7789_WIDTH; i++) {
            for (j = 0; j < ST7789_HEIGHT; j++) {
                uint8_t data[] = {color >> 8, color & 0xFF};
                ST7789_WriteData(data, sizeof(data));
            }
        }
    #endif
    ST7789_UnSelect();
}

/**
  * @brief Draw a Pixel
  */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x < 0) || (x >= ST7789_WIDTH) || (y < 0) || (y >= ST7789_HEIGHT)) return;
    ST7789_SetAddressWindow(x, y, x, y);
    uint8_t data[] = {color >> 8, color & 0xFF};
    ST7789_Select();
    ST7789_WriteData(data, sizeof(data));
    ST7789_UnSelect();
}

/**
  * @brief Fill an Area with single color (DMA Optimized)
  */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
    ST7789_DrawFilledRectangle(xSta, ySta, (xEnd - xSta + 1), (yEnd - ySta + 1), color);
}

/**
  * @brief Draw a big Pixel at a point
  */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x <= 0) || (x > ST7789_WIDTH) || (y <= 0) || (y > ST7789_HEIGHT)) return;
    ST7789_DrawFilledRectangle(x - 1, y - 1, 3, 3, color);
}

/**
  * @brief Draw a line with single color
  */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
        swap = x0; x0 = y0; y0 = swap;
        swap = x1; x1 = y1; y1 = swap;
    }
    if (x0 > x1) {
        swap = x0; x0 = x1; x1 = swap;
        swap = y0; y0 = y1; y1 = swap;
    }
    int16_t dx = x1 - x0, dy = ABS(y1 - y0);
    int16_t err = dx / 2, ystep;

    ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) {
            ST7789_DrawPixel(y0, x0, color);
        } else {
            ST7789_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
  * @brief Draw a Rectangle with single color
  */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    ST7789_DrawLine(x1, y1, x2, y1, color);
    ST7789_DrawLine(x1, y1, x1, y2, color);
    ST7789_DrawLine(x1, y2, x2, y2, color);
    ST7789_DrawLine(x2, y1, x2, y2, color);
}

/**
  * @brief Draw a filled Rectangle with single color (DMA Accelerated)
  */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT || w == 0 || h == 0) return;
    if ((x + w) > ST7789_WIDTH) w = ST7789_WIDTH - x;
    if ((y + h) > ST7789_HEIGHT) h = ST7789_HEIGHT - y;

    ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint16_t swapped_color = (color >> 8) | (color << 8);
    uint32_t total_pixels = w * h;

    #ifdef USE_DMA
        uint32_t buf_size = ST7789_WIDTH * HOR_LEN;
        uint32_t chunk_pixels = (total_pixels < buf_size) ? total_pixels : buf_size;

        for (uint32_t i = 0; i < chunk_pixels; i++) {
            disp_buf[i] = swapped_color;
        }

        ST7789_Select();
        ST7789_DC_Set();

        uint32_t pixels_sent = 0;
        while (pixels_sent < total_pixels) {
            uint32_t pixels_to_send = total_pixels - pixels_sent;
            if (pixels_to_send > chunk_pixels) pixels_to_send = chunk_pixels;

            HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)disp_buf, pixels_to_send * 2);
            while (HAL_SPI_GetState(&ST7789_SPI_PORT) != HAL_SPI_STATE_READY) {}

            pixels_sent += pixels_to_send;
        }
        ST7789_UnSelect();
    #else
        ST7789_Select();
        ST7789_DC_Set();
        for (uint32_t i = 0; i < total_pixels; i++) {
            uint8_t data[] = {color >> 8, color & 0xFF};
            HAL_SPI_Transmit(&ST7789_SPI_PORT, data, 2, HAL_MAX_DELAY);
        }
        ST7789_UnSelect();
    #endif
}

/**
  * @brief Draw a circle with single color
  */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
    ST7789_DrawPixel(x0, y0 + r, color);
    ST7789_DrawPixel(x0, y0 - r, color);
    ST7789_DrawPixel(x0 + r, y0, color);
    ST7789_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        ST7789_DrawPixel(x0 + x, y0 + y, color);
        ST7789_DrawPixel(x0 - x, y0 + y, color);
        ST7789_DrawPixel(x0 + x, y0 - y, color);
        ST7789_DrawPixel(x0 - x, y0 - y, color);
        ST7789_DrawPixel(x0 + y, y0 + x, color);
        ST7789_DrawPixel(x0 - y, y0 + x, color);
        ST7789_DrawPixel(x0 + y, y0 - x, color);
        ST7789_DrawPixel(x0 - y, y0 - x, color);
    }
}

/**
  * @brief Draw an Image on the screen
  */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT)) return;
    if ((x + w - 1) >= ST7789_WIDTH) return;
    if ((y + h - 1) >= ST7789_HEIGHT) return;

    ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    #ifdef USE_DMA
        uint32_t data_size = w * h;
        uint32_t sent = 0;
        uint8_t *ptr = (uint8_t *)data;

        ST7789_Select();
        ST7789_DC_Set();
        while (sent < data_size) {
            uint32_t chunk = ((data_size - sent) > 32767) ? 32767 : (data_size - sent); // Stay below 64k byte limit
            HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, ptr + (sent * 2), chunk * 2);
            while (HAL_SPI_GetState(&ST7789_SPI_PORT) != HAL_SPI_STATE_READY) {}
            sent += chunk;
        }
        ST7789_UnSelect();
    #else
        ST7789_Select();
        ST7789_WriteData((uint8_t *)data, sizeof(uint16_t) * w * h);
        ST7789_UnSelect();
    #endif
}

/**
  * @brief Invert Fullscreen color
  */
void ST7789_InvertColors(uint8_t invert)
{
    ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
}

/**
  * @brief Write a char (DMA Accelerated RAM buffering)
  */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;
    ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

    uint16_t swapped_color = (color >> 8) | (color << 8);
    uint16_t swapped_bg = (bgcolor >> 8) | (bgcolor << 8);

    #ifdef USE_DMA
    uint32_t total_pixels = font.width * font.height;
    if (total_pixels <= (ST7789_WIDTH * HOR_LEN)) {
        uint32_t idx = 0;
        for (i = 0; i < font.height; i++) {
            b = font.data[(ch - 32) * font.height + i];
            for (j = 0; j < font.width; j++) {
                if ((b << j) & 0x8000) disp_buf[idx++] = swapped_color;
                else disp_buf[idx++] = swapped_bg;
            }
        }
        ST7789_Select();
        ST7789_DC_Set();
        HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, (uint8_t *)disp_buf, total_pixels * 2);
        while (HAL_SPI_GetState(&ST7789_SPI_PORT) != HAL_SPI_STATE_READY) {}
        ST7789_UnSelect();
        return;
    }
    #endif

    // Fallback for non-DMA or giant fonts
    ST7789_Select();
    ST7789_DC_Set();
    for (i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for (j = 0; j < font.width; j++) {
            uint8_t data[] = { ((b << j) & 0x8000) ? (color >> 8) : (bgcolor >> 8),
                               ((b << j) & 0x8000) ? (color & 0xFF) : (bgcolor & 0xFF) };
            HAL_SPI_Transmit(&ST7789_SPI_PORT, data, 2, HAL_MAX_DELAY);
        }
    }
    ST7789_UnSelect();
}

/**
  * @brief Write a string
  */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    while (*str) {
        if (x + font.width >= ST7789_WIDTH) {
            x = 0;
            y += font.height;
            if (y + font.height >= ST7789_HEIGHT) break;
            if (*str == ' ') { str++; continue; }
        }
        ST7789_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }
}

/**
  * @brief Draw a Triangle with single color
  */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
    ST7789_DrawLine(x1, y1, x2, y2, color);
    ST7789_DrawLine(x2, y2, x3, y3, color);
    ST7789_DrawLine(x3, y3, x1, y1, color);
}

/**
  * @brief Draw a filled Triangle with single color
  */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
    int16_t deltax = ABS(x2 - x1), deltay = ABS(y2 - y1);
    int16_t x = x1, y = y1;
    int16_t xinc1 = (x2 >= x1) ? 1 : -1, xinc2 = xinc1;
    int16_t yinc1 = (y2 >= y1) ? 1 : -1, yinc2 = yinc1;
    int16_t den, num, numadd, numpixels;

    if (deltax >= deltay) {
        xinc1 = 0; yinc2 = 0; den = deltax; num = deltax / 2; numadd = deltay; numpixels = deltax;
    } else {
        xinc2 = 0; yinc1 = 0; den = deltay; num = deltay / 2; numadd = deltax; numpixels = deltay;
    }

    for (int16_t curpixel = 0; curpixel <= numpixels; curpixel++) {
        ST7789_DrawLine(x, y, x3, y3, color);
        num += numadd;
        if (num >= den) { num -= den; x += xinc1; y += yinc1; }
        x += xinc2; y += yinc2;
    }
}

/**
  * @brief Draw a Filled circle with single color
  */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
    ST7789_DrawPixel(x0, y0 + r, color);
    ST7789_DrawPixel(x0, y0 - r, color);
    ST7789_DrawPixel(x0 + r, y0, color);
    ST7789_DrawPixel(x0 - r, y0, color);
    ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
        ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);
        ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
        ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
    }
}

/**
  * @brief Open/Close tearing effect line
  */
void ST7789_TearEffect(uint8_t tear)
{
    ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
}

void ST7789_Test(void)
{
    ST7789_Fill_Color(WHITE);
    HAL_Delay(500);
    ST7789_Fill_Color(BLUE);
    HAL_Delay(500);
    ST7789_Fill_Color(RED);
    HAL_Delay(500);
}
