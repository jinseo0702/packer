
제공해주신 `syscall(2)` 리눅스 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

SYSCALL(2) 리눅스 프로그래머 매뉴얼 SYSCALL(2) 

### 이름 (NAME)

syscall - 간접 시스템 호출 (indirect system call) 

### 시놉시스 (SYNOPSIS)

```c
#include <unistd.h>
#include <sys/syscall.h>   /* SYS_xxx 정의를 위함 */

[cite_start]long syscall(long number, ...); [cite: 242, 243]

```

**glibc를 위한 기능 테스트 매크로 요구 사항 (feature_test_macros(7) 참조):** 

* 
**syscall()**: 


* glibc 2.19부터: `_DEFAULT_SOURCE` 


* glibc 2.19 이전: `_BSD_SOURCE || [cite_start]_SVID_SOURCE` 





### 설명 (DESCRIPTION)

`syscall()`은 지정된 인수를 사용하여 지정된 번호를 가진 어셈블리 언어 인터페이스의 시스템 호출을 호출하는 작은 라이브러리 함수입니다. `syscall()`을 사용하는 것은, 예를 들어 C 라이브러리에 래퍼(wrapper) 함수가 없는 시스템 호출을 호출할 때 유용합니다. `syscall()`은 시스템 호출을 수행하기 전에 CPU 레지스터를 저장하고, 시스템 호출에서 복귀할 때 레지스터를 복원하며, 시스템 호출에 의해 반환된 모든 에러를 `errno(3)`에 저장합니다. 시스템 호출 번호에 대한 기호 상수(symbolic constants)는 헤더 파일 `<sys/syscall.h>`에서 찾을 수 있습니다.

### 반환 값 (RETURN VALUE)

반환 값은 호출되는 시스템 호출에 의해 정의됩니다. 일반적으로 0 반환 값은 성공을 나타냅니다. -1 반환 값은 에러를 나타내며, 에러 번호는 `errno`에 저장됩니다.

### 참고 사항 (NOTES)

`syscall()`은 4BSD에서 처음 등장했습니다.

#### 아키텍처별 요구 사항 (Architecture-specific requirements)

각 아키텍처 ABI는 시스템 호출 인수가 커널에 전달되는 방식에 대한 고유한 요구 사항을 가집니다 . glibc 래퍼가 있는 시스템 호출(예: 대부분의 시스템 호출)의 경우, glibc는 아키텍처에 적합한 방식으로 인수를 올바른 레지스터에 복사하는 상세 사항을 처리합니다. 그러나 시스템 호출을 수행하기 위해 `syscall()`을 사용할 때, 호출자는 아키텍처 의존적인 상세 사항을 처리해야 할 수도 있습니다. 이 요구 사항은 특정 32비트 아키텍처에서 가장 흔하게 마주치게 됩니다.

예를 들어, ARM 아키텍처 Embedded ABI (EABI)에서 64비트 값(예: `long long`)은 짝수 레지스터 쌍에 정렬되어야 합니다. 따라서 glibc에서 제공하는 래퍼 대신 `syscall()`을 사용한다면, 리틀 엔디안 모드의 EABI를 사용하는 ARM 아키텍처에서 `readahead(2)` 시스템 호출은 다음과 같이 호출될 것입니다: 

```c
syscall(SYS_readahead, fd, 0,
        (unsigned int) (offset & 0xFFFFFFFF),
        (unsigned int) (offset >> 32),
        [cite_start]count); [cite: 257, 258]

```

