#include "stm32f4xx.h"
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

// For ITM/SWO debug output
#if defined(STM32F4)
volatile int32_t ITM_RxBuffer; // Unused but required for ITM
#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000 + 4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000 + 4*n)))
#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000
#endif

// FreeRTOS memory management
extern uint8_t _end;  // End of bss section, provided by linker
static uint8_t *heap_end = 0;

// Close file
int _close(int file) {
    errno = EBADF;
    return -1;
}

// Write to file
int _write(int file, char *ptr, int len) {
#if defined(STM32F4)
    // Check if ITM is enabled
    if (!(DEMCR & TRCENA))
        return 0;
    
    // Use ITM/SWO for debug output (typically port 0)
    for (int i = 0; i < len; i++) {
        while (!ITM_Port32(0)); // Wait until port is ready
        ITM_Port8(0) = *ptr++;
    }
    return len;
#else
    errno = EBADF;
    return -1;
#endif
}

// Read from file
int _read(int file, char *ptr, int len) {
    errno = EBADF;
    return -1;
}

// Seek file
int _lseek(int file, int ptr, int dir) {
    errno = EBADF;
    return -1;
}

// Increase program data space
void *_sbrk(int incr) {
    uint8_t *prev_heap_end;
    
    if (heap_end == 0) {
        heap_end = &_end;
    }
    
    prev_heap_end = heap_end;
    
    // Check if we're not colliding with the stack
    if (heap_end + incr > (uint8_t*)__get_MSP()) {
        errno = ENOMEM;
        return (void*)-1;
    }
    
    heap_end += incr;
    return (void*)prev_heap_end;
}

// Additional stubs
int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

void _exit(int status) {
    while(1);
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

