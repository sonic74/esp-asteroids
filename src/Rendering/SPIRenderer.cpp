#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SPIRenderer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/timer.h"

#ifdef ARDUINO_M5STACK_CORES3
#define PIN_NUM_MOSI 23 // ToDo
#else
#define PIN_NUM_MOSI GPIO_NUM_23
#endif
#define PIN_NUM_CLK GPIO_NUM_18
//#define PIN_NUM_CS GPIO_NUM_27
#define PIN_NUM_LDAC GPIO_NUM_19

int SPIRenderer::objectTypes[COUNT_SPIS]={/*SHIP|ASTEROID|BULLET|HUD|UFO*/ALL}; int SPIRenderer::spics_io_nums[COUNT_SPIS]={GPIO_NUM_27};
//int SPIRenderer::objectTypes[COUNT_SPIS]={SHIP|ASTEROID|BULLET, HUD}; int SPIRenderer::spics_io_nums[COUNT_SPIS]={GPIO_NUM_27, GPIO_NUM_13};
//int SPIRenderer::objectTypes[COUNT_SPIS]={SHIP|ASTEROID|BULLET|HUD, SHIP|ASTEROID|BULLET, SHIP|ASTEROID}; int SPIRenderer::spics_io_nums[COUNT_SPIS]={GPIO_NUM_27, GPIO_NUM_13, GPIO_NUM_14};

void IRAM_ATTR spi_draw_timer(void *para)
{
  timer_spinlock_take(TIMER_GROUP_0);
  SPIRenderer *renderer = static_cast<SPIRenderer *>(para);
  renderer->draw();
  timer_spinlock_give(TIMER_GROUP_0);
}

void IRAM_ATTR SPIRenderer::draw()
{
  // Clear the interrupt
  timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
  // After the alarm has been triggered we need enable it again, so it is triggered the next time
  timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
//for(int spi=0; spi<render_buffers.size(); spi++) {
static int spi=0;
spi=(spi+1)%render_buffers.size();
  // do the actual drawing
  if (hold[spi] > 0)
  {
    //delays[spi]++;
    hold[spi]--;
  }
  else
  {
    // do we still have things to draw?
    if (draw_position[spi] < render_buffers[spi]->display_frame->size())
    {
      const DrawInstruction_t &instruction = render_buffers[spi]->display_frame->at(draw_position[spi]);
      hold[spi] = instruction.hold;
      int x = 4095 - instruction.y;
      int y = 4095 - instruction.x;
      // channel A
      spi_transaction_t t1;
      memset(&t1, 0, sizeof(t1)); //Zero out the transaction
      t1.length = 16;
      t1.flags = SPI_TRANS_USE_TXDATA;
      t1.tx_data[0] = 0b01010000 | ((x >> 8) & 0xF);
      t1.tx_data[1] = x & 0xFF;
      //ESP_ERROR_CHECK(spi_device_acquire_bus(spis[spi],portMAX_DELAY));
/*if(spi==1)*/      ESP_ERROR_CHECK(spi_device_polling_transmit(spis[spi], &t1));
      // channel B
/*      spi_transaction_t t2;
      memset(&t2, 0, sizeof(t2)); //Zero out the transaction
      t2.length = 16;
      t2.flags = SPI_TRANS_USE_TXDATA;*/
      t1.tx_data[0] = 0b11010000 | ((y >> 8) & 0xF);
      t1.tx_data[1] = y & 0xFF;
/*if(spi==1)*/      ESP_ERROR_CHECK(spi_device_polling_transmit(spis[spi], &t1));
      //spi_device_release_bus(spis[spi]);

      // set the laser state
      set_laser(instruction.laser, spi);

      // load the DAC
      gpio_set_level(PIN_NUM_LDAC, 0);
      gpio_set_level(PIN_NUM_LDAC, 1);

      draw_position[spi]++;
      transactions[spi]++;
    }
    else
    {
      // trigger a re-render
      rendered_frames[spi]++;
      render_buffers[spi]->swapBuffers();
      draw_position[spi] = 0;
    }
  }
//}
}

