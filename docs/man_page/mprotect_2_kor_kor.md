
제공해주신 `mprotect(2)` 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

## MPROTECT(2) 리눅스 프로그래머 매뉴얼 MPROTECT(2)

### 이름 (NAME)

mprotect, pkey_mprotect - 메모리 영역에 대한 보호를 설정함 

### 시놉시스 (SYNOPSIS)

```c
#include <sys/mman.h>

[cite_start]int mprotect(void *addr, size_t len, int prot); [cite: 148]

#define _GNU_SOURCE             /* feature_test_macros(7)를 참조 */
#include <sys/mman.h>

[cite_start]int pkey_mprotect(void *addr, size_t len, int prot, int pkey); [cite: 149]

```

### 설명 (DESCRIPTION)

`mprotect()`는 주소 범위 `[addr, addr+len-1]`의 어떠한 부분이라도 포함하는 호출 프로세스의 메모리 페이지들에 대한 접근 보호를 변경합니다. `addr`은 페이지 경계(page boundary)에 정렬되어야 합니다. 만약 호출 프로세스가 보호를 위반하는 방식으로 메모리에 접근하려고 시도하면, 커널은 해당 프로세스에 대해 `SIGSEGV` 신호를 생성합니다.

`prot`는 다음 접근 플래그들의 조합입니다: `PROT_NONE` 또는 아래 목록에 있는 다른 값들의 비트별-or(bitwise-or) 조합입니다:

* **PROT_NONE**: 메모리에 전혀 접근할 수 없습니다.


* **PROT_READ**: 메모리를 읽을 수 있습니다.


* **PROT_WRITE**: 메모리를 수정할 수 있습니다.


* **PROT_EXEC**: 메모리를 실행할 수 있습니다.


* **PROT_SEM** (리눅스 2.5.7부터): 메모리를 원자적 작업(atomic operations)을 위해 사용할 수 있습니다. 이 플래그는 `futex(2)` 구현의 일부로 도입되었으나(FUTEX_WAIT과 같은 명령에 의해 요구되는 원자적 작업 수행 능력을 보장하기 위해), 현재 어떤 아키텍처에서도 사용되지 않습니다.


* **PROT_SAO** (리눅스 2.6.26부터): 메모리가 강한 접근 순서(strong access ordering)를 가져야 합니다. 이 기능은 PowerPC 아키텍처에 특화되어 있습니다.



추가적으로 (리눅스 2.6.0부터), `prot`에 다음 플래그 중 하나를 설정할 수 있습니다:

* **PROT_GROWSUP**: 위쪽으로 자라는 매핑의 끝까지 보호 모드를 적용합니다. (이러한 매핑은 HP-PARISC와 같이 위쪽으로 자라는 스택을 가진 아키텍처의 스택 영역을 위해 생성됩니다) .


* **PROT_GROWSDOWN**: 아래쪽으로 자라는 매핑의 시작 부분까지 보호 모드를 적용합니다 (이는 스택 세그먼트이거나 `MAP_GROWSDOWN` 플래그가 설정되어 매핑된 세그먼트여야 합니다).



`mprotect()`와 마찬가지로, `pkey_mprotect()`는 `addr`과 `len`으로 지정된 페이지들에 대한 보호를 변경합니다. `pkey` 인수는 메모리에 할당할 보호 키(protection key, `pkeys(7)` 참조)를 지정합니다. 보호 키는 `pkey_mprotect()`에 전달되기 전에 `pkey_alloc(2)`으로 할당되어야 합니다. 이 시스템 호출의 사용 예는 `pkeys(7)`를 참조하십시오.

### 반환 값 (RETURN VALUE)

성공 시, `mprotect()`와 `pkey_mprotect()`는 0을 반환합니다. 에러 발생 시, 이 시스템 호출들은 -1을 반환하고 `errno`가 적절하게 설정됩니다.

### 에러 (ERRORS)

