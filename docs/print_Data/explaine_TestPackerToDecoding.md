# testPackerToDecoding.s 학습용 상세 해설

대상 코드: `TestPage/testPackerToDecoding.s`  
(현재 설명 문서는 위 파일 기준으로 작성)

---

## 1) 이 스텁이 하는 일 한 줄 요약

packer가 **파일(디스크)에서 암호화**해 둔 ELF 세그먼트를, 실행 시점에 스텁이 **메모리에서 복호화**하고 원래 엔트리(OEP)로 점프한다.

---

## 2) 큰 그림 (파일 vs 메모리)

```text
[디스크의 ELF 파일]
  packer가 PT_LOAD 내용 일부/전체를 XOR 암호화
  (ELF header, PHDR는 보통 유지)

          실행
            |
            v
[메모리에 로드된 ELF 이미지]
  _TestPackerToDecoding 시작
    1) PHDR 순회
    2) 복호화 대상 PT_LOAD 선택
    3) mprotect로 쓰기 가능하게 변경
    4) XOR 복호화 (8바이트 + tail)
    5) 메타/키 흔적 일부 지우기
    6) OEP로 jmp
```

핵심 포인트:
- packer는 **물리적 오프셋(p_offset)** 기준으로 암호화한다.
- stub은 **논리적 가상주소(p_vaddr)** 기준으로 복호화한다.
- 이 둘이 서로 정확히 대응해야 실행이 살아난다.

---

## 3) packed_info(패치 데이터) 의미

코드 하단의 `packed_info`는 packer가 채워 넣는 메타 영역이다.

```asm
packed_magic              dq 0x57504445434F4445 ; placeholder (packer가 0x00000000434F4445로 패치)
packed_target_phdr_index  dq ... ; 복호화할 PT_LOAD의 PHDR index (packer 패치)
packed_reserved           dq ... ; 현재는 미사용(확장용)
packed_xor_key            dq ... ; XOR 키(8바이트, packer 패치)
original_entry_delta      dq ... ; OEP - anchor (packer 패치)
```

왜 `original_entry_delta`를 쓰는가?
- PIE/ASLR 환경에서도 `anchor`는 RIP-relative로 구할 수 있다.
- 그래서 `anchor + delta` 방식이면 실제 런타임 OEP를 안정적으로 계산 가능하다.

왜 `packed_target_phdr_index`인가?
- PIE(ET_DYN)는 로드 베이스가 실행 때마다 달라서 packer가 **런타임 절대 주소(예: AT_PHDR 포인터)**를 미리 박아 넣을 수 없다.
- 그래서 packer는 “몇 번째 PHDR를 복호화할지(인덱스)”만 넣고, 스텁은 런타임에 auxv에서 AT_PHDR/AT_PHNUM을 찾아 실제 주소를 계산한다.

업데이트된 현재 버전의 포인트:
- 지금 스텁은 `packed_target_phdr_index`를 더 이상 사용하지 않는다. (호환을 위해 packed_info에 남겨둔 자리)\n+- 대신 PHDR를 전부 순회하면서 “암호화했을 법한 세그먼트”를 스스로 판단해 복호화한다.

---

## 4) 코드 흐름 ASCII

```text
[_TestPackerToDecoding]
    |
    v
[push들로 레지스터 보존]
    |
    v
[r14 = &packed_info]
    |
    v
[magic 검사]
    | fail -------------------------------> [레지스터 복원 -> OEP jump]
    |
    v
[xor_key, target_phdr_index 로드]
    |
    v
[초기 스택에서 auxv 찾기]
  - AT_PHDR (PHDR 테이블 런타임 주소)
  - AT_PHNUM (PHDR 개수)
  - AT_PAGESZ (페이지 크기)
    |
    v
[PT_PHDR 찾아서 base 계산]
  base = AT_PHDR - p_vaddr(PT_PHDR)
    |
    v
[target PHDR 포인터 계산]
  target = AT_PHDR + target_index * sizeof(Elf64_Phdr)
    |
    v
[p_type==PT_LOAD 확인 + p_offset==0이면 중단]
    |
    v
[runtime 세그먼트 주소 계산]
  runtime_addr = base + p_vaddr
    |
    v
[mprotect 범위 계산]
  align = max(p_align, pagesz)
    |
    v
[mprotect(PROT_RWX)]
    |
    v
[XOR decrypt]
  - qword(8byte) 반복
  - 남은 tail 바이트 처리
    |
    v
[packed_info 페이지 mprotect 후 wipe]
    |
    v
[레지스터 pop 복원]
    |
    v
[anchor + original_entry_delta]
    |
    v
[jmp OEP]
```

