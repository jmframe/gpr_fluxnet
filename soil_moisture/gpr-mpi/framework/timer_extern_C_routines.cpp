#include <stdio.h>
#include <iostream>
#include <string>
#include "timer.h"

extern "C"
{

  timer* _timer()
  {
     return new timer();
  }

  void _timer_start_1(timer *tm, char* msg)
  {
      std::string _msg(msg);
      tm->start(_msg);
  }

  //void _timer_start_2(timer *tm, const std::string &msg, const std::string& color)
  void _timer_start_2(timer *tm, const char* msg, const char* color)
  {
      std::string _msg(msg);
      std::string _color(color);
      tm->start(_msg, _color);
  }

  void _timer_stop(timer *tm)
  {
      tm->stop();
  }

  void _delete_timer(timer *tm)
  {
      delete tm;
  }

}
