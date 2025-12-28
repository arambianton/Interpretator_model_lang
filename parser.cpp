#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <cctype>

// Enum for token types
enum TokenType
{
	KEYWORD,
	IDENTIFIER,
	INTEGER_LITERAL,
	REAL_LITERAL,
	STRING_LITERAL,
	OPERATOR,
	SEPARATOR,
	END_OF_FILE
};

// Token structure
struct Token
{
	TokenType type;
	std::string value;
	int line;
	int column;
};

// Lexer class
class Lexer
{
public:
	Lexer(const std::string& filename);
	Token get_next_token();
	void next();
	char peek();
	void skip_whitespace();
	void skip_comment();
	Token read_identifier_or_keyword();
	Token read_number();
	Token read_string();
	Token read_operator();
	Token read_separator();
private:
	std::ifstream file;
	char current_char;
	int line;
	int column;
};

Lexer::Lexer(const std::string& filename)
	: file(filename), current_char('\0'), line(1), column(0)
{
	if (!file.is_open())
	{
		throw std::runtime_error("Unable to open file: " + filename);
	}
	file >> std::noskipws;
	next();
}

void Lexer::next()
{
	if (file.get(current_char))
	{
		if (current_char == '\n')
		{
			line++;
			column = 0;
		}
		else
		{
			column++;
		}
	}
	else
	{
		current_char = EOF;
	}
}

char Lexer::peek()
{
	char next_char = file.peek();
	return (next_char == EOF) ? EOF : next_char;
}

void Lexer::skip_whitespace()
{
	while (std::isspace(current_char) && current_char != EOF)
	{
		next();
	}
}

void Lexer::skip_comment()
{
	int start_line = line, start_column = column;
	if (current_char == '/' && peek() == '/')
	{
		while (current_char != EOF && current_char != '\n')
		{
			next();
		}
		if (current_char == '\n')
		{
			next();
		}
		return;
	}
	else if (current_char == '/' && peek() == '*')
	{
		next();
		next();
		while (current_char != EOF)
		{
			if (current_char == '*' && peek() == '/')
			{
				next();
				next();
				return;
			}
			next();
		}
		throw std::runtime_error("Unclosed comment starting at line " + std::to_string(start_line) +
		                         ", column " + std::to_string(start_column));
	}
}

Token Lexer::read_identifier_or_keyword()
{
	std::string value;
	int start_line = line, start_column = column;
	while (std::isalnum(current_char) || current_char == '_')
	{
		value += current_char;
		next();
	}
	static const std::set<std::string> keywords =
	{
		"program", "int", "string", "if", "else", "while", "read", "write", "break", "real"
	};
	if (keywords.count(value) > 0)
	{
		return {KEYWORD, value, start_line, start_column};
	}
	return {IDENTIFIER, value, start_line, start_column};
}

Token Lexer::read_number()
{
	std::string value;
	int start_line = line, start_column = column;
	bool is_real = false;

	if (current_char == '+' || current_char == '-')
	{
		value += current_char;
		next();
	}

	while (std::isdigit(current_char))
	{
		value += current_char;
		next();
	}

	if (current_char == '.')
	{
		is_real = true;
		value += current_char;
		next();
		while (std::isdigit(current_char))
		{
			value += current_char;
			next();
		}
	}

	if (current_char == 'e' || current_char == 'E')
	{
		is_real = true;
		value += current_char;
		next();
		if (current_char == '+' || current_char == '-')
		{
			value += current_char;
			next();
		}
		while (std::isdigit(current_char))
		{
			value += current_char;
			next();
		}
	}

	TokenType type = is_real ? REAL_LITERAL : INTEGER_LITERAL;
	return {type, value, start_line, start_column};
}

Token Lexer::read_string()
{
	std::string value;
	int start_line = line, start_column = column;
	next();
	while (current_char != '"' && current_char != EOF)
	{
		value += current_char;
		next();
	}
	if (current_char == '"')
	{
		next();
		return {STRING_LITERAL, value, start_line, start_column};
	}
	throw std::runtime_error("Unclosed string literal at line " + std::to_string(start_line) +
	                         ", column " + std::to_string(start_column));
}

Token Lexer::read_operator()
{
	std::string value;
	int start_line = line, start_column = column;
	value += current_char;
	next();

	if ((value == "+" && current_char == '+') ||
	    (value == "-" && current_char == '-') ||
	    (value == "&" && current_char == '&') ||
	    (value == "|" && current_char == '|') ||
	    (value == "<" && current_char == '=') ||
	    (value == ">" && current_char == '=') ||
	    (value == "=" && current_char == '=') ||
	    (value == "!" && current_char == '='))
	{
		value += current_char;
		next();
	}

	return {OPERATOR, value, start_line, start_column};
}

