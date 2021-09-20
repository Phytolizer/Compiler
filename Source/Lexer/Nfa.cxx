#include "Nfa.hxx"
#include <cctype>
#include <iostream>

static void SetAll(std::unordered_set<char>* set);

enum struct TokenClass
{
	Literal,
	EndOfInput,
	Pipe,
	Carat,
	Dollar,
	LeftBracket,
	RightBracket,
	Hyphen,
	LeftParenthesis,
	RightParenthesis,
	Period,
	LeftBrace,
	RightBrace,
	Asterisk,
	Plus,
	Question,
};

struct NfaParseState
{
	enum struct CanStartExpressionResult
	{
		Yes,
		No,
		MisplacedClosureSymbol,
		MisplacedRightBracket,
		MisplacedCarat,
	};

	std::string_view OriginalInput;
	std::string_view::iterator Input;
	TokenClass CurrentToken;
	char CurrentLexeme;
	std::vector<NfaNode> Nodes;
	bool InQuotations = false;

	NfaParseState(std::string_view OriginalInput) : OriginalInput{OriginalInput}, Input{OriginalInput.begin()}
	{
	}

	Nfa ParseExpression();
	Nfa ParseConcatenation();
	Nfa ParseUnion();
	Nfa ParseClosure();
	Nfa ParseAtom();
	void ParseCharacterClass(std::unordered_set<char>* set);
	CanStartExpressionResult CanStartExpression(TokenClass cls);
	void Advance();
};

Nfa NfaFromRegex(std::string_view regex)
{
	return NfaParseState{regex}.ParseExpression();
}

Nfa NfaParseState::ParseUnion()
{
	Nfa a = ParseConcatenation();
	Nfa result = a;
	while (CurrentToken == TokenClass::Pipe)
	{
		Advance();
		Nfa b = ParseConcatenation();
		auto start = std::make_shared<NfaNode>();
		start->EdgeType = NfaEdgeType::Epsilon;
		start->Next.emplace_back(a.StartState);
		start->Next.emplace_back(b.StartState);
		auto end = std::make_shared<NfaNode>();
		end->EdgeType = NfaEdgeType::Empty;
		a.EndState->EdgeType = NfaEdgeType::Epsilon;
		a.EndState->Next.emplace_back(end);
		b.EndState->EdgeType = NfaEdgeType::Epsilon;
		b.EndState->Next.emplace_back(end);
		result = Nfa{
		    .StartState = start,
		    .EndState = end,
		};
	}
	return result;
}

Nfa NfaParseState::ParseExpression()
{
	Advance();
	Nfa result;
	std::bitset<2> anchor;
	if (CurrentToken == TokenClass::Carat)
	{
		result.StartState = std::make_shared<NfaNode>();
		result.StartState->EdgeType = NfaEdgeType::CharacterClass;
		result.StartState->Edge.emplace('\n');
		anchor.set(static_cast<std::size_t>(NfaAnchor::Start));
		Advance();
		Nfa temp = ParseUnion();
		result.StartState->Next.emplace_back(temp.StartState);
		result.EndState = temp.EndState;
	}
	else
	{
		result = ParseUnion();
	}

	if (CurrentToken == TokenClass::Dollar)
	{
		Advance();
		result.EndState->Next.emplace_back(std::make_unique<NfaNode>());
		result.EndState->EdgeType = NfaEdgeType::CharacterClass;
		result.EndState->Edge.emplace('\n');
		result.EndState->Edge.emplace('\r');
		result.EndState = result.EndState->Next.back();
		anchor.set(static_cast<std::size_t>(NfaAnchor::End));
	}

	while (Input != OriginalInput.end() && std::isspace(*Input))
	{
		++Input;
	}
	result.EndState->IsAccepting = true;
	if (Input != OriginalInput.end())
	{
		result.EndState->AcceptString = std::string{Input, OriginalInput.end()};
	}
	result.EndState->Anchor = std::move(anchor);
	Advance();
	return result;
}

Nfa NfaParseState::ParseConcatenation()
{
	Nfa result;
	constexpr auto CLOSURE_ERROR = "Error: encountered a *, +, or ? operator in an invalid position.\n";
	constexpr auto BOL_ERROR = "Error: encountered a ^ opereator in an invalid position.\n";
	constexpr auto BRACKET_ERROR = "Error: encountered a mismatched ] symbol.\n";
	switch (CanStartExpression(CurrentToken))
	{
	case CanStartExpressionResult::Yes:
		result = ParseClosure();
		break;
	case CanStartExpressionResult::No:
		return Nfa{nullptr, nullptr};
	case CanStartExpressionResult::MisplacedClosureSymbol:
		std::cerr << CLOSURE_ERROR;
		return Nfa{nullptr, nullptr};
	case CanStartExpressionResult::MisplacedCarat:
		std::cerr << BOL_ERROR;
		return Nfa{nullptr, nullptr};
	case CanStartExpressionResult::MisplacedRightBracket:
		std::cerr << BRACKET_ERROR;
		return Nfa{nullptr, nullptr};
	}

	bool concatenating = true;
	while (true)
	{
		switch (CanStartExpression(CurrentToken))
		{
		case CanStartExpressionResult::Yes:
			break;
		case CanStartExpressionResult::No:
			concatenating = false;
			break;
		case CanStartExpressionResult::MisplacedCarat:
			std::cerr << BOL_ERROR;
			return Nfa{nullptr, nullptr};
		case CanStartExpressionResult::MisplacedClosureSymbol:
			std::cerr << CLOSURE_ERROR;
			return Nfa{nullptr, nullptr};
		case CanStartExpressionResult::MisplacedRightBracket:
			std::cerr << BRACKET_ERROR;
			return Nfa{nullptr, nullptr};
		}
		if (!concatenating)
		{
			break;
		}
		Nfa rhs = ParseClosure();
		*result.EndState = *rhs.StartState;
		result.EndState = rhs.EndState;
	}
	return result;
}

