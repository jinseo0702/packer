IMPLEMENTATION DIAGRAM

[COREPRINCIPLES]
- 복호화 코드는 SHELLCODE 를 사용한다. 
- 반드시 아래 순서를 고정한다.
- key 는 hex 형식이며 print 해야한다.

[PROCESSINGFLOW]
1. open + lseek + mmap → unit(base=mmap, limit=file_size) → Encryption → open(new file name woody) → write
2. detect_format(unit)
    - ELF → process_elf_unit(unit)
    - else → ERR_FORMAT

[MODULERESPONSIBILITY]
1. File name is Example and accountability is import
	* A) arg.c / arg.h
		- Responsibility: Parse command line arguments and manage encryption key
		- Arguments:
			* argv[1]: path to input ELF binary (required)
			* argv[2]: encryption key in hex format (optional)
				- Format: "0x1234567890abcdef" or "1234567890abcdef"
				- Length: exactly 16 hex characters (8bytes)
		- Key generation:
			* if argv[2] not provoded: read 8 bytes form /dev/urandom\
			* Vaildation: hex string must be vaild and exactly 16 characters
		- Key output:
			* Print to stdout: "KEY: 0x%016lx\n"
			* Execute immediately after key generation/parsing
		- return key
		- Error handling:
			* argc < 2 or > 3: return ERR_USAGE
			* Invalid hex format: return ERR_KEY_FORMAT
			* /dev/urandom read failure: return ERR_KEY_GEN
		- Function signature
			* int parse_args(int argc, char **argv, uint64_t *key);
	* B) io_unit.c / io_unit.h (resource/unit/abstract)
		- Responsibility: Manage memory-mapped filecycle and provide t_uint abstraction
		- Resource management functions:
			* int create_unit_from_path(const char *path, t_unit *unit);
			* void destroy_unit(t_unit *unit);
		- Lifecycle contract:
			* create_unit_from_path() is responsible for: open -> lseek -> mmap -> close
			* destroy_unit() is responsible for: munmap
			* Caller must call destroy_unit() in all exit paths(success for error)
		- Error cases:
			* open() fails: return ERR_OPEN
			* lseek() fails: close fd, return ERR_LSEEK
			* mmap() fails: close fd, return ERR_MMAP
			* file size = 0: close fd, return ERR_EMPTY_FILE
	* C) format_router.c
		- Responsibility: Detect file format by checking magic bytes
		- Function signature:
			* int detect_format(const t_unit *unit);
		- Detection logic:
			1. CHECK_RANGE(0, 4, unit->limit) - minimum 4 bytes for ELF magic
			2. Compare first 4 bytes with ELFMAG ("\x7fELF")
			3. If match: return FORMAT_ELF
			4. If not match: return ERR_FORMAT
		- Magic validation:
			* ELF magic: unit->base[0] == 0x7f
			* ELF magic: unit->base[1] == 'E'
			* ELF magic: unit->base[2] == 'L'
			* ELF magic: unit->base[3] == 'F'
			* Or use: memcmp(unit->base, ELFMAG, SELFMAG) == 0
		- Error handling:
			* File too small (< 4 bytes): return ERR_FORMAT
			* Magic mismatch: return ERR_FORMAT
		- Return values:
			* FORMAT_ELF: valid ELF format detected
			* ERR_FORMAT: not an ELF file or invalid
	* D) elf_parser.c
		- Responsibility: Validate ELF header and initialize t_unit ELF-specific fields
		- Function signature:
			* int parse_elf_header(t_unit *unit);
		- Validation sequence (must be in this order):
			1. CHECK_RANGE(0, sizeof(Elf64_Ehdr), unit->limit)
				- Use Elf64_Ehdr as maximum size (both 32/64 bit)
			2. ELF magic: already checked by format_router
			3. EI_CLASS validation:
				- Must be ELFCLASS32 or ELFCLASS64
				- Store in unit->elf_class
				- If invalid: return ERR_INVALID_CLASS
			4. EI_DATA validation:
				- Must be ELFDATA2LSB (little-endian only)
				- If invalid: return ERR_INVALID_ENDIAN
			5. e_type validation:
				- Must be ET_EXEC or ET_DYN
				- ET_DYN: PIE executable (position-independent)
				- If invalid: return ERR_INVALID_TYPE
			6. e_machine validation:
				- If ELFCLASS32: must be EM_386
				- If ELFCLASS64: must be EM_X86_64
				- If mismatch: return ERR_INVALID_MACHINE
		- Program header table validation:
			1. e_phoff must be != 0
			2. e_phnum must be != 0 and <= PN_XNUM (65535)
			3. e_phentsize validation:
				- If e_phentsize == 0: use sizeof(Elf32_Phdr) or sizeof(Elf64_Phdr)
				- Else: must match expected size
			4. Range check: CHECK_RANGE(e_phoff, e_phnum * phentsize, unit->limit)
			5. If validation fails: return ERR_INVALID_PHDR
		- Union initialization:
			* Set unit->ElfN_Ehdr.Ehdr32 or Ehdr64 = (ElfXX_Ehdr *)unit->base
			* Set unit->ElfN_Phdr.Phdr32 or Phdr64 = (ElfXX_Phdr *)(unit->base + e_phoff)
	* E) check_meta.c
		- Responsibility: Identify encryption targets and validate stub injection space
		- Function signatures:
			* int collect_encryption_segments(const t_unit *unit, t_encryption **enc_array, t_meta **meta_array, size_t *count);
		- Segment filtering (iterate all program headers):
			1. p_type must be PT_LOAD
			2. p_offset must be != 0 (skip first PT_LOAD if p_offset == 0)
			3. p_filesz must be > 0 (skip empty segments)
			4. Range validation: CHECK_RANGE(p_offset, p_filesz, unit->limit)
		- For each valid PT_LOAD segment:
			* Store t_encryption:
				- p_offset: segment file offset
				- p_filesize: segment file size (p_filesz)
				- in_stub: initially 0
			* Store t_meta:
				- p_vaddr: segment virtual address
				- p_memsz: segment memory size
				- p_reverse_flags: convert_pflags_to_prot(p_flags)
		- Stub space calculation:
			* Required size:
				- If ELFCLASS32: 352 + 16 + 32 + (N * 24) bytes
				- If ELFCLASS64: 496 + 16 + 32 + (N * 24) bytes
				- Where N = number of PT_LOAD segments only encryption_segments
			* Available space formula:
				- For each PT_LOAD: available = p_memsz - p_filesz
				- (size + 4095) & ~(0xfff); padding 
			* Stub injection criteria:
				- p_flags must have (PF_R | PF_X) - readable and executable
				- available >= required_size
				- Set in_stub = 1 for first segment meeting criteria
		- p_flags to mprotect conversion:

		```c
		int convert_pflags_to_prot(uint32_t p_flags) {
			int prot = 0;
			if (p_flags & PF_R) prot |= PROT_READ;   // 0x4 → 0x1
			if (p_flags & PF_W) prot |= PROT_WRITE;  // 0x2 → 0x2
			if (p_flags & PF_X) prot |= PROT_EXEC;   // 0x1 → 0x4
			return prot;
		}
		```
		- Memory allocation:
			* Allocate *enc_array = malloc(sizeof(t_encryption) * count)
			* Allocate *meta_array = malloc(sizeof(t_meta) * count)
			* Caller must free both arrays
		- Error handling:
			* No PT_LOAD segments: return ERR_NO_LOAD_SEGMENT
			* No suitable stub space: return ERR_NO_STUB_SPACE
			* Allocation failure: return ERR_MALLOC
	* F) encryption_xor.c
		- Responsibility: XOR encrypt PT_LOAD segments in-place
		- Function signature:
			* int encrypt_segment(unsigned char *base, uint64_t limit, const t_encryption *enc, uint64_t key);
		- Encryption algorithm:
			1. Range validation: CHECK_RANGE(enc->p_offset, enc->p_filesize, limit)
			2. Get segment pointer: ptr = base + enc->p_offset
			3. XOR in 8-byte units:
				```c
				for (i = 0; i + 8 <= enc->p_filesize; i += 8) {
					*(uint64_t *)(ptr + i) ^= key;
				}
				```
			4. XOR remaining bytes (< 8):
				```c
				uint8_t *key_bytes = (uint8_t *)&key;
				for (; i < enc->p_filesize; i++) {
				    ptr[i] ^= key_bytes;
				}
				```
		- Important notes:
			* Encryption modifies unit->base memory (mmap with PROT_WRITE)
			* XOR is symmetric: same operation for encrypt/decrypt
			* Key must be in native endianness (uint64_t)
			* Range check prevents buffer overflow
		- Error handling:
			* Invalid range: return ERR_RANGE
			* enc->p_filesize == 0: skip encryption, return OK
	* G) enter_data.c / write_output.c (split responsibility)
		- Responsibility: Create output file with shellcode and metadata embedded
		- Module G1: enter_data.c
			* Function: int prepare_payload(uint8_t **payload, size_t *payload_size, const t_unit *unit, const t_encryption *stub_enc,const t_meta *meta_array, size_t meta_count, uint64_t key);
			* Payload structure (sequential bytes):
				1. Shellcode:
					- If ELFCLASS32: shellcode_32_bin (352 bytes)
					- If ELFCLASS64: shellcode_64_bin (496 bytes)
				2. Marker string (16 bytes):
					- "....WOODY....\n\0\0"
				3. Metadata (all uint64_t, little-endian):
					- real_entry: original e_entry from ELF header
					- stub_vaddr: stub_enc->t_meta.p_vaddr (injection segment vaddr)
					- key: encryption key
					- meta_count: number of PT_LOAD segments
					- For each meta in meta_array (24 bytes each):
						* p_vaddr (8 bytes)
						* p_memsz (8 bytes)
						* p_reverse_flags (8 bytes)
						* Memory allocation:
					- Total size = shellcode_len + 16 + 40 + (meta_count * 24)
					- Allocate payload buffer with malloc
					- Serialize all data sequentially
					- Return allocated buffer and size
		- Module G2: write_output.c
			* Function: int write_woody_file(const t_unit *unit, const uint8_t *payload, size_t payload_size, const t_encryption *stub_enc);
			* File creation process:
				1. Open output file: fd = open("woody", O_CREAT|O_WRONLY|O_TRUNC, 0755)
				2. Write original file content:
					- write(fd, unit->base, unit->limit)
				3. Modify ELF header in output:
					- Seek to e_entry offset: lseek(fd, offsetof(ElfXX_Ehdr, e_entry), SEEK_SET)
					- Calculate new entry: stub_enc->t_meta.p_vaddr + stub_enc->p_offset
					- Write new e_entry value
				4. Modify program header (stub segment):
					- Seek to stub phdr: lseek(fd, e_phoff + stub_index * e_phentsize, SEEK_SET)
					- Update p_filesz: p_filesz += payload_size
					- Write modified program header
				5. Append payload:
					- Seek to end of stub segment: lseek(fd, stub_enc->p_offset + original_p_filesz, SEEK_SET)
					- write(fd, payload, payload_size)
				6. Close file
					* Error handling:
						- open() fails: return ERR_OPEN
						- write() fails: close fd, unlink("woody"), return ERR_WRITE
						- lseek() fails: close fd, unlink("woody"), return ERR_LSEEK


