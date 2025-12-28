#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <stack>
#include <variant>
#include <cctype>

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

enum VarType
{
	TYPE_INT,
	TYPE_REAL,
	TYPE_STRING
};

struct Token
{
	TokenType type;
	std::string value;
	int line;
	int column;
};

using Value = std::variant<int, double, std::string>;

struct Symbol
{
	std::string name;
	VarType type;
	Value value;
};

class SymbolTable
{
public:
	void addVariable(const std::string& name, VarType type, int line, int column)
	{
		if (symbols.find(name) != symbols.end())
		{
			throw std::runtime_error("Semantic error: variable '" + name +
			                         "' already declared at line " + std::to_string(line) +
			                         ", column " + std::to_string(column));
		}
		Value initialValue;
		switch (type)
		{
			case TYPE_INT: initialValue = 0; break;
			case TYPE_REAL: initialValue = 0.0; break;
			case TYPE_STRING: initialValue = std::string(""); break;
		}
		symbols[name] = {name, type, initialValue};
	}

	Symbol* findVariable(const std::string& name, int line, int column)
	{
		if (symbols.find(name) == symbols.end())
		{
			throw std::runtime_error("Semantic error: undeclared variable '" + name +
			                         "' used at line " + std::to_string(line) +
			                         ", column " + std::to_string(column));
		}
		return &symbols[name];
	}

private:
	std::map<std::string, Symbol> symbols;
};

class Poliz
{
public:
	void addToken(const Token& token)
	{
		expression.push_back(token);
	}

	std::string toString() const
	{
		std::string result;
		for (const auto& token : expression)
		{
			result += token.value + " ";
		}
		return result;
	}

	const std::vector<Token>& getExpression() const
	{
		return expression;
	}

private:
	std::vector<Token> expression;
};

struct Statement
{
	enum Type
	{
		ASSIGN,
		READ,
		WRITE,
		IF,
		WHILE,
		BREAK,
		COMPOUND
	};
	Type type;
	Token identifier; // For ASSIGN, READ
	Poliz expression; // For ASSIGN, WRITE, IF, WHILE
	std::vector<Statement> statements; // For COMPOUND, IF, WHILE
	std::vector<Statement> elseStatements; // For IF
};

class Runtime
{
public:
	Runtime(SymbolTable& symTable) : symbolTable(symTable) {}

	Value evaluatePoliz(const Poliz& poliz, int line)
	{
		std::stack<Value> valueStack;
		for (const auto& token : poliz.getExpression())
		{
			if (token.type == IDENTIFIER)
			{
				Symbol* sym = symbolTable.findVariable(token.value, line, token.column);
				valueStack.push(sym->value);
			}
			else if (token.type == INTEGER_LITERAL)
			{
				valueStack.push(std::stoi(token.value));
			}
			else if (token.type == REAL_LITERAL)
			{
				valueStack.push(std::stod(token.value));
			}
			else if (token.type == STRING_LITERAL)
			{
				valueStack.push(token.value);
			}
			else if (token.type == OPERATOR && token.value != "=")
			{
				if (valueStack.size() < 2)
				{
					throw std::runtime_error("Runtime error: insufficient operands at line " + std::to_string(line));
				}
				Value b = valueStack.top();
				valueStack.pop();
				Value a = valueStack.top();
				valueStack.pop();
				Value result = applyOperator(a, b, token.value, line);
				valueStack.push(result);
			}
		}
		if (valueStack.empty())
		{
			throw std::runtime_error("Runtime error: empty expression at line " + std::to_string(line));
		}
		return valueStack.top();
	}

	void interpret(const std::vector<Statement>& program)
	{
		for (size_t i = 0; i < program.size(); ++i)
		{
			if (!interpretStatement(program[i]))
			{
				break;
			}
		}
	}

private:
	SymbolTable& symbolTable;
	bool breakFlag = false;

