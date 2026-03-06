#ifndef PARSER_H
#define PARSER_H

#include "lexer.hpp"
#include <vector>
#include <stack>
#include <map>

class Term;
class Rule;
class Definition;
class TreeNode;
class AstNode;

class TreeNode {
	public:
	Token token;
	std::vector<TreeNode*> children;
};

class AstNode {
	public:
	enum Type {
		DEFINITION,
		RULE,
		TERM
	} nodeType;

	virtual const std::string &getName() const = 0;
};

class Term : public AstNode {
	public:
	enum Type {
		TERMINAL,
		NONTERMINAL
	};
	const std::string &getName() const;
	Term(enum Type type, const std::string& term);
	Term();
	enum Type type() const { return _type; };

	protected:
	std::string name;
	enum Type _type;
};

class Rule : public AstNode {
	std::vector<const Term*> terms;
	const Term *_lhs = nullptr;
	std::string name;
	int _id;
	
	public:
	bool userProvided = false;
	Rule();
	void addTerm(const Term* term);
	void addTerm(const Term* term, int idx);
	const std::vector<const Term*>& getTerms() const;
	const Term* getTerm(int idx) const;
	int id() const;
	const std::string &getName() const;
	const Term *lhs() const;
	void lhs(const Term *_lhs);
};

class Ast {
	std::string _startSymbol;

	public:
	class TermStore {
		std::map<std::string, int> ids;
		std::vector<const Term*> terms;

		public:
		int insert(const Term* term);
		bool contains(const Term* term) const ;
		bool contains(const std::string& name) const;
		bool contains(int id) const ;
		int query(const Term* term) const;
		int query(const std::string& name) const;
		const Term& query(int id) const;
		int size() const;
		~TermStore();
	} termStore;

	class RuleStore {
		std::map<std::string, std::vector<const Rule*>> def;
		std::vector<const Rule*> rules;

		public:
		void insert(const Rule* rule);
		void insert(const std::vector<Rule*> &rules);
		const std::vector<const Rule*> &query(const Term* term) const;
		const std::vector<const Rule*> &query(const std::string& term) const;
		const Rule &query(int id) const;
		int size() const;
		~RuleStore();
	} ruleStore;

	bool addRule(const Rule* rule);
	bool addRules(const std::vector<Rule*> &rules);
	const std::vector<const Rule*>& getDefinition(const std::string& nt) const;
	const std::vector<const Rule*>& getDefinition(const Term *nt) const;
	const std::string &startSymbol(void) const;
};

class Parser {
	private:
	void reduce(int rule, Ast& ast, std::stack<std::pair<TreeNode*, int>>& stk, std::map<TreeNode*, const AstNode*>& mp);
	void shift(Token tkn, int state, std::stack<std::pair<TreeNode*, int>>& stk);

	public:
	
	Parser() {};
	Ast* parse(Lexer lexer);
};

#endif