#include <lexer.hpp>
#include <stdio.h>
#include <string>

using namespace std;

#define OCT	8
#define DEC	10
#define HEX	16
	
void Lexer::advance(){
	current = fgetc(file);
}

void Lexer::skipWhiteSpace(){
	while(true){
		switch(current){
			case '\n':
			case ' ':
			case '\t':
			case '\r':{
				advance();
				break;
			}
			default:{
				return;
			}
		}
	}
}

void Lexer::parseNonTerminal(string& value){
	value.clear();

	while((current >= 'A' && current <= 'Z') || current == '_'){
		value += current;
		advance();
	}
}

void Lexer::parseTerminal(string& value){
	value.clear();

	while((current >= 'a' && current <= 'z') || current == '_'){
		value += current;
		advance();
	}
}

Lexer::Lexer(FILE* file){
	this->file = file;
	current = fgetc(file);
}

Token Lexer::peek(){
	if(!consumed)	return prev;
	consumed = false;
	skipWhiteSpace();
	switch(current){
		case ':':{
			advance();
			return prev = Token(COLON);
		}
		case '|':{
			advance();
			return prev = Token(OR);
		}
		case '[':{
			advance();
			return prev = Token(OBRACKET);
		}
		case ']':{
			advance();
			return prev = Token(CBRACKET);
		}
		case '{':{
			advance();
			return prev = Token(OBRACE);
		}
		case '}':{
			advance();
			return prev = Token(CBRACE);
		}
		case '(':{
			advance();
			return prev = Token(OPAREN);
		}
		case ')':{
			advance();
			return prev = Token(CPAREN);
		}
		case ';':{
			advance();
			return prev = Token(SEMI_COLON);
		}
		case EOF:{
			return prev = Token(END);
		}
		default: {
			string *value = new string();
			if(current >= 'a' && current <= 'z'){
				parseTerminal(*value);
				return prev = Token(TERMINAL, value);
			}else if(current >= 'A' && current <= 'Z'){
				parseNonTerminal(*value);
				return prev = Token(NON_TERMINAL, value);
			}else{
				return prev = Token(UNIDENTIFIED_TOK);
			}
		}
	}
	consumed = false;
	return Token(UNIDENTIFIED_TOK);
}

void Lexer::consume(){
	consumed = true;
}