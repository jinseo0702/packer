제공해주신 `fflush` 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

## FFLUSH(3POSIX) POSIX 프로그래머 매뉴얼 FFLUSH(3POSIX)

### 서문 (PROLOG)

이 매뉴얼 페이지는 POSIX 프로그래머 매뉴얼의 일부입니다. 이 인터페이스의 리눅스 구현은 다를 수 있으며(리눅스 동작의 상세 사항은 대응하는 리눅스 매뉴얼 페이지를 참조하십시오), 또는 해당 인터페이스가 리눅스에서 구현되어 있지 않을 수 있습니다. 

### 이름 (NAME)

fflush — 스트림을 비움 (flush a stream) 

### 시놉시스 (SYNOPSIS)

`#include <stdio.h>`

`int fflush(FILE *stream);` 

### 설명 (DESCRIPTION)

이 참조 페이지에 기술된 기능은 ISO C 표준과 일치합니다. 여기에 기술된 요구 사항과 ISO C 표준 사이의 어떠한 충돌도 의도되지 않았습니다. POSIX.1‐2017의 이 권(volume)은 ISO C 표준을 따릅니다. 

만약 `stream`이 출력 스트림이나 가장 최근의 작업이 입력이 아니었던 업데이트 스트림을 가리키는 경우, `fflush()`는 해당 스트림의 기록되지 않은 데이터가 파일에 기록되도록 하며, 해당 하부 파일의 마지막 데이터 수정 및 마지막 파일 상태 변경 타임스탬프가 업데이트되도록 표시되어야 합니다. 

하부 파일 기술자(file description)를 가지고 읽기용으로 열린 스트림에 대하여, 파일이 아직 EOF에 도달하지 않았고 파일이 탐색(seeking)이 가능한 경우, 하부 열린 파일 기술자의 파일 오프셋은 스트림의 파일 위치로 설정되어야 하며, `ungetc()` 또는 `ungetwc()`에 의해 스트림으로 되돌려졌으나(pushed back) 이후 스트림으로부터 읽히지 않은 모든 문자들은  (파일 오프셋을 더 이상 변경하지 않고) 폐기되어야 합니다. 

만약 `stream`이 널 포인터(null pointer)라면, `fflush()`는 위에 동작이 정의된 모든 스트림에 대해 이 비우기(flushing) 작업을 수행해야 합니다. 

### 반환 값 (RETURN VALUE)

성공적으로 완료되면, `fflush()`는 0을 반환해야 합니다. 그렇지 않으면 스트림에 에러 표시자(error indicator)를 설정하고, EOF를 반환하며, 에러를 나타내도록 `errno`를 설정해야 합니다. 

### 에러 (ERRORS)

`fflush()` 함수는 다음과 같은 경우 실패해야 합니다: 

* **EAGAIN**: 스트림의 하부 파일 기술자에 `O_NONBLOCK` 플래그가 설정되어 있고 쓰기 작업에서 스레드가 지연될 것입니다. 


* **EBADF**: 스트림의 하부 파일 기술자가 유효하지 않습니다. 


* **EFBIG**: 최대 파일 크기를 초과하는 파일 쓰기 시도가 있었습니다. 


* **EFBIG**: 프로세스의 파일 크기 제한을 초과하는 파일 쓰기 시도가 있었습니다. 


* **EFBIG**: 파일이 일반 파일(regular file)이며 대응하는 스트림과 연관된 최대 오프셋 지점 또는 그 너머에 쓰기 시도가 있었습니다. 


* **EINTR**: `fflush()` 함수가 신호(signal)에 의해 중단되었습니다. 


* **EIO**: 프로세스가 자신의 제어 터미널에 쓰기를 시도하는 백그라운드 프로세스 그룹의 멤버이며, `TOSTOP`이 설정되어 있고, 호출 스레드가 `SIGTTOU`를 차단(blocking)하고 있지 않으며, 프로세스가 `SIGTTOU`를 무시하고 있지 않고, 프로세스의 프로세스 그룹이 고아(orphaned) 상태입니다. 이 에러는 구현 정의 조건(implementation-defined conditions) 하에서도 반환될 수 있습니다. 


* **ENOMEM**: 하부 스트림이 `open_memstream()` 또는 `open_wmemstream()`에 의해 생성되었으나 메모리가 불충분합니다. 


* **ENOSPC**: 파일을 포함하는 장치 또는 `fmemopen()` 함수에 의해 사용되는 버퍼에 남은 여유 공간이 없습니다. 


