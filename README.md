# 🧊 Flappy Cube: 60 FPS STM32 Game Engine

![STM32](https://img.shields.io/badge/MCU-STM32F103C8T6-blue)
![Display](https://img.shields.io/badge/Display-ST7789_SPI-green)
![Touch](https://img.shields.io/badge/Touch-XPT2046-orange)
![FPS](https://img.shields.io/badge/Performance-60_FPS-red)

A highly optimized, hardware-accelerated "Flappy Bird" clone built from scratch for the **STM32 Blue Pill**. This project pushes the limits of the STM32F103C8T6 by utilizing Direct Memory Access (DMA), fixed-point mathematics, and strict delta-rendering. 

Remarkably, this project achieves a buttery-smooth 60 FPS over an SPI bus—proving that smart software architecture and memory management can overcome hardware bandwidth limitations on constrained microcontrollers.

## ✨ Features

* **60 FPS Gameplay:** Runs flawlessly at ~60 frames per second on a 72MHz microcontroller.
* **Sub-Pixel Physics Engine:** Uses 8-bit shifted fixed-point math for perfectly smooth parabolas and gravity calculations without floating-point overhead.
* **Interactive Menus & Difficulties:** Select between Easy, Normal, and Hard modes using the XPT2046 touch screen.
* **Progressive Leveling & Dimensions:** The game speeds up every 10 points. At Level 2, a hardware-level color inversion triggers a "Night Mode" dimension shift.
* **Hit-Flash Feedback:** Hardware-accelerated screen flashing when you hit a pipe.
* **Zero-Flicker Rendering:** Custom graphics pipeline that completely eliminates the dreaded "SPI screen wipe" effect.

---

## 🛠️ Hardware Requirements

1. **Microcontroller:** STM32F103C8T6 (Blue Pill)
2. **Display:** 2.4" or 2.8" SPI TFT LCD (ST7789 Driver, 240x240 or 320x240)
3. **Touch Controller:** XPT2046 (Usually integrated into the display module)
4. **Programmer:** ST-Link V2 (For flashing via SWD)

### 🔌 Wiring Guide (SPI1)

| STM32 Pin | Display / Touch Pin | Function |
| :--- | :--- | :--- |
| **PA5** | SCL / SCK | SPI1 Clock |
| **PA7** | SDA / MOSI | SPI1 Master Out |
| **PA6** | MISO / T_DO | SPI1 Master In (Touch Data) |
| **PA4** | CS / TFT_CS | TFT Chip Select |
| **PA3** | T_CS | Touch Chip Select |
| **PA2** | DC / RS | TFT Data/Command |
| **PA1** | RES / RST | TFT Reset |
| **PA0** | T_IRQ | Touch Interrupt |
| **3.3V** | VCC & BLK | Power & Backlight |
| **GND** | GND | Ground |

---

## 🚀 Advanced Code Optimizations

If you were to clear a full 240x240 screen and redraw it every frame using standard HAL libraries, the massive amount of SPI data transmission would cap the game at a flickering, unplayable framerate. To hit a locked 60 FPS (which requires calculating and drawing a frame in under 16.6ms), this project uses several advanced embedded engineering techniques:

### 1. Direct Memory Access (DMA) & Buffer Chunking
Standard HAL libraries use `HAL_SPI_Transmit()`. This is a *blocking* function: the CPU loads one byte into the SPI register, waits for it to send, loads the next, and so on. 
* **The DMA Solution:** We enable the STM32's DMA controller for `SPI1_TX`. When we need to draw a block of color, the CPU sets up the DMA, says "send these bytes," and immediately goes back to calculating the next frame's physics. The DMA hardware takes over the memory bus and blasts the pixels to the screen with zero CPU overhead.
* **The Memory Constraint:** The Blue Pill only has 20KB of RAM. A full 240x240 16-bit framebuffer requires 115KB. To fix this, we use a **partial line buffer** (`disp_buf`) that only holds 5 rows of pixels (2,400 bytes). If we need to draw a huge background, the CPU uses DMA to send this 2.4KB chunk repeatedly until the screen is filled.

### 2. Strict Delta & Sliver Rendering
Because a full screen clear takes too long, the screen is *never* cleared during gameplay. We only draw the pixels that have changed since the last frame (Delta Rendering).
* **The Pipes:** Instead of drawing a massive 40x80 pipe every frame, we only draw the tiny "leading edge" entering the screen in green, and we draw a small black rectangle on the "trailing edge" to erase where the pipe used to be.
* **The Bird (Sliver Rendering):** If the bird falls by 2 pixels, we don't erase the whole bird. We calculate the 2-pixel sliver of space it left behind at the top and paint it black, and paint the 2-pixel sliver it entered at the bottom yellow. No pixel is ever sent to the display twice.

### 3. Targeted Text Erasing
Updating the score usually causes massive flickering because text rendering is slow. Instead of clearing the top of the screen when the score changes from `9` to `10`, the engine calculates the exact bounding box of the previous text and sends a targeted black rectangle to erase only those specific pixels before drawing the new digits.

### 4. Sub-Pixel Fixed-Point Mathematics
Standard integers (`int y = 5`) result in jerky, blocky movement. Floating-point numbers (`float y = 5.25`) are far too slow for the Blue Pill's Cortex-M3 (which lacks a Hardware Floating Point Unit).
* **The Fix:** We use 8-bit shifted fixed-point integers (`int32_t bird_y_fp = 120 << 8`). This scales up our numbers, allowing the MCU to calculate gravity and velocity down to 1/256th of a pixel using lightning-fast bitwise shifts. Before rendering, we shift back down (`bird_y_fp >> 8`). The result is a perfectly smooth, hardware-friendly physics curve.

### 5. Endianness Pre-Calculation
The STM32 stores data in Little-Endian format, but the ST7789 display expects Big-Endian colors. Instead of mathematically shifting bits during the SPI transmission loop, the code pre-swaps the color bits `(color >> 8) | (color << 8)` *before* loading the DMA buffer. The DMA then blindly copies the pre-formatted memory directly to the display.

---

## 📂 Project Structure

```text
├── Core/
│   ├── Inc/
│   │   ├── main.h          # Main header, MCU configurations
│   │   ├── st7789.h        # Display driver definitions (Fonts, Colors)
│   │   └── XPT2046.h       # Touch controller definitions
│   └── Src/
│       ├── main.c          # Game Engine (State Machine, Physics, Rendering logic)
│       ├── st7789.c        # DMA-Accelerated display driver
│       └── XPT2046.c       # SPI Touch driver
├── Drivers/                # STM32 HAL libraries
├── FlappyCube.ioc          # STM32CubeMX Configuration file
└── README.md               # You are here!