	Value applyOperator(const Value& a, const Value& b, const std::string& op, int line)
	{
		if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b))
		{
			int ia = std::get<int>(a);
			int ib = std::get<int>(b);
			if (op == "+") return ia + ib;
			if (op == "-") return ia - ib;
			if (op == "*") return ia * ib;
			if (op == "/")
			{
				if (ib == 0)
					throw std::runtime_error("Runtime error: division by zero at line " + std::to_string(line));
				return ia / ib;
			}
			if (op == "==") return ia == ib;
			if (op == "!=") return ia != ib;
			if (op == "<")  return ia <  ib;
			if (op == "<=") return ia <= ib;
			if (op == ">")  return ia >  ib;
			if (op == ">=") return ia >= ib;

		}
		else if ((std::holds_alternative<double>(a) || std::holds_alternative<int>(a)) &&
		         (std::holds_alternative<double>(b) || std::holds_alternative<int>(b)))
		{
			double da = std::holds_alternative<double>(a) ? std::get<double>(a) : std::get<int>(a);
			double db = std::holds_alternative<double>(b) ? std::get<double>(b) : std::get<int>(b);
			if (op == "+") return da + db;
			if (op == "-") return da - db;
			if (op == "*") return da * db;
			if (op == "/")
			{
				if (db == 0.0)
					throw std::runtime_error("Runtime error: division by zero at line " + std::to_string(line));
				return da / db;
			}
		}
		else if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b))
		{
			if (op == "+") return std::get<std::string>(a) + std::get<std::string>(b);
		}
		throw std::runtime_error("Runtime error: type mismatch for operator " + op + " at line " + std::to_string(line));
	}

	bool interpretStatement(const Statement& stmt)
	{
		switch (stmt.type)
		{
			case Statement::ASSIGN:
			{
				Value result = evaluatePoliz(stmt.expression, stmt.identifier.line);
				Symbol* sym = symbolTable.findVariable(stmt.identifier.value, stmt.identifier.line, stmt.identifier.column);
				if ((std::holds_alternative<int>(result) && sym->type == TYPE_INT) ||
				    (std::holds_alternative<double>(result) && (sym->type == TYPE_REAL || sym->type == TYPE_INT)) ||
				    (std::holds_alternative<std::string>(result) && sym->type == TYPE_STRING))
				{
					sym->value = result;
				}
				else
				{
					throw std::runtime_error("Runtime error: type mismatch in assignment at line " +
					                         std::to_string(stmt.identifier.line));
				}
				return true;
			}
			case Statement::READ:
			{
				Symbol* sym = symbolTable.findVariable(stmt.identifier.value, stmt.identifier.line, stmt.identifier.column);
				std::string input;
				std::cout << "Input for " << stmt.identifier.value << ": ";
				std::getline(std::cin, input);
				try
				{
					if (sym->type == TYPE_INT)
						sym->value = std::stoi(input);
					else if (sym->type == TYPE_REAL)
						sym->value = std::stod(input);
					else
						sym->value = input;
				}
				catch (...)
				{
					throw std::runtime_error("Runtime error: invalid input for type at line " +
					                         std::to_string(stmt.identifier.line));
				}
				return true;
			}
			case Statement::WRITE:
			{
				Value result = evaluatePoliz(stmt.expression, stmt.expression.getExpression()[0].line);
				if (std::holds_alternative<int>(result))
					std::cout << std::get<int>(result) << std::endl;
				else if (std::holds_alternative<double>(result))
					std::cout << std::get<double>(result) << std::endl;
				else
					std::cout << std::get<std::string>(result) << std::endl;
				return true;
			}
			case Statement::IF:
			{
				Value condition = evaluatePoliz(stmt.expression, stmt.expression.getExpression()[0].line);
				bool condTrue = false;
				if (std::holds_alternative<int>(condition))
					condTrue = std::get<int>(condition) != 0;
				else if (std::holds_alternative<double>(condition))
					condTrue = std::get<double>(condition) != 0.0;
				else
					throw std::runtime_error("Runtime error: condition must be numeric at line " +
					                         std::to_string(stmt.expression.getExpression()[0].line));
				if (condTrue)
				{
					for (const auto& s : stmt.statements)
					{
						if (!interpretStatement(s))
							return false;
					}
				}
				else
				{
					for (const auto& s : stmt.elseStatements)
					{
						if (!interpretStatement(s))
							return false;
					}
				}
				return true;
			}
			case Statement::WHILE:
			{
				while (true)
				{
					Value condition = evaluatePoliz(stmt.expression, stmt.expression.getExpression()[0].line);
					bool condTrue = false;
					if (std::holds_alternative<int>(condition))
						condTrue = std::get<int>(condition) != 0;
					else if (std::holds_alternative<double>(condition))
						condTrue = std::get<double>(condition) != 0.0;
					else
						throw std::runtime_error("Runtime error: condition must be numeric at line " +
						                         std::to_string(stmt.expression.getExpression()[0].line));
					if (!condTrue)
						break;
					for (const auto& s : stmt.statements)
					{
						if (!interpretStatement(s))
						{
							breakFlag = false;
							return true;
						}
					}
				}
				return true;
			}
			case Statement::BREAK:
			{
				breakFlag = true;
				return false;
			}
			case Statement::COMPOUND:
			{
				for (const auto& s : stmt.statements)
				{
					if (!interpretStatement(s))
						return false;
				}
				return true;
			}
		}
		return true;
	}
};

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

