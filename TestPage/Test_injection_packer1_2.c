#include <elf.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static void *add_offset(void *base, uint64_t offset)
{
    return (void *)((unsigned char *)base + offset);
}

static uint64_t align_up_16(uint64_t value)
{
    return (value + 15ULL) & ~15ULL;
}

static size_t find_u64_placeholder(const unsigned char *buffer, size_t buffer_size, uint64_t placeholder)
{
    size_t i = 0;

    while (i + sizeof(uint64_t) <= buffer_size)
    {
        uint64_t current_value = 0;
        memcpy(&current_value, buffer + i, sizeof(uint64_t));
        if (current_value == placeholder)
            return i;
        i++;
    }
    return SIZE_MAX;
}

static int patch_u64_placeholder(unsigned char *buffer, size_t buffer_size, uint64_t placeholder, uint64_t value)
{
    size_t offset = find_u64_placeholder(buffer, buffer_size, placeholder);

    if (offset == SIZE_MAX)
        return 0;
    memcpy(buffer + offset, &value, sizeof(uint64_t));
    return 1;
}

static Elf64_Phdr *find_first_load_segment(Elf64_Phdr *program_headers, uint16_t header_count)
{
    uint16_t index = 0;

    while (index < header_count)
    {
        if (program_headers[index].p_type == PT_LOAD)
            return &program_headers[index];
        index++;
    }
    return NULL;
}

static Elf64_Phdr *find_executable_load_segment(Elf64_Phdr *program_headers, uint16_t header_count, uint16_t *segment_index)
{
    uint16_t index = 0;

    while (index < header_count)
    {
        if (program_headers[index].p_type == PT_LOAD && (program_headers[index].p_flags & PF_X))
        {
            *segment_index = index;
            return &program_headers[index];
        }
        index++;
    }
    return NULL;
}

static uint64_t find_next_load_offset(Elf64_Phdr *program_headers, uint16_t header_count, uint64_t current_offset)
{
    uint16_t index = 0;
    uint64_t next_offset = UINT64_MAX;

    while (index < header_count)
    {
        if (program_headers[index].p_type == PT_LOAD && program_headers[index].p_offset > current_offset)
        {
            if (program_headers[index].p_offset < next_offset)
                next_offset = program_headers[index].p_offset;
        }
        index++;
    }
    return next_offset;
}

static void xor_encrypt_segment_like_stub(unsigned char *file_bytes, size_t file_size, uint64_t segment_offset, uint64_t segment_filesz, uint64_t xor_key)
{
    uint64_t processed = 0;
    uint64_t max_size = segment_filesz;

    if (segment_offset >= file_size)
        return;
    if (segment_filesz > (uint64_t)(file_size - segment_offset))
        max_size = (uint64_t)(file_size - segment_offset);
    while (processed + 8ULL <= max_size)
    {
        uint64_t qword_value = 0;
        memcpy(&qword_value, file_bytes + segment_offset + processed, sizeof(uint64_t));
        qword_value ^= xor_key;
        memcpy(file_bytes + segment_offset + processed, &qword_value, sizeof(uint64_t));
        processed += 8ULL;
    }
    while (processed < max_size)
    {
        file_bytes[segment_offset + processed] ^= (unsigned char)xor_key;
        processed++;
    }
}

static void xor_encrypt_range_excluding(unsigned char *file_bytes,
                                       size_t file_size,
                                       uint64_t range_offset,
                                       uint64_t range_size,
                                       uint64_t exclude_offset,
                                       uint64_t exclude_size,
                                       uint64_t xor_key)
{
    uint64_t range_end = range_offset + range_size;
    uint64_t exclude_end = exclude_offset + exclude_size;

    if (range_offset >= (uint64_t)file_size || range_size == 0)
        return;
    if (range_end > (uint64_t)file_size)
        range_end = (uint64_t)file_size;

    if (exclude_size == 0 || exclude_end <= range_offset || exclude_offset >= range_end)
    {
        xor_encrypt_segment_like_stub(file_bytes, file_size, range_offset, range_end - range_offset, xor_key);
        return;
    }

    if (exclude_offset > range_offset)
        xor_encrypt_segment_like_stub(file_bytes, file_size, range_offset, exclude_offset - range_offset, xor_key);
    if (exclude_end < range_end)
        xor_encrypt_segment_like_stub(file_bytes, file_size, exclude_end, range_end - exclude_end, xor_key);
}

