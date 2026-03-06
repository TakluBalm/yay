#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <stdio.h>

enum Type{
	NON_TERMINAL,
	COLON,
	SEMI_COLON,
	OR,
	TERMINAL,
	OPAREN,
	CPAREN,
	OBRACKET,
	CBRACKET,
	OBRACE,
	CBRACE,
	END,
	SYNTAX,
	DEF,
	RULE,
	DER,
	TERM,
	UNIDENTIFIED_TOK
};

class Token{
	public:
	enum Type type;
	std::string* val = NULL;

	Token(Type t, std::string* val){
		type = t;
		this->val = val;
	}

	Token(Type t){
		type = t;
	}

	Token(){};

	Type getType() {
		return type;
	}

	bool hasValue() {
		return val != NULL;
	}

	const std::string& getValue() {
		return *val;
	}
};

class Lexer{
	private:

	FILE* file;
	char current;
	Token prev;
	bool consumed = true;

	void advance();
	void skipWhiteSpace();
	void parseNonTerminal(std::string& container);
	void parseTerminal(std::string& container);

	public:
	
	Lexer(FILE* f);
	Lexer(){};
	Token peek();
	void consume();
};

#endif