class Parser
{
public:
	Parser(Lexer& lexer);
	void parseProgram();
	void interpret();
private:
	Lexer& lexer;
	Token currentToken;
	SymbolTable symbolTable;
	std::vector<Statement> program;

	void eat(TokenType type, const std::string& value = "");
	void parseDeclarations();
	void parseDeclaration();
	void parseVariableList(const std::string& type);
	void parseVariable(const std::string& type);
	void parseStatements();
	Statement parseStatement();
	Statement parseIfStatement();
	Statement parseWhileStatement();
	Statement parseIOStatement();
	Statement parseCompoundStatement();
	Poliz parseExpression();
	Poliz parseRelExpr();
	Poliz parseTerm();
	Poliz parseAddExpr();
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
	Token ident = currentToken;
	eat(IDENTIFIER);
	VarType varType = (type == "int") ? TYPE_INT : (type == "real") ? TYPE_REAL : TYPE_STRING;
	symbolTable.addVariable(ident.value, varType, ident.line, ident.column);
	if (currentToken.type == OPERATOR && currentToken.value == "=")
	{
		eat(OPERATOR, "=");
		Poliz poliz = parseExpression();
		if (currentToken.type == STRING_LITERAL && varType != TYPE_STRING)
		{
			throw std::runtime_error("Semantic error: cannot assign string to " + type +
			                         " at line " + std::to_string(currentToken.line));
		}
		if (currentToken.type == INTEGER_LITERAL && varType == TYPE_STRING)
		{
			throw std::runtime_error("Semantic error: cannot assign integer to string at line " +
			                         std::to_string(currentToken.line));
		}
		if (currentToken.type == REAL_LITERAL && varType == TYPE_STRING)
		{
			throw std::runtime_error("Semantic error: cannot assign real to string at line " +
			                         std::to_string(currentToken.line));
		}
		Statement stmt;
		stmt.type = Statement::ASSIGN;
		stmt.identifier = ident;
		stmt.expression = poliz;
		program.push_back(stmt);
	}
}

void Parser::parseStatements()
{
	while (currentToken.type != END_OF_FILE)
	{
		program.push_back(parseStatement());
	}
}