Token Lexer::read_separator()
{
	std::string value(1, current_char);
	int start_line = line, start_column = column;
	next();
	return {SEPARATOR, value, start_line, start_column};
}

bool is_operator(char c)
{
	static const std::string operators = "+-*/%<>=!&|";
	return operators.find(c) != std::string::npos;
}

bool is_separator(char c)
{
	static const std::string separators = "(){}[],;.:";
	return separators.find(c) != std::string::npos;
}

Token Lexer::get_next_token()
{
	while (current_char != EOF)
	{
		if (std::isspace(current_char))
		{
			skip_whitespace();
			continue;
		}
		if (current_char == '/' && (peek() == '*' || peek() == '/'))
		{
			skip_comment();
			continue;
		}
		if (std::isalpha(current_char) || current_char == '_')
		{
			return read_identifier_or_keyword();
		}
		if (is_operator(current_char))
		{
			return read_operator();
		}
		if (std::isdigit(current_char) || current_char == '+' || current_char == '-' || current_char == '.')
		{
			return read_number();
		}
		if (current_char == '"')
		{
			return read_string();
		}
		if (is_separator(current_char))
		{
			return read_separator();
		}
		throw std::runtime_error("Unexpected character '" + std::string(1, current_char) +
		                         "' at line " + std::to_string(line) + ", column " + std::to_string(column));
	}
	return {END_OF_FILE, "", line, column};
}

std::string token_type_to_string(TokenType type)
{
	switch (type)
	{
		case KEYWORD: return "KEYWORD";
		case IDENTIFIER: return "IDENTIFIER";
		case INTEGER_LITERAL: return "INTEGER_LITERAL";
		case REAL_LITERAL: return "REAL_LITERAL";
		case STRING_LITERAL: return "STRING_LITERAL";
		case OPERATOR: return "OPERATOR";
		case SEPARATOR: return "SEPARATOR";
		case END_OF_FILE: return "END_OF_FILE";
		default: return "UNKNOWN";
	}
}

// Parser class
class Parser
{
public:
	Parser(Lexer& lexer);
	void parseProgram();
private:
	Lexer& lexer;
	Token currentToken;

	void eat(TokenType type, const std::string& value = "");
	void parseDeclarations();
	void parseDeclaration();
	void parseVariableList(const std::string& type);
	void parseVariable(const std::string& type);
	void parseStatements();
	void parseStatement();
	void parseIfStatement();
	void parseWhileStatement();
	void parseIOStatement();
	void parseCompoundStatement();
	void parseExpression();
	void parseTerm();
};

Parser::Parser(Lexer& lexer) : lexer(lexer), currentToken(lexer.get_next_token()) {}

void Parser::eat(TokenType type, const std::string& value)
{
	if (currentToken.type == type && (value.empty() || currentToken.value == value))
	{
		currentToken = lexer.get_next_token();
	}
	else
	{
		std::string expected = value.empty() ? token_type_to_string(type) : value;
		throw std::runtime_error("Syntax error: expected " + expected +
		                         " but got " + token_type_to_string(currentToken.type) +
		                         " with value '" + currentToken.value + "'" +
		                         " at line " + std::to_string(currentToken.line) +
		                         ", column " + std::to_string(currentToken.column));
	}
}

void Parser::parseProgram()
{
	eat(KEYWORD, "program");
	parseDeclarations();
	parseStatements();
	eat(END_OF_FILE);
}

void Parser::parseDeclarations()
{
	while (currentToken.type == KEYWORD &&
	       (currentToken.value == "int" || currentToken.value == "string" || currentToken.value == "real"))
	{
		parseDeclaration();
	}
}

void Parser::parseDeclaration()
{
	std::string type = currentToken.value;
	eat(KEYWORD);
	parseVariableList(type);
	eat(SEPARATOR, ";");
}

void Parser::parseVariableList(const std::string& type)
{
	parseVariable(type);
	while (currentToken.type == SEPARATOR && currentToken.value == ",")
	{
		eat(SEPARATOR, ",");
		parseVariable(type);
	}
}

void Parser::parseVariable(const std::string& type)
{
	eat(IDENTIFIER);
	if (currentToken.type == OPERATOR && currentToken.value == "=")
	{
		eat(OPERATOR, "=");
		parseExpression();
	}
}

void Parser::parseStatements()
{
	while (currentToken.type != END_OF_FILE)
	{
		parseStatement();
	}
}

