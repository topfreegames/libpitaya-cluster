#include "string_utils.h"

#include <sstream>

using std::vector;
using std::string;

template<typename Out>
void DoSplit(const std::string &s, char delim, Out result)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

vector<string>
string_utils::Split(const string &str, char delim)
{
    vector<string> elems;
    DoSplit(str, delim, std::back_inserter(elems));
    return elems;
}
