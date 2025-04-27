#ifndef _esp32_game_h
#define _esp32_game_h

#include <vector>

typedef void *TaskHandle_t;

class RenderBuffer;
class Game;

class GameLoop
{
private:
  TaskHandle_t game_task_handle;
  Game *game;
  std::vector<RenderBuffer *> render_buffers;

public:
  GameLoop(Game *game, std::vector<RenderBuffer *> render_buffers) : game(game), render_buffers(render_buffers), steps(0) {}
  int steps;
  void start();
  friend void game_task(void *param);
  friend void timer_group1_isr(void *param);
};

#endif