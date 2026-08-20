# woody_woodpacker

ELF 실행 파일이 loader를 거쳐 프로세스로 올라가는 과정을 이해하기 위해 만든
학습용 binary transformer입니다. little-endian ELF32/i386와 ELF64/x86-64의
선택된 `PT_LOAD` 영역을 XOR 변환하고, 실행 시 원래 내용을 복원한 뒤 기존
entry point로 돌아가는 `woody`를 생성합니다.

취약점 악용·전파·탐지 회피 기능을 구현한 프로젝트가 아니라, ELF layout과
runtime loader 제약을 코드와 실행 결과로 확인하는 데 범위를 두었습니다.

## 만든 이유

실행 파일의 일부를 바꾸었을 때 loader가 무엇을 근거로 mapping하고 제어권을
넘기는지 직접 확인하고 싶었습니다. 변환 범위를 segment 단위로 제한한 것은
section까지 다루며 복잡도를 넓히기보다 program header와 loader의 관계에
집중하기 위한 선택이었습니다.

## 핵심 기능

- little-endian `ELFCLASS32`/`EM_386`, `ELFCLASS64`/`EM_X86_64` 처리
- `ET_EXEC`와 `ET_DYN` 입력 및 program header 범위 검증
- file offset이 0이 아닌 `R`, `R|X`, `R|W|X` `PT_LOAD`의 file-backed bytes XOR 변환
- decoder stub, 16-byte marker, 원래 entry, key, segment metadata 직렬화
- 실행 가능한 기존 segment의 page-tail에 payload 삽입
- `e_entry`와 해당 segment의 `p_filesz`/`p_memsz` 수정
- 명시한 64-bit key 또는 `/dev/urandom`으로 생성한 key 사용
- 입력은 read-only로 열고 `MAP_PRIVATE`에서 변환하여 원본을 보존

## 동작 구조

```text
ELF 입력 + 선택적 key
  -> header/PHDR 검증
  -> 대상 PT_LOAD와 payload 공간 선택
  -> segment XOR + decoder/metadata 삽입
  -> e_entry와 PHDR 수정 -> ./woody
  -> decoder 실행 -> mprotect/decode/권한 복원
  -> "....WOODY...." 출력 -> original entry
```

핵심 조정은 `src/woody-packer.c`, 입력 검증은 `src/elf_parser.c`, segment 선택은
`src/check_meta.c`, 출력 patch는 `src/write_output.c`가 담당합니다. runtime
decoder는 ELF class별 byte array로 포함되며 metadata를 읽어 load base와 원래
보호 속성을 복원합니다.

## 설계하면서 고민한 점

- section header나 symbol table에 의존하지 않고 program header만 사용했습니다.
- 새 segment를 추가하지 않고 기존 RX segment의 page-tail을 사용해 ELF 구조
  변경 범위를 줄였습니다. 그 대신 충분한 공간이 없는 layout은 처리할 수 없습니다.
- runtime 변환이 끝날 때까지 loader가 새 entry stub을 먼저 실행한다는 전제를
  두었습니다. 검증한 실행 파일에서는 동작했지만 일반 shared object에는 맞지 않습니다.
- 원본 mapping을 직접 덮어쓰지 않고 별도 파일을 생성하도록 I/O 수명주기를
  분리했습니다.

## Build

```sh
make
```

기본 target은 bundled `libft`와 `ft_printf`를 함께 빌드하고 루트에
`woody_woodpacker`를 생성합니다. GNU/Linux와 GCC가 필요합니다.

## Run

```sh
./woody_woodpacker <ELF-path>
./woody_woodpacker <ELF-path> 0123456789abcdef
./woody_woodpacker <ELF-path> 0x0123456789abcdef
./woody [args...]
```

key는 prefix를 제외하고 정확히 16개의 hexadecimal digit이어야 합니다. 생략하면
8 bytes를 `/dev/urandom`에서 읽습니다. 변환 결과는 현재 디렉터리의 고정 이름
`woody`로 생성되며 실행 시 marker 한 줄을 먼저 출력합니다. ELF32 결과를
실행하려면 host가 해당 32-bit ABI를 실행할 수 있어야 합니다.

## 검증 결과

| 범위 | 결과 |
|---|---|
| 비교 기준 | 원본과 동일한 argv/stdin/environment의 동작 + ELF 구조 invariant |
| PROFILE E 입력 | 8 fixtures · 22 cases |
| 결과 | PASS 21 · PARTIAL 0 · FAIL 0 · CRASH 1 |
| 대표 관찰 | E15 shared object를 `dlopen()`하면 packed artifact가 `SIGILL` |

PROFILE E에서는 ELF32/64 실행 파일, PIE, static, stripped, argv, stdout/stderr,
exit status, binary stdin, 환경 변수, key 경계값과 구조 변경을 확인했습니다.
별도의 adapted PROFILE B는 15 fixtures/23 cases에서 PASS 23을 기록했습니다.
admitted input의 GNU `nm` symbol metadata 보존과 예상된 입력 거부를 본 보조
검증이며, 실행 성공을 검증한 수치가 아닙니다.

전체 결과: [`portfolio_audit/profile_e_test_results.md`](portfolio_audit/profile_e_test_results.md)

## 확인된 한계

- E15에서 parser가 shared object도 `ET_DYN`으로 받아들이지만 `dlopen()`은
  `e_entry`를 거치지 않습니다. 암호화된 `.init`이 decoder보다 먼저 실행되어
  `SIGILL`이 발생했으며, 이 문제는 현재 소스에서 해결되지 않았습니다.
- payload가 들어갈 실행 가능 segment의 page-tail 공간이 없으면 변환을 거부합니다.
- big-endian ELF, i386/x86-64 이외의 machine, `ET_REL`, archive는 지원 범위 밖입니다.

## 상세 문서

- [Audit 개요](portfolio_audit/README.md)
- [Architecture](portfolio_audit/architecture.md)
- [기능 범위](portfolio_audit/feature_inventory.md)
- [PROFILE E failure 분석](portfolio_audit/profile_e_failures.md)
- [설계 근거와 주장 경계](portfolio_audit/design_rationale.md)