[FUNCCONTRACT]
- 공통 반환 규약
    - OK: 계속 진행
    - FATAL: 프로그램 종료해야 하는 수준


[RULETABLE]
- ranege rule (global)

| item | rule |
| --- | --- |
| Required Before Access | CHECK_RANGE(offset,size,limit) |
| Inspection sequence | offset <= limit first, and then size <= limit-offset |
| MOVE ADDRESS | MOVE_ADDRESS(base,offset)is not check range (Pre-call inspection required) |
| base mean | Start current interpretation unit (file mmap or member payload) |

[DEFINITIONOFDONE]
- 아래는 코드 실행 없이도 정적으로 확인 가능한 DoD입니다.

- DoD-ELF (필수 6)
    1. ELF magic/클래스/엔디안 검증이 process_elf_unit 초반에 존재
    2. e_phoff/e_phnum/e_phentsize 검증 및 e_phoff + e_phnum*e_phentsize range 검사 존재
    3. program header 접근 전에 반드시 range 검사(엔트리 단위)
    4. p_offset/p_filesz 조합에 대해 p_offset + p_filesz range 검사 존재
    5. phdr 엔트리 size가 0이면 sizeof(ElfXX_Phdr)로 치환하는 로직 존재
    6. base/limit이 “unit 기준”으로만 사용됨(파일 전체 기준 참조 금지)