---

## 5) 블록별 상세 해설

## 5-1. 프로로그: 레지스터 보존

```asm
push rbp
mov rbp, rsp
push rax ... push r15
```

의도:
- 스텁이 중간 계산에서 레지스터를 많이 쓰므로 원래 상태를 보존한다.
- 복호화 후 OEP에 최대한 "원래 상태"로 넘기기 위함.

생각 포인트:
- 정말 모든 레지스터를 다 보존해야 할까?
- System V ABI 기준 caller-saved/callee-saved 관점에서 최소화하면 stub 크기를 줄일 수 있다.

---

## 5-2. magic 검사

```asm
mov eax, dword [r14 + (packed_magic - packed_info)]
cmp eax, 0x434F4445
jne .restore_and_jump
```

의도:
- packer가 패치했는지 빠르게 확인.
- 메타가 깨졌거나 미패치 상태면 바로 탈출.

포인트:
- x86-64에서 `cmp rax, imm64`는 직접 인코딩이 안 되고 보통 imm32 sign-extend 형태가 된다.
- 그래서 이 스텁은 의도적으로 32-bit 매직(여기서는 `"EDOC"`)만 비교한다.

---

## 5-3. auxv + base + target PHDR

```asm
; 초기 스택에서 auxv를 찾아 AT_PHDR / AT_PHNUM / AT_PAGESZ 읽기
; base = AT_PHDR - p_vaddr(PT_PHDR)
; target = AT_PHDR + (target_index * 56)
```

의도:
- PIE(ET_DYN)까지 지원하기 위해 “런타임 주소(베이스)”를 스텁이 직접 계산한다.
- packer는 절대주소를 모르는 대신 **PHDR index**만 패치한다.

생각 포인트:
- auxv 파싱이 왜 가능할까? `_start` 시점 스택 레이아웃( argc/argv/envp/auxv )을 직접 이용하기 때문이다.
- `AT_PHDR`가 주는 값은 “PHDR 테이블의 런타임 절대주소”다. 이게 PIE에서 제일 중요한 힌트다.
- base 계산을 `PT_PHDR`로 하는 게 항상 안전할까? 못 찾으면 어떤 fallback이 필요할까?

---

## 5-4. mprotect 범위 계산

코드가 하는 계산:

```text
pagesz = AT_PAGESZ (없으면 0x1000)
align  = max(p_align, pagesz)
start  = align_down(runtime_addr, align)
end    = align_up(runtime_addr + p_memsz, align)
len   = end - start
mprotect(start, len, PROT_RWX)
```

왜 필요한가?
- 텍스트 세그먼트는 보통 쓰기 금지(R-X)라서 복호화 쓰기 시도 시 SIGSEGV가 난다.
- 먼저 쓰기 권한을 열어야 메모리 수정 가능.

생각 포인트:
- `p_align`이 항상 page size와 같을까?
- Linux `mprotect`는 page 단위 정렬을 요구한다. `p_align`을 그대로 쓰는 방식이 모든 ELF에서 안전한지 검토해볼 것.

---

## 5-5. XOR 복호화 루프

```asm
; rcx = remaining, rdi = current ptr
; 8바이트씩 xor
; 남은 <8 바이트는 1바이트씩 xor
```

의도:
- 속도/단순성 균형: qword 루프 + tail 루프.

생각 포인트:
- 이 더미는 “타겟 세그먼트 1개만” 복호화하므로, 키/메타 위치를 skip할 필요가 거의 없다.
- 복호화 길이를 `p_memsz`로 잡으면 BSS 같은 영역을 건드릴 수 있다. 그래서 packer가 고른 세그먼트가 `p_memsz == p_filesz`인지가 중요해진다.

