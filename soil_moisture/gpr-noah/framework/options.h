#ifndef OPTIONS_H
#define OPTIONS_H
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <istream>
#include <ostream>
#include <iostream>
#include <stdexcept>

class invalid_input {
  std::string input;

public:
  invalid_input(std::string input) : input(input) { }
};


class options_source {
  virtual bool get_str(std::string name, std::string description, std::string& value) = 0;

public:
  template<typename T>
  bool get(std::string name, std::string description, T& value)
  {
    std::string str;
    if (!get_str(name, description, str))
      return false;

    std::stringstream ss(str);
    ss.exceptions(std::istream::failbit | std::istream::badbit);
    try {
      ss >> value;
    } catch (std::ios_base::failure& e) {
      throw invalid_input(str);
    }
    return true;
  }

  virtual ~options_source() {}
};

struct option_base {
  std::string name;
  std::string description;
  bool set;

public:
  option_base(std::string name, std::string description) :
    name(name), description(description), set(false) { }

  virtual void get(options_source& src) = 0;
  virtual void print_value(std::ostream& os) = 0;

  virtual size_t get_binary_size() = 0;
  virtual void   write_data(char*) = 0;
  virtual void   read_data(const char*)  = 0;

  virtual ~option_base() {}
};


template<typename T>
class option : public option_base {
  T& value;

public:
  option(std::string name, T& value, std::string description) : 
    option_base(name, description), value(value) { }

  void get(options_source& src)
  {
    if (src.template get<T>(name, description, value))
      set = true;
  }

  void print_value(std::ostream& os)
  {
    os << std::scientific;
    os << value;
  }
  
  size_t get_binary_size() { return sizeof(T); }
  void   write_data(char* p) { memcpy(p, &value, sizeof(T)); }
  void   read_data (const char* p) { memcpy(&value, p, sizeof(T)); }
};


// Specializations for string class
template<> size_t option<std::string>::get_binary_size();
template<> void   option<std::string>::write_data(char* p);
template<> void   option<std::string>::read_data (const char* p);

template<typename T>
class option<std::vector<T> > : public option_base {
  std::vector<T>& value;

public:
  option(std::string name, std::vector<T>& value, std::string description) : 
    option_base(name, description), value(value) { }

  void get(options_source& src)
  {
    std::vector<T> va;
    int len;
    std::stringstream tmp;
    tmp << name << ":length";
    if (!src.template get<int>(tmp.str(), std::string("List length"), len))
      return;

    for (int i=0; i<len; i++) {
      std::stringstream t;
      T v;
      t << name << ":" << i;
      src.get(t.str(), "List element", v);
      va.push_back(v);
    }
    set = true;
    value = va;
  }

  void print_value(std::ostream& os)
  {
    os << "{ ";
    for (typename std::vector<T>::iterator i=value.begin(); i != value.end(); i++) {
      os << *i << ", ";
    }
    os << "}";
  }

  size_t get_binary_size() { return sizeof(int) + value.size()*sizeof(T); }
  void   write_data(char* p)
  {
    int size = value.size();
    memcpy(p, &size, sizeof(int));
    p += sizeof(int);
    for(int i=0; i<size; ++i)
    {
      memcpy(p, &value[i], sizeof(T));
      p += sizeof(T);
    }
  } 
  void   read_data(const char* p)
  {
    int size;
    memcpy(&size, p, sizeof(int));
    p += sizeof(int);
    value.resize(size);
    for(int i=0; i<size; ++i)
    {
      memcpy(&value[i], p, sizeof(T));
      p += sizeof(T);
    }
  } 
};

class options {
  std::vector<option_base*> opts;

private:
  void get_all(options_source& src);

public:
  template<typename T>
  option<T>* add(std::string name, T& value, std::string description="")
  {
    option<T>* opt = new option<T>(name, value, description);
    opts.push_back(opt);
    return opt;
  }

  ~options() 
  {
    for (std::vector<option_base*>::iterator i=opts.begin(); i != opts.end(); i++)
      delete *i;
  }

  // This is generally what should be used to ask the user for options
  void read_options(int argc, char* argvp[]);

  void broadcast_options();
};


// This class represents a line of text
class textline: public std::string 
{
  public:
    textline(const char* p) : std::string(p) {}
    textline() {}
};

std::istream& operator>>(std::istream& input, textline& val);

// Specializations for textline class
template<> size_t option<textline>::get_binary_size();
template<> void   option<textline>::write_data(char* p);
template<> void   option<textline>::read_data (const char* p);
#endif