SPIRenderer::SPIRenderer(float world_size, Font *font)
{
for(int objectType=0; objectType<COUNT_SPIS; objectType++) {
  draw_position[objectType] = 0;
  hold[objectType] = 0;
  render_buffers.push_back(new RenderBuffer(
      0, 4095,
      0, 4095,
      2048,
      2048,
      2048.0f / world_size,
      font,
      objectTypes[objectType]));
}
}

void spi_timer_setup(void *param)
{
  // set up the renderer timer
  timer_config_t config = {
      .alarm_en = TIMER_ALARM_EN,
      .counter_en = TIMER_PAUSE,
      .intr_type = TIMER_INTR_LEVEL,
      .counter_dir = TIMER_COUNT_UP,
      .auto_reload = TIMER_AUTORELOAD_EN,
      .divider = 3000/COUNT_SPIS}; // default clock source is APB
  ESP_ERROR_CHECK(timer_init(TIMER_GROUP_0, TIMER_0, &config));

  // Timer's counter will initially start from value below.
  //   Also, if auto_reload is set, this value will be automatically reload on alarm
  ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL));

  // Configure the alarm value and the interrupt on alarm.
  ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 0x00000001ULL));
  ESP_ERROR_CHECK(timer_enable_intr(TIMER_GROUP_0, TIMER_0));
  ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_0, TIMER_0, spi_draw_timer,
                     param, ESP_INTR_FLAG_IRAM, NULL));

  ESP_ERROR_CHECK(timer_start(TIMER_GROUP_0, TIMER_0));
  while (true)
  {
    vTaskDelay(10000000);
  }
}

void SPIRenderer::start()
{
  // setup the LDAC output
  ESP_ERROR_CHECK(gpio_set_direction(PIN_NUM_LDAC, GPIO_MODE_OUTPUT));

  // setup SPI output
  spi_bus_config_t buscfg = {
      .mosi_io_num = PIN_NUM_MOSI,
      .miso_io_num = -1,
      .sclk_io_num = PIN_NUM_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num=-1,
      .data5_io_num=-1,
      .data6_io_num=-1,
      .data7_io_num=-1,
      .max_transfer_sz = 0,
      .flags=0,
      .intr_flags=0
      };
  spi_device_interface_config_t devcfg = {
      .command_bits = 0, ///< Default amount of bits in command phase (0-16), used when ``SPI_TRANS_VARIABLE_CMD`` is not used, otherwise ignored.
      .address_bits = 0, ///< Default amount of bits in address phase (0-64), used when ``SPI_TRANS_VARIABLE_ADDR`` is not used, otherwise ignored.
      .dummy_bits = 0,   ///< Amount of dummy bits to insert between address and data phase
      .mode = 0,         //SPI mode 0
      .duty_cycle_pos=0,
      .cs_ena_pretrans=0,
      .cs_ena_posttrans=0,
      .clock_speed_hz = SPI_MASTER_FREQ_8M, // 20002249B.pdf <= SPI_MASTER_FREQ_20M
      .input_delay_ns=0,
      .spics_io_num=0, // prevent compiler warning
      .flags = SPI_DEVICE_NO_DUMMY,
      .queue_size = 2, //We want to be able to queue 2 transactions at a time
                       // .post_cb = spi_post_callback // We'll use this for the LDAC line
      .pre_cb=NULL,
      .post_cb=NULL
  };
  //Initialize the SPI bus
#ifdef ARDUINO_M5STACK_CORES3
#define HSPI_HOST SPI3_HOST // TODO
#endif
  ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, /*SPI_DMA_CH1*//*1*//*SPI_DMA_CH_AUTO*/3/*SPI_DMA_DISABLED*//*0*/));
  //Attach the SPI device
for(int spi=0; spi<render_buffers.size(); spi++) {
  devcfg.spics_io_num=spics_io_nums[spi];
  ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &spis[spi]));
}

  // make sure to start the task on CPU 1
  TaskHandle_t timer_setup_handle;
  xTaskCreatePinnedToCore(spi_timer_setup, "Draw Task", 4096, this, 0, &timer_setup_handle, 1);

//for(int spi=0; spi<render_buffers.size(); spi++) {
  Renderer::start(/*spi*/);
//}
}