int main(int argc, char **argv)
{    unsigned char decode_stub_shellcode[] = {
  0x49, 0x89, 0xe1, 0x55, 0x48, 0x89, 0xe5, 0x50, 0x53, 0x51, 0x52, 0x56,
  0x57, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x41, 0x54, 0x41,
  0x55, 0x41, 0x56, 0x41, 0x57, 0x4c, 0x8d, 0x35, 0x1c, 0x02, 0x00, 0x00,
  0x41, 0x8b, 0x06, 0x3d, 0x45, 0x44, 0x4f, 0x43, 0x0f, 0x85, 0xd9, 0x01,
  0x00, 0x00, 0x4d, 0x8b, 0x7e, 0x18, 0x4c, 0x89, 0xce, 0x48, 0x83, 0xc6,
  0x08, 0x48, 0x8b, 0x06, 0x48, 0x83, 0xc6, 0x08, 0x48, 0x85, 0xc0, 0x75,
  0xf4, 0x48, 0x8b, 0x06, 0x48, 0x83, 0xc6, 0x08, 0x48, 0x85, 0xc0, 0x75,
  0xf4, 0x4d, 0x31, 0xe4, 0x4d, 0x31, 0xd2, 0xbb, 0x00, 0x10, 0x00, 0x00,
  0x48, 0x8b, 0x06, 0x48, 0x8b, 0x56, 0x08, 0x48, 0x85, 0xc0, 0x74, 0x21,
  0x48, 0x83, 0xf8, 0x03, 0x75, 0x03, 0x49, 0x89, 0xd4, 0x48, 0x83, 0xf8,
  0x05, 0x75, 0x03, 0x49, 0x89, 0xd2, 0x48, 0x83, 0xf8, 0x06, 0x75, 0x03,
  0x48, 0x89, 0xd3, 0x48, 0x83, 0xc6, 0x10, 0xeb, 0xd3, 0x4d, 0x85, 0xe4,
  0x0f, 0x84, 0x75, 0x01, 0x00, 0x00, 0x4d, 0x85, 0xd2, 0x0f, 0x84, 0x6c,
  0x01, 0x00, 0x00, 0x4d, 0x89, 0xe5, 0x4c, 0x89, 0xd1, 0x4d, 0x31, 0xc0,
  0x48, 0x85, 0xc9, 0x74, 0x1e, 0x41, 0x8b, 0x45, 0x00, 0x83, 0xf8, 0x06,
  0x75, 0x0c, 0x49, 0x8b, 0x45, 0x10, 0x4d, 0x89, 0xe0, 0x49, 0x29, 0xc0,
  0xeb, 0x09, 0x49, 0x83, 0xc5, 0x38, 0x48, 0xff, 0xc9, 0xeb, 0xdd, 0x4d,
  0x89, 0xe5, 0x4d, 0x85, 0xd2, 0x0f, 0x84, 0x06, 0x01, 0x00, 0x00, 0x41,
  0x8b, 0x45, 0x00, 0x83, 0xf8, 0x01, 0x0f, 0x85, 0xed, 0x00, 0x00, 0x00,
  0x49, 0x8b, 0x45, 0x08, 0x48, 0x85, 0xc0, 0x0f, 0x84, 0xe0, 0x00, 0x00,
  0x00, 0x41, 0x8b, 0x45, 0x04, 0xa9, 0x02, 0x00, 0x00, 0x00, 0x0f, 0x85,
  0xd1, 0x00, 0x00, 0x00, 0x4d, 0x8b, 0x4d, 0x10, 0x4d, 0x01, 0xc1, 0x4d,
  0x8b, 0x65, 0x20, 0x4d, 0x85, 0xe4, 0x0f, 0x84, 0xbd, 0x00, 0x00, 0x00,
  0x49, 0x8b, 0x55, 0x30, 0x48, 0x85, 0xd2, 0x75, 0x03, 0x48, 0x89, 0xda,
  0x48, 0x39, 0xda, 0x73, 0x03, 0x48, 0x89, 0xda, 0x48, 0x89, 0xd0, 0x48,
  0xff, 0xc8, 0x48, 0xf7, 0xd0, 0x4c, 0x89, 0xcf, 0x48, 0x21, 0xc7, 0x4b,
  0x8d, 0x34, 0x21, 0x48, 0x89, 0xd1, 0x48, 0xff, 0xc9, 0x48, 0x01, 0xce,
  0x48, 0x21, 0xc6, 0x48, 0x29, 0xfe, 0xb8, 0x0a, 0x00, 0x00, 0x00, 0xba,
  0x07, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x4c, 0x8d, 0x1d, 0xa3, 0xfe, 0xff,
  0xff, 0x48, 0x8d, 0x15, 0x04, 0x01, 0x00, 0x00, 0x4c, 0x89, 0xcf, 0x4c,
  0x89, 0xe1, 0x48, 0x83, 0xf9, 0x08, 0x72, 0x30, 0x4c, 0x39, 0xdf, 0x72,
  0x18, 0x48, 0x39, 0xd7, 0x73, 0x13, 0x48, 0x89, 0xd0, 0x48, 0x29, 0xf8,
  0x48, 0x39, 0xc8, 0x73, 0x4c, 0x48, 0x01, 0xc7, 0x48, 0x29, 0xc1, 0xeb,
  0xdd, 0x48, 0x8b, 0x07, 0x4c, 0x31, 0xf8, 0x48, 0x89, 0x07, 0x48, 0x83,
  0xc7, 0x08, 0x48, 0x83, 0xe9, 0x08, 0xeb, 0xca, 0x48, 0x85, 0xc9, 0x74,
  0x2c, 0x4c, 0x39, 0xdf, 0x72, 0x18, 0x48, 0x39, 0xd7, 0x73, 0x13, 0x48,
  0x89, 0xd0, 0x48, 0x29, 0xf8, 0x48, 0x39, 0xc8, 0x73, 0x17, 0x48, 0x01,
  0xc7, 0x48, 0x29, 0xc1, 0xeb, 0xe3, 0x8a, 0x07, 0x44, 0x30, 0xf8, 0x88,
  0x07, 0x48, 0xff, 0xc7, 0x48, 0xff, 0xc9, 0x75, 0xd4, 0x49, 0x83, 0xc5,
  0x38, 0x49, 0xff, 0xca, 0xe9, 0xf1, 0xfe, 0xff, 0xff, 0x4c, 0x89, 0xf7,
  0x48, 0x89, 0xd8, 0x48, 0xff, 0xc8, 0x48, 0xf7, 0xd0, 0x48, 0x21, 0xc7,
  0x48, 0x89, 0xde, 0xb8, 0x0a, 0x00, 0x00, 0x00, 0xba, 0x07, 0x00, 0x00,
  0x00, 0x0f, 0x05, 0x48, 0x8d, 0x3d, 0x3e, 0x00, 0x00, 0x00, 0xb9, 0x20,
  0x00, 0x00, 0x00, 0x31, 0xc0, 0xf3, 0xaa, 0x41, 0x5f, 0x41, 0x5e, 0x41,
  0x5d, 0x41, 0x5c, 0x41, 0x5b, 0x41, 0x5a, 0x41, 0x59, 0x41, 0x58, 0x5f,
  0x5e, 0x5a, 0x59, 0x5b, 0x58, 0x5d, 0x48, 0x8d, 0x05, 0x17, 0x00, 0x00,
  0x00, 0x48, 0x03, 0x05, 0x30, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
  0x45, 0x44, 0x4f, 0x43, 0x45, 0x44, 0x50, 0x57, 0x22, 0x22, 0x22, 0x22,
  0x11, 0x11, 0x11, 0x11, 0x44, 0x44, 0x44, 0x44, 0x33, 0x33, 0x33, 0x33,
  0x66, 0x66, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x88, 0x88, 0x88, 0x88,
  0x77, 0x77, 0x77, 0x77
    };

    const uint64_t placeholder_packed_magic = 0x57504445434F4445ULL;
    const uint64_t expected_packed_magic = 0x00000000434F4445ULL;
    const uint64_t placeholder_target_phdr_index = 0x1111111122222222ULL;
    const uint64_t placeholder_reserved = 0x3333333344444444ULL;
    const uint64_t placeholder_xor_key = 0x5555555566666666ULL;
    const uint64_t placeholder_original_entry_delta = 0x7777777788888888ULL;

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <xor_key> <elf_path>\n", argv[0]);
        return 1;
    }

    uint64_t xor_key = strtoull(argv[1], NULL, 0);
    const char *target_path = argv[2];

    int input_fd = open(target_path, O_RDONLY);
    if (input_fd < 0)
        return 1;

    off_t file_size = lseek(input_fd, 0, SEEK_END);
    if (file_size <= 0)
    {
        close(input_fd);
        return 1;
    }
    lseek(input_fd, 0, SEEK_SET);

    unsigned char *file_bytes = mmap(NULL, (size_t)file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_fd, 0);
    close(input_fd);
    if (file_bytes == MAP_FAILED)
        return 1;

    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)file_bytes;
    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }
    if (elf_header->e_type != ET_EXEC && elf_header->e_type != ET_DYN)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    if (elf_header->e_phoff + (uint64_t)elf_header->e_phnum * sizeof(Elf64_Phdr) > (uint64_t)file_size)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    Elf64_Phdr *program_headers = (Elf64_Phdr *)add_offset(file_bytes, elf_header->e_phoff);

    uint16_t executable_segment_index = 0;
    Elf64_Phdr *executable_segment = find_executable_load_segment(program_headers, elf_header->e_phnum, &executable_segment_index);
    Elf64_Phdr *first_load_segment = find_first_load_segment(program_headers, elf_header->e_phnum);
    if (executable_segment == NULL || first_load_segment == NULL)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    uint64_t stub_file_offset = align_up_16(executable_segment->p_offset + executable_segment->p_filesz);
    uint64_t next_load_offset = find_next_load_offset(program_headers, elf_header->e_phnum, executable_segment->p_offset);
    if (next_load_offset == UINT64_MAX)
        next_load_offset = (uint64_t)file_size;

    if (stub_file_offset + sizeof(decode_stub_shellcode) > next_load_offset)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    uint64_t stub_virtual_address = executable_segment->p_vaddr + (stub_file_offset - executable_segment->p_offset);

    size_t anchor_offset_in_stub = find_u64_placeholder(decode_stub_shellcode, sizeof(decode_stub_shellcode), placeholder_packed_magic);
    if (anchor_offset_in_stub == SIZE_MAX)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    uint64_t original_entry = elf_header->e_entry;
    uint64_t anchor_virtual_address = stub_virtual_address + (uint64_t)anchor_offset_in_stub;
    int64_t original_entry_delta = (int64_t)original_entry - (int64_t)anchor_virtual_address;

    if (!patch_u64_placeholder(decode_stub_shellcode, sizeof(decode_stub_shellcode), placeholder_packed_magic, expected_packed_magic) ||
        !patch_u64_placeholder(decode_stub_shellcode, sizeof(decode_stub_shellcode), placeholder_target_phdr_index, 0) ||
        !patch_u64_placeholder(decode_stub_shellcode, sizeof(decode_stub_shellcode), placeholder_reserved, 0) ||
        !patch_u64_placeholder(decode_stub_shellcode, sizeof(decode_stub_shellcode), placeholder_xor_key, xor_key) ||
        !patch_u64_placeholder(decode_stub_shellcode, sizeof(decode_stub_shellcode), placeholder_original_entry_delta, (uint64_t)original_entry_delta))
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    memcpy(file_bytes + stub_file_offset, decode_stub_shellcode, sizeof(decode_stub_shellcode));

    elf_header->e_entry = stub_virtual_address;

    uint64_t new_executable_file_size = stub_file_offset + sizeof(decode_stub_shellcode) - executable_segment->p_offset;
    executable_segment->p_filesz = new_executable_file_size;
    executable_segment->p_memsz = new_executable_file_size;

    uint64_t stub_exclude_offset = stub_file_offset;
    uint64_t stub_exclude_size = (uint64_t)sizeof(decode_stub_shellcode);

    uint16_t header_index = 0;
    while (header_index < elf_header->e_phnum)
    {
        Elf64_Phdr *current_header = &program_headers[header_index];
        if (current_header->p_type == PT_LOAD &&
            current_header->p_offset != 0 &&
            (current_header->p_flags & PF_W) == 0 &&
            current_header->p_filesz != 0)
        {
            xor_encrypt_range_excluding(file_bytes,
                                        (size_t)file_size,
                                        current_header->p_offset,
                                        current_header->p_filesz,
                                        stub_exclude_offset,
                                        stub_exclude_size,
                                        xor_key);
        }
        header_index++;
    }

    int output_fd = open("woody", O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (output_fd < 0)
    {
        munmap(file_bytes, (size_t)file_size);
        return 1;
    }

    ssize_t written_size = write(output_fd, file_bytes, (size_t)file_size);
    close(output_fd);
    munmap(file_bytes, (size_t)file_size);

    if (written_size != (ssize_t)file_size)
        return 1;

    return 0;
}
