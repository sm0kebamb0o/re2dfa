#include "api.hpp"
#include <string>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <queue>

using Pos = unsigned;

/* Merge two hash sets into new one */
std::unordered_set<Pos> merge(const std::unordered_set<Pos> &s1, const std::unordered_set<Pos> &s2){
	std::unordered_set<Pos> result(s1);
	for (auto pos : s2){
		result.insert(pos);
	}
	return result;
}

/* Merge elements from the second hash set into first hash set */
void merge_in_place(std::unordered_set<Pos> &s1, const std::unordered_set<Pos> &s2){
	for (auto pos : s2){
		s1.insert(pos);
	}
}

class FollowPosTable{
public:

	FollowPosTable(const std::string &s, const Alphabet &alphabet) :
		s_(s), alphabet_(alphabet), last_pos_(0){}

	std::set<Pos> create(){

		bool concat_required = false;
		bool eps_required = true;

		Pos ind = 0;
		TreeNode *buf;

		for (auto symbol : s_){

			if (alphabet_.has_char(symbol)){

				terminals_pos_[symbol].insert(ind);

				buf = new TreeNode(symbol);
				buf->first_.insert(ind);
				buf->last_.insert(ind);
				++ind;

				operands_.push(buf);

				if (concat_required) operations.push('&');

				concat_required = true;
				eps_required = false;

				continue;
			}

			switch (symbol){
				case '|':
					make_coherence(symbol);
					operations.push(symbol);

					if (eps_required){
						buf = new TreeNode('@');
						buf->nullable_ = true;

						operands_.push(buf);
					}

					concat_required = false;
					eps_required = true;

					continue;
				case '*':
					operands_.push(execute_operation(symbol));

					concat_required = true;
					eps_required = false;

					continue;
				case '(':
					if (concat_required) operations.push('&');

					operations.push(symbol);

					concat_required = false;
					eps_required = true;

					continue;
				case ')':
					if (eps_required){

						buf = new TreeNode('@');
						buf->nullable_ = true;

						operands_.push(buf);
					}

					while (operations.top() != '('){
						operands_.push(execute_operation(operations.top()));
						operations.pop();
					}
					operations.pop();

					concat_required = true;
					eps_required = false;
					continue;
				default:
					break;
			}
		}

		if (eps_required){
			buf = new TreeNode('@');
			buf->nullable_ = true;

			operands_.push(buf);
		}

		while (!operations.empty()){
			operands_.push(execute_operation(operations.top()));
			operations.pop();
		}

		buf = new TreeNode('#');
		buf->first_.insert(ind);
		buf->last_.insert(ind);
		last_pos_ = ind;

		operands_.push(buf);

		TreeNode *root = execute_operation('&');

		return std::set<Pos>(root->first_.begin(), root->first_.end());
	}

	Pos get_last_pos() const{
		return last_pos_;
	}

	const auto &get_follow_pos() const{
		return follow_pos_;
	}

	const auto &get_terminals_pos() const{
		return terminals_pos_;
	}

private:
	class TreeNode{
	public:
		TreeNode(char value) : label_(value), nullable_(false){}

		char label_;
		bool nullable_;
		std::unordered_set<Pos> first_;
		std::unordered_set<Pos> last_;
	};

	inline void make_coherence(char operation){
		while (!operations.empty() && operations_priority[operations.top()] >= operations_priority[operation]){
			operands_.push(execute_operation(operations.top()));
			operations.pop();
		}
	}

	TreeNode *execute_operation(char operation){
		TreeNode *new_node = new TreeNode(operation);
		if (operation == '*'){
			auto operand = operands_.top();
			operands_.pop();

			for (auto pos : operand->last_){
				merge_in_place(follow_pos_[pos], operand->first_);
			}

			new_node->nullable_ = true;
			new_node->first_ = operand->first_;
			new_node->last_ = operand->last_;

			delete operand;

		} else{
			auto operand_second = operands_.top();
			operands_.pop();

			auto operand_first = operands_.top();
			operands_.pop();

			switch (operation){
				case '|':
					new_node->nullable_ = operand_first->nullable_ || operand_second->nullable_;
					new_node->first_ = merge(operand_first->first_, operand_second->first_);
					new_node->last_ = merge(operand_first->last_, operand_second->last_);
					break;
				case '&':

					for (auto pos : operand_first->last_){
						merge_in_place(follow_pos_[pos], operand_second->first_);
					}

					new_node->nullable_ = operand_first->nullable_ && operand_second->nullable_;

					if (operand_first->nullable_){
						new_node->first_ = merge(operand_first->first_, operand_second->first_);
					} else{
						new_node->first_ = operand_first->first_;
					}

					if (operand_second->nullable_){
						new_node->last_ = merge(operand_first->last_, operand_second->last_);
					} else{
						new_node->last_ = operand_second->last_;
					}
					break;
				default:
					break;
			}
			delete operand_first;
			delete operand_second;
		}
		return new_node;
	}

	const std::string &s_;
	const Alphabet &alphabet_;
	std::stack<TreeNode *> operands_;
	std::unordered_map<Pos, std::unordered_set<Pos>> follow_pos_;
	Pos last_pos_;
	std::unordered_map<char, unsigned> operations_priority = { {'|', 1}, {'&', 2} , {'(', 0} };
	std::stack<char> operations;

	std::unordered_map<char, std::unordered_set<Pos>> terminals_pos_;
};

std::string create_state_name(const std::set<Pos> &state){
	std::string out;
	bool first_pos = true;
	for (auto pos : state){
		if (first_pos) first_pos = false;
		else out += ',';
		out += std::to_string(pos);
	}
	return out;
}

DFA re2dfa(const std::string &s) {
	if (s.empty()){
		DFA res = DFA(Alphabet("@"));
		res.create_state("0", true);
		return res;

	}
	Alphabet alphabet(s);

	FollowPosTable follow_pos_table(s, alphabet);
	auto first_state = follow_pos_table.create();
	int last_pos = follow_pos_table.get_last_pos();

	const auto &follow_pos = follow_pos_table.get_follow_pos();
	const auto &terminals_pos = follow_pos_table.get_terminals_pos();

	DFA res(alphabet);

	std::queue<std::pair<std::set<Pos>, std::string>> work_list;
	std::string new_state_name;
	std::set<Pos> new_state;
	work_list.push({ first_state, create_state_name(first_state) });

	res.create_state(work_list.front().second, first_state.find(last_pos) != first_state.end());

	while (!work_list.empty()){

		for (auto terminal = alphabet.begin(); terminal != alphabet.end(); ++terminal){
			new_state.clear();

			for (auto pos : terminals_pos.at(*terminal)){
				if (work_list.front().first.find(pos) != work_list.front().first.end()){
					new_state.insert(follow_pos.at(pos).begin(), follow_pos.at(pos).end());
				}
			}

			new_state_name = create_state_name(new_state);

			if (!new_state_name.empty()){
				if (!res.has_state(new_state_name)){
					res.create_state(new_state_name, new_state.find(last_pos) != new_state.end());
					work_list.push({ new_state, new_state_name });
				}
				res.set_trans(work_list.front().second, *terminal, new_state_name);
			}
		}
		work_list.pop();
	}

	return res;
}
