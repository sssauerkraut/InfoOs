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
int strcmp( const char *s1, const char *s2 );
void out_str(int color, const char* ptr, unsigned int strnum);
void print_char_and_move_cursor(char c, unsigned int strnum, unsigned int pos);

void ticks_init();
void ticks_handler();
void keyb_init();
void intr_init();
void  intr_start();
void intr_disable();
void intr_enable() ;
void on_key(unsigned char scan_code);
void cursor_moveto(unsigned int strnum, unsigned int pos);

void cmd_info();
void cmd_clear();
void cmd_help();
void cmd_ticks();
void cmd_loadtime();
void cmd_curtime();
void cmd_uptime();
void cmd_cpuid();
void cmd_shutdown();
void cmd_notfound();

void process_command();
void start_time();


int cursor_x = 0;
int cursor_y = 0;
bool is_starting = 1;
bool clear_from_func = 0;
unsigned long int ticks = 0;
unsigned char start_seconds, start_minutes, start_hours;



// Функция преобразования BCD -> десятичное число
unsigned char bcd_to_decimal(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}


void clear_screen() {
    unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	for (int i = 0; i < VIDEO_WIDTH * 30; i++)
	{
		*(video_buf + i * 2) = '\0'; 
	}
    cursor_x = 0;
    cursor_y = 0;
    
    if (!is_starting && !clear_from_func){
        const char* prompt = "InfoOOPS> ";
        out_str(0xA, prompt, 0);
        cursor_x += 9;  // После вывода строки "infoOOPS>" перемещаем курсор
    }
    cursor_moveto(cursor_y, cursor_x);
}