* **EACCES**: 메모리에 지정된 접근 권한을 부여할 수 없습니다. 예를 들어, 읽기 전용 접근 권한만 가진 파일을 `mmap(2)`한 후, `mprotect()`로 `PROT_WRITE`로 표시하도록 요청하는 경우 발생할 수 있습니다.


* **EINVAL**: `addr`이 유효한 포인터가 아니거나, 시스템 페이지 크기의 배수가 아닙니다.


* **EINVAL** (`pkey_mprotect()`): `pkey`가 `pkey_alloc(2)`으로 할당되지 않았습니다.


* **EINVAL**: `prot`에 `PROT_GROWSUP`과 `PROT_GROWSDOWN`이 모두 지정되었습니다.


* **EINVAL**: `prot`에 유효하지 않은 플래그가 지정되었습니다.


* **EINVAL** (PowerPC 아키텍처): `prot`에 `PROT_SAO`가 지정되었으나, SAO 하드웨어 기능을 사용할 수 없습니다.


* **ENOMEM**: 커널 내부 구조체를 할당할 수 없었습니다.


* **ENOMEM**: `[addr, addr+len-1]` 범위의 주소들이 프로세스의 주소 공간에 유효하지 않거나, 매핑되지 않은 하나 이상의 페이지를 지정하고 있습니다.


* **ENOMEM**: 메모리 영역의 보호를 변경하는 것이 서로 다른 속성을 가진 매핑의 총 개수를 허용된 최대치를 초과하게 만듭니다. (예를 들어, 현재 `PROT_READ|PROT_WRITE`로 보호된 영역의 중간 범위를 `PROT_READ`로 만드는 것은 세 개의 매핑을 결과로 가져옵니다: 양 끝의 두 개 읽기/쓰기 매핑과 중간의 하나의 읽기 전용 매핑) .



### 버전 (VERSIONS)

`pkey_mprotect()`는 리눅스 4.9에서 처음 등장했습니다. 라이브러리 지원은 glibc 2.27에 추가되었습니다.

### 준수 (CONFORMING TO)

* `mprotect()`: POSIX.1-2001, POSIX.1-2008, SVr4. POSIX는 `mmap(2)`를 통해 얻지 않은 메모리 영역에 적용될 경우 `mprotect()`의 동작은 미지정(unspecified)이라고 말합니다.


* `pkey_mprotect()`는 이식 불가능한 리눅스 확장입니다.



### 참고 사항 (NOTES)

리눅스에서, 프로세스의 주소 공간 내의 모든 주소(커널 vsyscall 영역 제외)에 대해 `mprotect()`를 호출하는 것은 항상 허용됩니다. 특히, 기존의 코드 매핑을 쓰기 가능하도록 변경하는 데 사용될 수 있습니다.

`PROT_EXEC`이 `PROT_READ`와 다른 효과를 갖는지 여부는 프로세서 아키텍처, 커널 버전, 그리고 프로세스 상태에 달려 있습니다. 만약 프로세스의 personality 플래그에 `READ_IMPLIES_EXEC`이 설정되어 있다면, `PROT_READ`를 지정하는 것은 암시적으로 `PROT_EXEC`을 추가할 것입니다. 일부 하드웨어 아키텍처(예: i386)에서 `PROT_WRITE`는 `PROT_READ`를 포함합니다.

POSIX.1은 구현체가 `prot`에 지정된 것 외의 접근을 허용할 수 있다고 말하지만, 최소한 `PROT_WRITE`가 설정된 경우에만 쓰기 접근을 허용할 수 있고, `PROT_NONE`이 설정된 경우에는 어떠한 접근도 허용해서는 안 됩니다.

