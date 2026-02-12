ARCHITECTURE

[GOAL] 
woody_woodpacker : Linux x_86_32 /x_86_64 대상 ELF(32/64) 포멧의 복사본의 세그먼트를 xor 암호화를 진행한다.
woody : ELF 포멧을 암호화한 woody_woodpacker 의 생산물이다. 실행시 자동으로 복호화 한다.
안전성 최우선 : 모든 메모리 점근은 범위 검사 후 수행한다.

[P0:MUST] : If not implemented, it is considered a failure.
1. 포맷 검출 게이트
    - 입력 path 는 ELF 실행 파일이다.
	- KEY는 존재 할 수 도 있고 없을 수 도 있다.
	- KEY가 없다면 랜덤으로 생성한다.
    - ELF magic(ELFMAG)는 반드시 검사한다.
2. base/limit 정의
    - base는 "현재 해석 단위의 시작 주소"
    - 일반 ELF: mmap 시작 주소
    - limit 은 해당 단위의 유효 크기 (일반 파일 크기 또는 member payload 크기)
3. 범위검사 강제
    - 모든 접근은 CHECK_RANGE(offset, size, limit)로만 허용
    - 검사 형태 : offset <= limit 먼저 , 다음 size + offset <= limit
    - MOVE_ADDRESS(base, offset)는 범위검사 없이 주소만 이동 (호출 전 검사 필수)
4. ELF 필수 검증
    - EI_DATA : ELFDATA2LSB 만
    - EI_CALSS : ELFCLASS32 , ELFCLASS64 만
    - e_type : ET_EXEC, ET_DYN
    - e_machine : EM_386, EM_X86_64
	- program header table : e_phoff != 0, e_phnum != 0, e_phentsize 검증, 그리고 e_phoff + e_phnum*e_phentsize 범위검사
	- program header table은 0 <= phdr index < e_phnum 를 지켜야한다.
5. 프로그램 종료 정책
    - 시스템 오류(malloc 등)이 아니면 프로그램을 종료하지 않는다.
    - 알 수 없는 옵션은 fatal(즉시 실패 처리)
6. 리소스 회수 정책
    - 분기 / 에러 로 다음 단계로 넘어갈 때, 그 분기에서 얻은 자원은 반드시 반납:
        - malloc/free , open/close, mmap/munmap
    - free 후 NULL guard로 안정성 강화(가능한 범위에서)
7. bit Type 캐스팅 정책
    - 32 bit 의 아키텍쳐에서는 데이터 추출 후 64 bit 의 format 와 같이 upscale casted 를 사용해 보관
8. SHELL CODE 정책
	- Implementaion_diagram.md 의 [SHELLCODE] 의 복호화 코드를 사용한다.

[IMPLEMENTATION:CONSTRAINTS]
    - 사용 라이브러리 : libft, ft_printf 해당 디렉토리 수정 금지
    - 허용함수 :  open / close / mmap / munmap / write / read / malloc / free / exit / perror / strerror / fputs / fflush / lseek / mprotect / printf series
    - 작업 디렉토리 : /home/jinseo/jinseo/42_gs/packer/src , /home/jinseo/jinseo/42_gs/packer/include
	- header path : #include "../include/woody.h"
    - 주석 작성 금지
    - 실행 검증(런타임 테스트)은 하지 않음(정적 검증/명세 준수로 확인)