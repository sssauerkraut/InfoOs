[BITS 16] 
[ORG 0x7C00] 

start:
    ; Initialize segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00  ; Stack setup

    ; Print "Loading kernel..."
    mov si, loading_msg
    call print_string
get_time:
    mov ah, 0x02       ; Функция получения текущего времени
    int 0x1A           ; Вызов BIOS

    mov [0x7E00], ch ; Сохранение часов
    mov [0x7E01], cl  ; Сохранение минут
    mov [0x7E02], dh  ; Сохранение секунд

loading_kernel_data: 
    ; Drive setup
    mov dl, 0x01   ; Drive B:

    ; Set ES:BX to point to 0x00011000
    mov ax, 0x1100  
    mov es, ax
    mov bx, 0x0000  ; Offset 0x0000 (final address: 0x00011000)

    ;Becouse of PE format of kernel.bin, kernel's data should be copied
    ;in special way (check dumpbin /headers kernel.bin)
    ; Read 2 sectors from disk (sector 2) /.text
    mov ah, 0x02   ; Read function
    mov al, 10      ; Read 2 sectors
    mov ch, 0      ; Cylinder 0
    mov cl, 2      ; Sector 2
    mov dh, 0      ; Head 0
    int 0x13       
    jc disk_error  ; If error, print message and halt

    ; Print confirmation message
    mov si, es_ok_msg
    call print_string
    
    ; Set ES:BX to point to 0x00012000
    mov ax, 0x1300  
    mov es, ax
    mov bx, 0x0000  ; Offset 0x0000 (final address: 0x00012000)

    ; Read 5 sector from disk (sector 3) /.data
    mov ah, 0x02   ; Read function
    mov al, 7      ; Read 5 sectors
    mov ch, 0      ; Cylinder 0
    mov cl, 12      ; Sector 3
    mov dh, 0      ; Head 0
    int 0x13       
    jc disk_error  ; If error, print message and halt

    ; Print confirmation message
    mov si, es_ok_msg
    call print_string

loading_kernel:
    cli                     ; Disable interrupts
    lgdt [gdt_info]         ; Load GDT (Global Descriptor Table)

    in al, 0x92             ; Enable A20 line
    or al, 2
    out 0x92, al

    mov eax, cr0            ; Set PE bit in CR0 to switch to protected mode
    or al, 1                
    mov cr0, eax

    jmp 0x8:protected_mode  ; Far jump to load correct CS

; Handle disk error

disk_error:
    mov si, disk_err_msg
    call print_string
    hlt

; --- Function to print string to screen ---
print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

loading_msg db "Loading kernel...", 0
disk_err_msg db "Disk read error!", 0
es_ok_msg db "Data loaded, checking...", 0

; --- Function to dump memory (512 bytes - 1 sector) ---
print_memory_dump:
    pusha              
    mov cx, 512        
    mov bx, 0          

.loop:
    mov al, [es:bx]    ; Read byte from memory (ES:BX)
    call print_al_hex  ; Print it in HEX
    inc bx             ; Move to the next byte
    loop .loop        

    popa
    ret

; --- Function to print a byte in HEX ---
print_al_hex:
    pusha
    mov ah, 0x0E
    mov cl, al
    shr al, 4
    call print_digit
    mov al, cl
    and al, 0x0F
    call print_digit
    mov al, ' '
    int 0x10
    popa
    ret

print_digit:
    add al, '0'
    cmp al, '9'
    jbe .ok
    add al, 7
.ok:
    int 0x10
    ret

; --- GDT (Global Descriptor Table) ---
gdt:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF

gdt_info:
    dw gdt_info - gdt - 1 ; Maybe incorrect about -1
    dd gdt

[BITS 32] 
protected_mode:
    ; Loading segment selectors for stack and data into registers
    mov ax, 0x10        ; Descriptor number 2 in GDT is used
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Giving control to the loaded kernel
    call 0x00011000     ; Address equals load address of kernel 
                        ; (look at es:bx of .text segment of kernel)
    hlt

; Fill remaining space to 512 bytes with zeros
times (512 - ($ - start) - 2) db 0
db 0x55, 0xAA  ; Boot sector signature