---

## 5-6. wipe와 점프

```asm
; packed_info가 있는 페이지를 mprotect로 W 허용
; (스텁은 보통 R-X 세그먼트에 들어가므로, 이 작업 없이 wipe하면 SIGSEGV가 날 수 있음)
mov rdi, r14
mov rax, rbx
dec rax
not rax
and rdi, rax
mov rsi, rbx
mov rax, SYS_MPROTECT
mov rdx, PROT_RWX
syscall

; 그리고 packed_info 일부를 0으로 wipe
lea rdi, [rel packed_info]
mov rcx, packed_wipe_end - packed_info
xor eax, eax
rep stosb
...
lea rax, [rel anchor]
add rax, [rel original_entry_delta]
jmp rax
```

의도:
- 키/메타 흔적 일부를 메모리에서 지운다.
- 마지막에 OEP로 control transfer.

중요:
- `original_entry_delta`는 wipe 범위 밖에 둬야 한다.
- 그렇지 않으면 점프 주소가 0으로 변해 즉시 크래시.

---

## 6) 너가 꼭 생각해볼 질문 (사고력 훈련)

아래 질문은 "정답 맞추기"보다 "근거 만들기"가 목표다.

1. 복호화 길이를 `p_memsz`로 할지 `p_filesz`로 할지, 네 packer 방식과 어떻게 맞출 것인가?
2. `p_offset == 0`을 “header PT_LOAD”의 기준으로 쓰는 게 충분한가?
3. mprotect 정렬 기준을 `p_align`으로 둘 때의 위험은 무엇인가?
4. XOR key를 8바이트 하나로 고정했을 때 보안상/탐지상 약점은?
5. PT_LOAD 중 어느 세그먼트를 암호화 제외해야 실행 안정성이 높아질까?
6. 복호화 이후 권한을 원래(R-X 등)로 되돌리는 것이 필요한가?
7. stub 자기 자신을 복호화 대상에서 확실히 제외하려면 어떤 정보가 추가로 필요할까?
8. PIE에서 base를 구하는 방법은 여러 가지다. auxv(AT_PHDR) 외에 어떤 방법들이 있을까?

---

## 7) 실습 체크리스트

1. packer가 patch한 값이 런타임에서 실제로 올바른지 먼저 검증
2. PHDR 덤프를 찍어 어떤 세그먼트를 복호화하는지 로그로 확인
3. 복호화 전/후 특정 바이트 패턴 비교
4. OEP 계산값(`anchor + delta`)이 기대값과 같은지 확인
5. 크래시 시점이 `mprotect`, XOR loop, OEP jump 중 어디인지 먼저 분리

---

## 8) 간단한 디버깅 전략

```text
증상: 바로 SIGSEGV
-> mprotect 인자(start/len) 먼저 의심

증상: OEP 점프 후 크래시
-> 실제 복호화 범위와 packer 암호화 범위 mismatch 의심

증상: 간헐적 성공/실패
-> ASLR/PIE에서 주소 계산 방식(특히 delta) 재검증
```

---

## 9) 이 코드의 강점과 한계

강점:
- 구조가 단순해서 학습용으로 매우 좋다.
- PHDR 기반으로 동작해 하드코딩 섹션명 의존이 적다.

한계:
- 무결성 검증(해시/체크섬)이 없다.
- 키/메타 관리가 정적이라 분석에 취약하다.
- mprotect 실패값 검사/예외처리가 없다.

---

## 10) 다음 학습 단계 제안

1. `mprotect` 반환값 검사 및 실패 fallback 추가
2. `p_filesz`/`p_memsz` 선택을 packer 정책과 일치시키기
3. 복호화 완료 후 권한 복구(`PROT_READ|PROT_EXEC`) 추가
4. key 스케줄(rolling xor 등) 실험
5. stub 영역 제외 로직을 범위 기반으로 개선

---

## 11) 샌드박스에서 실제로 검증한 방법 (packer -> objdump -> run)

목표는 2개다.

1. **파일이 진짜로 암호화되었는지** (디스크 관점)
2. **실행 시 복호화가 진짜로 되었는지** (메모리 관점)

