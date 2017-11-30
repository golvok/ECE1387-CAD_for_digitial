#include "cnf_expression.hpp"

#include <util/logging.hpp>
#include <util/utils.hpp>

#include <unordered_map>

template<typename T>
void f(const std::vector<std::vector<int>>& data, T m_disjunctions, T m_all_literals) {
	std::unordered_set<LiteralID> liteals_seen;
	for (const auto& disjunction : data) {
		if (disjunction.empty()) { continue; }
		m_disjunctions.emplace_back();
		std::transform(begin(disjunction), end(disjunction), std::back_inserter(m_disjunctions.back()), [&](const auto& datum) {
			auto literal_id = util::make_id<LiteralID>(static_cast<LiteralID::IDType>(abs(datum)));
			if (liteals_seen.insert(literal_id).second) {
				m_all_literals.push_back(literal_id);
			}
			return Literal(datum < 0, literal_id);
		});
	}
}


CNFExpression::CNFExpression(const std::vector<VariableOrder>& ordering, const std::vector<std::vector<int>>& data)
	: m_disjunctions()
	, m_all_literals()
{
	for (const auto& disjunction : data) {
		if (disjunction.empty()) { continue; }
		m_disjunctions.emplace_back();
		std::transform(begin(disjunction), end(disjunction), std::back_inserter(m_disjunctions.back()), [&](const auto& datum) {
			auto literal_id = util::make_id<LiteralID>(static_cast<LiteralID::IDType>(abs(datum)));
			return Literal(datum < 0, literal_id);
		});
	}

	std::unordered_map<Literal, int> literal_counts;
	std::unordered_map<Literal, int> literal_file_pos;
	std::unordered_map<LiteralID, int> literal_id_file_pos;

	int curr_file_pos = 0;
	for (const auto& disjunction : all_disjunctions()) {
		for (const auto& lit : disjunction) {
			literal_counts[lit] += 1;
			literal_id_file_pos.emplace(lit.id(), curr_file_pos);
			literal_file_pos.emplace(lit, curr_file_pos);
			curr_file_pos += 1;
		}
	}

	std::unordered_map<int, std::vector<Literal>> count_to_literal;
	for (const auto& lit_and_count : literal_counts) {
		count_to_literal[lit_and_count.second].push_back(lit_and_count.first);
	}
	{auto indent = dout(DL::DATA_READ1).indentWithTitle("Occurance Counts");
		util::print_assoc_container(dout(DL::DATA_READ1), count_to_literal, "\n", "", "", [&](auto& str, auto& lit_list) { util::print_container(str, lit_list); });
		dout(DL::DATA_READ1) << '\n';
	}

	for (const auto& lit_id_and_count : literal_id_file_pos) {
		m_all_literals.push_back(lit_id_and_count.first);
	}

	auto by_literal_id_file_pos = [&](auto& lhs, auto& rhs) {
		return literal_id_file_pos.at(lhs) < literal_id_file_pos.at(rhs);
	};

	auto by_literal_count = [&](auto& lhs, auto& rhs) {
		return std::max(literal_counts.at(Literal(true,lhs)), literal_counts.at(Literal(false,lhs)))
			> std::max(literal_counts.at(Literal(true,rhs)), literal_counts.at(Literal(false,rhs)));
	};

	for (auto it = rbegin(ordering); it != rend(ordering); ++it) {
		std::stable_sort(begin(m_all_literals), end(m_all_literals), [&](auto& lhs, auto& rhs) {
			switch (*it) {
				case VariableOrder::FILE: {
					return by_literal_id_file_pos(lhs, rhs);
				break; } case VariableOrder::MOST_COMMON_FIRST: {
					return by_literal_count(lhs, rhs);
				break; } default: {
					util::print_and_throw<std::runtime_error>([&](auto& str) {
						str << "unhandled vaiable ordering type: " << (int)*it;
					}); throw "impossible";
				}
			}
		});
	}
}
