
제공해주신 `fputs` 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

## FPUTS(3POSIX) POSIX 프로그래머 매뉴얼 FPUTS(3POSIX)

### 서문 (PROLOG)

이 매뉴얼 페이지는 POSIX 프로그래머 매뉴얼의 일부입니다. 이 인터페이스의 리눅스 구현은 다를 수 있으며(리눅스 동작의 상세 사항은 대응하는 리눅스 매뉴얼 페이지를 참조하십시오), 또는 해당 인터페이스가 리눅스에서 구현되어 있지 않을 수 있습니다.

### 이름 (NAME)

fputs — 스트림에 문자열을 넣음 (put a string on a stream) 

### 시놉시스 (SYNOPSIS)

`#include <stdio.h>` 

`int fputs(const char *restrict s, FILE *restrict stream);` 

### 설명 (DESCRIPTION)

이 참조 페이지에 기술된 기능은 ISO C 표준과 일치합니다. 여기에 기술된 요구 사항과 ISO C 표준 사이의 어떠한 충돌도 의도되지 않았습니다. POSIX.1‐2017의 이 권(volume)은 ISO C 표준을 따릅니다.

`fputs()` 함수는 `s`가 가리키는 널 종료 문자열(null-terminated string)을 `stream`이 가리키는 스트림에 기록해야 합니다. 종료 널 바이트(terminating null byte)는 기록되지 않아야 합니다.

파일의 마지막 데이터 수정 및 마지막 파일 상태 변경 타임스탬프는 `fputs()`의 성공적인 실행과, 동일한 스트림에 대한 다음번 `fflush()` 또는 `fclose()`의 성공적인 완료, 혹은 `exit()` 또는 `abort()` 호출 사이에 업데이트되도록 표시되어야 합니다.

### 반환 값 (RETURN VALUE)

성공적으로 완료되면, `fputs()`는 음수가 아닌 수를 반환해야 합니다. 그렇지 않으면 EOF를 반환하고, 스트림에 에러 표시자를 설정하며, 에러를 나타내도록 `errno`를 설정해야 합니다.

### 에러 (ERRORS)

`fputc()`를 참조하십시오.

다음 섹션들은 정보 제공용입니다.

### 예제 (EXAMPLES)

#### 표준 출력에 인쇄하기 (Printing to Standard Output)

다음 예제는 현재 시간을 가져와서 `localtime()` 및 `asctime()`을 사용하여 문자열로 변환하고, `fputs()`를 사용하여 표준 출력에 인쇄합니다. 그런 다음 대기 중인 이벤트까지 남은 분(minutes) 수를 인쇄합니다.

```c
[cite_start]#include <time.h> [cite: 55]
[cite_start]#include <stdio.h> [cite: 55]
...
[cite_start]time_t now; [cite: 55]
[cite_start]int minutes_to_event; [cite: 56]
...
[cite_start]time(&now); [cite: 56]
[cite_start]printf("The time is "); [cite: 56]
[cite_start]fputs(asctime(localtime(&now)), stdout); [cite: 56]
[cite_start]printf("There are still %d minutes to the event.\n", [cite: 57]
       [cite_start]minutes_to_event); [cite: 57]
[cite_start]... [cite: 58]

```

### 애플리케이션 사용법 (APPLICATION USAGE)

`puts()` 함수는 개행 문자(`<newline>`)를 추가하는 반면 `fputs()`는 추가하지 않습니다.

POSIX.1‐2017의 이 권은 성공적인 완료 시 단순히 음수가 아닌 정수를 반환할 것을 요구합니다. 이 요구 사항에 대해 적어도 세 가지 알려진 서로 다른 구현 관행이 있습니다:

* 상수 값을 반환함.


* 기록된 마지막 문자를 반환함.


* 기록된 바이트 수를 반환함. 이 구현 관행은 값이 함수의 반환 타입으로 표현될 수 없기 때문에 `{INT_MAX}` 바이트보다 긴 문자열에 대해서는 준수될 수 없음에 유의하십시오. 하위 호환성을 위해, 구현체들은 `{INT_MAX}` 바이트까지의 문자열에 대해서는 바이트 수를 반환하고, 그보다 긴 모든 문자열에 대해서는 `{INT_MAX}`를 반환할 수 있습니다.



### 이론적 근거 (RATIONALE)

`fputs()` 함수는 참조된 "The C Programming Language"에 소스 코드가 명시된 함수 중 하나입니다. 초판에서 이 함수는 정의된 반환 값이 없었으나, 많은 실제 구현들이 부수 효과(side-effect)로서 기록된 마지막 문자의 값을 반환하곤 했는데, 이는 그 값이 반환 값으로 사용되는 누산기(accumulator)에 남아 있는 값이었기 때문입니다. 이 책의 제2판에서는 `ferror()`의 반환 값에 따라 고정된 값 0 또는 EOF가 반환될 것이라고 하였습니다. 그러나 현존하는 구현들과의 호환성을 위해, 여러 구현들은 성공 시 기록된 마지막 바이트를 나타내는 양수 값을 반환하곤 합니다.

### 향후 방향 (FUTURE DIRECTIONS)

없음.

### 참고 항목 (SEE ALSO)

섹션 2.5, 표준 I/O 스트림 (Standard I/O Streams), `fopen()`, `putc()`, `puts()` 

POSIX.1‐2017의 기본 정의 권(Base Definitions volume), `<stdio.h>` 

### 저작권 (COPYRIGHT)

이 텍스트의 일부는 IEEE Std 1003.1-2017, 정보 기술 표준 -- 이식 가능 운영 체제 인터페이스 (Portable Operating System Interface, POSIX), The Open Group Base Specifications Issue 7, 2018 Edition으로부터 전자적 형태로 전재 및 복제되었습니다. 저작권 (C) 2018 IEEE 및 The Open Group. 이 버전과 원본 IEEE 및 The Open Group 표준 사이에 불일치가 있는 경우, 원본 IEEE 및 The Open Group 표준이 결정적인 문서(referee document)입니다. 원본 표준은 온라인 [http://www.opengroup.org/unix/online.html](http://www.opengroup.org/unix/online.html) 에서 구할 수 있습니다. 이 페이지에 나타나는 모든 인쇄상의 또는 서식상의 오류는 소스 파일을 man 페이지 형식으로 변환하는 동안 발생했을 가능성이 매우 높습니다. 그러한 오류를 보고하려면 [https://www.kernel.org/doc/man-pages/reporting_bugs.html](https://www.kernel.org/doc/man-pages/reporting_bugs.html) 을 참조하십시오.

IEEE/The Open Group 2017 FPUTS(3POSIX) 

---

**다른 함수에 대한 번역이나 추가적인 작업이 필요하시면 말씀해 주세요.**