#include <lexer.hpp>
#include <parser.hpp>
#include <stateGenerator.hpp>
#include <iostream>
#include <set>
#include <map>
#include <queue>

using namespace std;

int convert(Ast *ast, Token t){
	string str;
	switch(t.getType()) {
	case Type::NON_TERMINAL:
		str = "nonterminal";
		break;
	case Type::TERMINAL:
		str = "terminal";
		break;
	case Type::CBRACE:
		str = "cbrace";
		break;
	case Type::OBRACE:
		str = "obrace";
		break;
	case Type::CBRACKET:
		str = "cbracket";
		break;
	case Type::OBRACKET:
		str = "obracket";
		break;
	case Type::CPAREN:
		str = "cparen";
		break;
	case Type::OPAREN:
		str = "oparen";
		break;
	case Type::COLON:
		str = "colon";
		break;
	case Type::SEMI_COLON:
		str = "semi";
		break;
	case Type::END:
		str = "$";
		break;
	case Type::OR:
		str = "or";
		break;
	}
	return ast->termStore.query(str);
}

int main(int argc, char** argv){
	string filename;
	if (argc == 1) {
		cout << "File: ";
		cin >> filename;
	} else {
		filename = string(argv[1]);
	}

	cout << "Opening " << filename << "...\n";

	FILE* file = fopen(filename.c_str(), "r");
	
	if(file == NULL){
		cout << "File could not be opened\n";
		return 1;
	}
	Lexer lex(file);
	Parser parser;
	Ast* ast = parser.parse(lex);
	if(ast != nullptr){
		map<string, bool> mp;
		string start = ast->startSymbol();
		StateGenerator sg(*ast);

		if(!sg.generateStateTable()){
			cout << "Not an LALR Grammar\n";
			return 1;
		}
		
		
		auto& table = sg.table();

		int maxVal = 0;
		for(auto row: table){
			for(auto act: row){
				maxVal = max(maxVal, act.val);
			}
		}
		maxVal = max(maxVal, (int)table[0].size());
		
		int maxLen = to_string(maxVal).size()+1;

		for(int i = 0; i < maxLen; i++){
			cout << " ";
		}
		cout << "|";
		for(int i = 0; i < table[0].size(); i++){
			auto str = to_string(i);
			for(int j = 0; j + str.size() < maxLen; j++)	cout << " ";
			cout << str << "|";
		}
		cout << endl;

		int idx = 0;
		for(auto row: table){
			auto str = to_string(idx++);
			for(int i = 0; i + str.size() < maxLen; i++){
				cout << " ";
			}
			cout << str << "|";

			for(auto act: row){
				if(act.type == Action::REJECT){
					for(int i = 0; i < maxLen; i++){
						cout << " ";
					}
					cout << "|";
					continue;
				}
				auto str = to_string(act.val);
				for(int i = 0; i + str.size() + 1 < maxLen; i++){
					cout << " ";
				}
				if(act.type == Action::SHIFT)	cout << "S";
				if(act.type == Action::REDUCE)	cout << "R";
				if(act.type == Action::GOTO)	cout << "G";

				cout << str << "|";
			}
			cout << endl;
		}

		stack<int> stk;
		stk.push(0);
		fseek(file, 0, SEEK_SET);
		while(true){
			Token tkn = lex.peek();
			int id = convert(ast, tkn);
			int state = stk.top();
			Action action = table[state][id];
			if(action.type == Action::REDUCE) {
				const Rule &rule = ast->ruleStore.query(action.val);
				for(int i = 0; i < rule.getTerms().size(); i++){
					stk.pop();
					stk.pop();
				}
				const Term *lhs = rule.lhs();
				if(lhs->getName() == ast->startSymbol()){
					cout << "Successfully parsed\n";
					break;
				}

				int st = stk.top();
				id = ast->termStore.query(lhs);
				stk.push(id);
				Action a = table[st][id];

				if(a.type != Action::GOTO){
					cout << "Invalid Syntax" << endl;
					break;
				}

				stk.push(a.val);
				continue;
			}
			if(action.type == Action::REJECT){
				cout << "Invalid Syntax" << endl;
				break;
			}
			stk.push(id);
			stk.push(action.val);
			lex.consume();
		}
	}
	else {
		cout << "Syntax error in " << filename << endl;
	}
}