//cmpstr( s1, s2 ) = -1
//cmpstr( s2, s1 ) = 1
//cmpstr( s1, s1 ) = 0
int strcmp( const char *s1, const char *s2 )
{
    while ( *s1 && *s1 == *s2 ) ++s1, ++s2;

    return ( ( unsigned char )*s1 > ( unsigned char )*s2 ) - 
           ( ( unsigned char )*s1 < ( unsigned char )*s2 );
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
    start_time();
    clear_screen();
    intr_start();
    intr_init();
    keyb_init();
    ticks_init();
	intr_enable();
    is_starting = 0;
    const char* hello = "Welcome to My Info Operating System!";
    out_str(0x07, hello, 0);
    // Печатаем приглашение `infoOOPS> ` на следующей строке
    const char* prompt = "InfoOOPS> ";
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

__inline void outw(unsigned short port, unsigned short data)
{
    __asm {
        push dx         // Сохраняем значение регистра DX
        mov dx, port    // Загружаем порт в регистр DX
        mov ax, data    // Загружаем 16-битное значение в регистр AX
        out dx, al      // Отправляем младший байт (AL) на порт
        mov al, ah      // Перемещаем старший байт (AH) в AL
        out dx, al      // Отправляем старший байт (AL) на порт
        pop dx          // Восстанавливаем регистр DX
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
           // out_str(0xA, cmd_buffer, cursor_y);  
            process_command();  // Обработка команды

            cmd_length = 0;  // Сбрасываем буфер
            cursor_y++;  

            out_str(0xA,  "InfOOPS> ", cursor_y);  
            cursor_x = 9;
            cursor_moveto(cursor_y, cursor_x);
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

char *commands[] = {
    "info",
    "clear",
    "help",
    "ticks",
    "loadtime",
    "curtime",
    "uptime",
    "cpuid",
    "shutdown"
};

void process_command(){
    if (cmd_buffer[0] != '\0'){
        if (strcmp(commands[0],cmd_buffer)==0) cmd_info();
        else if (strcmp(commands[1],cmd_buffer)==0) cmd_clear();
        else if (strcmp(commands[2],cmd_buffer)==0) cmd_help();
        else if (strcmp(commands[3],cmd_buffer)==0) cmd_ticks();
        else if (strcmp(commands[4],cmd_buffer)==0) cmd_loadtime();
        else if (strcmp(commands[5],cmd_buffer)==0) cmd_curtime();
        else if (strcmp(commands[6],cmd_buffer)==0) cmd_uptime();
        else if (strcmp(commands[7],cmd_buffer)==0) cmd_cpuid();
        else if (strcmp(commands[8],cmd_buffer)==0) cmd_shutdown();
        else cmd_notfound();
    }
}

void cmd_info(){
    char *info_str = "This Info_OS developed by Veronika Kvashennikova, \
a student of SPbPU group      5131001/30002   of the Higher School of Information Security. Implemented using the syntax of yasm and Microsoft C Compiler.\
Delight in using. (^._.^)";
    out_str(0x0f, info_str , cursor_y);  
    cursor_y+=2;
    cursor_moveto(cursor_y,cursor_x);
}

void cmd_clear(){
    clear_from_func = 1;
    clear_screen();
    clear_from_func = 0;
}

void cmd_help(){
     // Массив команд и их описаний
     char *help_text[] = {
        "info    - Displays information about the OS.",
        "clear   - Clears the screen.",
        "help    - Shows the help menu with a list of available commands.",
        "ticks   - Shows the number of ticks since the OS started.",
        "loadtime- Shows the time the OS was loaded.",
        "curtime - Displays the current system time.",
        "uptime  - Displays how long the system has been running.",
        "cpuid   - Displays information about the CPU.",
        "shutdown- Shuts down the OS."
    };

    // Выводим каждую строку из массива help_text
    for (int i = 0; i < sizeof(help_text) / sizeof(help_text[0]); i++) {
        out_str(0x0f, help_text[i], cursor_y);  // Выводим строку с командой
        cursor_y++;  // Перемещаем курсор на следующую строку
        cursor_moveto(cursor_y, cursor_x);  // Обновляем позицию курсора
    }
}

void ticks_init(){
    // Регистрация обработчика прерывания 
    intr_reg_handler(0x08, GDT_CS, 0x80 | IDT_TYPE_INTR, ticks_handler); 
    // segm_sel=0x8, P=1, DPL=0, Type=Intr  
    // Разрешение только прерываний клавиатуры от контроллера 8259 
    outb(PIC1_PORT + 1, 0xFF ^ 0x01); // 0xFF - все прерывания, 0x02 - 
    //бит IRQ1 (клавиатура).  
    // Разрешены будут только прерывания, чьи биты установлены в 0
}

__declspec(naked) void ticks_handler() 
{ 
    __asm pusha; 
    // Обработка поступивших данных 
    keyb_process_keys();  
    // Отправка контроллеру 8259 нотификации о том, что прерывание 
    //обработано. Если не отправлять нотификацию, то контроллер не будет посылать 
    //новых сигналов о прерываниях до тех пор, пока ему не сообщать что 
    //прерывание обработано.  
    ticks++;

    outb(PIC1_PORT , 0x20);  
    __asm { 
        popa 
        iretd 
    } 
} 

void cmd_ticks(){
    out_str(0x0F, "Ticks: ", cursor_y++);
    
    unsigned long int tmp_ticks = ticks;
    // Массив для хранения строки
    char arr_ticks[11];  // Максимум 10 символов для 32-битного числа и один для '\0'
    int i = 0;

     // Преобразование числа в строку
     if (tmp_ticks == 0) {
        arr_ticks[i++] = '0';  // Если ticks == 0, просто выводим '0'
    } else {
        while (tmp_ticks > 0) {
            arr_ticks[i++] = (tmp_ticks % 10) + '0';  // Заполняем массив цифрами
            tmp_ticks /= 10;
        }
    }
    
    arr_ticks[i] = '\0';  // Завершаем строку

    // Инвертируем строку
    for (int j = 0; j < i / 2; j++) {
        char temp = arr_ticks[j];
        arr_ticks[j] = arr_ticks[i - j - 1];
        arr_ticks[i - j - 1] = temp;
    }

    // Печатаем строку с ticks
    out_str(0x0F, arr_ticks, cursor_y++);  // Выводим всю строку целиком
    cursor_moveto(cursor_y, cursor_x);
}

#define CUR_TIME_PORT 0x70
#define GET_TIME_PORT 0x71

void cmd_curtime()
{
    // Буферы для секунд, минут и часов
    unsigned char seconds, minutes, hours;

    // Чтение секунд
    outb(0x70, 0);          // Устанавливаем адрес регистра секунд
    // Необходимо небольшое время ожидания
    seconds = inb(0x71);    // Чтение значения из порта 0x71

    // Чтение минут
    outb(0x70, 2);          // Устанавливаем адрес регистра минут
    // Задержка для синхронизации
    minutes = inb(0x71);    // Чтение значения из порта 0x71

    // Чтение часов
    outb(0x70, 4);          // Устанавливаем адрес регистра часов
    // Задержка для синхронизации
    hours = inb(0x71);      // Чтение значения из порта 0x71

    // Преобразуем значения BCD в нормальный формат
    seconds = bcd_to_decimal(seconds);
    minutes = bcd_to_decimal(minutes);
    hours = bcd_to_decimal(hours);

    // Создаем строку времени
    char time_str[9];
    time_str[0] = '0' + (hours / 10);
    time_str[1] = '0' + (hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (minutes / 10);
    time_str[4] = '0' + (minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (seconds / 10);
    time_str[7] = '0' + (seconds % 10);
    time_str[8] = '\0'; // Конец строки

    // Выводим строку
    out_str(0x0F, "Current time is:", cursor_y++);
    out_str(0x0F, time_str, cursor_y++);
    cursor_moveto(cursor_y, cursor_x);  // Обновляем позицию курсора
}

void start_time()
{
	outb(CUR_TIME_PORT, 0);
	start_seconds = inb(GET_TIME_PORT);
	outb(CUR_TIME_PORT, 2);
	start_minutes = inb(GET_TIME_PORT);
	outb(CUR_TIME_PORT, 4);
	start_hours = inb(GET_TIME_PORT);
    start_seconds = bcd_to_decimal(start_seconds);
    start_minutes = bcd_to_decimal(start_minutes);
    start_hours = bcd_to_decimal(start_hours);

}

void cmd_uptime(){
    // Буферы для секунд, минут и часов
    unsigned char seconds, minutes, hours;

    // Чтение секунд
    outb(0x70, 0);          // Устанавливаем адрес регистра секунд
    // Необходимо небольшое время ожидания
    seconds = inb(0x71);    // Чтение значения из порта 0x71

    // Чтение минут
    outb(0x70, 2);          // Устанавливаем адрес регистра минут
    // Задержка для синхронизации
    minutes = inb(0x71);    // Чтение значения из порта 0x71

    // Чтение часов
    outb(0x70, 4);          // Устанавливаем адрес регистра часов
    // Задержка для синхронизации
    hours = inb(0x71);      // Чтение значения из порта 0x71

    // Преобразуем значения BCD в нормальный формат
    seconds = bcd_to_decimal(seconds);
    minutes = bcd_to_decimal(minutes);
    hours = bcd_to_decimal(hours);

    // Вычисляем разницу между текущим временем и временем старта
    int uptime_seconds = seconds - start_seconds;
    int uptime_minutes = minutes - start_minutes;
    int uptime_hours = hours - start_hours;

    // Если количество секунд или минут меньше нуля, скорректируем время
    if (uptime_seconds < 0) {
        uptime_seconds += 60;
        uptime_minutes--;
    }

    if (uptime_minutes < 0) {
        uptime_minutes += 60;
        uptime_hours--;
    }

    // Если количество часов меньше нуля, скорректируем время (это можно сделать, если система работает больше 24 часов)
    if (uptime_hours < 0) {
        uptime_hours += 24;
    }               /// 01234567890123456789012345678901234567890123456789
    char *uptime_str = "Uptime is .. hour_, .. minut__ and .. second_";
    uptime_str[17] = (uptime_hours == 1) ? ' ' : 's';
    uptime_str[29] = (uptime_minutes == 1) ? ' ' : 's';
    uptime_str[44] = (uptime_seconds == 1) ? ' ' : 's';

    uptime_str[10] = '0' + (uptime_hours / 10);
    uptime_str[11] = '0' + (uptime_hours % 10);
    uptime_str[20] = '0' + (uptime_minutes / 10);
    uptime_str[21] = '0' + (uptime_minutes % 10);
    uptime_str[35] = '0' + (uptime_seconds / 10);
    uptime_str[36] = '0' + (uptime_seconds % 10);

    // Выводим строку с временем работы
    out_str(0x0F, uptime_str, cursor_y++);
    cursor_moveto(cursor_y, cursor_x);  // Обновляем позицию курсора
}

void cmd_cpuid()
{    
    char res[13];  // Буфер для строки (12 символов + '\0')
    // Используем временные переменные для хранения значений
    int ebx_val, edx_val, ecx_val;

    __asm {
        xor eax, eax         // Запрашиваем Vendor ID с EAX = 0
        cpuid                // Выполняем инструкцию CPUID

        mov ebx_val, ebx     // Сохраняем EBX в ebx_val (первые 4 байта Vendor ID)
        mov edx_val, edx     // Сохраняем EDX в edx_val (следующие 4 байта Vendor ID)
        mov ecx_val, ecx     // Сохраняем ECX в ecx_val (последние 4 байта Vendor ID)
        
    }
    // Заполняем строку из значений EBX, EDX и ECX
    ((int*)res)[0] = ebx_val;  // Первые 4 байта Vendor ID
    ((int*)res)[1] = edx_val;  // Следующие 4 байта Vendor ID
    ((int*)res)[2] = ecx_val;  // Последние 4 байта Vendor ID

    res[12] = 0;  // Завершаем строку нулевым символом ('\0')
    out_str(0x0F, "VendorId: ", cursor_y++);  // Заголовок
    out_str(0x0F, res, cursor_y++);

    cursor_moveto(cursor_y, cursor_x);  // Обновляем позицию курсора
}

void cmd_shutdown(){
    outw (0xB004, 0x2000); // qemu < 1.7, ex. 1.6.2
    outw (0x604, 0x2000);  // qemu >= 1.7  
}

void cmd_loadtime() {
    // Получаем BCD-время из памяти (где его сохранил загрузчик)
    unsigned char hours_bcd   = *(unsigned char*)0x7E00;
    unsigned char minutes_bcd = *(unsigned char*)0x7E01;
    unsigned char seconds_bcd = *(unsigned char*)0x7E02;

    // Преобразуем BCD -> десятичное
    unsigned char hours   = bcd_to_decimal(hours_bcd);
    unsigned char minutes = bcd_to_decimal(minutes_bcd);
    unsigned char seconds = bcd_to_decimal(seconds_bcd);

    // Выводим строку
    char *info_str = "Loadtime is: ";
    out_str(0x0F, info_str, cursor_y);  
    cursor_y++;
    cursor_moveto(cursor_y, cursor_x);

    // Создаем строку времени
    char time_str[9];
    time_str[0] = '0' + (hours / 10);
    time_str[1] = '0' + (hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (minutes / 10);
    time_str[4] = '0' + (minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (seconds / 10);
    time_str[7] = '0' + (seconds % 10);
    time_str[8] = '\0'; // Конец строки

    // Выводим время
    out_str(0x0F, time_str, cursor_y);
}

void cmd_notfound(){
    out_str(0x0C, "Command not found", cursor_y);
    cursor_y++;  // Перемещаем курсор на следующую строку
    cursor_moveto(cursor_y, cursor_x);  // Обновляем позицию курсора
}