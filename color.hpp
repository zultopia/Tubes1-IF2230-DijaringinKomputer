#ifndef COLOR_HPP
#define COLOR_HPP

#include <string>

class Color {
public:
    static const std::string RESET;

    static const std::string RED;
    static const std::string GREEN;
    static const std::string YELLOW;
    static const std::string BLUE;
    static const std::string MAGENTA;
    static const std::string CYAN;
    static const std::string WHITE;


    static std::string color(const std::string& text, const std::string& color);
};

#endif 
