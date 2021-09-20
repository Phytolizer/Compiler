#include "Lexer/Dfa.hxx"
#include "Lexer/Nfa.hxx"
#include <iostream>
#include <string>

int main()
{
	std::string line;
	while (std::getline(std::cin, line))
	{
		Nfa test = NfaFromRegex(line);
		Dfa testDfa = DfaFromNfa(test);
		std::cout << "DFA constructed\n";
	}
}
