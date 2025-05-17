# (ε-)NFA → DFA → Reduced DFA 변환기

## 개요
이 프로그램은 (ε-)NFA(엡실론 전이 포함 비결정적 유한 오토마타)를 입력받아, 다음 세 단계를 자동으로 수행합니다.
1. (ε-)NFA → DFA 변환 (부분집합 구성법, ε-closure 포함)
2. DFA → Reduced DFA(최소 DFA) 변환 (Hopcroft 알고리즘)
3. 각 단계별 결과를 지정된 포맷의 텍스트 파일로 출력

상태 이름은 항상 `qXXX`(3자리 0패딩) 형식으로 생성됩니다.

---

## 빌드 및 실행 방법

### 1. 리눅스/유닉스(macOS 포함) 환경
```sh
make clean && make
./nfa_to_dfa <inputfile.txt>
```

### 2. Windows 환경 (MinGW 사용)
1. [MinGW-w64](https://www.mingw-w64.org/)를 설치합니다.
2. 명령 프롬프트(cmd) 또는 PowerShell에서 아래 명령어를 실행합니다:
```sh
g++ -std=c++17 -o nfa_to_dfa.exe main.cpp
```
3. 실행:
```sh
nfa_to_dfa.exe <inputfile.txt>
```
- `<inputfile.txt>`: (ε-)NFA가 정의된 입력 파일

> **참고:**
> - Windows에서는 `make` 대신 위와 같이 직접 g++ 명령어를 사용하세요.
> - 입력/출력 파일 인코딩은 UTF-8을 권장합니다.

### 3. 출력
- `<inputfile>_nfa.txt`: 입력 NFA를 재정렬하여 출력
- `<inputfile>_dfa.txt`: 변환된 DFA
- `<inputfile>_reduced_dfa.txt`: 최소화된 DFA

---

## 입력 파일 포맷
아래 5가지 항목이 반드시 순서대로 포함되어야 합니다.

```
StateSet = { q000, q001, q002, ... }
TerminalSet = { a, b, ... }
DeltaFunctions = {
  (q000, a) = {q000, q001}
  (q000, b) = {q000}
  (q001, a) = {q002}
  (q001, b) = {q002}
  (q002, a) = {q003}
  (q002, b) = {q003}
}
StartState = q000
FinalStateSet = { q003 }
```
- 상태는 반드시 `q`+3자리 숫자
- 터미널 심볼: a ~ z, A ~ Z, 0~9
- 입실론 심볼: `ε`
- 전이함수는 `(상태, 심볼) = {상태, ...}`

---

## 출력 파일 예시
- 각 단계별로 위와 동일한 포맷으로 출력됩니다.
- DFA, Reduced DFA에는 상태 병합/최소화 결과가 반영됩니다.
- 각 DFA/Reduced DFA 상태가 어떤 NFA/DFA 상태 집합에서 유도됐는지 주석으로 함께 출력됩니다.

---

## 주요 기능 및 구현 특징
- **ε-Closure 및 부분집합 구성법**: ε-closure를 이용해 NFA를 DFA로 변환
- **Hopcroft 알고리즘**: DFA를 최소화
- **접근 불가능한 상태 자동 제거**
- **상태 추적 주석**: DFA/Reduced DFA 상태가 어떤 NFA/DFA 집합에서 유도됐는지 파일에 주석으로 기록
- **모든 주석 및 코드 한글화**

---

## 클래스 구조 요약
- **NFA**: ε-NFA 표현, 파일 입출력, DFA 변환
- **DFA**: DFA 표현, 접근 불가능 상태 제거, 파일 입출력, 최소화
- **ReducedDFA**: 최소화 DFA 표현, 파일 입출력

---

## 테스트 예시
`test/` 폴더에 다양한 예제 입력 파일이 있습니다.

예시 입력(`test/ex3.txt`):
```
StateSet = { q000, q001, q002, q003 }
TerminalSet = { a, b }
DeltaFunctions = {
(q000, a) = {q000, q001}
(q000, b) = {q000}
(q001, a) = {q002}
(q001, b) = {q002}
(q002, a) = {q003}
(q002, b) = {q003}
}
StartState = q000
FinalStateSet = { q003 }
```

실행:
```sh
./nfa_to_dfa test/ex3.txt
```

출력 파일(`test/ex3_dfa.txt`, `test/ex3_reduced_dfa.txt`)에서 상태 병합 및 최소화 결과를 확인할 수 있습니다.

---

## 요구사항
- C++17 이상
- GCC 또는 호환 컴파일러
- (Windows: MinGW-w64 권장)