`offset` 인수가 64비트이고 첫 번째 인수(`fd`)가 `r0`로 전달되므로, 호출자는 64비트 값을 수동으로 분할하고 정렬하여 `r2/r3` 레지스터 쌍으로 전달되도록 해야 합니다. 이는 `r1`에 더미 값(0인 두 번째 인수)을 삽입하는 것을 의미합니다. 또한 분할이 (플랫폼의 C ABI에 따른) 엔디안 관례를 따르도록 주의를 기울여야 합니다. 유사한 문제가 O32 ABI를 사용하는 MIPS, 32비트 ABI를 사용하는 PowerPC 및 parisc, 그리고 Xtensa에서도 발생할 수 있습니다 . parisc C ABI 또한 정렬된 레지스터 쌍을 사용하지만, 사용자 공간에서 이 문제를 숨기기 위해 심(shim) 계층을 사용함에 유의하십시오. 영향을 받는 시스템 호출은 `fadvise64_64(2)`, `ftruncate64(2)`, `posix_fadvise(2)`, `pread64(2)`, `pwrite64(2)`, `readahead(2)`, `sync_file_range(2)`, 및 `truncate64(2)`입니다. 이는 `_llseek(2)`, `preadv(2)`, `preadv2(2)`, `pwritev(2)`, 및 `pwritev2(2)`와 같이 수동으로 64비트 값을 분할하고 조립하는 시스템 호출에는 영향을 미치지 않습니다. 역사적 산물(historical baggage)의 경이로운 세계에 오신 것을 환영합니다.

#### 아키텍처 호출 관례 (Architecture calling conventions)

모든 아키텍처는 커널을 호출하고 인수를 전달하는 자신만의 방식을 가지고 있습니다. 다양한 아키텍처에 대한 상세 사항은 아래의 두 테이블에 나열되어 있습니다.

첫 번째 테이블은 커널 모드로 전환하는 데 사용되는 명령어(커널로 전환하는 가장 빠르거나 최선의 방법이 아닐 수 있으므로 `vdso(7)`를 참조해야 할 수도 있음), 시스템 호출 번호를 나타내는 데 사용되는 레지스터, 시스템 호출 결과를 반환하는 데 사용되는 레지스터(들), 그리고 에러를 알리는 데 사용되는 레지스터를 나열합니다.

**표 1: 시스템 호출 호출 방식** 

| Arch/ABI | Instruction | System call # | Ret val | Ret val2 | Error | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| alpha | callsys | v0 | v0 | a4 | a3 | 1, 6 |
| arc | trap0 | r8 | r0 | - | - |  |
| arm/OABI | swi NR | - | r0 | - | - | 2 |
| arm/EABI | swi 0x0 | r7 | r0 | r1 | - |  |
| arm64 | svc #0 | w8 | x0 | x1 | - |  |
| blackfin | excpt 0x0 | P0 | R0 | - | - |  |
| i386 | int $0x80 | eax | eax | edx | - |  |
| ia64 | break 0x100000 | r15 | r8 | r9 | r10 | 1, 6 |
| m68k | trap #0 | d0 | d0 | - | - |  |
| microblaze | brki r14,8 | r12 | r3 | - | - |  |
| mips | syscall | v0 | v0 | v1 | a3 | 1, 6 |
| nios2 | trap | r2 | r2 | - | r7 |  |
| parisc | ble 0x100(%sr2, %r0) | r20 | r28 | - | - |  |
| powerpc | sc | r0 | r3 | - | r0 | 1 |
| powerpc64 | sc | r0 | r3 | - | cr0.SO | 1 |
| riscv | ecall | a7 | a0 | a1 | - |  |
| s390 | svc 0 | r1 | r2 | r3 | - | 3 |
| s390x | svc 0 | r1 | r2 | r3 | - | 3 |
| superh | trap #0x17 | r3 | r0 | r1 | - | 4, 6 |
| sparc/32 | t 0x10 | g1 | o0 | o1 | psr/csr | 1, 6 |
| sparc/64 | t 0x6d | g1 | o0 | o1 | psr/csr | 1, 6 |
| tile | swint1 | R10 | R00 | - | R01 | 1 |
| x86-64 | syscall | rax | rax | rdx | - | 5 |
| x32 | syscall | rax | rax | rdx | - | 5 |
| xtensa | syscall | a2 | a2 | - | - |  |

