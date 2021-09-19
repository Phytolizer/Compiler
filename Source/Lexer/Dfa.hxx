#ifndef COMPILER_LEXER_DFA_HXX
#define COMPILER_LEXER_DFA_HXX

#include "Nfa.hxx"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct DfaNode
{
    std::unordered_set<std::shared_ptr<NfaNode>> Identifier;
    bool Mark = false;
	std::unordered_map<char, std::shared_ptr<DfaNode>> Next;
	bool IsAccepting;
	std::string AcceptString;
};

struct Dfa
{
    std::unordered_set<std::shared_ptr<DfaNode>> States;
};

Dfa DfaFromNfa(const Nfa& nfa);

#endif
