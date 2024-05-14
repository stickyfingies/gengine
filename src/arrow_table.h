#pragma once

#include <unordered_map>
#include <unordered_set>

template <typename A, typename B> class ArrowTable {
public:
	void associate(const A* from, const B* to)
	{
		forward[from].insert(to);
		reverse[to].insert(from);
	}

	void disassociate(const A* from, const B* to)
	{
		forward[from].remove(to);
		reverse[to].remove(from);
	}

	const std::unordered_set<B>& from(const A* source) const { return forward[source]; }
	std::unordered_set<B>& from(const A* source) { return forward[source]; }

	const std::unordered_set<A>& to(const B* target) const { return reverse[target]; }
	std::unordered_set<A>& to(const B* target) { return reverse[target]; }

private:
	std::unordered_map<A, std::unordered_set<B>> forward;
	std::unordered_map<B, std::unordered_set<A>> reverse;
};
