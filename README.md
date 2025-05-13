# NFA to DFA 변환기

## 개요
이 프로그램은 ε-NFA(엡실론 전이를 포함한 비결정적 유한 오토마타)를 DFA(결정적 유한 오토마타)로 변환한 후, Hopcroft 알고리즘을 사용하여 DFA를 최소화합니다. 상태 이름은 `qXXX` 형식(3자리 0으로 패딩된 숫자)으로 생성됩니다.

## 사용법
```sh
./nfa_to_dfa <inputfile.txt>
```

## 입력 형식
입력 파일은 다음과 같은 형식으로 ε-NFA를 설명해야 합니다:
- **StateSet**: 중괄호로 둘러싸인 쉼표로 구분된 상태 목록입니다.
- **TerminalSet**: 중괄호로 둘러싸인 쉼표로 구분된 터미널 심볼 목록입니다(ε 제외).
- **DeltaFunctions**: `(source_state, symbol) = {target_states}` 형식의 전이 함수 집합입니다. 각 전이는 쉼표와 개행으로 구분됩니다.
- **StartState**: NFA의 초기 상태입니다.
- **FinalStateSet**: 중괄호로 둘러싸인 쉼표로 구분된 최종 상태 목록입니다.

예시:
```
StateSet = { q000, q001, q002, q003 }
TerminalSet = { a, b }
DeltaFunctions = {
  (q000, a) = {q001},
  (q001, b) = {q002},
  (q002, ε) = {q003}
}
StartState = q000
FinalStateSet = { q003 }
```

## 출력 파일
프로그램은 세 개의 출력 파일을 생성합니다:
1. `<inputfile>_nfa.txt`: 원본 ε-NFA입니다.
2. `<inputfile>_dfa.txt`: 변환된 DFA입니다.
3. `<inputfile>_reduced_dfa.txt`: 최소화된 DFA입니다.

## 주요 기능
- **ε-Closure 및 부분집합 구성**: 상태의 ε-Closure를 계산하고 부분집합 구성을 적용하여 ε-NFA를 DFA로 변환합니다.
- **Hopcroft 알고리즘**: DFA를 최소화하여 오토마타가 인식하는 언어를 유지하면서 상태 수를 줄입니다.
- **일관된 상태 명명**: 상태 이름을 `qXXX` 형식으로 생성하여 고유성과 가독성을 보장합니다.

## 클래스 상세 설명
- **NFA 클래스**: ε-NFA를 표현합니다.
  - `states`: 상태 집합을 저장하는 vector<string>입니다.
  - `alphabet`: 입력 알파벳(터미널 심볼)을 저장하는 vector<char>입니다.
  - `transitions`: 일반 전이 함수를 저장하는 중첩된 unordered_map입니다.
  - `epsilonTransitions`: ε-전이를 저장하는 unordered_map입니다.
  - `startState`: 시작 상태를 저장하는 string입니다.
  - `finalStates`: 최종 상태 집합을 저장하는 vector<string>입니다.

- **DFA 클래스**: NFA를 변환한 DFA를 표현합니다.
  - `states`: DFA의 상태 집합을 저장합니다.
  - `alphabet`: 입력 알파벳(ε 제외)을 저장합니다.
  - `transitions`: DFA의 전이 함수를 저장하는 중첩된 unordered_map입니다.
  - `startState`: DFA의 시작 상태입니다.
  - `finalStates`: DFA의 최종 상태 집합입니다.

- **ReducedDFA 클래스**: DFA를 최소화한 결과를 표현합니다.
  - 구조는 DFA와 유사하지만, 최소화 과정을 거쳐 상태 수가 줄어든 DFA입니다.

## 변환 원리 상세 설명
1. **ε-Closure 계산**: 각 상태에서 ε-전이를 통해 도달할 수 있는 모든 상태를 계산합니다.
2. **부분집합 구성**: ε-Closure를 기반으로 DFA의 상태를 구성합니다. 각 DFA 상태는 NFA 상태의 부분집합입니다.
3. **전이 함수 구성**: DFA의 전이 함수는 NFA의 전이 함수를 기반으로 구성됩니다.
4. **Hopcroft 알고리즘**: DFA를 최소화하여 동일한 언어를 인식하는 최소 상태 수의 DFA를 생성합니다.

## 요구사항
- C++17 이상
- GCC 또는 호환되는 컴파일러

## 빌드 방법
```sh
make clean && make
```

## 라이선스
이 프로젝트는 MIT 라이선스 하에 오픈 소스로 제공됩니다. 