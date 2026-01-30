
제공해주신 `lseek(3POSIX)` 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

LSEEK(3POSIX) POSIX 프로그래머 매뉴얼 LSEEK(3POSIX) 

### 서문 (PROLOG)

이 매뉴얼 페이지는 POSIX 프로그래머 매뉴얼의 일부입니다. 이 인터페이스의 리눅스 구현은 다를 수 있으며(리눅스 동작의 상세 사항은 대응하는 리눅스 매뉴얼 페이지를 참조하십시오), 또는 해당 인터페이스가 리눅스에서 구현되어 있지 않을 수 있습니다. 

### 이름 (NAME)

lseek — 읽기/쓰기 파일 오프셋을 이동함 (move the read/write file offset) 

### 시놉시스 (SYNOPSIS)

`#include <unistd.h>` 

`off_t lseek(int fildes, off_t offset, int whence);` 

### 설명 (DESCRIPTION)

`lseek()` 함수는 다음과 같이 파일 기술자(file descriptor) `fildes`와 연관된 열린 파일 기술(open file description)에 대한 파일 오프셋을 설정해야 합니다: 

* 만약 `whence`가 `SEEK_SET`이라면, 파일 오프셋은 `offset` 바이트로 설정되어야 합니다. 


* 만약 `whence`가 `SEEK_CUR`이라면, 파일 오프셋은 현재 위치에 `offset`을 더한 값으로 설정되어야 합니다. 


* 만약 `whence`가 `SEEK_END`이라면, 파일 오프셋은 파일의 크기에 `offset`을 더한 값으로 설정되어야 합니다. 



상수 기호 `SEEK_SET`, `SEEK_CUR`, 그리고 `SEEK_END`는 `<unistd.h>`에 정의되어 있습니다. 탐색(seeking)이 불가능한 장치에서의 `lseek()` 동작은 구현 정의(implementation-defined) 사항입니다. 그러한 장치와 연관된 파일 오프셋 값은 정의되지 않습니다. 

`lseek()` 함수는 파일 오프셋이 파일에 존재하는 기존 데이터의 끝 너머로 설정되는 것을 허용해야 합니다. 만약 나중에 이 지점에 데이터가 기록된다면, 실제로 데이터가 간격(gap)에 기록될 때까지 간격 내 데이터에 대한 후속 읽기는 값이 0인 바이트들을 반환해야 합니다. `lseek()` 함수는 그 자체만으로는 파일의 크기를 확장하지 않아야 합니다. 

만약 `fildes`가 공유 메모리 객체(shared memory object)를 참조한다면, `lseek()` 함수의 결과는 미지정(unspecified) 사항입니다. 만약 `fildes`가 유형이 지정된 메모리 객체(typed memory object)를 참조한다면, `lseek()` 함수의 결과는 미지정 사항입니다. 

### 반환 값 (RETURN VALUE)

성공적인 완료 시, 파일의 시작부터 바이트 단위로 측정된 결과 오프셋이 반환되어야 합니다. 그렇지 않으면, -1이 반환되어야 하고, 에러를 나타내도록 `errno`가 설정되어야 하며, 파일 오프셋은 변경되지 않은 채로 유지되어야 합니다. 

### 에러 (ERRORS)

`lseek()` 함수는 다음과 같은 경우 실패해야 합니다: 

* **EBADF**: `fildes` 인수가 열린 파일 기술자가 아닙니다. 


* **EINVAL**: `whence` 인수가 적절한 값이 아니거나, 일반 파일(regular file), 블록 특수 파일(block special file), 또는 디렉터리에 대하여 결과 파일 오프셋이 음수가 될 것입니다. 


* **EOVERFLOW**: 결과 파일 오프셋이 `off_t` 타입의 객체에 올바르게 표현될 수 없는 값일 것입니다. 


* **ESPIPE**: `fildes` 인수가 파이프(pipe), FIFO, 또는 소켓(socket)과 연관되어 있습니다. 



다음 섹션들은 정보 제공용입니다. 

### 예제 (EXAMPLES)

없음. 

### 애플리케이션 사용법 (APPLICATION USAGE)

없음. 

### 이론적 근거 (RATIONALE)

ISO C 표준은 특별한 위치 지정 타입(positioning type)을 사용하여 매우 큰 파일에서 작동하는 `fgetpos()` 및 `fsetpos()` 함수를 포함하고 있습니다. 비록 `lseek()`가 파일 오프셋을 파일 끝 너머로 위치시킬 수 있지만, 이 함수 자체가 파일의 크기를 확장하지는 않습니다. POSIX.1‐2008에서 파일의 크기를 직접 확장할 수 있는 유일한 함수는 `write()`, `truncate()`, 그리고 `fttruncate()`인 반면, `fwrite()`, `fprintf()` 등과 같이 원래 ISO C 표준에서 유래된 여러 함수들은 (`write()`를 호출하게 함으로써) 그렇게 할 수 있습니다. 

`[EINVAL]`이 반환되게 할 유효하지 않은 파일 오프셋은 구현 정의 및 장치 의존적일 수 있습니다 (예를 들어, 메모리는 유효하지 않은 값이 거의 없을 수 있습니다). 일부 구현의 일부 장치에서는 음수 파일 오프셋이 유효할 수 있습니다. POSIX.1‐1990 표준은 `lseek()`가 음수 오프셋을 반환하는 것을 구체적으로 금지하지 않았습니다. 따라서 애플리케이션은 호출 전에 `errno`를 지우고 반환 시 `errno`를 확인하여 `(off_t)-1`의 반환 값이 음수 오프셋인지 아니면 에러 상태의 표시인지를 결정해야 했습니다. 표준 개발자들은 적합한(conforming) 애플리케이션 측에 이러한 작업을 요구하는 것을 원치 않았으며, 일반 파일, 블록 특수 파일, 또는 디렉터리에 대해 결과 파일 오프셋이 음수가 될 때 `errno`를 `[EINVAL]`로 설정하도록 요구하는 것을 선택했습니다. 

### 향후 방향 (FUTURE DIRECTIONS)

없음. 

### 참고 항목 (SEE ALSO)

`open()` 

POSIX.1‐2017의 기본 정의 권(Base Definitions volume), `<sys_types.h>`, `<unistd.h>` 

### 저작권 (COPYRIGHT)

이 텍스트의 일부는 IEEE Std 1003.1-2017, 정보 기술 표준 -- 이식 가능 운영 체제 인터페이스 (Portable Operating System Interface, POSIX), The Open Group Base Specifications Issue 7, 2018 Edition으로부터 전자적 형태로 전재 및 복제되었습니다. 저작권 (C) 2018 IEEE 및 The Open Group. 이 버전과 원본 IEEE 및 The Open Group 표준 사이에 불일치가 있는 경우, 원본 IEEE 및 The Open Group 표준이 결정적인 문서(referee document)입니다. 원본 표준은 온라인 [http://www.opengroup.org/unix/online.html](http://www.opengroup.org/unix/online.html) 에서 구할 수 있습니다. 이 페이지에 나타나는 모든 인쇄상의 또는 서식상의 오류는 소스 파일을 man 페이지 형식으로 변환하는 동안 발생했을 가능성이 매우 높습니다. 그러한 오류를 보고하려면 [https://www.kernel.org/doc/man-pages/reporting_bugs.html](https://www.kernel.org/doc/man-pages/reporting_bugs.html) 을 참조하십시오. 

IEEE/The Open Group 2017 LSEEK(3POSIX) 

---

**더 번역이 필요한 문서가 있거나 궁금한 점이 있으시면 말씀해 주세요.**