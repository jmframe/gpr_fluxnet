// options.cpp
// Ben Gamari
// August 2009

#include "comm.h"
#include <unistd.h> // for isatty()
#include <string>
#include "string.h"
#include <iostream>
#include <sstream>
#include "options.h"


// Specializations for string class
template<>
size_t option<std::string>::get_binary_size() { return value.size()+1; }
template<>
void   option<std::string>::write_data(char* p) { memcpy(p, value.c_str(), value.size()+1); }
template<>
void   option<std::string>::read_data (const char* p) { value = std::string(p); }

static std::string trim(std::string str)
{
  const char* whitespace = " \t";
  if (str.empty())
    return str;

  int first = str.find_first_not_of(whitespace);
  int last =  str.find_last_not_of(whitespace); 
  return str.substr(first, last-first+1);
}

template<>
bool options_source::get<std::string>(std::string name, std::string description, std::string& value)
{
  return get_str(name, description, value);
}

class file_source : public options_source {
  struct argument {
    std::string name, value;
  };
  std::vector<argument> args;

protected:
  std::vector<argument> parse(std::istream& is) {
    std::vector<argument> args;
    while (!is.eof()) {
      argument arg;
      std::string line;
      std::getline(is, line);
      if (line.empty()) continue;
      line = trim(line);
      int i = line.find_first_of("\t ");
      arg.name = line.substr(0, i);
      std::string rest = line.substr(i);
      arg.value = rest.substr(rest.find_first_not_of("\t "));
      args.push_back(arg);
    }
    return args;
  }

public:
  file_source(std::istream& is) : args(parse(is)) { }

  bool get_str(std::string name, std::string description, std::string& value) {
    for (std::vector<argument>::iterator i=args.begin(); i != args.end(); i++)
      if (i->name == name) 
      {
        value = i->value;
        return true;
      }
    return false;
  }
};


class argument_source : public options_source {
  struct argument {
    std::string name, value;
  };
  std::vector<argument> args;

protected:
  std::vector<argument> parse(int argc, char* argv[])
  {
    std::vector<argument> args;
    for (int i=0; i < argc; i++) {
      argument arg;

      std::string str = argv[i];
      if (str.compare(0, 2, "--") == 0) {
	size_t eq_pos = str.find('=');
	if (eq_pos == std::string::npos)
	  throw std::runtime_error("Command line arguments must be in form of --NAME=VALUE");
	  
	arg.name = str.substr(2, eq_pos-2);
	arg.value = str.substr(eq_pos+1);
	args.push_back(arg);
      }
    }
    return args;
  }

public:
  argument_source(int argc, char* argv[]) : args(parse(argc, argv)) { }
  
  bool get_str(std::string name, std::string description, std::string& value) {
    for (std::vector<argument>::iterator i=args.begin(); i != args.end(); i++)
      if (i->name == name) {
	value = i->value;
	return true;
      }
    return false;
  }
};

class prompt_source : public options_source {
  std::istream& is;
  std::ostream& os;

public:
  prompt_source(std::istream& is, std::ostream& os) : is(is), os(os) { }

  bool get_str(std::string name, std::string description, std::string& value) {
    os << "\n";
    if (description.length())
      os << "* " << description << "\n";
    os << name << " >  ";
    std::getline(is, value);
    return true;
  }
};


void options::get_all(options_source& src)
{
  for (std::vector<option_base*>::iterator i=opts.begin(); i != opts.end(); i++)
    (*i)->get(src);
}

void options::read_options(int argc, char* argv[])
{
  rank_t this_node = get_rank();
  if(this_node == 0)
  {
    argument_source src(argc, argv);
    get_all(src);
    if (isatty(0)) {
      prompt_source src(std::cin, std::cout);
      get_all(src);
    } else {
      file_source src(std::cin);
      get_all(src);
    }

    unsigned int sizes[opts.size()];
    unsigned int max = 0;
    int count = 0;
    for (std::vector<option_base*>::iterator i=opts.begin(); i != opts.end(); i++)
    {
      sizes[count] = (*i)->name.size(); 
      if(sizes[count] > max) max = sizes[count];
      count++;
    }

    count = 0;
    std::cout << "\nOptions:\n";
    for (std::vector<option_base*>::iterator i=opts.begin(); i != opts.end(); i++)
    {
      std::cout << "  " << (*i)->name;
      for(unsigned int j=0; j<(max-sizes[count]); j++) std::cout << " ";
      std::cout << " = ";
      (*i)->print_value(std::cout);
      std::cout << "\n";
      count++;
    }
    std::cout << "End options.\n\n";
  }

  broadcast_options();
}


void options::broadcast_options()
{
  if(get_comm_size() == 1) return;
  int my_rank = get_rank();

  size_t size = 0;
  for(unsigned int i=0; i<opts.size(); ++i) 
    size += opts[i]->get_binary_size();

  broadcast((char*)&size, sizeof(size));

  char* buff = new char[size];

  if(my_rank == 0)
  {
    char* cur = buff;
    for(unsigned int i=0; i<opts.size(); ++i) 
    {
      opts[i]->write_data(cur);
      cur += opts[i]->get_binary_size();
    }
  }
  broadcast(buff, size);

  if(my_rank != 0)
  {
    char* cur = buff;
    for(unsigned int i=0; i<opts.size(); ++i) 
    {
      opts[i]->read_data(cur);
      cur += opts[i]->get_binary_size();
    }
  }

  delete [] buff;
}

// Specializations for textline class
std::istream& operator>>(std::istream& input, textline& val)
{
  getline(input, val);
  return input;
}  

template<>
size_t option<textline>::get_binary_size() { return value.size()+1; }
template<>
void   option<textline>::write_data(char* p) { memcpy(p, value.c_str(), value.size()+1); }
template<>
void   option<textline>::read_data (const char* p) { value = textline(p); }

