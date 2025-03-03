extern "C" int kmain();

__declspec(naked) void startup() {
    __asm {
        call kmain;
    }
}

#define VIDEO_BUF_PTR 0xB8000

void out_str(int color, const char* ptr, unsigned int strnum) {
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    video_buf += 80 * 2 * strnum;
    while (*ptr) {
        video_buf[0] = (unsigned char)*ptr;
        video_buf[1] = color;
        video_buf += 2;
        ptr++;
    }
}

extern "C" int kmain() {
    const char* hello = "Welcome to MyOS!";
    out_str(0x07, hello, 0);

    while (1) {
        __asm hlt;
    }
    return 0;
}
