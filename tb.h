#include <SDL2/SDL.h>

class TB {

protected:

    int width, height, scale, frame_length, pticks;
    int x, y, _hs, _vs;

    SDL_Surface*        screen_surface;
    SDL_Window*         sdl_window;
    SDL_Renderer*       sdl_renderer;
    SDL_PixelFormat*    sdl_pixel_format;
    SDL_Texture*        sdl_screen_texture;
    SDL_Event           evt;
    Uint32*             screen_buffer;

    int vga_hs = 1, vga_vs = 1, vga_x = 0, vga_y = 2;

    // Память
    uint8_t* memory;

    // Модули
    Vc8088* mod_c8088;
    Vga*    mod_vga;

public:

    TB(int argc, char** argv) {

        x   = 0; y   = 0;
        _hs = 1; _vs = 1;

        memory  = (uint8_t*) malloc(64*1024);

        pticks      = 0;
        mod_c8088  = new Vc8088();
        mod_vga    = new Vga();

        // Удвоение пикселей
        scale        = 2;
        width        = 640;
        height       = 400;
        frame_length = 50;      // 20 кадров в секунду

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            exit(1);
        }

        SDL_ClearError();
        sdl_window          = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, scale*width, scale*height, SDL_WINDOW_SHOWN);
        sdl_renderer        = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
        screen_buffer       = (Uint32*) malloc(width * height * sizeof(Uint32));
        sdl_screen_texture  = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_SetTextureBlendMode(sdl_screen_texture, SDL_BLENDMODE_NONE);

        // Заполнить видеоинформацией
        for (int i = 0; i < 4096; i++) memory[0xB9000 + i] = font16[i];
        for (int i = 0; i < 4096; i += 2) {
            memory[0xB8000 + i    ] = i % 4 == 2 ? '/' : '\\';
            memory[0xB8000 + i + 1] = 0x07;
        }

        // Сброс процессора
        mod_c8088->reset_n = 0;
        mod_c8088->clock = 0; mod_c8088->eval();
        mod_c8088->clock = 1; mod_c8088->eval();
        mod_c8088->reset_n = 1;
        mod_c8088->ce = 1;
    }

    int main() {

        for (;;) {

            Uint32 ticks = SDL_GetTicks();

            // Прием событий
            while (SDL_PollEvent(& evt)) {

                // Событие выхода
                switch (evt.type) { case SDL_QUIT: return 0; }
            }

            // Обновление экрана
            if (ticks - pticks >= frame_length) {

                pticks = ticks;
                update();
                return 1;
            }

            SDL_Delay(1);
        }
    }

    // Один такт 25 мгц
    void frame() {

        // 100k x 20 = 2M in SEC
        int tps = 100000; // 100000
        for (int i = 0; i < tps; i++) {

            mod_vga->data = memory[0xB8000 + mod_vga->address];


            // ----------------------
            if (mod_c8088->we)
                memory[ mod_c8088->address ] = mod_c8088->out;

            mod_c8088->in = memory[ mod_c8088->address ];
            // ----------------------

            mod_c8088->clock = 0; mod_c8088->eval();
            mod_c8088->clock = 1; mod_c8088->eval();

            mod_vga->clock = 0; mod_vga->eval();
            mod_vga->clock = 1; mod_vga->eval();

            vga(mod_vga->hs, mod_vga->vs, (mod_vga->r*16)*65536 + (mod_vga->g*16)*256 + (mod_vga->b*16));
        }
    }

    // Обновить окно
    void update() {

        SDL_Rect dstRect;

        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = scale * width;
        dstRect.h = scale * height;

        SDL_UpdateTexture       (sdl_screen_texture, NULL, screen_buffer, width * sizeof(Uint32));
        SDL_SetRenderDrawColor  (sdl_renderer, 0, 0, 0, 0);
        SDL_RenderClear         (sdl_renderer);
        SDL_RenderCopy          (sdl_renderer, sdl_screen_texture, NULL, & dstRect);
        SDL_RenderPresent       (sdl_renderer);
    }

    // Убрать окно из памяти
    int destroy() {

        free(screen_buffer);
        free(memory);

        SDL_DestroyTexture(sdl_screen_texture);
        SDL_FreeFormat(sdl_pixel_format);
        SDL_DestroyRenderer(sdl_renderer);
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();

        return 0;
    }

    // Установка точки
    void pset(int x, int y, Uint32 cl) {

        if (x < 0 || y < 0 || x >= width || y >= height)
            return;

        screen_buffer[width*y + x] = cl;
    }

    // Отслеживание сигнала RGB по HS/VS
    void vga(int hs, int vs, int cl) {

        // Отслеживание на фронтах HS/VS
        if (hs) vga_x++;

        if (vga_hs == 1 && hs == 0) { vga_x = 0; vga_y++; }
        if (vga_vs == 0 && vs == 1) { vga_y = 0; }

        vga_hs = hs;
        vga_vs = vs;

        // Вывод на экран
        pset(vga_x-(96-48+2), vga_y-(35-2+4), cl);
    }
};
