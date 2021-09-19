#include "Dfa.hxx"
#include <limits>
#include <stack>

std::shared_ptr<DfaNode> ComputeEpsilonClosure(std::unordered_set<std::shared_ptr<NfaNode>> nodes);
std::unordered_set<std::shared_ptr<NfaNode>> ComputeMoveClosure(
    std::unordered_set<std::shared_ptr<NfaNode>> inputs, char c);
std::shared_ptr<DfaNode> FindUnmarked(std::unordered_set<std::shared_ptr<DfaNode>> dfa)
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
bool AlreadyInDfa(const std::unordered_set<std::shared_ptr<DfaNode>>& dfa, const std::shared_ptr<DfaNode>& node);

Dfa DfaFromNfa(const Nfa& nfa)
{
	std::unordered_set<std::shared_ptr<DfaNode>> DfaStates;
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

std::shared_ptr<DfaNode> ComputeEpsilonClosure(std::unordered_set<std::shared_ptr<NfaNode>> nodes)
{
	auto result = std::make_shared<DfaNode>();
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

std::unordered_set<std::shared_ptr<NfaNode>> ComputeMoveClosure(
    std::unordered_set<std::shared_ptr<NfaNode>> inputs, char c)
{
	std::unordered_set<std::shared_ptr<NfaNode>> outset;
	for (auto& node : inputs)
	{
		if (node->EdgeType == NfaEdgeType::CharacterClass && node->Edge.contains(c))
		{
			outset.emplace(node->Next.back());
		}
	}
	return outset;
}

bool AlreadyInDfa(const std::unordered_set<std::shared_ptr<DfaNode>>& dfa, const std::shared_ptr<DfaNode>& node)
{
	return std::find_if(dfa.begin(), dfa.end(), [&node](const std::shared_ptr<DfaNode>& n) {
		       return n->Identifier == node->Identifier;
	       }) != dfa.end();
}
