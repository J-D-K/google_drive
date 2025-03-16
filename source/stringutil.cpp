#include "stringutil.hpp"

void stringutil::strip_character(std::string &target, char c)
{
    size_t charPosition = 0;
    while ((charPosition = target.find_first_of(c, charPosition)) != target.npos)
    {
        target.erase(target.begin() + charPosition);
    }
}
