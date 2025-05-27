#ifndef PROTO_PARSER_H
#define PROTO_PARSER_H

#include <string>
#include <vector>

// ------------------- Tokenizer ----------------------

// Token types used for parsing the .proto file
enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    STRING,
    NUMBER,
    SYMBOL,
    COMMENT,
    EOF_TOKEN
};

// Represents a single token in the source
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

// Responsible for breaking the .proto source into tokens
class ProtoTokenizer {
public:
    ProtoTokenizer(const std::string& source);

    Token NextToken();

private:
    std::string source_;
    size_t pos_;
    int line_;
    int column_;

    void SkipWhitespace();
    Token ReadString();
    Token ReadNumber();
    Token ReadIdentifier();
};

// -------------------- AST Model ----------------------

// Represents a single field inside a message
struct Field {
    std::string type;
    std::string name;
    int number;
    bool repeated = false;
    bool optional = false;
};

// Represents a message definition
struct Message {
    std::string name;
    std::vector<Field> fields;
};

// Represents an enum definition
struct Enum {
    std::string name;
    std::vector<std::pair<std::string, int>> values;
};

// Top-level container for a parsed .proto file
struct ProtoFile {
    std::vector<Message> messages;
    std::vector<Enum> enums;
};

// --------------------- Parser ------------------------

// Parses a tokenized .proto file into a ProtoFile structure
class ProtoParser {
public:
    ProtoParser(const std::string& source);
    ProtoFile ParseFile();

private:
    ProtoTokenizer tokenizer_;
    Token current_;

    void Advance();
    void Expect(TokenType type, const std::string& val = "");

    Message ParseMessage();
    Field ParseField();
    Enum ParseEnum();
};

#endif // PROTO_PARSER_H
