#include <bits/stdc++.h>
using namespace std;
static string EPSILON_SYMBOL = "ε";
static int globalNextStateId = 0;

// 상태 이름을 qXXX(3자리) 형식으로 생성하는 함수
string generateStateName() {
    char buf[10];
    sprintf(buf, "q%03d", globalNextStateId++);
    return string(buf);
}

// 클래스를 위해 앞에서 미리 선언
class DFA;
class ReducedDFA;

class NFA {
public:
    vector<string> states; // 상태 집합
    vector<char> alphabet; // 입력 알파벳
    unordered_map<string, unordered_map<char, set<string>>> transitions; // 상태 전이 함수
    unordered_map<string, set<string>> epsilonTransitions; // 입실론 전이 함수
    string startState; // 시작 상태
    vector<string> finalStates; // 종료 상태 집합

    // NFA 정보를 파일에서 읽어오는 함수
    bool loadFromFile(const string& filename) {
        // NFA 관련 컨테이너 초기화
        states.clear();
        alphabet.clear();
        transitions.clear();
        epsilonTransitions.clear();
        finalStates.clear();
        ifstream fin(filename);
        if (!fin.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            return false;
        }
        string line;
        string deltaContent = "";
        bool readingDelta = false;
        int braceCount = 0;
        while (getline(fin, line)) {
            if (readingDelta) {
                // 여러 줄에 걸친 DeltaFunctions 계속 읽기
                for (char ch : line) {
                    if (ch == '{') braceCount++;
                    if (ch == '}') braceCount--;
                    deltaContent.push_back(ch);
                    if (braceCount == 0) {
                        readingDelta = false;
                        break;
                    }
                }
                if (readingDelta) deltaContent.push_back(' ');
                continue;
            }
            // StateSet보다 FinalStateSet을 먼저 파싱 (이름 중복 방지)
            if (line.find("FinalStateSet") != string::npos && line.find('{') != string::npos) {
                size_t l = line.find('{'), r = line.rfind('}');
                if (l != string::npos && r != string::npos && r > l) {
                    string inside = line.substr(l+1, r - l - 1);
                    string token;
                    stringstream ss(inside);
                    while (getline(ss, token, ',')) {
                        string st = token;
                        // 공백 제거
                        size_t startpos = st.find_first_not_of(" \t");
                        size_t endpos = st.find_last_not_of(" \t");
                        if (startpos == string::npos || endpos == string::npos) continue;
                        st = st.substr(startpos, endpos - startpos + 1);
                        if (!st.empty()) {
                            finalStates.push_back(st);
                        }
                    }
                }
            } else if (line.find("StateSet") != string::npos && line.find("FinalStateSet") == string::npos && line.find('{') != string::npos) {
                size_t l = line.find('{'), r = line.rfind('}');
                if (l != string::npos && r != string::npos && r > l) {
                    string inside = line.substr(l+1, r - l - 1);
                    string token;
                    stringstream ss(inside);
                    while (getline(ss, token, ',')) {
                        string state = token;
                        size_t startpos = state.find_first_not_of(" \t");
                        size_t endpos = state.find_last_not_of(" \t");
                        if (startpos == string::npos || endpos == string::npos) continue;
                        state = state.substr(startpos, endpos - startpos + 1);
                        if (!state.empty()) {
                            states.push_back(state);
                        }
                    }
                }
            } else if (line.find("TerminalSet") != string::npos && line.find('{') != string::npos) {
                size_t l = line.find('{'), r = line.rfind('}');
                if (l != string::npos && r != string::npos && r > l) {
                    string inside = line.substr(l+1, r - l - 1);
                    string token;
                    stringstream ss(inside);
                    while (getline(ss, token, ',')) {
                        string sym = token;
                        size_t startpos = sym.find_first_not_of(" \t");
                        size_t endpos = sym.find_last_not_of(" \t");
                        if (startpos == string::npos || endpos == string::npos) continue;
                        sym = sym.substr(startpos, endpos - startpos + 1);
                        if (!sym.empty()) {
                            if (sym == EPSILON_SYMBOL) continue; // ε는 알파벳에 포함하지 않음
                            if (sym.size() == 1) alphabet.push_back(sym[0]);
                        }
                    }
                }
            } else if (line.find("StartState") != string::npos && line.find('=') != string::npos) {
                size_t eq = line.find('=');
                if (eq != string::npos) {
                    string value = line.substr(eq+1);
                    size_t startpos = value.find_first_not_of(" \t");
                    size_t endpos = value.find_last_not_of(" \t");
                    if (startpos != string::npos && endpos != string::npos) {
                        startState = value.substr(startpos, endpos - startpos + 1);
                    }
                }
            } else if (line.find("DeltaFunctions") != string::npos && line.find('{') != string::npos) {
                // DeltaFunctions(상태전이함수) 여러 줄 읽기 시작
                size_t pos = line.find('{');
                braceCount = 0;
                for (size_t j = pos; j < line.size(); ++j) {
                    char ch = line[j];
                    if (ch == '{') braceCount++;
                    if (ch == '}') braceCount--;
                    deltaContent.push_back(ch);
                    if (braceCount == 0) break;
                }
                if (braceCount != 0) {
                    readingDelta = true;
                    deltaContent.push_back(' ');
                }
            }
        }
        fin.close();
        // DeltaFunctions 내용 파싱
        if (!deltaContent.empty()) {
            if (deltaContent.front() == '{') deltaContent.erase(deltaContent.begin());
            if (!deltaContent.empty() && deltaContent.back() == '}') deltaContent.pop_back();
            string inside = deltaContent;
            size_t i = 0;
            while (i < inside.size()) {
                // 엔트리 사이의 공백 및 콤마 건너뛰기
                while (i < inside.size() && (isspace((unsigned char)inside[i]) || inside[i] == ',')) i++;
                if (i >= inside.size()) break;
                if (inside[i] != '(') { i++; continue; }
                i++; // '(' 건너뛰기
                // 출발 상태 이름 파싱
                string srcState;
                while (i < inside.size() && inside[i] != ',') {
                    if (!isspace((unsigned char)inside[i])) srcState.push_back(inside[i]);
                    i++;
                }
                if (i < inside.size() && inside[i] == ',') i++;
                while (i < inside.size() && isspace((unsigned char)inside[i])) i++;
                // 입력 심볼 파싱
                string symStr;
                while (i < inside.size() && inside[i] != ')') {
                    if (!isspace((unsigned char)inside[i])) symStr.push_back(inside[i]);
                    i++;
                }
                if (i < inside.size() && inside[i] == ')') i++;
                // " = " 건너뛰기
                while (i < inside.size() && (isspace((unsigned char)inside[i]) || inside[i] == '=')) {
                    if (inside[i] == '{') break;
                    i++;
                }
                if (i < inside.size() && inside[i] == '=') i++;
                while (i < inside.size() && isspace((unsigned char)inside[i])) i++;
                if (i < inside.size() && inside[i] == '{') i++;
                // 도착 상태 집합 파싱
                set<string> tgtSet;
                string tgt;
                while (i < inside.size() && inside[i] != '}') {
                    if (inside[i] == ',') {
                        // 하나의 도착 상태 끝
                        string ttrim = tgt;
                        size_t sp = ttrim.find_first_not_of(" \t");
                        size_t ep = ttrim.find_last_not_of(" \t");
                        if (sp != string::npos && ep != string::npos) {
                            ttrim = ttrim.substr(sp, ep - sp + 1);
                        }
                        if (!ttrim.empty()) tgtSet.insert(ttrim);
                        tgt.clear();
                        i++;
                        continue;
                    }
                    tgt.push_back(inside[i]);
                    i++;
                }
                if (!tgt.empty()) {
                    string ttrim = tgt;
                    size_t sp = ttrim.find_first_not_of(" \t");
                    size_t ep = ttrim.find_last_not_of(" \t");
                    if (sp != string::npos && ep != string::npos) {
                        ttrim = ttrim.substr(sp, ep - sp + 1);
                    }
                    if (!ttrim.empty()) tgtSet.insert(ttrim);
                    tgt.clear();
                }
                if (i < inside.size() && inside[i] == '}') i++;
                // 전이 저장
                if (symStr == EPSILON_SYMBOL) {
                    // ε-전이
                    epsilonTransitions[srcState].insert(tgtSet.begin(), tgtSet.end());
                } else if (!symStr.empty()) {
                    char sym = symStr.size() == 1 ? symStr[0] : symStr[0]; // (여러 글자 심볼은 입력에 없음)
                    transitions[srcState][sym].insert(tgtSet.begin(), tgtSet.end());
                }
            }
        }
        return true;
    }

