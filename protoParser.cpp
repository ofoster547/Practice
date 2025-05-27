// protoParser.cpp : Defines the entry point for the application.
//

#include "protoParser.h"

enum class TokenType {
    KEYWORD,      // syntax, package, message, etc.
    IDENTIFIER,   // user-defined names
    STRING,       // "quoted strings"
    NUMBER,       // field numbers
    SYMBOL,       // {, }, =, ;
    COMMENT,      // // or /* */
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

class ProtoTokenizer {
    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;

public:
    ProtoTokenizer(const std::string& source) : source_(source) {}
    Token NextToken();
    void SkipWhitespace();
    Token ReadString();
    Token ReadNumber();
    Token ReadIdentifier();
};

class ProtoParser {
    ProtoTokenizer tokenizer_;
    Token current_token_;

public:
    ProtoParser(const std::string& source) : tokenizer_(source) {
        current_token_ = tokenizer_.NextToken();
    }

    ProtoFile ParseFile();
    Message ParseMessage();
    Field ParseField();
    Enum ParseEnum();

private:
    void Consume(TokenType expected);
    bool Match(TokenType type);
};

struct ProtoFile {
    std::string syntax;
    std::string package;
    std::vector<std::string> imports;
    std::vector<Message> messages;
    std::vector<Enum> enums;
};

struct Field {
    std::string type;
    std::string name;
    int number;
    bool repeated = false;
    bool optional = false;
};