애플리케이션은 `mprotect()`와 `pkey_mprotect()`의 사용을 혼합할 때 주의해야 합니다 . x86에서 `mprotect()`가 `prot`를 `PROT_EXEC`으로 설정하여 사용될 때, 커널에 의해 암시적으로 pkey가 할당되고 메모리에 설정될 수 있으나, 이는 이전에 pkey가 0이었을 때만 해당합니다. 하드웨어에서 보호 키를 지원하지 않는 시스템에서도 `pkey_mprotect()`가 사용될 수 있으나, `pkey`는 -1로 설정되어야 합니다. 이렇게 호출될 때 `pkey_mprotect()`의 동작은 `mprotect()`와 동일합니다.

### 예제 (EXAMPLES)

아래 프로그램은 `mprotect()`의 사용을 보여줍니다. 프로그램은 메모리 4페이지를 할당하고, 이 중 세 번째 페이지를 읽기 전용으로 만든 다음, 할당된 영역을 위쪽으로 걸어가며 바이트를 수정하는 루프를 실행합니다.

프로그램 실행 시 볼 수 있는 예는 다음과 같습니다:

```
$ ./a.out
Start of region:        0x804c000
Got SIGSEGV at address: 0x804e000

```

#### 프로그램 소스 (Program source)

```c
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
[cite_start]#include <stdlib.h> [cite: 191]
#include <errno.h>
#include <sys/mman.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); [cite_start]} while (0) [cite: 192]

static char *buffer;

static void
[cite_start]handler(int sig, siginfo_t *si, void *unused) [cite: 193]
{
    /* 참고: 신호 핸들러 내에서 printf()를 호출하는 것은 안전하지 않으며
       (실제 제품 프로그램에서는 수행되어서는 안 됩니다), printf()는
       비동기 신호 안전(async-signal-safe)이 아니기 때문입니다;
       [cite_start]signal-safety(7)를 참조하십시오. [cite: 194]
       그럼에도 불구하고, 여기서는 핸들러가 호출되었음을 보여주는
       단순한 방법으로 printf()를 사용합니다. */

    [cite_start]printf("Got SIGSEGV at address: %p\n", si->si_addr); [cite: 195]
    exit(EXIT_FAILURE);
[cite_start]} [cite: 196]

int
main(int argc, char *argv[])
{
    int pagesize;
    [cite_start]struct sigaction sa; [cite: 197]

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    [cite_start]if (sigaction(SIGSEGV, &sa, NULL) == -1) [cite: 198]
        handle_error("sigaction");

    [cite_start]pagesize = sysconf(_SC_PAGE_SIZE); [cite: 199]
    if (pagesize == -1)
        handle_error("sysconf");

    /* 페이지 경계에 정렬된 버퍼를 할당합니다;
       초기 보호는 PROT_READ | [cite_start]PROT_WRITE입니다 */ [cite: 200, 201]

    buffer = memalign(pagesize, 4 * pagesize);
    [cite_start]if (buffer == NULL) [cite: 202]
        handle_error("memalign");

    [cite_start]printf("Start of region:        %p\n", buffer); [cite: 203]

    if (mprotect(buffer + pagesize * 2, pagesize,
                [cite_start]PROT_READ) == -1) [cite: 204]
        handle_error("mprotect");

    [cite_start]for (char *p = buffer ; ; ) [cite: 205]
        *(p++) = 'a';

    printf("Loop completed\n");     [cite_start]/* 결코 발생하지 않아야 함 */ [cite: 206]
    exit(EXIT_SUCCESS);
[cite_start]} [cite: 207]

```

### 참고 항목 (SEE ALSO)

`mmap(2)`, `sysconf(3)`, `pkeys(7)` 

### 콜로폰 (COLOPHON)

이 페이지는 리눅스 매뉴얼 페이지(man-pages) 프로젝트의 5.10 릴리스의 일부입니다. 프로젝트에 대한 설명, 버그 보고에 대한 정보, 그리고 이 페이지의 최신 버전은 [https://www.kernel.org/doc/man-pages/](https://www.kernel.org/doc/man-pages/) 에서 찾을 수 있습니다.

리눅스 2020-11-01 MPROTECT(2) 

---

**추가적인 번역이나 도움이 필요하시면 언제든지 요청해 주세요.**