#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static inline  void *MOVE_ADDRESS( void *base, uint64_t offset) {
    if (base == NULL) return NULL;
     unsigned char *p = ( unsigned char*)base;
    return ( void*)(p + offset);
}

static inline uint8_t CHECK_RANGE(uint64_t offset, uint64_t size, uint64_t limit) {
    if (offset > limit) return 0;
    return (size <= (limit - offset));
}


void print_elf_phdr(Elf64_Ehdr *elf_64);

int main(int argc, char *argv[]) {

    unsigned char payload_bin[] = {
        0x55, 0x48, 0x89, 0xe5, 0x50, 0x57, 0x56, 0x52, 0x51, 0x41, 0x53, 0x41,
        0x50, 0x41, 0x51, 0x41, 0x52, 0xb8, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x01,
        0x00, 0x00, 0x00, 0x48, 0x8d, 0x35, 0x28, 0x00, 0x00, 0x00, 0xba, 0x10,
        0x00, 0x00, 0x00, 0x0f, 0x05, 0x41, 0x5a, 0x41, 0x59, 0x41, 0x58, 0x41,
        0x5b, 0x59, 0x5a, 0x5e, 0x5f, 0x58, 0x5d, 0x50, 0x48, 0x8d, 0x05, 0x0b,
        0x00, 0x00, 0x00, 0x48, 0x05, 0x44, 0x33, 0x22, 0x11, 0x48, 0x87, 0x04,
        0x24, 0xc3, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x50, 0x61,
        0x63, 0x6b, 0x65, 0x72, 0x0a, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
      };
      unsigned int payload_bin_len = 96;
      

    int fd = open(argv[1], O_RDONLY);
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    Elf64_Ehdr *temp = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    Elf64_Phdr *temp64 = MOVE_ADDRESS(temp, temp->e_phoff);
    Elf64_Phdr *inj;
    for (int i = 0; i < temp->e_phnum; i++) {
        if (temp64[i].p_type == PT_LOAD && temp64[i].p_flags == 5){
            inj = &temp64[i];
            break;
        }
    }
    int patch_offset = 0;
    for (size_t i = 0; i < payload_bin_len - 6; i++) {
        if (*(uint32_t *)(payload_bin + i) == 0x11223344){
            patch_offset = i;
        }
    }
    uint64_t real_offset = (inj->p_offset + inj->p_filesz + 15 ) & ~(0xF);
    int anchor_relative_pos = patch_offset + 4 + 4 + 1;
    int64_t anchor_file_offset = real_offset + anchor_relative_pos;
    int32_t jump_diff = (int32_t)(temp->e_entry - anchor_file_offset);
    memcpy(payload_bin + patch_offset, &jump_diff, sizeof(int32_t));
    unsigned char *injcode = MOVE_ADDRESS(temp, real_offset);
    temp->e_entry = inj->p_vaddr + (real_offset - inj->p_offset);
    uint64_t payload_end_offset = real_offset + payload_bin_len;
    uint64_t increased_size = payload_end_offset - inj->p_offset;
    inj->p_filesz = increased_size;
    inj->p_memsz = increased_size;
    memcpy(injcode, &payload_bin, payload_bin_len);
    int out_fd = open("Woody", O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out_fd != -1) {
        write(out_fd, temp, size); 
        close(out_fd);
    }
    munmap(temp, size);
    close(fd);
    return (0);
}