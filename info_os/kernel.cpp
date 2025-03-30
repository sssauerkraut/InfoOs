extern "C" int kmain();

__declspec(naked) void startup() {
    __asm {
        call kmain;
    }
}

#define VIDEO_BUF_PTR 0xB8000
#define VIDEO_WIDTH  (80) // Ширина текстового экрана 
#define MAX_ROWS 25  // Количество строк в текстовом режиме (обычно 25)


void clear_screen();
void out_str(int color, const char* ptr, unsigned int strnum);
void print_char_and_move_cursor(char c, unsigned int strnum, unsigned int pos);

void keyb_init();
void intr_init();
void  intr_start();
void intr_disable();
void intr_enable() ;
void on_key(unsigned char scan_code);
void cursor_moveto(unsigned int strnum, unsigned int pos);

int cursor_x = 0;
int cursor_y = 0;
bool is_starting = 1;

void clear_screen() {
    unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	for (int i = 0; i < VIDEO_WIDTH * 30; i++)
	{
		*(video_buf + i * 2) = '\0'; 
	}
    cursor_x = 0;
    cursor_y = 0;
    
    if (!is_starting){
        const char* prompt = "infoOOPS> ";
        out_str(0xA, prompt, 0);
        cursor_x += 9;  // После вывода строки "infoOOPS>" перемещаем курсор
    }
    cursor_moveto(cursor_y, cursor_x);
}

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

void print_str_and_move_cursor(char c, unsigned int strnum, unsigned int pos) {
    const char str[2] = { c, '\0' }; // Строка из одного символа
    out_str(0x07, str, strnum);      // Выводим символ на экран
    cursor_moveto(strnum, pos);     // Перемещаем курсор на новую позицию
}

extern "C" int kmain() {
    clear_screen();
    intr_start();
    intr_init();
    keyb_init();
	intr_enable();
    is_starting = 0;
    const char* hello = "Welcome to My Info Operating System!";
    out_str(0x07, hello, 0);
    // Печатаем приглашение `infoOOPS> ` на следующей строке
    const char* prompt = "infoOOPS> ";
    out_str(0xA, prompt, 1);
    // Устанавливаем курсор после `infoOOPS> `
    cursor_x = 10;  // Длина строки "infoOOPS> " — 10 символов
    cursor_y = 1;  
    cursor_moveto(cursor_y, cursor_x);
    while (1) {
        __asm hlt;
         // Проверяем, не вышли ли за пределы экрана
        if (cursor_y >= MAX_ROWS) {
            clear_screen();  // Очищаем экран
        }
    }
    return 0;
}

//!!! INTERRUPTS
void default_intr_handler();
void keyb_init();
void keyb_handler();
void keyb_process_keys();

#define IDT_TYPE_INTR (0x0E) 
#define IDT_TYPE_TRAP (0x0F) 
// Селектор секции кода, установленный загрузчиком ОС 
#define GDT_CS          (0x8) 
// Структура описывает данные об обработчике прерывания 
#pragma pack(push, 1) // Выравнивание членов структуры запрещено  
struct idt_entry 
{ 
    unsigned short base_lo;    // Младшие биты адреса обработчика 
    unsigned short segm_sel;   // Селектор сегмента кода 
    unsigned char always0;     // Этот байт всегда 0 
    unsigned char flags;       // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE... 
    unsigned short base_hi;    // Старшие биты адреса обработчика 
}; 

// Структура, адрес которой передается как аргумент команды lidt 
struct idt_ptr 
{ 
    unsigned short limit; 
    unsigned int base; 
};  
#pragma pack(pop)  

struct idt_entry g_idt[256]; // Реальная таблица IDT 
struct idt_ptr g_idtp;       // Описатель таблицы для команды lidt 

__declspec(naked) void default_intr_handler() 
{ 
 __asm { 
    pusha 
 } 
 
 // ... (реализация обработки) 
 
 __asm { 
    popa 
    iretd 
 } 
} 

typedef void (*intr_handler)(); 
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short 
flags, intr_handler hndlr) 
{ 
    unsigned int hndlr_addr = (unsigned int) hndlr; 
    g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF); 
    g_idt[num].segm_sel = segm_sel; 
    g_idt[num].always0 = 0; 
    g_idt[num].flags = flags; 
    g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16); 

} 

// Функция инициализации системы прерываний: заполнение массива с адресами обработчиков 
void intr_init() 
{ 
    int i; 
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]); 
    for(i = 0; i < idt_count; i++) 
        intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR, 
    default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr 
} 

void intr_start() 
{ 
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]); 
    
    g_idtp.base = (unsigned int) (&g_idt[0]); 
    g_idtp.limit = (sizeof (struct idt_entry) * idt_count) - 1; 
    
    __asm { 
    lidt g_idtp 
    } 
    
    //__lidt(&g_idtp); 
} 

