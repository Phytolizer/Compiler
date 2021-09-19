#ifndef COMPILER_LEXER_DFA_HXX
#define COMPILER_LEXER_DFA_HXX

#include "Nfa.hxx"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

using NfaNodeSet = std::unordered_set<std::shared_ptr<NfaNode>>;

struct DfaNode
{
    NfaNodeSet Identifier;
    bool Mark = false;
	std::unordered_map<char, std::shared_ptr<DfaNode>> Next;
	bool IsAccepting;
	std::string AcceptString;
};

using DfaNodeSet = std::unordered_set<std::shared_ptr<DfaNode>>;

struct Dfa
{
    DfaNodeSet States;
};

Dfa DfaFromNfa(const Nfa& nfa);

#endif
