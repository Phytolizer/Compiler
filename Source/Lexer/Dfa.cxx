#include "Dfa.hxx"
#include <limits>
#include <stack>

std::shared_ptr<DfaNode> ComputeEpsilonClosure(NfaNodeSet nodes);
NfaNodeSet ComputeMoveClosure(NfaNodeSet inputs, char c);
bool AlreadyInDfa(const DfaNodeSet& dfa, const std::shared_ptr<DfaNode>& node);

std::shared_ptr<DfaNode> FindUnmarked(DfaNodeSet dfa)
{
	for (auto& node : dfa)
	{
		if (!node->Mark)
		{
			return node;
		}
	}
	return nullptr;
}

Dfa DfaFromNfa(const Nfa& nfa)
{
	DfaNodeSet DfaStates;
	DfaStates.emplace(ComputeEpsilonClosure({nfa.StartState}));
	std::shared_ptr<DfaNode> Current;
	while ((Current = FindUnmarked(DfaStates)))
	{
		Current->Mark = true;
		for (int c = std::numeric_limits<char>::max(); c >= 0; --c)
		{
			auto NfaSet = ComputeMoveClosure(Current->Identifier, c);
			std::shared_ptr<DfaNode> Next;
			if (!NfaSet.empty())
			{
				Next = ComputeEpsilonClosure(NfaSet);
			}
			if (Next && !Next->Identifier.empty())
			{
				if (!AlreadyInDfa(DfaStates, Next))
				{
					DfaStates.emplace(Next);
				}
				Current->Next.emplace(c, Next);
			}
		}
	}
	return {DfaStates};
}

std::shared_ptr<DfaNode> ComputeEpsilonClosure(NfaNodeSet nodes)
{
	auto result = std::make_shared<DfaNode>();
	result->Mark = false;
	std::stack<std::shared_ptr<NfaNode>> WorkingStack;
	for (auto& node : nodes)
	{
		WorkingStack.emplace(node);
	}
	while (!WorkingStack.empty())
	{
		auto i = std::move(WorkingStack.top());
		WorkingStack.pop();
		if (i->IsAccepting)
		{
			result->IsAccepting = true;
			result->AcceptString = i->AcceptString;
		}
		if (i->EdgeType == NfaEdgeType::Epsilon)
		{
			for (auto& edge : i->Next)
			{
				if (std::find(result->Identifier.begin(), result->Identifier.end(), edge) == result->Identifier.end())
				{
					result->Identifier.emplace(edge);
					WorkingStack.emplace(edge);
				}
			}
		}
	}
	return result;
}

NfaNodeSet ComputeMoveClosure(NfaNodeSet inputs, char c)
{
	NfaNodeSet outset;
	for (auto& node : inputs)
	{
		if (node->EdgeType == NfaEdgeType::CharacterClass && node->Edge.contains(c))
		{
			outset.emplace(node->Next.back());
		}
	}
	return outset;
}

bool AlreadyInDfa(const DfaNodeSet& dfa, const std::shared_ptr<DfaNode>& node)
{
	return std::find_if(dfa.begin(), dfa.end(), [&node](const std::shared_ptr<DfaNode>& n) {
		       return n->Identifier == node->Identifier;
	       }) != dfa.end();
}
