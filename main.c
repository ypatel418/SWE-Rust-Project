#include "ourconversionlib.h"

int main() {

    convertToJPG("input.png", 100);
    convertToPNG("input.png", 100);
    convertToTIFF("input.png", 100);
    convertToWEBP("input.png", 100);

    makeGIF((const char*[]){"input.png"}, 1, "output.gif", 100, -1, 500, 500);

    return 0;
}