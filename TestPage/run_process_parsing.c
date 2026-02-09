#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <string.h>
#include <sys/types.h>

// /proc/pid/maps에서 베이스 주소 찾기
unsigned long find_base_address(pid_t pid) {
    char maps_path[256];
    char line[1024];
    unsigned long base_addr = 0;
    
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *maps = fopen(maps_path, "r");
    if (!maps) {
        perror("fopen maps");
        return 0;
    }
    
    // 첫 번째 r--p 또는 r-xp 영역 찾기 (보통 ELF 헤더가 여기)
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "r--p") || strstr(line, "r-xp")) {
            sscanf(line, "%lx", &base_addr);
            
            // ELF 매직 확인하기 위해 임시로 읽어봄
            char mem_path[256];
            snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
            int mem_fd = open(mem_path, O_RDONLY);
            if (mem_fd > 0) {
                unsigned char magic[4];
                lseek(mem_fd, base_addr, SEEK_SET);
                if (read(mem_fd, magic, 4) == 4) {
                    if (memcmp(magic, ELFMAG, SELFMAG) == 0) {
                        close(mem_fd);
                        fclose(maps);
                        return base_addr;
                    }
                }
                close(mem_fd);
            }
        }
    }
    
    fclose(maps);
    return 0;
}

void print_elf_from_memory(pid_t pid) {
    char mem_path[256];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    
    // 메모리 파일 열기
    int mem_fd = open(mem_path, O_RDONLY);
	printf("%s \n", mem_path);
    if (mem_fd < 0) {
        perror("open /proc/pid/mem");
        printf("Hint: root 권한이 필요할 수 있습니다.\n");
        return;
    }
    
    // 베이스 주소 찾기
    unsigned long base_addr = find_base_address(pid);
    if (base_addr == 0) {
        printf("ELF header not found in process memory\n");
        close(mem_fd);
        return;
    }
    
    printf("Found ELF at base address: 0x%lx\n\n", base_addr);
    
    // ELF Header 읽기
    Elf64_Ehdr ehdr;
    lseek(mem_fd, base_addr, SEEK_SET);
    if (read(mem_fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        perror("read ehdr");
        close(mem_fd);
        return;
    }
    
    printf("==================================================================\n");
    printf("ELF HEADER (from running process memory)\n");
    printf("==================================================================\n");
    printf("e_type:       0x%x\n", ehdr.e_type);
    printf("e_machine:    0x%x\n", ehdr.e_machine);
    printf("e_version:    0x%x\n", ehdr.e_version);
    printf("e_entry:      0x%lx\n", ehdr.e_entry);
    printf("e_phoff:      0x%lx\n", ehdr.e_phoff);
    printf("e_shoff:      0x%lx\n", ehdr.e_shoff);
    printf("e_flags:      0x%x\n", ehdr.e_flags);
    printf("e_ehsize:     0x%x (%d bytes)\n", ehdr.e_ehsize, ehdr.e_ehsize);
    printf("e_phentsize:  0x%x (%d bytes)\n", ehdr.e_phentsize, ehdr.e_phentsize);
    printf("e_phnum:      0x%x (%d entries)\n", ehdr.e_phnum, ehdr.e_phnum);
    printf("e_shentsize:  0x%x\n", ehdr.e_shentsize);
    printf("e_shnum:      0x%x\n", ehdr.e_shnum);
    printf("e_shstrndx:   0x%x\n", ehdr.e_shstrndx);
    
    // Program Headers 읽기
    printf("\n==================================================================\n");
    printf("PROGRAM HEADERS (from running process memory)\n");
    printf("==================================================================\n");
    
    // PHDR 위치로 이동 (base_addr + e_phoff)
    lseek(mem_fd, base_addr + ehdr.e_phoff, SEEK_SET);
    
    for (int i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr phdr;
        if (read(mem_fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
            perror("read phdr");
            break;
        }
        
        char flags[4] = "---";
        if (phdr.p_flags & PF_R) flags[0] = 'R';
        if (phdr.p_flags & PF_W) flags[1] = 'W';
        if (phdr.p_flags & PF_X) flags[2] = 'X';
        
        const char *type_str = "UNKNOWN";
        switch (phdr.p_type) {
            case PT_NULL: type_str = "PT_NULL"; break;
            case PT_LOAD: type_str = "PT_LOAD"; break;
            case PT_DYNAMIC: type_str = "PT_DYNAMIC"; break;
            case PT_INTERP: type_str = "PT_INTERP"; break;
            case PT_NOTE: type_str = "PT_NOTE"; break;
            case PT_PHDR: type_str = "PT_PHDR"; break;
            case 0x6474e550: type_str = "PT_GNU_EH_FRAME"; break;
            case 0x6474e551: type_str = "PT_GNU_STACK"; break;
            case 0x6474e552: type_str = "PT_GNU_RELRO"; break;
            case 0x6474e553: type_str = "PT_GNU_PROPERTY"; break;
        }
        
        printf("\n[Index %d] %s\n", i, type_str);
        printf("  p_type:    0x%x\n", phdr.p_type);
        printf("  p_flags:   0x%x (%s)\n", phdr.p_flags, flags);
        printf("  p_offset:  0x%lx\n", phdr.p_offset);
        printf("  p_vaddr:   0x%lx (runtime: 0x%lx)\n", 
               phdr.p_vaddr, base_addr + phdr.p_vaddr);
        printf("  p_paddr:   0x%lx\n", phdr.p_paddr);
        printf("  p_filesz:  0x%lx (%ld bytes)\n", phdr.p_filesz, phdr.p_filesz);
        printf("  p_memsz:   0x%lx (%ld bytes)\n", phdr.p_memsz, phdr.p_memsz);
        printf("  p_align:   0x%lx\n", phdr.p_align);
    }
    
    close(mem_fd);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        fprintf(stderr, "Example: sudo %s 12345\n", argv[0]);
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    print_elf_from_memory(pid);
    
    return 0;
}
