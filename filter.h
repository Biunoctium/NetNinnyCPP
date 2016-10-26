#ifndef NETNINNYCPP_FILTER_H
#define NETNINNYCPP_FILTER_H

#include <vector>
#include <stdlib.h>
#include <iostream>


class Filter{
public:
    std::vector<std::string> words;

    Filter(std::vector<std::string> words);
    bool process(std::string text);
};


#endif //NETNINNYCPP_FILTER_H
