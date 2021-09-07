#include <iostream>

#include <view_manager/view_manager.h>

int main(int argc, char** argv) {
    std::cout << "We're gaming!" << std::endl;
    std::cout << view_manager::get_context_id() << std::endl;
}