    // NFA 정보를 파일로 출력하는 함수
    void outputToFile(const string& filename) {
        ofstream fout(filename);
        // StateSet
        fout << "StateSet = { ";
        for (size_t i = 0; i < states.size(); ++i) {
            fout << states[i];
            if (i < states.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        // TerminalSet
        fout << "TerminalSet = { ";
        for (size_t i = 0; i < alphabet.size(); ++i) {
            fout << alphabet[i];
            if (i < alphabet.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        // DeltaFunctions
        fout << "DeltaFunctions = {\n";
        bool first = true;
        // Collect transitions (including epsilon transitions) for output
        vector< tuple<string, string, vector<string>> > deltaList;
        // regular symbol transitions
        for (auto& srcPair : transitions) {
            const string& src = srcPair.first;
            for (auto& symPair : srcPair.second) {
                char sym = symPair.first;
                string symStr(1, sym);
                vector<string> tgtList(symPair.second.begin(), symPair.second.end());
                sort(tgtList.begin(), tgtList.end());
                deltaList.emplace_back(src, symStr, tgtList);
            }
        }
        // epsilon transitions
        for (auto& srcPair : epsilonTransitions) {
            const string& src = srcPair.first;
            if (srcPair.second.empty()) continue;
            vector<string> tgtList(srcPair.second.begin(), srcPair.second.end());
            sort(tgtList.begin(), tgtList.end());
            deltaList.emplace_back(src, EPSILON_SYMBOL, tgtList);
        }
        // Sort by source state then symbol
        sort(deltaList.begin(), deltaList.end(), [](auto &a, auto &b) {
            if (get<0>(a) == get<0>(b))
                return get<1>(a) < get<1>(b);
            return get<0>(a) < get<0>(b);
        });
        // Output each transition entry
        for (size_t k = 0; k < deltaList.size(); ++k) {
            string src = get<0>(deltaList[k]);
            string sym = get<1>(deltaList[k]);
            vector<string> tgtList = get<2>(deltaList[k]);
            if (!first) fout << "\n";
            fout << "(" << src << ", " << sym << ") = {";
            for (size_t t = 0; t < tgtList.size(); ++t) {
                fout << tgtList[t];
                if (t < tgtList.size() - 1) fout << ", ";
            }
            fout << "}";
            first = false;
        }
        if (!first) fout << "\n";
        fout << "}" << endl;
        // StartState
        fout << "StartState = " << startState << endl;
        // FinalStateSet
        fout << "FinalStateSet = { ";
        vector<string> sortedFinals = finalStates;
        sort(sortedFinals.begin(), sortedFinals.end());
        for (size_t i = 0; i < sortedFinals.size(); ++i) {
            fout << sortedFinals[i];
            if (i < sortedFinals.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        fout.close();
    }

    // NFA를 DFA로 변환하는 함수 (부분집합 구성법, ε-closure 포함)
    DFA toDFA();
};

class DFA {
public:
    vector<string> states; // 상태 집합
    vector<char> alphabet; // 입력 알파벳
    unordered_map<string, unordered_map<char, string>> transitions; // 상태 전이 함수
    string startState; // 시작 상태
    vector<string> finalStates; // 종료 상태 집합
    unordered_map<string, set<string>> dfaStateToNfaSet; // DFA 상태 -> NFA 상태 집합 매핑

    // 접근 불가능한 상태를 제거하는 함수 (BFS)
    void removeUnreachableStates() {
        set<string> reachable;
        queue<string> q;
        q.push(startState);
        reachable.insert(startState);
        while (!q.empty()) {
            string cur = q.front(); q.pop();
            for (char c : alphabet) {
                if (transitions.count(cur) && transitions[cur].count(c)) {
                    string next = transitions[cur][c];
                    if (!reachable.count(next)) {
                        reachable.insert(next);
                        q.push(next);
                    }
                }
            }
        }
        // reachable 상태만 남기고 나머지는 모두 제거
        vector<string> newStates;
        for (const string& s : states) {
            if (reachable.count(s)) newStates.push_back(s);
        }
        states = newStates;
        unordered_map<string, unordered_map<char, string>> newTransitions;
        for (const string& s : states) {
            for (char c : alphabet) {
                if (transitions.count(s) && transitions[s].count(c)) {
                    string next = transitions[s][c];
                    if (reachable.count(next))
                        newTransitions[s][c] = next;
                }
            }
        }
        transitions = newTransitions;
        vector<string> newFinalStates;
        for (const string& s : finalStates) {
            if (reachable.count(s)) newFinalStates.push_back(s);
        }
        finalStates = newFinalStates;
        unordered_map<string, set<string>> newMap;
        for (const auto& p : dfaStateToNfaSet) {
            if (reachable.count(p.first)) newMap[p.first] = p.second;
        }
        dfaStateToNfaSet = newMap;
    }

    // DFA 정보를 파일로 출력하는 함수
    void outputToFile(const string& filename) {
        ofstream fout(filename);
        fout << "StateSet = { ";
        for (size_t i = 0; i < states.size(); ++i) {
            fout << states[i];
            if (i < states.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        // DFA 상태의 NFA 집합 정보 주석으로 출력
        fout << "# DFA State Origins" << endl;
        for (const auto& p : dfaStateToNfaSet) {
            fout << "# " << p.first << " = {";
            size_t cnt = 0;
            for (const auto& s : p.second) {
                fout << s;
                if (++cnt < p.second.size()) fout << ", ";
            }
            fout << "}" << endl;
        }
        fout << "TerminalSet = { ";
        for (size_t i = 0; i < alphabet.size(); ++i) {
            fout << alphabet[i];
            if (i < alphabet.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        fout << "DeltaFunctions = {\n";
        bool first = true;
        vector< tuple<string, char, string> > deltaList;
        for (auto& srcPair : transitions) {
            const string& src = srcPair.first;
            for (auto& symPair : srcPair.second) {
                char sym = symPair.first;
                string tgt = symPair.second;
                deltaList.emplace_back(src, sym, tgt);
            }
        }
        sort(deltaList.begin(), deltaList.end(), [](auto &a, auto &b) {
            if (get<0>(a) == get<0>(b))
                return get<1>(a) < get<1>(b);
            return get<0>(a) < get<0>(b);
        });
        for (auto& entry : deltaList) {
            string src = get<0>(entry);
            char sym = get<1>(entry);
            string tgt = get<2>(entry);
            if (!first) fout << "\n";
            fout << "(" << src << ", " << sym << ") = {";
            fout << tgt;
            fout << "}";
            first = false;
        }
        if (!first) fout << "\n";
        fout << "}" << endl;
        fout << "StartState = " << startState << endl;
        fout << "FinalStateSet = { ";
        vector<string> sortedFinals = finalStates;
        sort(sortedFinals.begin(), sortedFinals.end());
        for (size_t i = 0; i < sortedFinals.size(); ++i) {
            fout << sortedFinals[i];
            if (i < sortedFinals.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        fout.close();
    }

    // DFA를 최소화하는 함수 (Hopcroft 알고리즘)
    ReducedDFA minimize();
};

class ReducedDFA {
public:
    vector<string> states; // 상태 집합
    vector<char> alphabet; // 입력 알파벳
    unordered_map<string, unordered_map<char, string>> transitions; // 상태 전이 함수
    string startState; // 시작 상태
    vector<string> finalStates; // 종료 상태 집합
    unordered_map<string, set<string>> reducedStateToDfaSet; // ReducedDFA 상태 -> 원래 DFA 상태 집합
    unordered_map<string, set<string>> reducedStateToNfaSet; // ReducedDFA 상태 -> 원래 NFA 상태 집합

    // ReducedDFA 정보를 파일로 출력하는 함수
    void outputToFile(const string& filename) {
        ofstream fout(filename);
        fout << "StateSet = { ";
        for (size_t i = 0; i < states.size(); ++i) {
            fout << states[i];
            if (i < states.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        // ReducedDFA 상태의 DFA/NFA 집합 정보 주석으로 출력
        fout << "# ReducedDFA State Origins" << endl;
        for (const auto& p : reducedStateToDfaSet) {
            fout << "# " << p.first << " = DFA{";
            size_t cnt = 0;
            for (const auto& s : p.second) {
                fout << s;
                if (++cnt < p.second.size()) fout << ", ";
            }
            fout << "} NFA{";
            cnt = 0;
            for (const auto& s : reducedStateToNfaSet.at(p.first)) {
                fout << s;
                if (++cnt < reducedStateToNfaSet.at(p.first).size()) fout << ", ";
            }
            fout << "}" << endl;
        }
        fout << "TerminalSet = { ";
        for (size_t i = 0; i < alphabet.size(); ++i) {
            fout << alphabet[i];
            if (i < alphabet.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        fout << "DeltaFunctions = {\n";
        bool first = true;
        vector< tuple<string, char, string> > deltaList;
        for (auto& srcPair : transitions) {
            const string& src = srcPair.first;
            for (auto& symPair : srcPair.second) {
                char sym = symPair.first;
                string tgt = symPair.second;
                deltaList.emplace_back(src, sym, tgt);
            }
        }
        sort(deltaList.begin(), deltaList.end(), [](auto &a, auto &b) {
            if (get<0>(a) == get<0>(b))
                return get<1>(a) < get<1>(b);
            return get<0>(a) < get<0>(b);
        });
        for (auto& entry : deltaList) {
            string src = get<0>(entry);
            char sym = get<1>(entry);
            string tgt = get<2>(entry);
            if (!first) fout << "\n";
            fout << "(" << src << ", " << sym << ") = {";
            fout << tgt;
            fout << "}";
            first = false;
        }
        if (!first) fout << "\n";
        fout << "}" << endl;
        fout << "StartState = " << startState << endl;
        fout << "FinalStateSet = { ";
        vector<string> sortedFinals = finalStates;
        sort(sortedFinals.begin(), sortedFinals.end());
        for (size_t i = 0; i < sortedFinals.size(); ++i) {
            fout << sortedFinals[i];
            if (i < sortedFinals.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
        fout.close();
    }
};

// NFA를 DFA로 변환하는 함수 (부분집합 구성법, ε-closure 포함)
DFA NFA::toDFA() {
    DFA dfa;
    dfa.alphabet = alphabet;  // 입력 알파벳 복사
    // 시작 상태의 ε-closure 계산
    set<string> startSet = { startState };
    // 상태 집합의 ε-closure를 계산하는 람다 함수
    auto epsilonClosure = [&](const set<string>& stSet) {
        set<string> closure = stSet;
        stack<string> st;
        for (const string& s : stSet) st.push(s);
        while (!st.empty()) {
            string s = st.top(); st.pop();
            if (epsilonTransitions.count(s)) {
                for (const string& nxt : epsilonTransitions[s]) {
                    if (!closure.count(nxt)) {
                        closure.insert(nxt);
                        st.push(nxt);
                    }
                }
            }
        }
        return closure;
    };
    set<string> startClosure = epsilonClosure(startSet);
    // 각 NFA 상태 집합에 대해 DFA 상태 이름을 할당
    struct SetHash {
        size_t operator()(const set<string>& st) const noexcept {
            // 상태 이름들을 이어붙여서 해시값 생성
            std::string concat;
            for (auto &s : st) concat += s + ",";
            return std::hash<std::string>()(concat);
        }
    };
    struct SetEqual {
        bool operator()(const set<string>& a, const set<string>& b) const noexcept {
            return a == b;
        }
    };
    unordered_map<set<string>, string, SetHash, SetEqual> stateMap;
    // 시작 상태의 ε-closure에 이름 할당
    string startName = generateStateName();
    stateMap[startClosure] = startName;
    dfa.states.push_back(startName);
    dfa.startState = startName;
    dfa.dfaStateToNfaSet[startName] = startClosure; // 추가
    // ε-closure에 NFA 종료 상태가 포함되어 있으면 종료 상태로 지정
    bool isStartFinal = false;
    for (const string& s : startClosure) {
        if (find(finalStates.begin(), finalStates.end(), s) != finalStates.end()) {
            isStartFinal = true;
            break;
        }
    }
    if (isStartFinal) dfa.finalStates.push_back(startName);
    // 부분집합 구성법(BFS)
    queue< set<string> > q;
    q.push(startClosure);
    while (!q.empty()) {
        set<string> curSet = q.front();
        q.pop();
        string curName = stateMap[curSet];
        // 각 입력 심볼에 대해 전이 계산
        for (char c : alphabet) {
            // curSet의 각 상태에서 c로 이동
            set<string> moveResult;
            for (const string& s : curSet) {
                if (transitions.count(s) && transitions[s].count(c)) {
                    for (const string& t : transitions[s][c]) {
                        moveResult.insert(t);
                    }
                }
            }
            // 이동 결과의 ε-closure 계산
            set<string> newClosure = epsilonClosure(moveResult);
            if (newClosure.empty()) continue;
            if (!stateMap.count(newClosure)) {
                // 새로운 DFA 상태 발견
                string newName = generateStateName();
                stateMap[newClosure] = newName;
                dfa.states.push_back(newName);
                dfa.dfaStateToNfaSet[newName] = newClosure; // 추가
                // 종료 상태 여부 확인
                bool isFinal = false;
                for (const string& s : newClosure) {
                    if (find(finalStates.begin(), finalStates.end(), s) != finalStates.end()) {
                        isFinal = true;
                        break;
                    }
                }
                if (isFinal) dfa.finalStates.push_back(newName);
                q.push(newClosure);
            }
            // DFA 전이 기록
            dfa.transitions[curName][c] = stateMap[newClosure];
        }
    }
    return dfa;
}

// DFA를 최소화하는 함수 (Hopcroft 알고리즘)
ReducedDFA DFA::minimize() {
    ReducedDFA rdfa;
    rdfa.alphabet = alphabet;
    int n = states.size();
    if (n == 0) return rdfa;
    // 상태 이름을 인덱스로 매핑
    unordered_map<string,int> index;
    for (int i = 0; i < n; ++i) index[states[i]] = i;
    // 종료 상태 플래그
    vector<bool> isFinal(n, false);
    for (const string& fs : finalStates) {
        if (index.count(fs)) isFinal[index[fs]] = true;
    }
    // 파티션 초기화: 종료/비종료 상태로 분리
    set<int> finalSet, nonFinalSet;
    for (int i = 0; i < n; ++i) {
        if (isFinal[i]) finalSet.insert(i);
        else nonFinalSet.insert(i);
    }
    // 파티션 분할 및 refinement
    list< set<int> > P;
    if (!finalSet.empty()) P.push_back(finalSet);
    if (!nonFinalSet.empty()) P.push_back(nonFinalSet);
    list< list<set<int>>::iterator > W;
    if (!finalSet.empty()) W.push_back(P.begin());
    if (!nonFinalSet.empty()) {
        auto it = finalSet.empty() ? P.begin() : next(P.begin());
        W.push_back(it);
    }
    // 각 심볼별 역전이 맵 계산
    unordered_map<char, unordered_map<int, vector<int>>> revTrans;
    for (int i = 0; i < n; ++i) {
        for (char c : alphabet) {
            if (transitions.count(states[i]) && transitions[states[i]].count(c)) {
                int j = index[ transitions[states[i]][c] ];
                revTrans[c][j].push_back(i);
            }
        }
    }
    // Hopcroft 파티션 refinement 루프
    while (!W.empty()) {
        auto A_it = W.front();
        W.pop_front();
        const set<int>& A = *A_it;
        for (char c : alphabet) {
            set<int> X;
            if (revTrans.count(c)) {
                for (int t : A) {
                    if (revTrans[c].count(t)) {
                        for (int s : revTrans[c][t]) {
                            X.insert(s);
                        }
                    }
                }
            }
            if (X.empty()) continue;
            for (auto itY = P.begin(); itY != P.end(); ) {
                set<int>& Y = *itY;
                set<int> Y1, Y2;
                for (int s : Y) {
                    if (X.count(s)) Y1.insert(s);
                    else Y2.insert(s);
                }
                if (!Y1.empty() && !Y2.empty()) {
                    itY = P.erase(itY);
                    auto itY1 = P.insert(itY, Y1);
                    auto itY2 = P.insert(itY, Y2);
                    // W(작업 리스트) 업데이트
                    if (Y.size() < INT_MAX) {
                        bool wasInW = false;
                        for (auto itW = W.begin(); itW != W.end(); ++itW) {
                            if (*itW == itY) { wasInW = true; break; }
                        }
                        if (wasInW) {
                            W.push_back(itY1);
                            W.push_back(itY2);
                        } else {
                            if (Y1.size() <= Y2.size()) W.push_back(itY1);
                            else W.push_back(itY2);
                        }
                    }
                } else {
                    ++itY;
                }
            }
        }
    }
    // 최종 파티션별로 상태 생성 및 병합
    unordered_map<int,string> newName;
    for (auto& part : P) {
        string name = generateStateName();
        rdfa.states.push_back(name);
        set<string> dfaSet;
        set<string> nfaSet;
        for (int s : part) {
            newName[s] = name;
            dfaSet.insert(states[s]);
            for (const auto& nfa : dfaStateToNfaSet[states[s]]) {
                nfaSet.insert(nfa);
            }
        }
        rdfa.reducedStateToDfaSet[name] = dfaSet;
        rdfa.reducedStateToNfaSet[name] = nfaSet;
        bool partIsFinal = false;
        for (int s : part) {
            if (isFinal[s]) partIsFinal = true;
        }
        if (partIsFinal) rdfa.finalStates.push_back(name);
        if (index[startState] < n && part.count(index[startState])) {
            rdfa.startState = name;
        }
    }
    // 병합된 상태에 대해 전이 정의
    unordered_set<string> handled;
    for (int i = 0; i < n; ++i) {
        string srcNew = newName[i];
        if (handled.count(srcNew)) continue;
        handled.insert(srcNew);
        for (char c : alphabet) {
            if (transitions.count(states[i]) && transitions[states[i]].count(c)) {
                int j = index[ transitions[states[i]][c] ];
                string tgtNew = newName[j];
                rdfa.transitions[srcNew][c] = tgtNew;
            }
        }
    }
    return rdfa;
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <inputfile.txt>\n";
        return 1;
    }
    string inputFile = argv[1];
    // 출력 파일명을 위한 확장자 없는 기본 이름 결정
    string base = inputFile;
    size_t dotPos = base.find_last_of('.');
    if (dotPos != string::npos) base = base.substr(0, dotPos);

    NFA nfa;
    if (!nfa.loadFromFile(inputFile)) {
        cerr << "Failed to load NFA from file: " << inputFile << endl;
        return 1;
    }
    // 상태 이름 중복 방지를 위한 카운터 초기화
    int maxId = -1;
    for (const string& s : nfa.states) {
        if (s.size() > 1 && s[0] == 'q') {
            // 상태 이름에서 숫자 부분 파싱
            try {
                int id = stoi(s.substr(1));
                if (id > maxId) maxId = id;
            } catch (...) { /* 숫자가 아니면 무시 */ }
        }
    }
    globalNextStateId = maxId + 1;

    // 원본 NFA 출력
    string nfaFile = base + "_nfa.txt";
    nfa.outputToFile(nfaFile);
    // DFA로 변환 (부분집합 구성법)
    DFA dfa = nfa.toDFA();
    dfa.removeUnreachableStates(); // 접근 불가능한 상태 제거
    string dfaFile = base + "_dfa.txt";
    dfa.outputToFile(dfaFile);
    // DFA 최소화 (Hopcroft 알고리즘)
    ReducedDFA minDfa = dfa.minimize();
    string minFile = base + "_reduced_dfa.txt";
    minDfa.outputToFile(minFile);
    return 0;
} 