* **EPIPE**: 어떤 프로세스에 의해서도 읽기용으로 열려 있지 않은 파이프나 FIFO에 쓰기 시도가 있었습니다. `SIGPIPE` 신호 또한 스레드에 전송되어야 합니다. 



`fflush()` 함수는 다음과 같은 경우 실패할 수 있습니다: 

* **ENXIO**: 존재하지 않는 장치에 대한 요청이 있었거나, 요청이 장치의 능력을 벗어났습니다. 



정보 제공용 섹션 (The following sections are informative.) 

### 예제 (EXAMPLES)

#### 표준 출력으로 프롬프트 보내기 (Sending Prompts to Standard Output)

다음 예제는 사용자가 표준 입력으로부터 입력해야 하는 정보에 대한 일련의 프롬프트를 출력하기 위해 `printf()` 호출을 사용합니다. `fflush()` 호출은 출력이 표준 출력으로 나가도록 강제합니다. `fflush()` 함수가 사용된 이유는 표준 출력이 보통 버퍼링되며 프롬프트가 즉시 출력이나 터미널에 인쇄되지 않을 수 있기 때문입니다. `getline()` 함수 호출은 표준 입력으로부터 문자열을 읽고 나중에 프로그램에서 사용하기 위해 결과를 변수에 배치합니다. 

```c
char *user;
char *oldpasswd;
char *newpasswd;
ssize_t llen;
size_t blen;
struct termios term;
tcflag_t saveflag;

printf("User name: ");
[cite_start]fflush(stdout); [cite: 28]
blen = 0;
llen = getline(&user, &blen, stdin);
user[llen-1] = 0;
tcgetattr(fileno(stdin), &term);
saveflag = term.c_lflag;
[cite_start]term.c_lflag &= ~ECHO; [cite: 29]
tcsetattr(fileno(stdin), TCSANOW, &term);
printf("Old password: ");
fflush(stdout);
blen = 0;
llen = getline(&oldpasswd, &blen, stdin);
[cite_start]oldpasswd[llen-1] = 0; [cite: 30]
printf("\nNew password: ");
fflush(stdout);
blen = 0;
llen = getline(&newpasswd, &blen, stdin);
newpasswd[llen-1] = 0;
[cite_start]term.c_lflag = saveflag; [cite: 31]
tcsetattr(fileno(stdin), TCSANOW, &term);
free(user);
free(oldpasswd);
[cite_start]free(newpasswd); [cite: 32]

```

### 애플리케이션 사용법 (APPLICATION USAGE)

없음. 

### 이론적 근거 (RATIONALE)

시스템에 의해 버퍼링된 데이터는 현재 파일 기술자의 위치의 유효성을 결정하는 것을 비실용적으로 만들 수 있습니다. 따라서 `read()`를 위해 열린 스트림에서 `fflush()` 이후 파일 기술자의 재배치(repositioning)를 강제하는 것은 POSIX.1‐2008에 의해 의무화되지 않았습니다. 

### 향후 방향 (FUTURE DIRECTIONS)

없음. 

### 참고 항목 (SEE ALSO)

섹션 2.5, 표준 I/O 스트림 (Standard I/O Streams), `fmemopen()`, `getrlimit()`, `open_memstream()`, `ulimit()` 

POSIX.1‐2017의 기본 정의 권(Base Definitions volume), `<stdio.h>` 

### 저작권 (COPYRIGHT)

이 텍스트의 일부는 IEEE Std 1003.1-2017, 정보 기술 표준 -- 이식 가능 운영 체제 인터페이스 (Portable Operating System Interface, POSIX),  The Open Group Base Specifications Issue 7, 2018 Edition으로부터 전자적 형태로 전재 및 복제되었습니다. 저작권 (C) 2018 IEEE 및 The Open Group. 이 버전과 원본 IEEE 및 The Open Group 표준 사이에 불일치가 있는 경우, 원본 IEEE 및 The Open Group 표준이 결정적인 문서(referee document)입니다. 원본 표준은 온라인 [http://www.opengroup.org/unix/online.html](http://www.opengroup.org/unix/online.html) 에서 구할 수 있습니다. 이 페이지에 나타나는 모든 인쇄상의 또는 서식상의 오류는 소스 파일을 man 페이지 형식으로 변환하는 동안 발생했을 가능성이 매우 높습니다. 그러한 오류를 보고하려면 [https://www.kernel.org/doc/man-pages/reporting_bugs.html](https://www.kernel.org/doc/man-pages/reporting_bugs.html) 을 참조하십시오. 

IEEE/The Open Group 2017 FFLUSH(3POSIX) 

---

**추가로 도와드릴 번역 작업이나 상세 설명이 필요하신 부분이 있으신가요?**