void intr_enable() 
{ 
 __asm sti; 
} 
 
void intr_disable() 
{ 
 __asm cli; 
}

__inline unsigned char inb (unsigned short port) 
{ 
    unsigned char data; 
    __asm { 
        push dx 
        mov dx, port 
        in al, dx 
        mov data, al 
        pop dx 
    } 
    return data; 
} 

__inline void outb (unsigned short port, unsigned char data) 
{ 
    __asm { 
        push dx 
        mov dx, port 
        mov al, data 
        out dx, al 
        pop dx 
    } 
}

#define PIC1_PORT (0x20) 
void keyb_init() 
{ 
    // Регистрация обработчика прерывания 
    intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler); 
    // segm_sel=0x8, P=1, DPL=0, Type=Intr  
    // Разрешение только прерываний клавиатуры от контроллера 8259 
    outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1 (клавиатура).  
    // Разрешены будут только прерывания, чьи биты установлены в 0 
} 

__declspec(naked) void keyb_handler() 
{ 
    __asm pusha; 
    // Обработка поступивших данных 
    keyb_process_keys();  
    // Отправка контроллеру 8259 нотификации о том, что прерывание 
    //обработано. Если не отправлять нотификацию, то контроллер не будет посылать 
    //новых сигналов о прерываниях до тех пор, пока ему не сообщать что 
    //прерывание обработано.  
    outb(PIC1_PORT, 0x20);  
    __asm { 
        popa 
        iretd 
    } 
}

void keyb_process_keys() 
{ 
    // Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует) 
    if (inb(0x64) & 0x01)  
    { 
        unsigned char scan_code; 
        unsigned char state; 
        scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры 
        if (scan_code < 128)  // Скан-коды выше 128 - это отпускание клавиши 
            on_key(scan_code); 
    } 
}

// Простейшая таблица ASCII-символов для скан-кодов
char scan_code_table[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',  // Backspace
    '\t',  // Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',  // Enter
    0,  // Ctrl
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,  // Left Shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    0,  // Right Shift
    '*',
    0,  // Alt
    ' ',  // Space
    0,  // Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    // Остальные символы...
};

#define MAX_CMD_LEN 40  // Лимит на ввод команды (40 символов)
char cmd_buffer[MAX_CMD_LEN + 1];  // Буфер для команды
int cmd_length = 0;  // Текущая длина команды

void on_key(unsigned char scan_code)
{
    if (scan_code < 128) // Проверяем, что это нажатие, а не отпускание клавиши
    {
        char ch = scan_code_table[scan_code]; // Получаем ASCII-символ

        if (ch == '\b') {  // Backspace
            if (cmd_length > 0) {
                cmd_length--;  // Убираем символ из буфера
                cursor_x--;  // Двигаем курсор назад

                // Стираем символ с экрана (заменяем на пробел)
                unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
                int offset = (cursor_y * VIDEO_WIDTH + cursor_x) * 2;
                video_buf[offset] = ' ';
                video_buf[offset + 1] = 0x07;

                cursor_moveto(cursor_y, cursor_x);
            }
        }
        else if (ch == '\n') {  // Enter
            cmd_buffer[cmd_length] = '\0';  // Завершаем строку
            cursor_y++;  
            cursor_x = 0;

            //process_command();  // Обработка команды

            cmd_length = 0;  // Сбрасываем буфер
            cursor_y++;  

            out_str(0xA,  "InfOOPS> ", cursor_y);  
            cursor_x = 9;
        }
        else if (ch && cmd_length < MAX_CMD_LEN) {  // Ограничение в 40 символов
            unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
            int offset = (cursor_y * VIDEO_WIDTH + cursor_x) * 2;

            video_buf[offset] = ch;
            video_buf[offset + 1] = 0x07;

            cmd_buffer[cmd_length++] = ch;

            cursor_x++;
            cursor_moveto(cursor_y, cursor_x);
        }
    }
}

// Базовый порт управления курсором текстового экрана. Подходит для 
//большинства, но может отличаться в других BIOS и в общем случае адрес 
//должен быть прочитан из BIOS data area.  
#define CURSOR_PORT (0x3D4) 


// Функция переводит курсор на строку strnum (0 – самая верхняя) в позицию 
//pos на этой строке (0 – самое левое положение). 
void cursor_moveto(unsigned int strnum, unsigned int pos) 
{ 
    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos; 
    outb(CURSOR_PORT, 0x0F); 
    outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
    outb(CURSOR_PORT, 0x0E); 
    outb(CURSOR_PORT + 1, (unsigned char)( (new_pos >> 8) & 0xFF));
}