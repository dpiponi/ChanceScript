#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ChanceScript.h"

struct command
{
};

void skip_space(const char*&ptr)
{
  while (std::isspace(*ptr))
  {
    ++ptr;
  }
}

std::string parse_token(const char *&ptr)
{
    std::string token;

    while (std::isalpha(*ptr))
    {
        token.push_back(*ptr);
        ++ptr;
    }

    std::cout << token << std::endl;
    return token;
}

command *parse_command(const char *&ptr)
{
    skip_space(ptr);
    if (std::isalpha(*ptr))
    {
        parse_token(ptr);
    }

    skip_space(ptr);

    if (*ptr == '=')
    {
      skip_space(ptr);
    }

    return new command{};
}

void parse(const char *&ptr)
{
    command *c = parse_command(ptr);
}

int main(int argc, char *argv[])
{
    std::string filename(argv[1]);

    std::ifstream file(filename, std::ios::in);

    if (!file)
    {
        throw std::runtime_error("Could not open the file: " + filename);
    }

    std::ostringstream oss;
    oss << file.rdbuf();

    std::string code = oss.str();
    const char *ptr = code.data();
    parse(ptr);
}
