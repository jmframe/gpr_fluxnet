#include <string>
#include "string.h"
#include "sys/time.h"
#include <stdio.h>
#include "comm.h"

#ifndef H_TIMER
#define H_TIMER

struct timer
{
  timeval tm;
  std::string msg;
  double time;
  std::string color;
  bool colored, _quiet;

  timer() { _quiet = false;}

  void color_selector(std::string& color)
  {
    if(get_rank() == 0 )
    {
      if(color.compare("red") == 0)     if(get_rank()==0) printf("\x1b[31m");
      if(color.compare("green") == 0)   if(get_rank()==0) printf("\x1b[32m");
      if(color.compare("yellow") == 0)  if(get_rank()==0) printf("\x1b[33m");
      if(color.compare("blue") == 0)    if(get_rank()==0) printf("\x1b[34m");
      if(color.compare("magenta") == 0) if(get_rank()==0) printf("\x1b[35m");
      if(color.compare("cyan") == 0)    if(get_rank()==0) printf("\x1b[36m");
      if(color.compare("reset") == 0)   if(get_rank()==0) printf("\x1b[0m");
    }
  }
  
  void start(const std::string &msg)
  {
    this->msg.assign(msg);
    if(! _quiet)
    {
      if(get_rank() == 0 )
        printf("Start:%s\n", msg.c_str());
    }
    gettimeofday(&tm, NULL);
  }

  void quiet()
  {
    _quiet = true;
  }
  
  void start(const std::string &msg, const std::string& color)
  {
    std::string reset = "reset";
    colored = true;
    this->color.assign(color);
    color_selector(this->color);
    if(! _quiet)
    {
      if(get_rank() == 0 )
        printf("Start: %s\n", msg.c_str());
      fflush(stdout);
    }
    color_selector(reset);
    gettimeofday(&tm, NULL);
  }
  
  void stop()
  {
    timeval tmp;
    gettimeofday(&tmp, NULL);
    time = tmp.tv_sec - tm.tv_sec;
    time += (tmp.tv_usec - tm.tv_usec)/1000000.0;
    std::string reset = "reset";
    if(! _quiet)
    {
      if(colored) color_selector(color);
      {
        if(get_rank() == 0 )
          printf("End:%s...done in %f [s].\n", msg.c_str(), time);
        fflush(stdout);
      }
      if(colored) color_selector(reset);
    }
  }
  ~timer() {}
};
#endif
