#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>
#include "lodepng.h" //подгрузили
#include "lodepng.c" //подгрузили
//взяла функции из вашего кода, единственное что - поменяла тип данных pngsize на size_t, иначе командная строка ругается
float min(float a, float b) {
    if (a < b) {
        return a;
    }
    return b;
}
unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height) 
{
  unsigned char* image = NULL; 
  int error = lodepng_decode32_file(&image, width, height, filename);
  if(error != 0) {
    printf("error %u: %s\n", error, lodepng_error_text(error)); 
  }
  return (image);
}

void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
  unsigned char* png;
  size_t pngsize;
  int error = lodepng_encode32(&png, &pngsize, image, width, height);
  if(error == 0) {
      lodepng_save_file(png, pngsize, filename);
  } else { 
    printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  free(png);
}
//перевожу картинку в чб, luminosity method с сайта https://www.baeldung.com/cs/convert-rgb-to-grayscale
void rgb_bw(unsigned char *res_grsc, unsigned char *rgb_pic, int width, int height){
    int i;
    for(i = 0; i < width*height; i++){
        res_grsc[4 * i] = 0.3 * rgb_pic[4 * i] + 0.59*rgb_pic[4 * i+1] + 0.11 * rgb_pic[4 * i + 2];
	    res_grsc[4 * i + 1] = 0.3 * rgb_pic[4 * i] + 0.59*rgb_pic[4 * i+1] + 0.11 * rgb_pic[4 * i + 2];
	    res_grsc[4 * i + 2] = 0.3 * rgb_pic[4 * i] + 0.59*rgb_pic[4 * i+1] + 0.11 * rgb_pic[4 * i + 2];
	    res_grsc[4 * i + 3] = 255;
  }
  return ;
}
// гауссово ядро 5 на 5 для размытия
const float kernel[5][5] = {
    {1.0f/273,  4.0f/273,  7.0f/273,  4.0f/273, 1.0f/273},
    {4.0f/273, 16.0f/273, 26.0f/273, 16.0f/273, 4.0f/273},
    {7.0f/273, 26.0f/273, 41.0f/273, 26.0f/273, 7.0f/273},
    {4.0f/273, 16.0f/273, 26.0f/273, 16.0f/273, 4.0f/273},
    {1.0f/273,  4.0f/273,  7.0f/273,  4.0f/273, 1.0f/273}
};

void gaussian_blur_5x5(unsigned char* blur, unsigned char* gray, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float sum = 0.0f;
            for (int ky = -2; ky <= 2; ky++) {
                for (int kx = -2; kx <= 2; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    if (nx < 0) {
                        nx = 0;
                    } 
                    else {
                        if (nx >= width) {
                            nx = width - 1;
                        }
                    }
                    if (ny < 0) {
                        ny = 0;
                    } 
                    else {
                        if (ny >= height) {
                            ny = height - 1;
                        }
                    }
                    int index = (ny * width + nx) * 4;
                    sum += gray[index] * kernel[ky + 2][kx + 2];
                }
                int index = (y * width + x) * 4;
                blur[index] = (unsigned char)min(255, sum);
                blur[index + 1] = (unsigned char)min(255, sum);
                blur[index + 2] = (unsigned char)min(255, sum);
                blur[index + 3] = 255; //не троагем, как был так и остается
            }
        }
    }
}
//поиск компонент с помощью bfs
void find_connected_components(unsigned char* gray_img, int* components, unsigned width, unsigned height, int threshold, int min_component_size) {
    memset(components, -1, width * height * sizeof(int)); //метки для каждой компоненты
    int current_component = 0;
    int* queue_x = malloc(width * height * sizeof(int));
    int* queue_y = malloc(width * height * sizeof(int));
    for(unsigned y = 0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
            int idx = y * width + x;
            unsigned char pixel_value = gray_img[idx * 4];
            if(pixel_value < 30) {
                components[idx] = -3;
            }
            if(components[idx] == -1) {
                int queue_start = 0, queue_end = 1;
                queue_x[0] = x;
                queue_y[0] = y;
                components[idx] = current_component;
                while(queue_start < queue_end) {
                    int cx = queue_x[queue_start];
                    int cy = queue_y[queue_start];
                    queue_start++;
                    int dx[4] = {1, -1, 0, 0}; //вдохновилась тем как делала обход в задаче "химия"
                    int dy[4] = {0, 0, 1, -1};
                    for(int i = 0; i < 4; i++) {
                        int nx = cx + dx[i];
                        int ny = cy + dy[i];
                        if(nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int n_idx = ny * width + nx;
                            unsigned char n_value = gray_img[n_idx * 4]; // сосед
                            if(abs(pixel_value - n_value) <= threshold && 
                                components[n_idx] == -1) {
                                components[n_idx] = current_component;
                                queue_x[queue_end] = nx;
                                queue_y[queue_end] = ny;
                                queue_end++;
                            }
                        }
                    }
                }
                if(queue_end < min_component_size) {
                    for(int i = 0; i < queue_end; i++) {
                        components[queue_y[i] * width + queue_x[i]] = -2; //помечаем как шум
                    }
                } 
                else {
                    current_component++;
                }
            }
        }
    }
    free(queue_x);
    free(queue_y);
}

void color(int* components, unsigned char* finish, unsigned width, unsigned height) {
    const unsigned char palette[][4] = {
        {255, 179, 186, 255}, 
        {255, 223, 186, 255}, 
        {255, 255, 186, 255}, 
        {186, 255, 201, 255}, 
        {186, 225, 255, 255}, 
        {225, 186, 255, 255}
    };
    int palette_size = sizeof(palette) / (4 * sizeof(unsigned char));
    for(unsigned i = 0; i < width * height; i++) {
        int idx = i * 4;
        if(components[i] == -3) {
            finish[idx] = finish[idx+1] = finish[idx+2] = 0;
            finish[idx+3] = 255;
        } 
        else {
            if(components[i] == -2) {
                finish[idx] = finish[idx+1] = finish[idx+2] = 0;
                finish[idx+3] = 255;
            } 
            else {
                if(components[i] >= 0) {
                    int color_idx = components[i] % palette_size;
                    memcpy(&finish[idx], palette[color_idx], 4);
                }
            }
        }
    }
}


int main() {
    const char* filename = "skull.png";
    unsigned int width, height;
    unsigned char* pic = load_png("skull.png", &width, &height);
    if (pic == NULL) {
        printf("Problem reading picture from the file %s. Error.\n", filename);
        return -1;
    }
    int size = 4*width*height;
    unsigned char* gray = (unsigned char*) malloc(size*sizeof(unsigned char));
    unsigned char* blur = (unsigned char*) malloc(size*sizeof(unsigned char));
    unsigned char* finish = (unsigned char*) malloc(size*sizeof(unsigned char));
    rgb_bw(gray, pic, width, height); //переводим в чб 
    write_png("grayscale2.png", gray, width, height); //результат перевода в чб, если всмотреться, то картинка светлее, чем исходная
    gaussian_blur_5x5(blur, gray, width, height);  //подразмыли изображение с помощью ядра 5 на 5
    write_png("blur2.png", blur, width, height);
    int* components = malloc(width * height * sizeof(int));
    int threshold = 30; //порог различия яркостей
    int min_size = 100; // минимальный размер компоненты связности
    find_connected_components(blur, components, width, height, threshold, min_size);
    color(components, finish, width, height);
    write_png("finish2.png", finish, width, height);
    free(gray);
    free(blur);
    free(finish);
    free(pic);
    return 0;
}
