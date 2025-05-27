#include <iostream>                 // For console I/O
#include <vector>                   // For std::vector
#include <string>                   // For std::string
#include <sstream>                  // For stringstream (used in string parsing)
#include <cctype>                   // For character functions like isdigit, isalpha
#include <stdexcept>               // For throwing runtime errors

// ------------------- Tokenizer ----------------------

// Token types that will be used for parsing the .proto file
enum class TokenType {
    KEYWORD,      // e.g., message, enum, repeated
    IDENTIFIER,   // e.g., field or message names
    STRING,       // e.g., "value"
    NUMBER,       // e.g., field numbers like = 1
    SYMBOL,       // e.g., { } = ; etc.
    COMMENT,      // Not handled yet
    EOF_TOKEN     // End-of-file marker
};

// Struct to hold individual tokens
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

// Tokenizer class to break input into tokens
class ProtoTokenizer {
    std::string source_;           // Full source code
    size_t pos_ = 0;               // Current position in the source string
    int line_ = 1;                 // Current line (for debugging/errors)
    int column_ = 1;               // Current column

public:
    ProtoTokenizer(const std::string& source) : source_(source) {}

    // Main function to get the next token
    Token NextToken() {
        SkipWhitespace();                            // Skip spaces and newlines

        if (pos_ >= source_.length())                // If end of file
            return {TokenType::EOF_TOKEN, "", line_, column_};

        char c = source_[pos_];

        if (std::isalpha(c) || c == '_') return ReadIdentifier();  // keywords/identifiers
        if (std::isdigit(c)) return ReadNumber();                 // numeric values
        if (c == '"') return ReadString();                        // quoted string
        if (ispunct(c)) {                                         // single character symbol
            pos_++;
            column_++;
            return {TokenType::SYMBOL, std::string(1, c), line_, column_ - 1};
        }

        throw std::runtime_error("Unknown character in input");
    }

private:
    // Skips over spaces, tabs, and newlines
    void SkipWhitespace() {
        while (pos_ < source_.size()) {
            char c = source_[pos_];
            if (c == ' ' || c == '\t' || c == '\r') {
                ++pos_;
                ++column_;
            } else if (c == '\n') {
                ++pos_;
                ++line_;
                column_ = 1;
            } else {
                break;
            }
        }
    }

    // Reads an identifier or keyword
    Token ReadIdentifier() {
        size_t start = pos_;
        int startCol = column_;
        while (pos_ < source_.length() && (std::isalnum(source_[pos_]) || source_[pos_] == '_')) {
            ++pos_;
            ++column_;
        }

        std::string word = source_.substr(start, pos_ - start);

        // Check if it's a keyword
        if (word == "message" || word == "enum" || word == "repeated" || word == "optional") {
            return {TokenType::KEYWORD, word, line_, startCol};
        }
        return {TokenType::IDENTIFIER, word, line_, startCol};
    }

    // Reads a number token
    Token ReadNumber() {
        size_t start = pos_;
        int startCol = column_;
        while (pos_ < source_.length() && std::isdigit(source_[pos_])) {
            ++pos_;
            ++column_;
        }
        return {TokenType::NUMBER, source_.substr(start, pos_ - start), line_, startCol};
    }

    // Reads a string (e.g., "abc")
    Token ReadString() {
        int startCol = column_;
        ++pos_; // skip opening quote
        ++column_;
        std::ostringstream oss;
        while (pos_ < source_.length() && source_[pos_] != '"') {
            oss << source_[pos_++];
            ++column_;
        }
        ++pos_; // skip closing quote
        ++column_;
        return {TokenType::STRING, oss.str(), line_, startCol};
    }
};

// -------------------- AST Model ----------------------

// Struct representing a field in a message
struct Field {
    std::string type;     // e.g., int32, string
    std::string name;     // e.g., age, name
    int number;           // e.g., = 1
    bool repeated = false;
    bool optional = false;
};

// Message structure representing a 'message' block
struct Message {
    std::string name;
    std::vector<Field> fields;
};

// Enum structure representing an 'enum' block
struct Enum {
    std::string name;
    std::vector<std::pair<std::string, int>> values;
};

// Top-level schema container
struct ProtoFile {
    std::vector<Message> messages;
    std::vector<Enum> enums;
};

