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
    int tps = 50000;
    int irq_mask = 0;
    int irq_pend = 0;

    // PS2-KEYBOARD
    int     ps_clock = 0, ps_data = 0, kbd_phase = 0, kbd_ticker = 0;
    uint8_t kbd[256], kbd_top = 0, kb_hit_cnt = 0, kb_latch = 0, kb_data = 0;

    // SDCARD: sd_status бит 7-timeout, 0-busy
    uint8_t     spi_data, spi_st = 2, spi_status, spi_command, spi_crc, spi_resp, sd_status = 0x80;
    uint32_t    spi_arg, spi_lba;
    uint16_t    spi_phase;
    uint8_t     spi_sector[512];
    FILE*       spi_file;

    // Память
    uint8_t* memory;

    // Модули
    Vcore*  mod_core;
    Vga*    mod_vga;
    Vps2*   mod_ps2;

public:

    TB(int argc, char** argv) {

        x   = 0; y   = 0;
        _hs = 1; _vs = 1;

        // 1MB alloc
        memory  = (uint8_t*) malloc(1024*1024 + 65536);

        pticks      = 0;
        mod_core   = new Vcore();
        mod_vga    = new Vga();
        mod_ps2    = new Vps2();

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
            memory[0xB8000 + i    ] = 0x00;
            memory[0xB8000 + i + 1] = 0x07;
        }

        // Сброс процессора
        mod_core->reset_n = 0;
        mod_core->clock = 0; mod_core->eval();
        mod_core->clock = 1; mod_core->eval();
        mod_core->reset_n = 1;
        mod_core->ce = 1;

        // Загрузка BIOS
        if (argc > 1) {

            FILE* fp = fopen(argv[1], "rb");
            if (fp) { fread(memory + 0xF0000, 1, 65536, fp); fclose(fp); }
        }
    }

    int main() {

        for (;;) {

            Uint32 ticks = SDL_GetTicks();

            // Прием событий
            while (SDL_PollEvent(& evt)) {

                // Событие выхода
                switch (evt.type) {

                    case SDL_QUIT: return 0;

                    case SDL_KEYDOWN:

                        kbd_scancode(evt.key.keysym.scancode, 0);
                        break;

                    case SDL_KEYUP:

                        kbd_scancode(evt.key.keysym.scancode, 1);
                        break;
                }
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

        int t_start = SDL_GetTicks();
        for (int i = 0; i < tps; i++) {

            // Обработка клавиатуры
            kbd_pop(ps_clock, ps_data);

            // ----------------------
            if (mod_core->we) memory[ mod_core->address ] = mod_core->out;
            mod_core->in = memory[ mod_core->address ];

            // Контроллер прерываний
            // ----------------------

            // Есть прерывание от клавиатуры. Если размаскировано, то записать в порт
            if (mod_ps2->done && (irq_mask & 2) && !irq_pend) {

                kb_data = mod_ps2->data;

                mod_core->irq = !mod_core->irq;
                mod_core->irq_in = 2;

                irq_pend = 1;
            }

            // Чтение из портов
            if (mod_core->port_r) {

                switch (mod_core->port_a) {

                    case 0x60: mod_core->port_i = kb_data; break;
                    case 0xFE: mod_core->port_i = sd_status; break;
                    case 0xFF: mod_core->port_i = spi_data; break;
                }
            }

            // Запись в порт
            if (mod_core->port_w) {

                switch (mod_core->port_a) {

                    case 0x20: irq_pend = 0; break; // EOI
                    case 0xA0: irq_mask = mod_core->port_o; break; // IOMASK
                    case 0xFE: sd_status = 0; /* Управление SD */ break;
                    case 0xFF: sdspi(mod_core->port_o); break; // Отослать данные к SD
                }
            }

            // printf("%x\n", mod_c8088->address);

            mod_core->clock = 0; mod_core->eval();
            mod_core->clock = 1; mod_core->eval();

            // PS/2 клавиатура
            mod_ps2->ps_clock = ps_clock;
            mod_ps2->ps_data  = ps_data;
            mod_ps2->clock = 0; mod_ps2->eval();
            mod_ps2->clock = 1; mod_ps2->eval();

            // Видеопамять
            mod_vga->data  = memory[0xB8000 + mod_vga->address];
            mod_vga->clock = 0; mod_vga->eval();
            mod_vga->clock = 1; mod_vga->eval();

            vga(mod_vga->hs, mod_vga->vs, (mod_vga->r*16)*65536 + (mod_vga->g*16)*256 + (mod_vga->b*16));
        }
        int t_interval = SDL_GetTicks() - t_start;

        // Автоматическая подстройка tps
        tps = (tps * frame_length) / t_interval;
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

    // Сканирование нажатой клавиши
    // https://ru.wikipedia.org/wiki/Скан-код
    void kbd_scancode(int scancode, int release) {

        switch (scancode) {

            // Коды клавиш A-Z
            case SDL_SCANCODE_A: if (release) kbd_push(0xF0); kbd_push(0x1C); break;
            case SDL_SCANCODE_B: if (release) kbd_push(0xF0); kbd_push(0x32); break;
            case SDL_SCANCODE_C: if (release) kbd_push(0xF0); kbd_push(0x21); break;
            case SDL_SCANCODE_D: if (release) kbd_push(0xF0); kbd_push(0x23); break;
            case SDL_SCANCODE_E: if (release) kbd_push(0xF0); kbd_push(0x24); break;
            case SDL_SCANCODE_F: if (release) kbd_push(0xF0); kbd_push(0x2B); break;
            case SDL_SCANCODE_G: if (release) kbd_push(0xF0); kbd_push(0x34); break;
            case SDL_SCANCODE_H: if (release) kbd_push(0xF0); kbd_push(0x33); break;
            case SDL_SCANCODE_I: if (release) kbd_push(0xF0); kbd_push(0x43); break;
            case SDL_SCANCODE_J: if (release) kbd_push(0xF0); kbd_push(0x3B); break;
            case SDL_SCANCODE_K: if (release) kbd_push(0xF0); kbd_push(0x42); break;
            case SDL_SCANCODE_L: if (release) kbd_push(0xF0); kbd_push(0x4B); break;
            case SDL_SCANCODE_M: if (release) kbd_push(0xF0); kbd_push(0x3A); break;
            case SDL_SCANCODE_N: if (release) kbd_push(0xF0); kbd_push(0x31); break;
            case SDL_SCANCODE_O: if (release) kbd_push(0xF0); kbd_push(0x44); break;
            case SDL_SCANCODE_P: if (release) kbd_push(0xF0); kbd_push(0x4D); break;
            case SDL_SCANCODE_Q: if (release) kbd_push(0xF0); kbd_push(0x15); break;
            case SDL_SCANCODE_R: if (release) kbd_push(0xF0); kbd_push(0x2D); break;
            case SDL_SCANCODE_S: if (release) kbd_push(0xF0); kbd_push(0x1B); break;
            case SDL_SCANCODE_T: if (release) kbd_push(0xF0); kbd_push(0x2C); break;
            case SDL_SCANCODE_U: if (release) kbd_push(0xF0); kbd_push(0x3C); break;
            case SDL_SCANCODE_V: if (release) kbd_push(0xF0); kbd_push(0x2A); break;
            case SDL_SCANCODE_W: if (release) kbd_push(0xF0); kbd_push(0x1D); break;
            case SDL_SCANCODE_X: if (release) kbd_push(0xF0); kbd_push(0x22); break;
            case SDL_SCANCODE_Y: if (release) kbd_push(0xF0); kbd_push(0x35); break;
            case SDL_SCANCODE_Z: if (release) kbd_push(0xF0); kbd_push(0x1A); break;

            // Цифры
            case SDL_SCANCODE_0: if (release) kbd_push(0xF0); kbd_push(0x45); break;
            case SDL_SCANCODE_1: if (release) kbd_push(0xF0); kbd_push(0x16); break;
            case SDL_SCANCODE_2: if (release) kbd_push(0xF0); kbd_push(0x1E); break;
            case SDL_SCANCODE_3: if (release) kbd_push(0xF0); kbd_push(0x26); break;
            case SDL_SCANCODE_4: if (release) kbd_push(0xF0); kbd_push(0x25); break;
            case SDL_SCANCODE_5: if (release) kbd_push(0xF0); kbd_push(0x2E); break;
            case SDL_SCANCODE_6: if (release) kbd_push(0xF0); kbd_push(0x36); break;
            case SDL_SCANCODE_7: if (release) kbd_push(0xF0); kbd_push(0x3D); break;
            case SDL_SCANCODE_8: if (release) kbd_push(0xF0); kbd_push(0x3E); break;
            case SDL_SCANCODE_9: if (release) kbd_push(0xF0); kbd_push(0x46); break;

            // Keypad
            case SDL_SCANCODE_KP_0: if (release) kbd_push(0xF0); kbd_push(0x70); break;
            case SDL_SCANCODE_KP_1: if (release) kbd_push(0xF0); kbd_push(0x69); break;
            case SDL_SCANCODE_KP_2: if (release) kbd_push(0xF0); kbd_push(0x72); break;
            case SDL_SCANCODE_KP_3: if (release) kbd_push(0xF0); kbd_push(0x7A); break;
            case SDL_SCANCODE_KP_4: if (release) kbd_push(0xF0); kbd_push(0x6B); break;
            case SDL_SCANCODE_KP_5: if (release) kbd_push(0xF0); kbd_push(0x73); break;
            case SDL_SCANCODE_KP_6: if (release) kbd_push(0xF0); kbd_push(0x74); break;
            case SDL_SCANCODE_KP_7: if (release) kbd_push(0xF0); kbd_push(0x6C); break;
            case SDL_SCANCODE_KP_8: if (release) kbd_push(0xF0); kbd_push(0x75); break;
            case SDL_SCANCODE_KP_9: if (release) kbd_push(0xF0); kbd_push(0x7D); break;

            // Специальные символы
            case SDL_SCANCODE_GRAVE:        if (release) kbd_push(0xF0); kbd_push(0x0E); break;
            case SDL_SCANCODE_MINUS:        if (release) kbd_push(0xF0); kbd_push(0x4E); break;
            case SDL_SCANCODE_EQUALS:       if (release) kbd_push(0xF0); kbd_push(0x55); break;
            case SDL_SCANCODE_BACKSLASH:    if (release) kbd_push(0xF0); kbd_push(0x5D); break;
            case SDL_SCANCODE_LEFTBRACKET:  if (release) kbd_push(0xF0); kbd_push(0x54); break;
            case SDL_SCANCODE_RIGHTBRACKET: if (release) kbd_push(0xF0); kbd_push(0x5B); break;
            case SDL_SCANCODE_SEMICOLON:    if (release) kbd_push(0xF0); kbd_push(0x4C); break;
            case SDL_SCANCODE_APOSTROPHE:   if (release) kbd_push(0xF0); kbd_push(0x52); break;
            case SDL_SCANCODE_COMMA:        if (release) kbd_push(0xF0); kbd_push(0x41); break;
            case SDL_SCANCODE_PERIOD:       if (release) kbd_push(0xF0); kbd_push(0x49); break;
            case SDL_SCANCODE_SLASH:        if (release) kbd_push(0xF0); kbd_push(0x4A); break;
            case SDL_SCANCODE_BACKSPACE:    if (release) kbd_push(0xF0); kbd_push(0x66); break;
            case SDL_SCANCODE_SPACE:        if (release) kbd_push(0xF0); kbd_push(0x29); break;
            case SDL_SCANCODE_TAB:          if (release) kbd_push(0xF0); kbd_push(0x0D); break;
            case SDL_SCANCODE_CAPSLOCK:     if (release) kbd_push(0xF0); kbd_push(0x58); break;
            case SDL_SCANCODE_LSHIFT:       if (release) kbd_push(0xF0); kbd_push(0x12); break;
            case SDL_SCANCODE_LCTRL:        if (release) kbd_push(0xF0); kbd_push(0x14); break;
            case SDL_SCANCODE_LALT:         if (release) kbd_push(0xF0); kbd_push(0x11); break;
            case SDL_SCANCODE_RSHIFT:       if (release) kbd_push(0xF0); kbd_push(0x59); break;
            case SDL_SCANCODE_RETURN:       if (release) kbd_push(0xF0); kbd_push(0x5A); break;
            case SDL_SCANCODE_ESCAPE:       if (release) kbd_push(0xF0); kbd_push(0x76); break;
            case SDL_SCANCODE_NUMLOCKCLEAR: if (release) kbd_push(0xF0); kbd_push(0x77); break;
            case SDL_SCANCODE_KP_MULTIPLY:  if (release) kbd_push(0xF0); kbd_push(0x7C); break;
            case SDL_SCANCODE_KP_MINUS:     if (release) kbd_push(0xF0); kbd_push(0x7B); break;
            case SDL_SCANCODE_KP_PLUS:      if (release) kbd_push(0xF0); kbd_push(0x79); break;
            case SDL_SCANCODE_KP_PERIOD:    if (release) kbd_push(0xF0); kbd_push(0x71); break;
            case SDL_SCANCODE_SCROLLLOCK:   if (release) kbd_push(0xF0); kbd_push(0x7E); break;

            // F1-F12 Клавиши
            case SDL_SCANCODE_F1:   if (release) kbd_push(0xF0); kbd_push(0x05); break;
            case SDL_SCANCODE_F2:   if (release) kbd_push(0xF0); kbd_push(0x06); break;
            case SDL_SCANCODE_F3:   if (release) kbd_push(0xF0); kbd_push(0x04); break;
            case SDL_SCANCODE_F4:   if (release) kbd_push(0xF0); kbd_push(0x0C); break;
            case SDL_SCANCODE_F5:   if (release) kbd_push(0xF0); kbd_push(0x03); break;
            case SDL_SCANCODE_F6:   if (release) kbd_push(0xF0); kbd_push(0x0B); break;
            case SDL_SCANCODE_F7:   if (release) kbd_push(0xF0); kbd_push(0x83); break;
            case SDL_SCANCODE_F8:   if (release) kbd_push(0xF0); kbd_push(0x0A); break;
            case SDL_SCANCODE_F9:   if (release) kbd_push(0xF0); kbd_push(0x01); break;
            case SDL_SCANCODE_F10:  if (release) kbd_push(0xF0); kbd_push(0x09); break;
            case SDL_SCANCODE_F11:  if (release) kbd_push(0xF0); kbd_push(0x78); break;
            case SDL_SCANCODE_F12:  if (release) kbd_push(0xF0); kbd_push(0x07); break;

            // Расширенные клавиши
            case SDL_SCANCODE_LGUI:         kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x1F); break;
            case SDL_SCANCODE_RGUI:         kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x27); break;
            case SDL_SCANCODE_APPLICATION:  kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x2F); break;
            case SDL_SCANCODE_RCTRL:        kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x14); break;
            case SDL_SCANCODE_RALT:         kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x11); break;
            case SDL_SCANCODE_KP_DIVIDE:    kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x4A); break;
            case SDL_SCANCODE_KP_ENTER:     kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x5A); break;

            case SDL_SCANCODE_INSERT:       kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x70); break;
            case SDL_SCANCODE_HOME:         kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x6C); break;
            case SDL_SCANCODE_END:          kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x69); break;
            case SDL_SCANCODE_PAGEUP:       kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x7D); break;
            case SDL_SCANCODE_PAGEDOWN:     kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x7A); break;
            case SDL_SCANCODE_DELETE:       kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x71); break;

            case SDL_SCANCODE_UP:           kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x75); break;
            case SDL_SCANCODE_DOWN:         kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x72); break;
            case SDL_SCANCODE_LEFT:         kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x6B); break;
            case SDL_SCANCODE_RIGHT:        kbd_push(0xE0); if (release) kbd_push(0xF0); kbd_push(0x74); break;

            // Клавиша PrnScr
            case SDL_SCANCODE_PRINTSCREEN: {

                if (release == 0) {

                    kbd_push(0xE0); kbd_push(0x12);
                    kbd_push(0xE0); kbd_push(0x7C);

                } else {

                    kbd_push(0xE0); kbd_push(0xF0); kbd_push(0x7C);
                    kbd_push(0xE0); kbd_push(0xF0); kbd_push(0x12);
                }

                break;
            }

            // Клавиша Pause
            case SDL_SCANCODE_PAUSE: {

                kbd_push(0xE1);
                kbd_push(0x14); if (release) kbd_push(0xF0); kbd_push(0x77);
                kbd_push(0x14); if (release) kbd_push(0xF0); kbd_push(0x77);
                break;
            }
        }
    }

    // Нажатие на клавишу
    void kbd_push(int data) {

        if (kbd_top >= 255) return;
        kbd[kbd_top] = data;
        kbd_top++;
    }

    // Извлечение PS/2
    void kbd_pop(int& ps_clock, int& ps_data) {

        // В очереди нет клавиш для нажатия
        if (kbd_top == 0) return;

        // 25000000/2000 = 12.5 kHz Очередной полутакт для PS/2
        if (++kbd_ticker >= 2000) {

            ps_clock = kbd_phase & 1;

            switch (kbd_phase) {

                // Старт-бит [=0]
                case 0: case 1: ps_data = 0; break;

                // Бит четности
                case 18: case 19:

                    ps_data = 1;
                    for (int i = 0; i < 8; i++)
                        ps_data ^= !!(kbd[0] & (1 << i));

                    break;

                // Стоп-бит [=1]
                case 20: case 21: ps_data = 1; break;

                // Небольшая задержка между нажатиями клавиш
                case 22: case 23:
                case 24: case 25:

                    ps_clock = 1;
                    ps_data  = 1;
                    break;

                // Завершение
                case 26:

                    // Удалить символ из буфера
                    for (int i = 0; i < kbd_top - 1; i++)
                        kbd[i] = kbd[i+1];

                    kbd_top--;
                    kbd_phase = -1;
                    ps_clock  = 1;
                    break;

                // Отсчет битов от 0 до 7
                // 0=2,3   | 1=4,5   | 2=6,7   | 3=8,9
                // 4=10,11 | 5=12,13 | 6=14,15 | 7=16,17
                default:

                    ps_data = !!(kbd[0] & (1 << ((kbd_phase >> 1) - 1)));
                    break;
            }

            kbd_ticker = 0;
            kbd_phase++;
        }
    }

    // Эмуляция протокола SDCard
    void sdspi(int data) {

        switch (spi_status) {

            // IDLE
            case 0:

                // Чтение в 0-м дает последний ответ от команды
                if (data == 0xFF) {

                    spi_data = spi_resp;
                    spi_resp = 0xFF;
                }
                // Запуск приема команды
                else if ((data & 0xC0) == 0x40) {

                    spi_status  = 1;
                    spi_command = data & 0x3F;
                    spi_arg     = 0;
                    spi_phase   = 0;
                }

                break;

            // COMMAND (прием)
            case 1: {

                if (spi_phase < 4) {

                    spi_arg = (spi_arg << 8) | data;
                    spi_phase++;

                } else {

                    spi_phase = 0;
                    spi_crc   = data;

                    /* Ответ зависит от команды */
                    switch (spi_command) {

                        /* CMDxx */
                        case 0:  spi_status = 0; spi_resp = 0x01; break;
                        case 8:  spi_status = 2; spi_resp = 0x00; break;
                        case 13: spi_status = 6; spi_resp = 0x00; break;    // STATUS
                        case 17: spi_status = 4; spi_lba  = spi_arg; break; // BLOCK SEARCH READ
                        case 24: spi_status = 5; spi_lba  = spi_arg; break; // BLOCK SEARCH WRITE
                        case 41: spi_status = 0; spi_resp = 0x00; break;    // READY=0
                        case 55: spi_status = 0; spi_resp = 0x01; break;    // ACMD=1
                        case 58: spi_status = 3; spi_resp = 0x00; break;    // CHECK=0
                        default: spi_status = 0; spi_resp = 0xFF; break;    // Unknown
                    }
                }

                break;
            }

            // OCR Read (5 bytes)
            case 2: {

                if (data == 0xFF) {

                    if (spi_phase == 4) {
                        spi_data = 0xAA;
                        spi_status = 0;
                    } else {
                        spi_data = 0x00;
                    }

                    spi_phase++;
                }
                else {
                    printf("SPI Illegal Write #1"); exit(1);
                }

                break;
            }

            // Информация о SDHC поддержке
            case 3: {

                if (data == 0xFF) {

                    if (spi_phase == 0) {
                        spi_data = 0x00;
                    } else if (spi_phase == 1) {
                        spi_data = 0xC0;
                    } else if (spi_phase < 4) {
                        spi_data = 0xFF;
                    } else {
                        spi_data = 0xFF;
                        spi_status = 0;
                    }

                    spi_phase++;

                } else {
                    printf("SPI Illegal Write #2"); exit(1);
                }

                break;
            }

            // Чтение с диска
            case 4: {

                if (spi_phase == 0) {

                    spi_data = 0x00;
                    spi_file = fopen("sd.img", "ab+");
                    if (spi_file == NULL) { printf("Error open file\n"); exit(0); }
                    fseek(spi_file, 512 * spi_lba, SEEK_SET);
                    (void) fread(spi_sector, 1, 512, spi_file);
                    fclose(spi_file);

                } else if (spi_phase == 1) {
                    spi_data = 0xFE;
                } else if (spi_phase < 514) {
                    spi_data = spi_sector[spi_phase - 2];
                }

                spi_phase++;
                if (spi_phase == 514) {

                    spi_status = 0;
                    spi_resp   = 0xFF;
                }

                break;
            }

            // Запись на диск
            case 5: {

                if (spi_phase == 0) {
                    spi_data = 0x00; // ACK

                } else if (spi_phase == 1) {

                    if (data != 0xFE) {
                        printf("Illegal opcode (non FE)\n"); exit(1);
                    }

                } else if (spi_phase < 514) {
                    spi_sector[spi_phase - 2] = data;

                } else if (spi_phase == 516) {
                    spi_data = 0x05; // ACK

                } else if (spi_phase < 520) {
                    spi_data = 0xFF;
                }

                spi_phase++;

                // Окончание программирования
                if (spi_phase == 520) {

                    spi_status = 0;
                    spi_resp   = 0x00;

                    // Запись новых данных на диск
                    spi_file = fopen("sd.img", "r+b");
                    if (spi_file == NULL) { printf("Error open file\n"); exit(0); }
                    fseek(spi_file, 512 * spi_lba, SEEK_SET);
                    (void) fwrite(spi_sector, 1, 512, spi_file);
                    fclose(spi_file);
                }

                break;
            }

            // STATUS [2 Byte 00 00]
            case 6: {

                if (data == 0xFF) {

                    if (spi_phase == 1)
                        spi_status = 0;

                    spi_data = 0x00;
                    spi_phase++;
                }
                else {
                    printf("SPI Illegal Write #1"); exit(1);
                }
            }
        }
    }
};
