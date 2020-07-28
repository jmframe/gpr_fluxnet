#include <stdio.h>
#include <iostream>
#include <string>
#include "options.h"

extern "C"
{
  /******************************* options.cpp ****************************************************/
  
  std::string* _string()
  {
     return new std::string;
  }

  void set_string(std::string* str,  char* val)
  {
       str->assign(val);
  }

  void get_string_size(std::string* str, int *size)
  {
    *size = str->size();
  }
  
  void get_string(std::string* str,  char* val, int len)
  {
    /* set entire buffer to blanks like fortran likes */
    for(int i=0; i<len; i++) val[i] = ' ';
    /* copy everything in str except c-null char */
    memcpy(val,str->c_str(),str->size()*sizeof(char));
  }
  
  void print_string(std::string* str)
  {
    std::cout << *str << std::endl;
  }

  void delete_string(std::string* str)
  {
     delete str;
  }
  
  options* _options()
  {
    return new options;
  }

  void _dealloc(options* opt)
  {
       delete opt;
  }
  
  void _read_options(options* opt, int argc, char* argvp[])
  {
     opt->read_options(argc,argvp); 
  }

  void add_double(options* opt, const char* name, double& value)
  {
    std::string _name(name);
    opt->add(_name, value);
  }
  
  void add_float(options* opt, const char* name, float& value)
  {
    std::string _name(name);
    opt->add(_name, value);
  }
  
  void add_int(options* opt, const char* name, int& value)
  {
    std::string _name(name);
    opt->add(_name, value);
  }

  void add_string(options* opt, const char* name,  std::string* value)
  {
    std::string _name(name);
    opt->add(_name, *value);
  }
}

