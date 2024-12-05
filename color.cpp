#include "color.hpp"

const std::string Color::RESET = "\033[0m";

const std::string Color::RED = "\033[31m";
const std::string Color::GREEN = "\033[32m";
const std::string Color::YELLOW = "\033[33m";
const std::string Color::BLUE = "\033[34m";
const std::string Color::MAGENTA = "\033[35m";
const std::string Color::CYAN = "\033[36m";
const std::string Color::WHITE = "\033[37m";

std::string Color::color(const std::string& text, const std::string& color) {
    return color + text + RESET;
}
