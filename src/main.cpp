#include <iostream>
#include <utility>

int main(int argc, char** argv) {
    auto [a, b] = std::make_pair(1, "Hello");
    std::cout << "Hello, world!" << std::endl;
    std::cout << (a + 2) << std::endl;
    return 0;
}
