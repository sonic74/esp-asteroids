#ifndef _renderer_h_
#define _renderer_h_

#include "driver/gpio.h"
#include <vector>
#include "RenderBuffer.hpp"

//#define PIN_NUM_LASER GPIO_NUM_32

//class Game;

typedef enum {
    NORMAL,
    INVERTED,
    PWM
} DIM_MODE;

class Renderer
{
protected:
  std::vector<RenderBuffer *> render_buffers;
  int mode=0;
  gpio_num_t gpio_num[3]={GPIO_NUM_32, GPIO_NUM_33, /*GPIO_NUM_NC*/GPIO_NUM_25};
  DIM_MODE dim_modes[3]={INVERTED, PWM, NORMAL};

public:
  Renderer();

  virtual void start(/*int laser=0*/)/* = 0*/;
  void IRAM_ATTR set_laser(bool on, int laser=0);
  void set_mode(int mode);
  std::vector<RenderBuffer *> get_render_buffers() { return render_buffers; }

  volatile int rendered_frames[3]={0,0,0};
  volatile int transactions[3]={0,0,0};
  //volatile int delays;
};

#endif