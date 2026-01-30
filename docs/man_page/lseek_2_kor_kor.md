
제공해주신 `lseek` 매뉴얼 페이지의 전체 내용을 지침에 따라 직역하였습니다.

---

LSEEK(2) 리눅스 프로그래머 매뉴얼 LSEEK(2) 

### 이름 (NAME)

lseek - 읽기/쓰기 파일 오프셋의 위치를 재설정함 (reposition read/write file offset) 

### 시놉시스 (SYNOPSIS)

`#include <sys/types.h>` `#include <unistd.h>` 

`off_t lseek(int fd, off_t offset, int whence);` 

### 설명 (DESCRIPTION)

`lseek()`는 파일 기술자(file descriptor) `fd`와 연관된 열린 파일 기술(open file description)의 파일 오프셋을 지시어(directive) `whence`에 따라 다음과 같이 인수 `offset`으로 재설정합니다: 

* **SEEK_SET**: 파일 오프셋이 `offset` 바이트로 설정됩니다. 


* **SEEK_CUR**: 파일 오프셋이 현재 위치에 `offset` 바이트를 더한 값으로 설정됩니다. 


* **SEEK_END**: 파일 오프셋이 파일의 크기에 `offset` 바이트를 더한 값으로 설정됩니다. 



`lseek()`는 파일 오프셋을 파일의 끝 너머로 설정하는 것을 허용합니다 (하지만 이것이 파일의 크기를 변경하지는 않습니다). 만약 나중에 이 지점에 데이터가 기록된다면, 실제로 데이터가 간격(gap)에 기록될 때까지 간격("홀(hole)") 내의 데이터에 대한 후속 읽기는 널 바이트('\0')를 반환합니다. 

파일 데이터와 홀 탐색하기 (Seeking file data and holes) 

버전 3.1부터 리눅스는 `whence`에 대해 다음과 같은 추가적인 값들을 지원합니다: 

* **SEEK_DATA**: 파일 오프셋을 `offset`보다 크거나 같으면서 데이터를 포함하고 있는 파일 내의 다음 위치로 조정합니다. 만약 `offset`이 데이터를 가리키고 있다면, 파일 오프셋은 `offset`으로 설정됩니다. 


* **SEEK_HOLE**: 파일 오프셋을 `offset`보다 크거나 같으면서 파일 내의 다음 홀(hole)로 조정합니다. 만약 `offset`이 홀의 중간을 가리키고 있다면, 파일 오프셋은 `offset`으로 설정됩니다. 만약 `offset` 이후에 홀이 없다면, 파일 오프셋은 파일의 끝으로 조정됩니다 (즉, 모든 파일의 끝에는 암시적인 홀이 존재합니다). 



위의 두 경우 모두, `offset`이 파일의 끝 너머를 가리키면 `lseek()`는 실패합니다. 이러한 작업들은 애플리케이션이 희소하게 할당된 파일(sparsely allocated file) 내의 홀들을 매핑할 수 있게 해줍니다. 이것은 백업을 생성할 때 공간을 절약하고 홀을 발견할 메커니즘이 있다면 홀을 보존할 수 있는 파일 백업 도구와 같은 애플리케이션에 유용할 수 있습니다. 

이러한 작업의 목적을 위해, 홀(hole)은 하부 파일 저장소에 (통상적으로) 할당되지 않은 일련의 0(zeros)입니다. 그러나 파일 시스템은 홀을 보고할 의무가 없으므로, 이러한 작업들이 파일에 실제로 할당된 저장 공간을 매핑하기 위한 보장된 메커니즘은 아닙니다.  (더욱이, 하부 저장소에 실제로 기록된 일련의 0은 홀로 보고되지 않을 수 있습니다.) 가장 단순한 구현에서, 파일 시스템은 `SEEK_HOLE`이 항상 파일 끝의 오프셋을 반환하게 하고, `SEEK_DATA`가 항상 `offset`을 반환하게 함으로써 (즉, `offset`이 참조하는 위치가 홀일지라도, 그것이 0의 시퀀스인 데이터로 구성된 것으로 간주될 수 있음) 이 작업들을 지원할 수 있습니다. 

