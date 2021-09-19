#ifndef COMPILER_LEXER_NFA_HXX
#define COMPILER_LEXER_NFA_HXX

#include <bitset>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

enum struct NfaEdgeType
{
	Epsilon,
	Empty,
	CharacterClass,
};

enum struct NfaAnchor : std::size_t
{
	Start,
	End,
};

struct NfaNode
{
	bool IsAccepting;
	std::string AcceptString;
	std::unordered_set<char> Edge;
	NfaEdgeType EdgeType = NfaEdgeType::Empty;
	std::vector<std::shared_ptr<NfaNode>> Next;
	std::bitset<2> Anchor;
};

struct Nfa
{
	std::shared_ptr<NfaNode> StartState;
	std::shared_ptr<NfaNode> EndState;
};

Nfa NfaFromRegex(std::string_view regex);

#endif
