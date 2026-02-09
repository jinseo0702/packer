#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = getpid();
    
    printf("========================================\n");
    printf("Hello, World!\n");
    printf("My PID: %d\n", pid);
    printf("========================================\n");
    printf("\n");
    printf("이 프로세스의 메모리를 읽으려면:\n");
    printf("  sudo ./parse_runtime_mem %d\n\n", pid);
    printf("Press Enter to exit...\n");
    
    getchar();  // 엔터 누를 때까지 대기
    
    printf("Exiting...\n");
    return 0;
}