Statement Parser::parseStatement()
{
	if (currentToken.type == KEYWORD)
	{
		if (currentToken.value == "if")
		{
			return parseIfStatement();
		}
		else if (currentToken.value == "while")
		{
			return parseWhileStatement();
		}
		else if (currentToken.value == "read" || currentToken.value == "write")
		{
			return parseIOStatement();
		}
		else if (currentToken.value == "break")
		{
			Statement stmt;
			stmt.type = Statement::BREAK;
			eat(KEYWORD, "break");
			eat(SEPARATOR, ";");
			return stmt;
		}
		else
		{
			throw std::runtime_error("Syntax error: unexpected keyword '" + currentToken.value +
			                         "' at line " + std::to_string(currentToken.line));
		}
	}
	else if (currentToken.type == SEPARATOR && currentToken.value == "{")
	{
		return parseCompoundStatement();
	}
	else if (currentToken.type == IDENTIFIER)
	{
		Token ident = currentToken;
		eat(IDENTIFIER);
		symbolTable.findVariable(ident.value, ident.line, ident.column);
		if (currentToken.type == OPERATOR && currentToken.value == "=")
		{
			eat(OPERATOR, "=");
			Poliz poliz = parseExpression();
			if (currentToken.type == STRING_LITERAL &&
			    symbolTable.findVariable(ident.value, ident.line, ident.column)->type != TYPE_STRING)
			{
				throw std::runtime_error("Semantic error: cannot assign string to non-string at line " +
				                         std::to_string(currentToken.line));
			}
			if (currentToken.type == INTEGER_LITERAL &&
			    symbolTable.findVariable(ident.value, ident.line, ident.column)->type == TYPE_STRING)
			{
				throw std::runtime_error("Semantic error: cannot assign integer to string at line " +
				                         std::to_string(currentToken.line));
			}
			if (currentToken.type == REAL_LITERAL &&
			    symbolTable.findVariable(ident.value, ident.line, ident.column)->type == TYPE_STRING)
			{
				throw std::runtime_error("Semantic error: cannot assign real to string at line " +
				                         std::to_string(currentToken.line));
			}
			Statement stmt;
			stmt.type = Statement::ASSIGN;
			stmt.identifier = ident;
			stmt.expression = poliz;
			eat(SEPARATOR, ";");
			return stmt;
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

Statement Parser::parseIfStatement()
{
	Statement stmt;
	stmt.type = Statement::IF;
	eat(KEYWORD, "if");
	eat(SEPARATOR, "(");
	stmt.expression = parseExpression();
	eat(SEPARATOR, ")");
	stmt.statements.push_back(parseStatement());
	if (currentToken.type == KEYWORD && currentToken.value == "else")
	{
		eat(KEYWORD, "else");
		stmt.elseStatements.push_back(parseStatement());
	}
	return stmt;
}

Statement Parser::parseWhileStatement()
{
	Statement stmt;
	stmt.type = Statement::WHILE;
	eat(KEYWORD, "while");
	eat(SEPARATOR, "(");
	stmt.expression = parseExpression();
	eat(SEPARATOR, ")");
	stmt.statements.push_back(parseStatement());
	return stmt;
}

Statement Parser::parseIOStatement()
{
	Statement stmt;
	if (currentToken.value == "read")
	{
		stmt.type = Statement::READ;
		eat(KEYWORD, "read");
		eat(SEPARATOR, "(");
		stmt.identifier = currentToken;
		eat(IDENTIFIER);
		symbolTable.findVariable(stmt.identifier.value, stmt.identifier.line, stmt.identifier.column);
		eat(SEPARATOR, ")");
		eat(SEPARATOR, ";");
	}
	else if (currentToken.value == "write")
	{
		stmt.type = Statement::WRITE;
		eat(KEYWORD, "write");
		eat(SEPARATOR, "(");
		stmt.expression = parseExpression();
		std::cout << "POLIZ for write: " << stmt.expression.toString() << std::endl;
		eat(SEPARATOR, ")");
		eat(SEPARATOR, ";");
	}
	return stmt;
}

Statement Parser::parseCompoundStatement()
{
	Statement stmt;
	stmt.type = Statement::COMPOUND;
	eat(SEPARATOR, "{");
	while (currentToken.type != SEPARATOR || currentToken.value != "}")
	{
		stmt.statements.push_back(parseStatement());
	}
	eat(SEPARATOR, "}");
	return stmt;
}

Poliz Parser::parseAddExpr()
{
	Poliz result;
	std::stack<Token> opStack;
	Poliz term = parseTerm();
	for (const auto& token : term.getExpression())
	{
		result.addToken(token);
	}

	while (currentToken.type == OPERATOR &&
	       (currentToken.value == "+" || currentToken.value == "-"))
	{
		while (!opStack.empty() &&
		       opStack.top().type == OPERATOR &&
		       (opStack.top().value == "+" || opStack.top().value == "-" ||
		        opStack.top().value == "*" || opStack.top().value == "/"))
		{
			result.addToken(opStack.top());
			opStack.pop();
		}
		opStack.push(currentToken);
		eat(OPERATOR);
		term = parseTerm();
		for (const auto& token : term.getExpression())
		{
			result.addToken(token);
		}
	}

	while (!opStack.empty())
	{
		result.addToken(opStack.top());
		opStack.pop();
	}

	return result;
}

Poliz Parser::parseExpression()
{
    return parseRelExpr();
}

Poliz Parser::parseRelExpr()
{
    Poliz result = parseAddExpr();

    while (currentToken.type == OPERATOR &&
          (currentToken.value == "==" || currentToken.value == "!=" ||
           currentToken.value == "<"  || currentToken.value == "<=" ||
           currentToken.value == ">"  || currentToken.value == ">="))
    {
        Token op = currentToken;
        eat(OPERATOR);                     

        Poliz rhs = parseAddExpr();        
        for (auto& t : rhs.getExpression())
            result.addToken(t);            

        result.addToken(op);              
    }
    return result;
}



Poliz Parser::parseTerm()
{
	Poliz result;
	std::stack<Token> opStack;

	if (currentToken.type == IDENTIFIER)
	{
		symbolTable.findVariable(currentToken.value, currentToken.line, currentToken.column);
		result.addToken(currentToken);
		eat(IDENTIFIER);
	}
	else if (currentToken.type == INTEGER_LITERAL)
	{
		result.addToken(currentToken);
		eat(INTEGER_LITERAL);
	}
	else if (currentToken.type == REAL_LITERAL)
	{
		result.addToken(currentToken);
		eat(REAL_LITERAL);
	}
	else if (currentToken.type == STRING_LITERAL)
	{
		result.addToken(currentToken);
		eat(STRING_LITERAL);
	}
	else if (currentToken.type == SEPARATOR && currentToken.value == "(")
	{
		eat(SEPARATOR, "(");
		Poliz expr = parseExpression();
		for (const auto& token : expr.getExpression())
		{
			result.addToken(token);
		}
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
		while (!opStack.empty() &&
		       opStack.top().type == OPERATOR &&
		       (opStack.top().value == "*" || opStack.top().value == "/"))
		{
			result.addToken(opStack.top());
			opStack.pop();
		}
		opStack.push(currentToken);
		eat(OPERATOR);

		if (currentToken.type == IDENTIFIER)
		{
			symbolTable.findVariable(currentToken.value, currentToken.line, currentToken.column);
			result.addToken(currentToken);
			eat(IDENTIFIER);
		}
		else if (currentToken.type == INTEGER_LITERAL)
		{
			result.addToken(currentToken);
			eat(INTEGER_LITERAL);
		}
		else if (currentToken.type == REAL_LITERAL)
		{
			result.addToken(currentToken);
			eat(REAL_LITERAL);
		}
		else if (currentToken.type == SEPARATOR && currentToken.value == "(")
		{
			eat(SEPARATOR, "(");
			Poliz expr = parseExpression();
			for (const auto& token : expr.getExpression())
			{
				result.addToken(token);
			}
			eat(SEPARATOR, ")");
		}
		else
		{
			throw std::runtime_error("Syntax error: expected term after operator at line " +
			                         std::to_string(currentToken.line));
		}
	}

	while (!opStack.empty())
	{
		result.addToken(opStack.top());
		opStack.pop();
	}

	return result;
}

void Parser::interpret()
{
	Runtime runtime(symbolTable);
	runtime.interpret(program);
}

// Main function
int main(int argc, char *argv[])
{
	try
	{
		Lexer lexer(argv[1]);
		Parser parser(lexer);
		parser.parseProgram();
		std::cout << "Parsing completed successfully." << std::endl;
		parser.interpret();
		std::cout << "Execution completed successfully." << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}