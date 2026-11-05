# Linux ELF 실행 파일 패커

Linux는 ELF 파일의 어느 부분을 메모리에 올리고 어디서 실행을 시작할까? 이 프로젝트는 그 두 지점을 직접 바꿔 보는 C·어셈블리 도구다. `woody_woodpacker`는 실행 파일에서 선택한 영역을 XOR로 변환하고 시작 주소를 복원 코드로 바꿔 `woody`를 만든다. `woody`를 실행하면 복원 코드가 메모리에서 변환을 되돌리고, 표시 한 줄을 쓴 뒤 원래 프로그램에 제어를 넘긴다. 키는 결과 파일에도 들어가므로 용도는 실행 구조 실험에 한정된다.

## 직접 실행해 보기

Ubuntu 22.04 x86-64(WSL2), GCC, GNU Make에서 확인했다. 저장소 루트에서 다음 예제를 실행한다. 결과 파일 이름은 항상 현재 디렉터리의 `woody`이며, 같은 이름의 파일이 있으면 덮어쓴다.

```sh
make
gcc -fPIE -pie HelloWorld.c -o demo-hello
./woody_woodpacker ./demo-hello 0123456789abcdef
./woody
```

패커는 사용한 키를 출력한다. 마지막 명령의 첫 줄은 복원 코드가, 둘째 줄은 원래 프로그램이 쓴다.

```text
....WOODY....
Hello World
```

같은 입력과 키로 변환을 반복할 수 있다. 키를 지정할 때는 16자리 16진수를 사용하며 `0x` 접두사도 허용한다. 키를 생략하면 `/dev/urandom`에서 8바이트를 읽는다.

## 파일에서 프로세스까지

1. **입력을 검사한다.** 원본을 읽기 전용으로 열고 전용 메모리 매핑(`MAP_PRIVATE`)에서 작업한다. ELF 클래스, 바이트 순서, 대상 CPU, 프로그램 헤더 테이블을 검사한다. [입력 처리](src/io_unit.c) · [ELF 검사](src/elf_parser.c)
2. **바꿀 바이트를 고른다.** 로더가 적재 단위로 쓰는 `PT_LOAD` 중 파일 오프셋이 0이 아니고 파일 바이트가 있는 R, RX, RWX 영역을 선택한다. 새 세그먼트를 추가하는 대신 읽기·실행 가능한 세그먼트의 끝에서 페이지 경계까지 남은 공간을 찾는다. 여기에 복원 코드와 메타데이터가 들어가야 한다. [영역 선택](src/check_meta.c)
3. **새 시작 주소를 기록한다.** 선택 영역을 XOR로 바꾼 뒤 복원 코드, 키, 원래 시작 주소와 세그먼트 정보를 출력 파일에 쓴다. ELF의 시작 주소(`e_entry`)와 복원 코드가 들어갈 세그먼트의 크기도 갱신한다. [변환 흐름](src/woody-packer.c) · [출력 작성](src/write_output.c)
4. **메모리에서 되돌린다.** 복원 코드는 실제 적재 주소를 계산하고, 쓰기 권한이 필요한 영역은 `mprotect`로 권한을 바꿔 복원한 뒤 원래 권한으로 되돌린다. 마지막으로 `....WOODY....`를 출력하고 원래 시작 주소로 돌아간다. [복원 코드](src/stub.s)

## 공유 라이브러리를 거부하게 된 이유

첫 구현은 위치 독립 실행 파일(PIE)과 공유 라이브러리가 모두 ELF 타입 `ET_DYN`이라는 점만 보고 둘 다 처리했다. 공유 라이브러리 예제는 변환된 헤더와 데이터가 구조 검사 9개를 모두 통과했지만, `dlopen()`으로 불러오면 `SIGILL`로 종료했다. 실패 주소를 원본의 `.init`과 대조하니 로더가 복원 코드를 거치기 전에 XOR로 바뀐 초기화 명령을 실행하고 있었다. [재현과 원인 분석](portfolio_audit/profile_e_failures.md)

현재는 `ET_DYN`에서 동적 플래그 `DF_1_PIE`가 확인돼야 변환을 시작한다. 이 기준은 일반 공유 라이브러리와 해당 표시가 없는 구형 PIE를 함께 거부한다. 지원 형식은 little-endian ELF32/i386 및 ELF64/x86-64의 `ET_EXEC`와 위 조건의 PIE다. 형식이 맞아도 복원 코드를 넣을 여유 공간이 부족하면 거부한다.

## 검증

수정 후 Ubuntu 22.04 x86-64에서 **심볼·입력 판별 23/23**, **실행 동작·파일 구조 22/22**를 통과했다. 전자는 변환 전후 GNU `nm` 심볼 비교와 잘못된 입력의 거부를 본다. 후자는 변환한 실행 파일에 같은 인자와 입력을 주고, 마커를 제외한 표준 출력·표준 오류·종료 상태와 변경된 파일 범위를 검사한다. ELF32 실행 사례에는 i386 실행을 지원하는 커널과 32비트 동적 로더가 필요하다. [심볼 검사 코드](portfolio_audit/tools/run_baseline.py) · [실행·구조 검사 코드](portfolio_audit/tools/run_profile_e.py)

다음 명령은 현재 커밋을 임시 디렉터리에 풀어 다시 빌드하고, 그 안에서만 검사 결과를 생성한다. Python 3와 GNU binutils가 필요하다.

```sh
audit_dir=$(mktemp -d)
git archive HEAD | tar -x -C "$audit_dir"
(
  cd "$audit_dir" &&
  make &&
  cp woody_woodpacker portfolio_audit/bin/ &&
  python3 portfolio_audit/tools/run_baseline.py &&
  python3 portfolio_audit/tools/run_profile_e.py
)
```