// --------------------- Parser ------------------------

class ProtoParser {
    ProtoTokenizer tokenizer_;     // Tokenizer instance
    Token current_;                // Current token

public:
    ProtoParser(const std::string& source) : tokenizer_(source) {
        current_ = tokenizer_.NextToken();   // Prime the first token
    }

    // Parses an entire file into a ProtoFile object
    ProtoFile ParseFile() {
        ProtoFile file;

        // Loop until EOF
        while (current_.type != TokenType::EOF_TOKEN) {
            if (current_.value == "message") {
                file.messages.push_back(ParseMessage());   // Parse message blocks
            } else if (current_.value == "enum") {
                file.enums.push_back(ParseEnum());         // Parse enum blocks
            } else {
                Advance(); // Skip unrecognized tokens
            }
        }
        return file;
    }

private:
    // Moves to the next token
    void Advance() { current_ = tokenizer_.NextToken(); }

    // Checks for expected token type (and value optionally)
    void Expect(TokenType type, const std::string& val = "") {
        if (current_.type != type || (!val.empty() && current_.value != val)) {
            throw std::runtime_error("Unexpected token: " + current_.value);
        }
    }

    // Parses a `message` block
    Message ParseMessage() {
        Advance(); // skip 'message'
        Expect(TokenType::IDENTIFIER);
        Message msg;
        msg.name = current_.value;
        Advance(); // move past message name

        Expect(TokenType::SYMBOL, "{");
        Advance(); // move into block

        // Keep parsing fields until we find closing brace
        while (!(current_.type == TokenType::SYMBOL && current_.value == "}")) {
            msg.fields.push_back(ParseField());
        }

        Advance(); // skip '}'
        return msg;
    }

    // Parses a single field line (e.g., repeated int32 age = 1;)
    Field ParseField() {
        Field field;
        if (current_.value == "repeated") {
            field.repeated = true;
            Advance();
        }

        if (current_.value == "optional") {
            field.optional = true;
            Advance();
        }

        Expect(TokenType::IDENTIFIER);
        field.type = current_.value;
        Advance();

        Expect(TokenType::IDENTIFIER);
        field.name = current_.value;
        Advance();

        Expect(TokenType::SYMBOL, "=");
        Advance();

        Expect(TokenType::NUMBER);
        field.number = std::stoi(current_.value);
        Advance();

        if (current_.value == ";") Advance(); // optional semicolon
        return field;
    }

    // Parses an `enum` block
    Enum ParseEnum() {
        Advance(); // skip 'enum'
        Expect(TokenType::IDENTIFIER);
        Enum e;
        e.name = current_.value;
        Advance();

        Expect(TokenType::SYMBOL, "{");
        Advance();

        while (!(current_.type == TokenType::SYMBOL && current_.value == "}")) {
            Expect(TokenType::IDENTIFIER);
            std::string name = current_.value;
            Advance();

            Expect(TokenType::SYMBOL, "=");
            Advance();

            Expect(TokenType::NUMBER);
            int value = std::stoi(current_.value);
            Advance();

            if (current_.value == ";") Advance();
            e.values.push_back({name, value});
        }

        Advance(); // skip '}'
        return e;
    }
};

// --------------------- Main ---------------------------

int main() {
    // Example proto input
    std::string proto = R"(
        message Person {
            string name = 1;
            int32 age = 2;
            repeated string emails = 3;
        }

        enum Status {
            OK = 0;
            ERROR = 1;
        }
    )";

    ProtoParser parser(proto);             // Create parser
    ProtoFile file = parser.ParseFile();   // Parse input

    // Output parsed messages
    std::cout << "Parsed Messages:\n";
    for (const auto& msg : file.messages) {
        std::cout << "- " << msg.name << "\n";
        for (const auto& f : msg.fields) {
            std::cout << "  * " << (f.repeated ? "repeated " : "") << f.type << " " << f.name << " = " << f.number << "\n";
        }
    }

    // Output parsed enums
    std::cout << "\nParsed Enums:\n";
    for (const auto& e : file.enums) {
        std::cout << "- " << e.name << "\n";
        for (const auto& val : e.values) {
            std::cout << "  * " << val.first << " = " << val.second << "\n";
        }
    }

    return 0;
}
