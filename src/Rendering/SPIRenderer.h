#pragma once

#include <esp_attr.h>
#include "driver/gpio.h"
#include "Renderer.h"

#define COUNT_SPIS 1

class RenderBuffer;
class Font;

typedef struct spi_device_t *spi_device_handle_t; ///< Handle for a device on a SPI bus

class SPIRenderer : public Renderer
{
private:
  TaskHandle_t spi_task_handle;
  spi_device_handle_t spis[COUNT_SPIS];
  void IRAM_ATTR draw();
  volatile int draw_position[COUNT_SPIS];
  volatile int hold[COUNT_SPIS];

public:
  SPIRenderer(float world_size, Font *font);
  void start();
  friend void spi_draw_timer(void *para);
//  int objectTypes[1]={SHIP|ASTEROID|BULLET|HUD}; int spics_io_nums[1]={GPIO_NUM_27}; //CS pin
  static int objectTypes[COUNT_SPIS]/*={SHIP|ASTEROID|BULLET, HUD}*/; static int spics_io_nums[COUNT_SPIS]/*={GPIO_NUM_27, GPIO_NUM_13}*/;
};
