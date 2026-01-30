
제공해주신 `mprotect(3POSIX)` 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

## MPROTECT(3POSIX) POSIX 프로그래머 매뉴얼 MPROTECT(3POSIX)

### 서문 (PROLOG)

이 매뉴얼 페이지는 POSIX 프로그래머 매뉴얼의 일부입니다. 이 인터페이스의 리눅스 구현은 다를 수 있으며(리눅스 동작의 상세 사항은 대응하는 리눅스 매뉴얼 페이지를 참조하십시오), 또는 해당 인터페이스가 리눅스에서 구현되어 있지 않을 수 있습니다.

### 이름 (NAME)

mprotect — 메모리 매핑의 보호를 설정함 

### 시놉시스 (SYNOPSIS)

`#include <sys/mman.h>` 

`int mprotect(void *addr, size_t len, int prot);` 

### 설명 (DESCRIPTION)

`mprotect()` 함수는 주소 `addr`에서 시작하여 `len` 바이트 동안 계속되는 프로세스의 주소 공간의 임의의 부분을 포함하는 해당 전체 페이지들에 대하여, `prot`에 의해 지정된 것으로 접근 보호를 변경해야 합니다. 매개변수 `prot`는 매핑되는 데이터에 대해 읽기, 쓰기, 실행 또는 이들의 특정 조합의 접근이 허용되는지 여부를 결정합니다. `prot` 인수는 `PROT_NONE`이거나, `PROT_READ`, `PROT_WRITE`, 및 `PROT_EXEC` 중 하나 이상의 비트 단위 포함적 OR(bitwise-inclusive OR)이어야 합니다.

만약 구현체가 `prot`에 의해 지정된 접근 유형의 조합을 지원할 수 없다면, `mprotect()` 호출은 실패해야 합니다. 구현체는 `prot`에 의해 지정된 것 이외의 접근을 허용할 수 있으나 , 어떠한 구현체도 `PROT_WRITE`가 설정되지 않은 곳에서 쓰기가 성공하도록 허용하거나, `PROT_NONE`만 설정된 곳에서 어떠한 접근도 허용해서는 안 됩니다. 구현체는 적어도 다음과 같은 `prot` 값들을 지원해야 합니다: `PROT_NONE`, `PROT_READ`, `PROT_WRITE`, 그리고 `PROT_READ`와 `PROT_WRITE`의 비트 단위 포함적 OR.

만약 `PROT_WRITE`가 지정된 경우, 객체를 매핑하는 데 사용된 파일 기술자가 이후에 닫혔는지 여부와 관계없이, 원래 매핑에서 `MAP_PRIVATE`이 지정되지 않았다면 애플리케이션은 지정된 주소 범위 내의 매핑된 객체들을 쓰기 권한으로 열었음을 보장해야 합니다. 구현체는 `addr`이 `sysconf()`에 의해 반환된 페이지 크기의 배수일 것을 요구할 수 있습니다. 이 함수의 동작은 매핑이 `mmap()` 호출에 의해 설정되지 않은 경우 미지정(unspecified) 사항입니다. `mprotect()`가 `[EINVAL]` 이외의 사유로 실패할 때, 범위 `[addr, addr+len)`에 있는 일부 페이지들의 보호가 변경되었을 수 있습니다.

### 반환 값 (RETURN VALUE)

성공적인 완료 시, `mprotect()`는 0을 반환해야 합니다. 그렇지 않으면, -1을 반환하고 에러를 나타내도록 `errno`를 설정해야 합니다.

### 에러 (ERRORS)

`mprotect()` 함수는 다음과 같은 경우 실패해야 합니다:

* **EACCES**: `prot` 인수가 프로세스가 하부 메모리 객체에 대해 가진 접근 권한을 위반하는 보호를 지정합니다.


* **EAGAIN**: `prot` 인수가 `MAP_PRIVATE` 매핑에 대해 `PROT_WRITE`를 지정하며, 프라이빗 페이지를 잠그기 위해 예약할 메모리 자원이 불충분합니다.


* **ENOMEM**: 범위 `[addr, addr+len)`의 주소들이 프로세스의 주소 공간에 유효하지 않거나, 매핑되지 않은 하나 이상의 페이지를 지정합니다.


* **ENOMEM**: `prot` 인수가 `MAP_PRIVATE` 매핑에 대해 `PROT_WRITE`를 지정하며, 필요한 경우 프라이빗 페이지를 잠그기 위해 시스템이 공급할 수 있는 것보다 더 많은 공간을 요구할 것입니다.


* **ENOTSUP**: 구현체가 `prot` 인수에서 요청된 접근의 조합을 지원하지 않습니다.



`mprotect()` 함수는 다음과 같은 경우 실패할 수 있습니다:

* **EINVAL**: `addr` 인수가 `sysconf()`에 의해 반환된 페이지 크기의 배수가 아닙니다.



정보 제공용 섹션 (The following sections are informative.) 

### 예제 (EXAMPLES)

없음.

### 애플리케이션 사용법 (APPLICATION USAGE)

대부분의 구현체는 `addr`이 `sysconf()`에 의해 반환된 페이지 크기의 배수일 것을 요구합니다.

### 이론적 근거 (RATIONALE)

없음.

### 향후 방향 (FUTURE DIRECTIONS)

없음.

### 참고 항목 (SEE ALSO)

`mmap()`, `sysconf()` 

POSIX.1‐2017의 기본 정의 권(Base Definitions volume), `<sys_mman.h>` 

### 저작권 (COPYRIGHT)

이 텍스트의 일부는 IEEE Std 1003.1-2017, 정보 기술 표준 -- 이식 가능 운영 체제 인터페이스 (Portable Operating System Interface, POSIX), The Open Group Base Specifications Issue 7, 2018 Edition으로부터 전자적 형태로 전재 및 복제되었습니다. 저작권 (C) 2018 IEEE 및 The Open Group. 이 버전과 원본 IEEE 및 The Open Group 표준 사이에 불일치가 있는 경우, 원본 IEEE 및 The Open Group 표준이 결정적인 문서(referee document)입니다. 원본 표준은 온라인 [http://www.opengroup.org/unix/online.html](http://www.opengroup.org/unix/online.html) 에서 구할 수 있습니다. 이 페이지에 나타나는 모든 인쇄상의 또는 서식상의 오류는 소스 파일을 man 페이지 형식으로 변환하는 동안 발생했을 가능성이 매우 높습니다. 그러한 오류를 보고하려면 [https://www.kernel.org/doc/man-pages/reporting_bugs.html](https://www.kernel.org/doc/man-pages/reporting_bugs.html) 을 참조하십시오.

IEEE/The Open Group 2017 MPROTECT(3POSIX) 

---

**추가로 번역이 필요한 문서가 있거나 도움이 필요하시면 말씀해 주세요.**