관련 파일:
- 더미 packer: `TestPage/Test_injection_packer1_2.c`
- decode stub 원본 asm: `TestPage/testPackerToDecoding.s`

### 11-1) 테스트용 바이너리 만들기 (no-pie/PIE)

현재 더미 packer/stub은 **ET_EXEC(no-pie)** 와 **PIE(ET_DYN)** 둘 다 동작한다.
그래서 두 종류를 모두 만들어서 같은 루틴으로 검증하는 게 좋다.

```bash
cat >/tmp/hello.c << 'EOF'
#include <stdio.h>
int main(void){ puts("hello-pack"); return 0; }
EOF

gcc -no-pie /tmp/hello.c -o /tmp/hello_nopie
gcc        /tmp/hello.c -o /tmp/hello_pie

readelf -h /tmp/hello_nopie | rg "Type:|Entry point"
readelf -h /tmp/hello_pie   | rg "Type:|Entry point"
```

### 11-2) packer로 woody 만들기

```bash
gcc -Wall -Wextra -Werror TestPage/Test_injection_packer1_2.c -o /tmp/packer_dummy

/tmp/packer_dummy 0x1122334455667788 /tmp/hello_nopie
readelf -h woody | rg "Type:|Entry point"
./woody

/tmp/packer_dummy 0x1122334455667788 /tmp/hello_pie
readelf -h woody | rg "Type:|Entry point"
./woody
```

샌드박스 관찰:
- `Entry point`가 “원래 엔트리”에서 “스텁이 들어간 위치”로 바뀐다.
- ET_EXEC 예: `0x401050` -> `0x401170`
- PIE(ET_DYN) 예: `0x1060` -> `0x1170`

### 11-3) objdump로 “암호화 됐는지” 확인

이 더미에서는 (안전하게 이해하기 위해) 보통 `.rodata`에 매핑되는 `PT_LOAD` 하나를 골라 XOR 암호화를 한다.

```bash
objdump -s -j .rodata /tmp/hello_nopie | sed -n '1,40p'
objdump -s -j .rodata woody        | sed -n '1,40p'
```

샌드박스 관찰(실제 확인됨):
- 원본 `.rodata`에는 `hello-pack`이 평문으로 보인다.
- `woody`의 `.rodata`는 같은 위치가 의미 없는 바이트처럼 보인다(즉, 파일이 실제로 바뀜).

보조 확인(`strings`):
```bash
strings /tmp/hello_nopie | rg "hello-pack"
strings woody          | rg "hello-pack" || true
```

### 11-4) 실행해서 “복호화 됐는지” 확인

```bash
./woody
```

샌드박스 관찰(실제 확인됨):
- `strings woody`에는 안 보이던 `hello-pack`이 **실행 결과로는 정상 출력**된다.

---

## 12) 지금 더미 구현에서 꼭 생각해야 하는 포인트 (실제 겪은 이슈 포함)

### 12-1) wipe(rep stosb)와 메모리 권한

스텁은 마지막에 `packed_info`를 0으로 지우는 코드가 있다. 그런데 스텁이 놓인 페이지가 쓰기 불가면 여기서 SIGSEGV가 난다.

이번 버전의 스텁은 wipe 직전에 `packed_info`가 있는 페이지를 `mprotect`로 W 가능하게 바꾼 뒤 wipe를 수행한다(그래서 wipe가 켜져 있어도 크래시가 안 난다).

네가 생각해볼 것:
- wipe를 살리려면 무엇을 해야 하나?
  - 스텁이 존재하는 페이지도 `mprotect`로 쓰기 가능하게 열기
  - 또는 스텁/메타를 원래부터 쓰기 가능한 세그먼트에 두기(단, 설계 난이도 증가)

### 12-2) ET_EXEC vs PIE(ET_DYN)

PIE는 로드 베이스가 실행 때마다 달라진다. 그래서 packer가 “런타임 절대주소”를 미리 패치하는 방식은 깨지기 쉽다.

이번 버전은 auxv에서 `AT_PHDR`/`AT_PHNUM`/`AT_PAGESZ`를 읽어서 PHDR의 런타임 주소와 페이지 크기를 얻고, `PT_PHDR`의 `p_vaddr`로 base를 계산해서 PIE에서도 동작한다.