`<unistd.h>`로부터 `SEEK_DATA` 및 `SEEK_HOLE`의 정의를 얻기 위해서는 `_GNU_SOURCE` 기능 테스트 매크로가 정의되어야 합니다. 

`SEEK_HOLE` 및 `SEEK_DATA` 작업은 다음 파일 시스템들에서 지원됩니다: 

* trfs (리눅스 3.1부터) 


* CFS (리눅스 3.2부터) 


* FS (리눅스 3.5부터) 


* xt4 (리눅스 3.8부터) 


* mpfs(5) (리눅스 3.8부터) 


* FS (리눅스 3.18부터) 


* USE (리눅스 4.5부터) 


* FS2 (리눅스 4.15부터) 



### 반환 값 (RETURN VALUE)

성공적인 완료 시, `lseek()`는 파일의 시작부터 바이트 단위로 측정된 결과 오프셋 위치를 반환합니다. 에러 발생 시, 값 `(off_t) -1`이 반환되고 에러를 나타내도록 `errno`가 설정됩니다. 

### 에러 (ERRORS)

* **EBADF**: `fd`가 열린 파일 기술자가 아닙니다. 


* *EINVAL**: `whence`가 유효하지 않습니다. 또는: 결과 파일 오프셋이 음수가 되거나, 탐색 가능한 장치의 끝 범위를 벗어납니다. 


* *ENXIO**: `whence`가 `SEEK_DATA` 또는 `SEEK_HOLE`이고 `offset`이 파일의 끝 너머입니다. 또는 `whence`가 `SEEK_DATA`이고 `offset`이 파일 끝의 홀 내부에 있습니다. 


* **EOVERFLOW**: 결과 파일 오프셋을 `off_t`로 표현할 수 없습니다. 


* **ESPIPE**: `fd`가 파이프, 소켓, 또는 FIFO와 연관되어 있습니다. 



### 준수 (CONFORMING TO)

POSIX.1-2001, POSIX.1-2008, SVr4, 4.3BSD. `SEEK_DATA` 및 `SEEK_HOLE`은 비표준 확장이며 Solaris, FreeBSD, DragonFly BSD에도 존재합니다. 이들은 다음 POSIX 개정판(Issue 8)에 포함되도록 제안되었습니다. 

### 참고 사항 (NOTES)

파일 기술자, 열린 파일 기술(open file descriptions), 그리고 파일 사이의 관계에 대한 논의는 `open(2)`을 참조하십시오. 

만약 열린 파일 기술에 `O_APPEND` 파일 상태 플래그가 설정되어 있다면, `write(2)`는 `lseek()`의 사용과 관계없이 항상 파일 오프셋을 파일의 끝으로 이동시킵니다. 

`off_t` 데이터 타입은 POSIX.1에 의해 지정된 부호 있는 정수 데이터 타입입니다. 

일부 장치들은 탐색이 불가능하며 POSIX는 어떤 장치가 `lseek()`를 지원해야 하는지 명시하지 않습니다. 리눅스에서 터미널 장치에 `lseek()`를 사용하는 것은 `ESPIPE` 에러와 함께 실패합니다. 

### 참고 항목 (SEE ALSO)

`dup(2)`, `fallocate(2)`, `fork(2)`, `open(2)`, `fseek(3)`, `lseek64(3)`, `posix_fallocate(3)` 

### 콜로폰 (COLOPHON)

이 페이지는 리눅스 매뉴얼 페이지(man-pages) 프로젝트의 5.10 릴리스의 일부입니다. 프로젝트에 대한 설명, 버그 보고에 대한 정보, 그리고 이 페이지의 최신 버전은 [https://www.kernel.org/doc/man-pages/](https://www.kernel.org/doc/man-pages/) 에서 찾을 수 있습니다. 

리눅스 2020-08-13 LSEEK(2) 

---

**다른 시스템 호출이나 추가 번역이 필요하시면 말씀해 주세요.**