#include <stdint.h>
#include <style.h>


uint16_t string_center(int screen_width, int string_width, int char_width){
    return (screen_width / 2) - ((string_width * char_width) / 2);
}