- DoD-정책/리소스 (필수 6)
    3. 모든 분기에서 open/mmap/malloc 자원 해제가 보장되는 cleanup 구조 존재
    4. free 후 NULL guard 적용(가능한 범위에서)
    5. CHECK_RANGE 없이 MOVE_ADDRESS 결과를 역참조하는 코드가 없음
	8. Error 처리 지점은 NM_LOG 매크로를 사용해서 error 를 처리를 한다.


[DATASTRUCT]
```c
typedef struct s_unit{
	const unsigned char *base;
	uint64_t limit;
	uint64_t key;
	union {
		const Elf32_Ehdr    *Ehdr32;
		const Elf64_Ehdr    *Ehdr64;
	}ElfN_Ehdr;
	union {
		const Elf32_Phdr    *Phdr32;
		const Elf64_Phdr    *Phdr64;
	}ElfN_Phdr;
	uint32_t fd;
	uint8_t elf_class; // set bit type in elf header
}t_unit;

typedef struct s_encryption{
	uint64_t p_offset;
	uint64_t p_filesize;
	uint8_t  in_stub;
}t_encryption;

typedef struct s_meta{
	uint64_t p_vaddr; //
	uint64_t p_memsz; //
	uint64_t p_reverse_falgs; // reverse p_falg , because mprotec(2) prot have reverse p_flag value
}t_meta;

```

