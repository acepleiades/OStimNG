#include "StringUtil.h"

#include "Util.h"

namespace StringUtil {
    void toLower(std::string* string) {
        std::transform(string->begin(), string->end(), string->begin(), ::tolower);
    }

    void toLower(std::vector<std::string>* strings) {
        for (std::string string : *strings) {
            toLower(&string);
        }
    }

    std::vector<std::string> toTagVector(std::string string) {
        toLower(&string);
        return stl::string_split(string, ',');
    }

    std::vector<std::vector<std::string>> toTagMatrix(std::string string) {
        std::vector<std::vector<std::string>> ret;
        auto lists = stl::string_split(string, ';');
        for (auto &list : lists) {
            ret.push_back(toTagVector(list));
        }
        return ret;
    }

    std::string toTagCSV(std::vector<std::string> vector) {
        if (vector.empty()) {
            return "";
        }
        return std::accumulate(std::next(vector.begin()), vector.end(), vector[0], [](std::string a, std::string b) { return a + "," + b; });
    }

    void replaceAll(std::string& string, std::string const& find, std::string const& replace) {
        std::string buffer;
        size_t pos = 0;
        size_t prevPos;
        
        buffer.reserve(string.size());

        while (true) {
            prevPos = pos;
            pos = string.find(find, pos);
            if (pos == std::string::npos) {
                break;
            }
            buffer.append(string, prevPos, pos - prevPos);
            buffer += replace;
            pos += find.size();
        }

        buffer.append(string, prevPos, string.size() + prevPos);
        string.swap(buffer);
    }
}