샌드박스에서는 PIE 바이너리(예: `/bin/ls`)도 패킹 후 실행까지 확인했다.

네가 생각해볼 것:
- PIE를 지원하려면 “주소 계산”에서 무엇이 달라져야 하나?
- `anchor + delta` 아이디어가 어디까지는 도움이 되고, 어디부터는 추가 정보가 필요할까?

### 12-3) p_filesz vs p_memsz 정책 일치

파일 암호화는 `p_filesz` 기반이 자연스럽고, 메모리 복호화는 `p_memsz`를 쓰면 BSS 같은 영역을 건드릴 위험이 있다.

네가 생각해볼 것:
- 네 packer가 실제로 암호화한 길이와, 스텁이 복호화하는 길이가 반드시 1:1로 맞아야 한다.

### 12-4) “헤더/PHDR/스텁만 제외하고 전부 암호화”가 PIE에서 깨지는 이유

요구사항을 그대로 구현하려고 하면, 보통 `PF_W`(쓰기 가능한) 세그먼트까지 암호화 대상에 들어간다.
하지만 PIE/동적 링크 환경에서는 **엔트리(우리 스텁)로 오기 전에** 동적 로더가 relocation을 적용하면서 `.got`, `.data.rel.ro` 같은 영역에 값을 써 넣는다.

만약 packer가 그 영역을 디스크에서 XOR로 암호화해두면,
- 로더는 “암호화된 바이트” 위에 relocation 결과를 덮어쓴다.
- 그 다음 엔트리에서 스텁이 “세그먼트 전체를 XOR 복호화”하면,
- 로더가 방금 써 준 relocation 값까지 XOR돼서 깨진다.

그래서 **PIE에서도 안정적으로 동작하려면** 최소한 다음 중 하나가 필요하다.
1. `PF_W` 세그먼트는 암/복호화 대상에서 제외한다 (현재 구현)
2. relocation이 적용된 주소들을 따로 추적해서 그 부분은 스텁에서 복호화하지 않는다
3. relocation을 스텁이 직접 다시 적용한다 (난이도 급상승)

현재 코드는 공부/가독성/동작 안정성을 위해 1)로 구현되어 있다.
---

## 13) 추가로 공부하면 좋은 실험 과제

1. “복호화 타겟을 1개가 아니라 여러 개”로 늘리기: `packed_reserved`를 의미 있는 값(예: 개수/비트마스크/시작 index)로 바꾸고 정책을 정교화
2. 복호화 후 권한 복구(R-X / RW-): 실행 안정성과 탐지/보안 관점 비교
3. `mprotect` 실패 처리: 실패 시 어디서 죽는지, 어떻게 fail-safe로 빠질지
4. 어떤 세그먼트를 암호화하면 즉시 깨지는지 실험: `.dynamic`, GOT/PLT, reloc 관련 영역

---

## 부록 A) 빠른 용어 정리

- OEP: Original Entry Point, 원래 프로그램 시작 주소
- PHDR: Program Header Table, 로더 관점 세그먼트 정보
- PIE: Position Independent Executable, 보통 ELF `e_type == ET_DYN`
- auxv: 초기 스택에 있는 auxiliary vector (AT_* 값들)
- `AT_PHDR`: 프로그램 헤더 테이블의 런타임 주소
- `AT_PHNUM`: 프로그램 헤더 개수
- `AT_PAGESZ`: 페이지 크기
- `p_offset`: 파일 내 물리 오프셋
- `p_vaddr`: 프로세스 메모리 가상주소
- `p_filesz`: 파일에 존재하는 세그먼트 크기
- `p_memsz`: 메모리에서의 세그먼트 크기(BSS 포함 가능)

---

## 부록 B) 짧은 자기점검

아래를 본인 말로 설명할 수 있으면 이해가 깊어진 상태다.

1. "왜 packer는 p_offset을 보고, stub은 p_vaddr을 보나?"
2. "왜 mprotect 정렬이 필요한가?"
3. "왜 delta jump가 PIE에 유리한가?"
4. "왜 복호화 후 바로 ret이 아니라 jmp를 쓰는가?"
