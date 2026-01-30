#include <elf.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void print_elf_phdr(Elf64_Ehdr *elf_64);

int main(int argc, char *argv[]) {
    int fd = open(argv[1], O_RDONLY);
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    Elf64_Ehdr *temp = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    print_elf_phdr(temp);
    munmap(temp, size);
    close(fd);
    return (0);
}