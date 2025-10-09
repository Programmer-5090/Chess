#pragma once
#ifdef __cplusplus
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

inline std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
#endif