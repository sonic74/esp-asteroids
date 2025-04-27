#ifndef _i2s_renderer_h_
#define _i2s_renderer_h_

#include "Renderer.h"

class RenderBuffer;
class Font;

class I2SRenderer : public Renderer
{
private:
  volatile int draw_position;
  void IRAM_ATTR draw();

public:
  I2SRenderer(float world_size, Font *font);
  void start();
  friend void dma_dac_draw_timer(void *para);
};

#endif