void Parser::parseStatement()
{
	if (currentToken.type == KEYWORD)
	{
		if (currentToken.value == "if")
		{
			parseIfStatement();
		}
		else if (currentToken.value == "while")
		{
			parseWhileStatement();
		}
		else if (currentToken.value == "read" || currentToken.value == "write")
		{
			parseIOStatement();
		}
		else if (currentToken.value == "break")
		{
			eat(KEYWORD, "break");
			eat(SEPARATOR, ";");
		}
		else
		{
			throw std::runtime_error("Syntax error: unexpected keyword '" + currentToken.value +
			                         "' at line " + std::to_string(currentToken.line));
		}
	}
	else if (currentToken.type == SEPARATOR && currentToken.value == "{")
	{
		parseCompoundStatement();
	}
	else if (currentToken.type == IDENTIFIER)
	{
		eat(IDENTIFIER);
		if (currentToken.type == OPERATOR && currentToken.value == "=")
		{
			eat(OPERATOR, "=");
			parseExpression();
			eat(SEPARATOR, ";");
		}
		else
		{
			throw std::runtime_error("Syntax error: expected '=' after identifier at line " +
			                         std::to_string(currentToken.line));
		}
	}
	else
	{
		throw std::runtime_error("Syntax error: unexpected token " + token_type_to_string(currentToken.type) +
		                         " at line " + std::to_string(currentToken.line));
	}
}

void Parser::parseIfStatement()
{
	eat(KEYWORD, "if");
	eat(SEPARATOR, "(");
	parseExpression();
	eat(SEPARATOR, ")");
	parseStatement();
	if (currentToken.type == KEYWORD && currentToken.value == "else")
	{
		eat(KEYWORD, "else");
		parseStatement();
	}
}

void Parser::parseWhileStatement()
{
	eat(KEYWORD, "while");
	eat(SEPARATOR, "(");
	parseExpression();
	eat(SEPARATOR, ")");
	parseStatement();
}

void Parser::parseIOStatement()
{
	if (currentToken.value == "read")
	{
		eat(KEYWORD, "read");
		eat(SEPARATOR, "(");
		eat(IDENTIFIER);
		eat(SEPARATOR, ")");
		eat(SEPARATOR, ";");
	}
	else if (currentToken.value == "write")
	{
		eat(KEYWORD, "write");
		eat(SEPARATOR, "(");
		parseExpression();
		eat(SEPARATOR, ")");
		eat(SEPARATOR, ";");
	}
}

void Parser::parseCompoundStatement()
{
	eat(SEPARATOR, "{");
	while (currentToken.type != SEPARATOR || currentToken.value != "}")
	{
		parseStatement();
	}
	eat(SEPARATOR, "}");
}

void Parser::parseExpression()
{
	parseTerm();
	while (currentToken.type == OPERATOR &&
	       (currentToken.value == "+" || currentToken.value == "-"))
	{
		eat(OPERATOR);
		parseTerm();
	}
}

void Parser::parseTerm()
{
	if (currentToken.type == IDENTIFIER)
	{
		eat(IDENTIFIER);
	}
	else if (currentToken.type == INTEGER_LITERAL)
	{
		eat(INTEGER_LITERAL);
	}
	else if (currentToken.type == REAL_LITERAL)
	{
		eat(REAL_LITERAL);
	}
	else if (currentToken.type == STRING_LITERAL)
	{
		eat(STRING_LITERAL);
	}
	else if (currentToken.type == SEPARATOR && currentToken.value == "(")
	{
		eat(SEPARATOR, "(");
		parseExpression();
		eat(SEPARATOR, ")");
	}
	else
	{
		throw std::runtime_error("Syntax error: expected expression at line " +
		                         std::to_string(currentToken.line));
	}

	while (currentToken.type == OPERATOR &&
	       (currentToken.value == "*" || currentToken.value == "/"))
	{
		eat(OPERATOR);
		if (currentToken.type == IDENTIFIER)
		{
			eat(IDENTIFIER);
		}
		else if (currentToken.type == INTEGER_LITERAL)
		{
			eat(INTEGER_LITERAL);
		}
		else if (currentToken.type == REAL_LITERAL)
		{
			eat(REAL_LITERAL);
		}
		else if (currentToken.type == SEPARATOR && currentToken.value == "(")
		{
			eat(SEPARATOR, "(");
			parseExpression();
			eat(SEPARATOR, ")");
		}
		else
		{
			throw std::runtime_error("Syntax error: expected term after operator at line " +
			                         std::to_string(currentToken.line));
		}
	}
}

// Main function
int main()
{
	try
	{
		Lexer lexer("input.txt");
		Parser parser(lexer);
		parser.parseProgram();
		std::cout << "Syntax analysis completed successfully." << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}