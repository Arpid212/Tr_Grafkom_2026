#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

int main() {
    int width, height, channels;
    
    // Kita tes buka "test.jpg"
    unsigned char* data = stbi_load("test.jpg", &width, &height, &channels, 3);
    
    if (data) {
        std::cout << "BERHASIL!" << std::endl;
        std::cout << "Lebar: " << width << " piksel" << std::endl;
        std::cout << "Tinggi: " << height << " piksel" << std::endl;
        std::cout << "Channels: " << channels << std::endl;
        stbi_image_free(data);
    } else {
        std::cout << "GAGAL TOTAL!" << std::endl;
        std::cout << "Pesan error: " << stbi_failure_reason() << std::endl;
    }
    
    std::cout << "Tekan Enter untuk keluar..." << std::endl;
    std::cin.get();
    return 0;
}