**참고 (Notes):**

* [1] 몇몇 아키텍처에서는 시스템 호출이 실패했음을 알리기 위해 레지스터 하나가 불리언(0은 에러 없음, -1은 에러를 나타냄)으로 사용됩니다. 실제 에러 값은 여전히 반환 레지스터에 포함되어 있습니다 . sparc에서는 전체 레지스터 대신 프로세서 상태 레지스터(psr)의 캐리 비트(csr)가 사용됩니다 . powerpc64에서는 조건 레지스터(cr0)의 필드 0에 있는 요약 오버플로 비트(SO)가 사용됩니다.


* [2] NR은 시스템 호출 번호입니다.


* [3] s390 및 s390x의 경우, NR(시스템 호출 번호)이 256보다 작으면 `svc NR`로 직접 전달될 수 있습니다.


* [4] SuperH에서 트랩 번호는 전달되는 인수의 최대 개수를 제어합니다. `trap #0x10`은 인수가 0개인 시스템 호출에만 사용될 수 있고, `trap #0x11`은 0개 또는 1개 인수를 가진 시스템 호출에 사용될 수 있는 식으로, 인수가 7개인 시스템 호출을 위한 `trap #0x17`까지 가능합니다.


* [5] x32 ABI는 x86-64 ABI와 시스템 호출 테이블을 공유하지만, 몇 가지 뉘앙스가 있습니다: 


* 시스템 호출이 x32 ABI 하에서 호출됨을 나타내기 위해, 추가 비트인 `__X32_SYSCALL_BIT`가 시스템 호출 번호와 비트별-OR(bitwise-OR)됩니다. 프로세스에 의해 사용되는 ABI는 신호 처리나 시스템 호출 재시작을 포함한 일부 프로세스 동작에 영향을 미칩니다.


* x32는 long 및 포인터 타입의 크기가 다르기 때문에, 일부(전부는 아님; 예를 들어 `struct timeval`이나 `struct rlimit`은 64비트임) 구조체의 레이아웃이 다릅니다. 이를 처리하기 위해, 번호 512부터 시작하는 추가 시스템 호출들이 시스템 호출 테이블에 추가됩니다 (`__X32_SYSCALL_BIT` 없이). 예를 들어, `__NR_readv`는 x86-64 ABI의 경우 19로 정의되고 x32 ABI의 경우 `__X32_SYSCALL_BIT | [cite_start]515`로 정의됩니다. 이러한 추가 시스템 호출의 대부분은 실제로는 i386 호환성을 제공하기 위해 사용되는 시스템 호출과 동일합니다. 그러나 `preadv2(2)`와 같은 주목할 만한 예외가 있는데, 이는 4바이트 포인터와 크기를 가진 `struct iovec` 엔티티(커널 용어로 "compat_iovec")를 사용하지만, 다른 모든 ABI에서 하는 것처럼 두 개가 아닌 단일 레지스터로 8바이트 `pos` 인수를 전달합니다.




* [6] 일부 아키텍처(즉, Alpha, IA-64, MIPS, SuperH, sparc/32, 및 sparc/64)는 `pipe(2)` 시스템 호출로부터 두 번째 반환 값을 다시 전달하기 위해 추가 레지스터(위 테이블의 "Retval2")를 사용합니다. Alpha는 아키텍처 특정적인 `getxpid(2)`, `getxuid(2)`, 및 `getxgid(2)` 시스템 호출에서도 이 기술을 사용합니다. 다른 아키텍처들은 시스템 V ABI에 정의되어 있더라도 시스템 호출 인터페이스에서 두 번째 반환 값 레지스터를 사용하지 않습니다.



두 번째 테이블은 시스템 호출 인수를 전달하는 데 사용되는 레지스터를 보여줍니다.

**표 2: 인수 전달 레지스터** 

