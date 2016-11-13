#include "filter.h"

using namespace std;

Filter::Filter(vector<string> words) : words{words}
{

}

/**
 * looking for the words in a text
 * @param text to analyse
 * @return true if at least one of the words were found in the text
 */
bool Filter::process(string text){
    size_t i = 0;
    bool found = false;
    size_t res = 0;
    while(i < words.size() && !found) {
        res = text.find(words[i].data());
        if(res == string::npos){
            i++;
        }else{
            found = true;
        }
    }
    return found;
}