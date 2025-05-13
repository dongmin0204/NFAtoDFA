#include <bits/stdc++.h>
using namespace std;
static string EPSILON_SYMBOL = "ε";
static int globalNextStateId = 0;

// Generate a new state name in qXXX format (3-digit zero-padded)
string generateStateName() {
    char buf[10];
    sprintf(buf, "q%03d", globalNextStateId++);
    return string(buf);
}

// Forward declarations for classes
class DFA;
class ReducedDFA;

class NFA {
public:
    vector<string> states;
    vector<char> alphabet;
    unordered_map<string, unordered_map<char, set<string>>> transitions;
    unordered_map<string, set<string>> epsilonTransitions;
    string startState;
    vector<string> finalStates;

    bool loadFromFile(const string& filename) {
        // Initialize NFA containers
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
                // Continue reading multi-line DeltaFunctions
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
            // Parse FinalStateSet before StateSet to avoid substring confusion
            if (line.find("FinalStateSet") != string::npos && line.find('{') != string::npos) {
                size_t l = line.find('{'), r = line.rfind('}');
                if (l != string::npos && r != string::npos && r > l) {
                    string inside = line.substr(l+1, r - l - 1);
                    string token;
                    stringstream ss(inside);
                    while (getline(ss, token, ',')) {
                        string st = token;
                        // trim whitespace
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
                            if (sym == EPSILON_SYMBOL) continue; // do not include ε in alphabet
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
                // Begin reading DeltaFunctions (may span multiple lines)
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
        // Parse collected DeltaFunctions content
        if (!deltaContent.empty()) {
            if (deltaContent.front() == '{') deltaContent.erase(deltaContent.begin());
            if (!deltaContent.empty() && deltaContent.back() == '}') deltaContent.pop_back();
            string inside = deltaContent;
            size_t i = 0;
            while (i < inside.size()) {
                // skip whitespace and commas between entries
                while (i < inside.size() && (isspace((unsigned char)inside[i]) || inside[i] == ',')) i++;
                if (i >= inside.size()) break;
                if (inside[i] != '(') { i++; continue; }
                i++; // skip '('
                // parse source state name
                string srcState;
                while (i < inside.size() && inside[i] != ',') {
                    if (!isspace((unsigned char)inside[i])) srcState.push_back(inside[i]);
                    i++;
                }
                if (i < inside.size() && inside[i] == ',') i++;
                while (i < inside.size() && isspace((unsigned char)inside[i])) i++;
                // parse symbol
                string symStr;
                while (i < inside.size() && inside[i] != ')') {
                    if (!isspace((unsigned char)inside[i])) symStr.push_back(inside[i]);
                    i++;
                }
                if (i < inside.size() && inside[i] == ')') i++;
                // skip " = "
                while (i < inside.size() && (isspace((unsigned char)inside[i]) || inside[i] == '=')) {
                    if (inside[i] == '{') break;
                    i++;
                }
                if (i < inside.size() && inside[i] == '=') i++;
                while (i < inside.size() && isspace((unsigned char)inside[i])) i++;
                if (i < inside.size() && inside[i] == '{') i++;
                // parse target state set
                set<string> tgtSet;
                string tgt;
                while (i < inside.size() && inside[i] != '}') {
                    if (inside[i] == ',') {
                        // end of one target state
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
                // Store transitions
                if (symStr == EPSILON_SYMBOL) {
                    // ε-transition
                    epsilonTransitions[srcState].insert(tgtSet.begin(), tgtSet.end());
                } else if (!symStr.empty()) {
                    char sym = symStr.size() == 1 ? symStr[0] : symStr[0]; // (multi-char symbols not expected in this input)
                    transitions[srcState][sym].insert(tgtSet.begin(), tgtSet.end());
                }
            }
        }
        return true;
    }

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

    DFA toDFA();
};

class DFA {
public:
    vector<string> states;
    vector<char> alphabet;
    unordered_map<string, unordered_map<char, string>> transitions;
    string startState;
    vector<string> finalStates;

    void outputToFile(const string& filename) {
        ofstream fout(filename);
        fout << "StateSet = { ";
        for (size_t i = 0; i < states.size(); ++i) {
            fout << states[i];
            if (i < states.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
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

    ReducedDFA minimize();
};

class ReducedDFA {
public:
    vector<string> states;
    vector<char> alphabet;
    unordered_map<string, unordered_map<char, string>> transitions;
    string startState;
    vector<string> finalStates;

    void outputToFile(const string& filename) {
        ofstream fout(filename);
        fout << "StateSet = { ";
        for (size_t i = 0; i < states.size(); ++i) {
            fout << states[i];
            if (i < states.size() - 1) fout << ", ";
            else fout << " ";
        }
        fout << "}" << endl;
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

// Convert NFA to DFA (ε-closure and subset construction)
DFA NFA::toDFA() {
    DFA dfa;
    dfa.alphabet = alphabet;  // same alphabet (excluding ε)
    // Compute ε-closure of start state
    set<string> startSet = { startState };
    // Lambda for epsilon-closure of a set of states
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
    // Map each set of NFA states to a DFA state name
    // unordered_map<set<string>, string> subsetName; // 이 줄 삭제
    // Use a custom comparator for set of strings in map (or use string key)
    // We'll use an unordered_map with a custom hash for set<string> for efficiency
    struct SetHash {
        size_t operator()(const set<string>& st) const noexcept {
            // simple hash by concatenating state names
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
    // Assign name to start state closure
    string startName = generateStateName();
    stateMap[startClosure] = startName;
    dfa.states.push_back(startName);
    dfa.startState = startName;
    // Mark as final if any NFA final is in the closure
    bool isStartFinal = false;
    for (const string& s : startClosure) {
        if (find(finalStates.begin(), finalStates.end(), s) != finalStates.end()) {
            isStartFinal = true;
            break;
        }
    }
    if (isStartFinal) dfa.finalStates.push_back(startName);
    // Subset construction BFS
    queue< set<string> > q;
    q.push(startClosure);
    while (!q.empty()) {
        set<string> curSet = q.front();
        q.pop();
        string curName = stateMap[curSet];
        // For each input symbol, compute transition
        for (char c : alphabet) {
            // Move from each state in curSet on symbol c
            set<string> moveResult;
            for (const string& s : curSet) {
                if (transitions.count(s) && transitions[s].count(c)) {
                    for (const string& t : transitions[s][c]) {
                        moveResult.insert(t);
                    }
                }
            }
            // Compute ε-closure of the move result
            set<string> newClosure = epsilonClosure(moveResult);
            if (newClosure.empty()) continue;
            if (!stateMap.count(newClosure)) {
                // New DFA state discovered
                string newName = generateStateName();
                stateMap[newClosure] = newName;
                dfa.states.push_back(newName);
                // Check if it's a final state
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
            // Record DFA transition
            dfa.transitions[curName][c] = stateMap[newClosure];
        }
    }
    return dfa;
}

// Minimize DFA using Hopcroft's algorithm
ReducedDFA DFA::minimize() {
    ReducedDFA rdfa;
    rdfa.alphabet = alphabet;
    int n = states.size();
    if (n == 0) return rdfa;
    // Map state name to index for convenience
    unordered_map<string,int> index;
    for (int i = 0; i < n; ++i) index[states[i]] = i;
    // Final state flags
    vector<bool> isFinal(n, false);
    for (const string& fs : finalStates) {
        if (index.count(fs)) isFinal[index[fs]] = true;
    }
    // Initial partition: final vs non-final states
    set<int> finalSet, nonFinalSet;
    for (int i = 0; i < n; ++i) {
        if (isFinal[i]) finalSet.insert(i);
        else nonFinalSet.insert(i);
    }
    // Partition P and work list W
    list< set<int> > P;
    if (!finalSet.empty()) P.push_back(finalSet);
    if (!nonFinalSet.empty()) P.push_back(nonFinalSet);
    list< list<set<int>>::iterator > W;
    if (!finalSet.empty()) W.push_back(P.begin());
    if (!nonFinalSet.empty()) {
        auto it = finalSet.empty() ? P.begin() : next(P.begin());
        W.push_back(it);
    }
    // Precompute reverse transition map: for each symbol and state, which states have a transition to it
    unordered_map<char, unordered_map<int, vector<int>>> revTrans;
    for (int i = 0; i < n; ++i) {
        for (char c : alphabet) {
            if (transitions.count(states[i]) && transitions[states[i]].count(c)) {
                int j = index[ transitions[states[i]][c] ];
                revTrans[c][j].push_back(i);
            }
        }
    }
    // Hopcroft's partition refinement loop
    while (!W.empty()) {
        // Choose a partition A to refine others
        auto A_it = W.front();
        W.pop_front();
        const set<int>& A = *A_it;
        for (char c : alphabet) {
            // X = set of states whose transition on c leads into A
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
            // Refine each partition Y with X
            for (auto itY = P.begin(); itY != P.end(); ) {
                set<int>& Y = *itY;
                // Split Y into Y1 = X∩Y and Y2 = Y\X
                set<int> Y1, Y2;
                for (int s : Y) {
                    if (X.count(s)) Y1.insert(s);
                    else Y2.insert(s);
                }
                if (!Y1.empty() && !Y2.empty()) {
                    // Replace Y in P by Y1 and Y2
                    itY = P.erase(itY);
                    auto itY1 = P.insert(itY, Y1);
                    auto itY2 = P.insert(itY, Y2);
                    // Update W
                    // If Y was in W, replace with Y1 and Y2
                    for (auto itW = W.begin(); itW != W.end(); ++itW) {
                        if (*itW == itY1 || *itW == itY2) {
                            // (We handle addition below, so do nothing here)
                        }
                    }
                    // Add the smaller subset to W (or both if Y was in W)
                    if (Y.size() < INT_MAX) {
                        // Check if original Y was in W
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
    // Construct ReducedDFA from final partition P
    // Map each original state index to a new state name
    unordered_map<int,string> newName;
    for (auto& part : P) {
        string name = generateStateName();
        rdfa.states.push_back(name);
        bool partIsFinal = false;
        for (int s : part) {
            newName[s] = name;
            if (isFinal[s]) partIsFinal = true;
        }
        if (partIsFinal) rdfa.finalStates.push_back(name);
        // If this part contains the original DFA start state, mark as new start
        // (there will be exactly one such part)
        if (index[startState] < n && part.count(index[startState])) {
            rdfa.startState = name;
        }
    }
    // Define transitions for ReducedDFA
    // For each original DFA state and symbol, link its partition's representative to target's representative
    // We ensure each new state gets one outgoing per symbol (no duplicates due to partition equivalence)
    unordered_set<string> handled;
    for (int i = 0; i < n; ++i) {
        string srcNew = newName[i];
        if (handled.count(srcNew)) continue; // already set transitions for this new state
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
    // Determine base name without extension for output files
    string base = inputFile;
    size_t dotPos = base.find_last_of('.');
    if (dotPos != string::npos) base = base.substr(0, dotPos);

    NFA nfa;
    if (!nfa.loadFromFile(inputFile)) {
        cerr << "Failed to load NFA from file: " << inputFile << endl;
        return 1;
    }
    // Initialize name counter to avoid reusing existing state numbers
    int maxId = -1;
    for (const string& s : nfa.states) {
        if (s.size() > 1 && s[0] == 'q') {
            // parse numeric part of state name
            try {
                int id = stoi(s.substr(1));
                if (id > maxId) maxId = id;
            } catch (...) { /* ignore non-numeric */ }
        }
    }
    globalNextStateId = maxId + 1;

    // Output original NFA
    string nfaFile = base + "_nfa.txt";
    nfa.outputToFile(nfaFile);
    // Convert to DFA (subset construction)
    DFA dfa = nfa.toDFA();
    string dfaFile = base + "_dfa.txt";
    dfa.outputToFile(dfaFile);
    // Minimize DFA (Hopcroft's algorithm)
    ReducedDFA minDfa = dfa.minimize();
    string minFile = base + "_reduced_dfa.txt";
    minDfa.outputToFile(minFile);
    return 0;
} 