| Arch/ABI | arg1 | arg2 | arg3 | arg4 | arg5 | arg6 | arg7 | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| alpha | a0 | a1 | a2 | a3 | a4 | a5 | - |  |
| arc | r0 | r1 | r2 | r3 | r4 | r5 | -  |  |
| arm/OABI | r0 | r1 | r2 | r3 | r4 | r5 | r6 |  |
| arm/EABI | r0 | r1 | r2 | r3 | r4 | r5 | r6 |  |
| arm64 | x0 | x1 | x2 | x3 | x4 | x5 | -  |  |
| blackfin | R0 | R1 | R2 | R3 | R4 | R5 | - |  |
| i386 | ebx | ecx | edx | esi | edi | ebp | - |  |
| ia64 | out0 | out1 | out2 | out3 | out4 | out5 | -  |  |
| m68k | d1 | d2 | d3 | d4 | d5 | a0 | - |  |
| microblaze | r5 | r6 | r7 | r8 | r9 | r10 | - |  |
| mips/o32 | a0 | a1 | a2 | a3 | - | - | - | 1  |
| mips/n32,64 | a0 | a1 | a2 | a3 | a4 | a5 | - |  |
| nios2 | r4 | r5 | r6 | r7 | r8 | r9 | -  |  |
| parisc | r26 | r25 | r24 | r23 | r22 | r21 | - |  |
| powerpc | r3 | r4 | r5 | r6 | r7 | r8 | r9 |  |
| powerpc64 | r3 | r4 | r5 | r6 | r7 | r8 | -  |  |
| riscv | a0 | a1 | a2 | a3 | a4 | a5 | - |  |
| s390 | r2 | r3 | r4 | r5 | r6 | r7 | -  |  |
| s390x | r2 | r3 | r4 | r5 | r6 | r7 | - |  |
| superh | r4 | r5 | r6 | r7 | r0 | r1 | r2 |  |
| sparc/32 | o0 | o1 | o2 | o3 | o4 | o5 | -  |  |
| sparc/64 | o0 | o1 | o2 | o3 | o4 | o5 | - |  |
| tile | R00 | R01 | R02 | R03 | R04 | R05 | - |  |
| x86-64 | rdi | rsi | rdx | r10 | r8 | r9 | -  |  |
| x32 | rdi | rsi | rdx | r10 | r8 | r9 | - |  |
| xtensa | a6 | a3 | a4 | a5 | a8 | a9 | -  |  |

**참고 (Notes):**

* [1] mips/o32 시스템 호출 관례는 인수 5번부터 8번까지를 사용자 스택(user stack)으로 전달합니다. 이 테이블들이 전체 호출 관례를 다루지는 않음에 유의하십시오—일부 아키텍처는 여기에 나열되지 않은 다른 레지스터들을 무차별적으로 훼손(clobber)할 수 있습니다.



예제 (EXAMPLES) 

```c
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>

int
main(int argc, char *argv[])
{
    pid_t tid;

    [cite_start]tid = syscall(SYS_gettid); [cite: 311, 312]
    [cite_start]syscall(SYS_tgkill, getpid(), tid, SIGHUP); [cite: 312]
}

```

### 참고 항목 (SEE ALSO)

`_syscall(2)`, `intro(2)`, `syscalls(2)`, `errno(3)`, `vdso(7)` 

### 콜로폰 (COLOPHON)

이 페이지는 리눅스 매뉴얼 페이지(man-pages) 프로젝트의 5.10 릴리스의 일부입니다. 프로젝트에 대한 설명, 버그 보고에 대한 정보, 그리고 이 페이지의 최신 버전은 [https://www.kernel.org/doc/man-pages/](https://www.kernel.org/doc/man-pages/) 에서 찾을 수 있습니다.

리눅스 2020-06-09 SYSCALL(2) 

---

**다른 시스템 호출이나 매뉴얼 페이지 번역이 더 필요하시면 알려주세요.**