[INLINE:MACRO]
```c
    // 포인터 산술연산
    //MOVE_ADDRESS는 범위검사를 하지 않는다. 호출 전 CHECK_RANGE를 통과해야 한다.
        static inline const void *MOVE_ADDRESS(const void *base, uint64_t offset) {
            if (base == NULL) return NULL;
            const unsigned char *p = (const unsigned char*)base;
            return (const void*)(p + offset);
        }
    // 범위체크
        static inline uint8_t CHECK_RANGE(uint64_t offset, uint64_t size, uint64_t limit) {
                if (offset > limit) return 0;
                return (size <= (limit - offset));
        }
    // error enum
        #define ERROR_LIST \
			X(ERR_MALLOC, "Memory allocation failed") \
			X(ERR_OPEN, "File open failed") \
			X(ERR_USAGE, "Invalid arguments") \
			X(ERR_KEY_FORMAT, "Invalid key format") \
			X(ERR_KEY_GEN, "Failed to generate random key") \
			X(ERR_READ, "Read operation failed") \
			X(ERR_FORMAT, "Unknown file format") \
			X(ERR_LSEEK, "Lseek failed")
			X(ERR_MMAP, "Mmap failed")
			X(ERR_EMPTY_FILE, "File is empty")
			X(ERR_LSEEK, "Lseek failed")
			X(FORMAT_ELF, "ELF format detected")  // Not an error, but return code
			X(ERR_INVALID_CLASS, "Invalid ELF class")
			X(ERR_INVALID_ENDIAN, "Only little-endian supported")
			X(ERR_INVALID_TYPE, "Invalid ELF type (must be ET_EXEC or ET_DYN)")
			X(ERR_INVALID_MACHINE, "Invalid machine type")
			X(ERR_INVALID_PHDR, "Invalid program header table")
			X(ERR_NO_LOAD_SEGMENT, "No PT_LOAD segment found")
			X(ERR_NO_STUB_SPACE, "No space for shellcode injection")
			X(ERR_RANGE, "Range check failed")
			X(ERR_WRITE, "Write operation failed")
			X(ERR_UNLINK, "Failed to remove output file")
            ...

        typedef enum e_error {
            #define X(id, str) id,
            ERROR_LIST
            #undef X
            ERROR_END
        } t_error;

        const char *Error_table[] = {
            #define X(id, str) [id] = str,
            ERROR_LIST
            #undef X
        }

        #ifdef DEBUG
            #define NM_LOG(err) real_print_error(err, __FILE__, __LINE__)
        #else
            #define NM_LOG(err) real_print_error(err, NULL, 0)
        #endif

        static inline void real_print_error(t_error err, const char *file, int line) {
            ft_fprintf(2, "woody_packer : [%s] -> %s\n",get_error_msg(err) ,strerror(errno));
            if(file != NULL) ft_fprintf(2, "file : %s line : %d\n", file, line);
        }
```