Nfa NfaParseState::ParseClosure()
{
	Nfa result = ParseAtom();
	if (CurrentToken == TokenClass::Asterisk || CurrentToken == TokenClass::Plus ||
	    CurrentToken == TokenClass::Question)
	{
		auto start = std::make_shared<NfaNode>();
		auto end = std::make_shared<NfaNode>();
		start->EdgeType = NfaEdgeType::Epsilon;
		start->Next.emplace_back(result.StartState);
		result.EndState->EdgeType = NfaEdgeType::Epsilon;
		result.EndState->Next.emplace_back(end);

		if (CurrentToken == TokenClass::Asterisk || CurrentToken == TokenClass::Question)
		{
			start->EdgeType = NfaEdgeType::Epsilon;
			start->Next.emplace_back(end);
		}
		if (CurrentToken == TokenClass::Asterisk || CurrentToken == TokenClass::Plus)
		{
			result.EndState->EdgeType = NfaEdgeType::Epsilon;
			result.EndState->Next.emplace_back(result.StartState);
		}
		result.StartState = start;
		result.EndState = end;
		Advance();
	}
	return result;
}

Nfa NfaParseState::ParseAtom()
{
	Nfa result;
	if (CurrentToken == TokenClass::LeftParenthesis)
	{
		Advance();
		result = ParseUnion();
		if (CurrentToken == TokenClass::RightParenthesis)
		{
			Advance();
		}
		else
		{
			std::cerr << "Error: mismatched ( symbol.\n";
			return Nfa{nullptr, nullptr};
		}
	}
	else
	{
		result = {
		    .StartState = std::make_shared<NfaNode>(),
		    .EndState = std::make_shared<NfaNode>(),
		};
		auto start = result.StartState;
		start->EdgeType = NfaEdgeType::CharacterClass;
		start->Next.emplace_back(result.EndState);
		if (CurrentToken == TokenClass::Period || CurrentToken == TokenClass::LeftBracket)
		{
			if (CurrentToken == TokenClass::Period)
			{
				SetAll(&start->Edge);
			}
			else
			{
				Advance();
				if (CurrentToken == TokenClass::Carat)
				{
					Advance();
					SetAll(&start->Edge);
				}
				if (CurrentToken == TokenClass::RightBracket)
				{
					for (char c = 0x00; c <= ' '; ++c)
					{
						start->Edge.emplace(c);
					}
				}
				else
				{
					ParseCharacterClass(&start->Edge);
				}
			}
			Advance();
		}
		else
		{
			start->Edge.emplace(CurrentLexeme);
			Advance();
		}
	}
	return result;
}

void NfaParseState::ParseCharacterClass(std::unordered_set<char>* set)
{
	char first = 0;
	while (CurrentToken != TokenClass::EndOfInput && CurrentToken != TokenClass::RightBracket)
	{
		if (CurrentToken == TokenClass::Hyphen)
		{
			Advance();
			for (; first <= CurrentLexeme; ++first)
			{
				set->emplace(first);
			}
		}
		else
		{
			first = CurrentLexeme;
			set->emplace(first);
		}
		Advance();
	}
}

NfaParseState::CanStartExpressionResult NfaParseState::CanStartExpression(TokenClass cls)
{
	switch (cls)
	{
	case TokenClass::RightParenthesis:
	case TokenClass::Dollar:
	case TokenClass::Pipe:
	case TokenClass::EndOfInput:
		return CanStartExpressionResult::No;
	case TokenClass::Asterisk:
	case TokenClass::Plus:
	case TokenClass::Question:
		return CanStartExpressionResult::MisplacedClosureSymbol;
	case TokenClass::RightBracket:
		return CanStartExpressionResult::MisplacedRightBracket;
	case TokenClass::Carat:
		return CanStartExpressionResult::MisplacedCarat;
	default:
		return CanStartExpressionResult::Yes;
	}
}

void NfaParseState::Advance()
{
	if (Input == OriginalInput.end() || std::isspace(*Input))
	{
		CurrentToken = TokenClass::EndOfInput;
		CurrentLexeme = '\0';
		return;
	}
	switch (*Input)
	{
	case '|':
		CurrentToken = TokenClass::Pipe;
		break;
	case '.':
		CurrentToken = TokenClass::Period;
		break;
	case '^':
		CurrentToken = TokenClass::Carat;
		break;
	case '$':
		CurrentToken = TokenClass::Dollar;
		break;
	case ']':
		CurrentToken = TokenClass::RightBracket;
		break;
	case '[':
		CurrentToken = TokenClass::LeftBracket;
		break;
	case '}':
		CurrentToken = TokenClass::RightBrace;
		break;
	case ')':
		CurrentToken = TokenClass::RightParenthesis;
		break;
	case '{':
		CurrentToken = TokenClass::LeftBrace;
		break;
	case '(':
		CurrentToken = TokenClass::LeftParenthesis;
		break;
	case '*':
		CurrentToken = TokenClass::Asterisk;
		break;
	case '-':
		CurrentToken = TokenClass::Hyphen;
		break;
	case '?':
		CurrentToken = TokenClass::Question;
		break;
	case '+':
		CurrentToken = TokenClass::Plus;
		break;
	default:
		CurrentToken = TokenClass::Literal;
		break;
	}
	CurrentLexeme = *Input;
	++Input;
}

void SetAll(std::unordered_set<char>* set)
{
	for (int c = 0x01; c <= 0x7F; ++c)
	{
		if (c == '\n' || c == '\r')
		{
			continue;
		}
		set->emplace(c);
	}
}
