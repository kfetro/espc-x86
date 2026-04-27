bits 16

section .text
    ; --- VBIOS ROM HEADER ---
    db 0x55, 0xAA          ; Mandatory signature (Byte 0-1)
    db 0x01                ; Size in 512-byte blocks (Byte 2)
    
    ; Jump to the beginning of the code
    jmp short start

    ; --- DATA AREA ---
header_text:
    db "EGA Dummy BIOS", 0

start:
    ; Ensure DS points to our ROM segment (0xC000) to read the string
    push cs
    pop ds

    ; (1) Change the INT 10h vector (0000:0040h)
    push es                ; Save ES
    xor ax, ax
    mov es, ax             ; ES = 0000h
    
    ; Point the vector to the int10_handler at the end of the code
    mov word [es:0x40], int10_handler  ; Offset within segment 0xC000
    mov word [es:0x42], 0xC000         ; Segment of the VBIOS
    pop es

    ; (2) Update BDA equipment word (0040:0010h) to indicate EGA
    push es
    mov ax, 0x0040
    mov es, ax             ; ES = 0040h (BIOS Data Area)
    or byte [es:0x10], 0x20 ; Set bits for EGA/VGA
    pop es

    ; (3) Call INT 10h to initialize mode 3 (80x25 text)
    mov ax, 0x0003
    int 0x10

    ; (4) Write the string using INT 10h, Service 0Eh (TTY)
    mov si, header_text    ; SI points to "EGA Dummy BIOS"
    mov ah, 0x0E           ; BIOS TTY function
    
print_loop:
    lodsb                  ; Load byte from DS:SI into AL and INC SI
    test al, al            ; Check for null terminator
    jz end_print           ; If zero, we are done
    int 0x10               ; Call video service (TTY)
    jmp print_loop

end_print:
    ; Return far (RETF) to the POST process
    retf

; --- INT 10h HANDLER PROCEDURE ---
align 2
int10_handler:
    iret