[SHELLCODE]
```c
//shellcode_64.bin
unsigned char shellcode_64_bin[] = {
  0x48, 0x31, 0xc9, 0x48, 0x31, 0xc0, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x48,
  0x31, 0xc9, 0x59, 0x48, 0x83, 0xe9, 0x0b, 0x48, 0x81, 0xc1, 0xf0, 0x01,
  0x00, 0x00, 0x48, 0x83, 0xc1, 0x10, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x48,
  0x31, 0xc0, 0x58, 0x48, 0x83, 0xe8, 0x23, 0x48, 0x03, 0x01, 0x50, 0x50,
  0x53, 0x51, 0x52, 0x56, 0x57, 0x55, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52,
  0x41, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x31,
  0xed, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x31, 0xd2, 0x41, 0x5a, 0x49,
  0x83, 0xea, 0x4e, 0x49, 0x83, 0xc2, 0x00, 0x48, 0x31, 0xc0, 0x48, 0x31,
  0xff, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x31, 0xff, 0x5f, 0x48, 0x83,
  0xef, 0x66, 0x48, 0x81, 0xc7, 0xf0, 0x01, 0x00, 0x00, 0x48, 0x83, 0xc7,
  0x28, 0x48, 0x8b, 0x07, 0x48, 0x39, 0xc5, 0x0f, 0x84, 0x4d, 0x01, 0x00,
  0x00, 0x48, 0x89, 0xe8, 0x48, 0x6b, 0xc0, 0x18, 0x4d, 0x31, 0xc0, 0xe8,
  0x00, 0x00, 0x00, 0x00, 0x4d, 0x31, 0xc0, 0x41, 0x58, 0x49, 0x81, 0xe8,
  0x94, 0x00, 0x00, 0x00, 0x49, 0x81, 0xc0, 0xf0, 0x01, 0x00, 0x00, 0x49,
  0x83, 0xc0, 0x30, 0x49, 0x01, 0xc0, 0x49, 0x8b, 0x00, 0x4c, 0x01, 0xd0,
  0x49, 0x8b, 0x50, 0x08, 0x49, 0x8b, 0x70, 0x10, 0x48, 0xf7, 0xc6, 0x02,
  0x00, 0x00, 0x00, 0x75, 0x35, 0x48, 0x31, 0xff, 0x48, 0x89, 0xc7, 0x48,
  0x31, 0xf6, 0x48, 0x89, 0xd6, 0x48, 0x31, 0xd2, 0xba, 0x07, 0x00, 0x00,
  0x00, 0x48, 0x31, 0xc0, 0xb8, 0x0a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x48,
  0x85, 0xc0, 0x78, 0x02, 0xeb, 0x10, 0x48, 0x31, 0xff, 0x48, 0xf7, 0xd8,
  0x48, 0x89, 0xc7, 0xb8, 0x3c, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x48, 0x89,
  0xe8, 0x48, 0x6b, 0xc0, 0x18, 0x4d, 0x31, 0xc0, 0xe8, 0x00, 0x00, 0x00,
  0x00, 0x4d, 0x31, 0xc0, 0x41, 0x58, 0x49, 0x81, 0xe8, 0x09, 0x01, 0x00,
  0x00, 0x49, 0x81, 0xc0, 0xf0, 0x01, 0x00, 0x00, 0x49, 0x83, 0xc0, 0x30,
  0x49, 0x01, 0xc0, 0x49, 0x8b, 0x00, 0x4c, 0x01, 0xd0, 0x49, 0x8b, 0x50,
  0x08, 0x49, 0x8b, 0x70, 0x10, 0x48, 0x89, 0xc7, 0x50, 0x48, 0x31, 0xc9,
  0x48, 0x31, 0xdb, 0x48, 0x31, 0xc0, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x48,
  0x31, 0xc0, 0x58, 0x48, 0x2d, 0x43, 0x01, 0x00, 0x00, 0x48, 0x05, 0xf0,
  0x01, 0x00, 0x00, 0x48, 0x83, 0xc0, 0x20, 0x48, 0x8b, 0x18, 0x48, 0x89,
  0xd0, 0x48, 0x29, 0xc8, 0x48, 0x83, 0xf8, 0x08, 0x7c, 0x11, 0x48, 0x8b,
  0x04, 0x0f, 0x48, 0x31, 0xd8, 0x48, 0x89, 0x04, 0x0f, 0x48, 0x83, 0xc1,
  0x08, 0xeb, 0xe3, 0x48, 0x39, 0xd1, 0x7d, 0x0d, 0x8a, 0x04, 0x0f, 0x30,
  0xd8, 0x88, 0x04, 0x0f, 0x48, 0xff, 0xc1, 0xeb, 0xee, 0x48, 0xff, 0xc5,
  0x58, 0x48, 0xf7, 0xc6, 0x02, 0x00, 0x00, 0x00, 0x0f, 0x85, 0xc1, 0xfe,
  0xff, 0xff, 0x48, 0x31, 0xff, 0x48, 0x89, 0xc7, 0x48, 0x31, 0xf6, 0x48,
  0x89, 0xd6, 0x48, 0x31, 0xd2, 0x48, 0x89, 0xf2, 0x48, 0x31, 0xc0, 0xb8,
  0x0a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x48, 0x85, 0xc0, 0x78, 0x02, 0xeb,
  0x10, 0x48, 0x31, 0xff, 0x48, 0xf7, 0xd8, 0x48, 0x89, 0xc7, 0xb8, 0x3c,
  0x00, 0x00, 0x00, 0x0f, 0x05, 0xe9, 0x89, 0xfe, 0xff, 0xff, 0x41, 0x5f,
  0x41, 0x5e, 0x41, 0x5d, 0x41, 0x5c, 0x41, 0x5b, 0x41, 0x5a, 0x41, 0x59,
  0x41, 0x58, 0x5d, 0x5f, 0x5e, 0x5a, 0x59, 0x5b, 0x58, 0xc3, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90
};
unsigned int shellcode_64_bin_len = 496;

//shellcode_32.bin
unsigned char shellcode_32_bin[] = {
  0x31, 0xc9, 0x31, 0xc0, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x31, 0xc9, 0x59,
  0x83, 0xe9, 0x09, 0x81, 0xc1, 0x60, 0x01, 0x00, 0x00, 0x83, 0xc1, 0x10,
  0xe8, 0x00, 0x00, 0x00, 0x00, 0x31, 0xc0, 0x58, 0x83, 0xe8, 0x1d, 0x03,
  0x01, 0x50, 0x60, 0x31, 0xed, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x31, 0xf6,
  0x5e, 0x83, 0xee, 0x2e, 0x83, 0xc6, 0x00, 0x31, 0xc0, 0x31, 0xff, 0xe8,
  0x00, 0x00, 0x00, 0x00, 0x31, 0xff, 0x5f, 0x83, 0xef, 0x40, 0x81, 0xc7,
  0x60, 0x01, 0x00, 0x00, 0x83, 0xc7, 0x28, 0x8b, 0x07, 0x39, 0xc5, 0x0f,
  0x84, 0x04, 0x01, 0x00, 0x00, 0x89, 0xe8, 0x6b, 0xc0, 0x18, 0x31, 0xff,
  0xe8, 0x00, 0x00, 0x00, 0x00, 0x31, 0xff, 0x5f, 0x83, 0xef, 0x65, 0x81,
  0xc7, 0x60, 0x01, 0x00, 0x00, 0x83, 0xc7, 0x30, 0x01, 0xc7, 0x8b, 0x07,
  0x01, 0xf0, 0x8b, 0x57, 0x08, 0x8b, 0x77, 0x10, 0xf7, 0xc6, 0x02, 0x00,
  0x00, 0x00, 0x75, 0x2b, 0x31, 0xdb, 0x89, 0xc3, 0x31, 0xc9, 0x89, 0xd1,
  0x31, 0xd2, 0xba, 0x07, 0x00, 0x00, 0x00, 0x31, 0xc0, 0xb8, 0x7d, 0x00,
  0x00, 0x00, 0x0f, 0x05, 0x85, 0xc0, 0x78, 0x02, 0xeb, 0x0d, 0x31, 0xdb,
  0xf7, 0xd8, 0x89, 0xc3, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x05, 0x89,
  0xe8, 0x6b, 0xc0, 0x18, 0x31, 0xff, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x31,
  0xff, 0x5f, 0x81, 0xef, 0xbf, 0x00, 0x00, 0x00, 0x81, 0xc7, 0x60, 0x01,
  0x00, 0x00, 0x83, 0xc7, 0x30, 0x01, 0xc7, 0x8b, 0x07, 0x01, 0xf0, 0x8b,
  0x57, 0x08, 0x8b, 0x77, 0x10, 0x89, 0xc7, 0x50, 0x31, 0xc9, 0x31, 0xdb,
  0x31, 0xc0, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x31, 0xc0, 0x58, 0x2d, 0xeb,
  0x00, 0x00, 0x00, 0x05, 0x60, 0x01, 0x00, 0x00, 0x83, 0xc0, 0x20, 0x8b,
  0x18, 0x89, 0xd0, 0x29, 0xc8, 0x83, 0xf8, 0x04, 0x7c, 0x0d, 0x8b, 0x04,
  0x0f, 0x31, 0xd8, 0x89, 0x04, 0x0f, 0x83, 0xc1, 0x04, 0xeb, 0xea, 0x39,
  0xd1, 0x7d, 0x0b, 0x8a, 0x04, 0x0f, 0x30, 0xd8, 0x88, 0x04, 0x0f, 0x41,
  0xeb, 0xf1, 0x45, 0x58, 0xf7, 0xc6, 0x02, 0x00, 0x00, 0x00, 0x0f, 0x85,
  0x07, 0xff, 0xff, 0xff, 0x31, 0xdb, 0x89, 0xc3, 0x31, 0xc9, 0x89, 0xd1,
  0x31, 0xd2, 0x89, 0xf2, 0x31, 0xc0, 0xb8, 0x7d, 0x00, 0x00, 0x00, 0x0f,
  0x05, 0x85, 0xc0, 0x78, 0x02, 0xeb, 0x0d, 0x31, 0xdb, 0xf7, 0xd8, 0x89,
  0xc3, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xe9, 0xda, 0xfe, 0xff,
  0xff, 0x61, 0xc3, 0x90
};
unsigned int shellcode_32_bin_len = 352;
```