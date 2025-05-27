#include <iostream>                 
#include <vector>                   
#include <string>                   
#include <sstream>                  
#include <cctype>                
#include <stdexcept>               



// Token types that will be used for parsing the .proto file
enum class TokenType {
    KEYWORD,      // Just looking for message, enum, repeated, optional right now 
    IDENTIFIER,   // These will be field or message names
    STRING,       
    NUMBER,       // for field numbers
    SYMBOL,       
    COMMENT,      // Not handled yet
    EOF_TOKEN     // end of file 
};

//Individual tokens 
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

// Tokenizer class to break .proto input into tokens 
class ProtoTokenizer {
    std::string source_;           // Full .proto
    size_t pos_ = 0;               // Current position in the source .proto
    int line_ = 1;               
    int column_ = 1;               

public:
    ProtoTokenizer(const std::string& source) : source_(source) {}

    // Getting to the next token
    Token NextToken() {
        SkipWhitespace();                            // Skip spaces and newlines

        if (pos_ >= source_.length())                // If we reached the end of file
            return { TokenType::EOF_TOKEN, "", line_, column_ }; //done

        char c = source_[pos_];

        if (std::isalpha(c) || c == '_') return ReadIdentifier();  // will be keywords and identifiers
        if (std::isdigit(c)) return ReadNumber();                 // reading a number
        if (c == '"') return ReadString();                        // if " start of string
        if (ispunct(c)) {                                         // skipping punctuation 
            pos_++;
            column_++;
            return { TokenType::SYMBOL, std::string(1, c), line_, column_ - 1 }; 
        }

        throw std::runtime_error("Unreadable Character in .proto");
    }

private:
    // Method to skip over spaces, tabs, and newlines
    void SkipWhitespace() {
        while (pos_ < source_.size()) { //if we aren't at EOF
            char c = source_[pos_];
            if (c == ' ' || c == '\t' || c == '\r') {
                ++pos_;
                ++column_; //skip
            }
            else if (c == '\n') {
                ++pos_;
                ++line_;
                column_ = 1;
            }
            else {
                break; //Checks then breaks 
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

        std::string word = source_.substr(start, pos_ - start); //We loop characters until we reach a nonletter or '_". The identifier is where we started to the pos minus start. 

        // see if it matches any keywords 
        if (word == "message" || word == "enum" || word == "repeated" || word == "optional") {
            return { TokenType::KEYWORD, word, line_, startCol };
        }
        return { TokenType::IDENTIFIER, word, line_, startCol }; 
    }

    // Reads a number token
    Token ReadNumber() {
        size_t start = pos_;
        int startCol = column_;
        while (pos_ < source_.length() && std::isdigit(source_[pos_])) { //go until we no longer read a number and not EOF
            ++pos_;
            ++column_;
        }
        return { TokenType::NUMBER, source_.substr(start, pos_ - start), line_, startCol };
    }

   
    Token ReadString() {
        int startCol = column_;
        ++pos_; // Don't wanna grab opening quotation 
        ++column_;
        std::ostringstream oss;
        while (pos_ < source_.length() && source_[pos_] != '"') { //Not EOF and not the end of the string marked by " 
            oss << source_[pos_++];
            ++column_;
        }
        ++pos_; // skip closing quote
        ++column_;
        return { TokenType::STRING, oss.str(), line_, startCol };
    }
};




struct Field {
    std::string type;     
    std::string name;     
    int number;           
    bool repeated = false;
    bool optional = false;
};


struct Message {
    std::string name;
    std::vector<Field> fields;
};


struct Enum {
    std::string name;
    std::vector<std::pair<std::string, int>> values; // name = id
};


struct ProtoFile {
    std::vector<Message> messages;
    std::vector<Enum> enums;
};



class ProtoParser {
    ProtoTokenizer tokenizer_;    
    Token current_;               

public:
    ProtoParser(const std::string& source) : tokenizer_(source) {
        current_ = tokenizer_.NextToken();  
    }

    // Going to take entire string .proto and turn it into a C++ ProtoFile object 
    ProtoFile ParseFile() {
        ProtoFile file;

        // looping over the entire file string and bringing everything above together
        while (current_.type != TokenType::EOF_TOKEN) {
            if (current_.value == "message") {
                file.messages.push_back(ParseMessage());   // add message struct
            }
            else if (current_.value == "enum") {
                file.enums.push_back(ParseEnum());         // add enum struct
            }
            else {
                Advance(); // Haven't made structs to deal with other keywords yet
            }
        }
        return file;
    }

private:
    // go to the next token
    void Advance() { current_ = tokenizer_.NextToken(); }

    // Checks for expected token type/val
    void Expect(TokenType type, const std::string& val = "") {
        if (current_.type != type || (!val.empty() && current_.value != val)) {
            throw std::runtime_error("Unexpected token: " + current_.value);
        }
    }

   
    Message ParseMessage() {
        Advance(); // skipping over 'message'
        Expect(TokenType::IDENTIFIER);
        Message msg; //creating message struct
        msg.name = current_.value; //First word should be message name 
        Advance(); // grabbed it so move on

        Expect(TokenType::SYMBOL, "{"); //Checking formatting is right
        Advance(); 

        // we will keep looping until we find the closing brace 
        while (!(current_.type == TokenType::SYMBOL && current_.value == "}")) {
            msg.fields.push_back(ParseField()); //everything inside is saved as a field 
        }

        Advance(); //don't need closing brace 
        return msg; //return message struct 
    }

    // reads and saves a single field line 
    Field ParseField() {
        Field field; //declaring struct field
        if (current_.value == "repeated") {
            field.repeated = true;
            Advance(); 
        }

        if (current_.value == "optional") {
            field.optional = true;
            Advance();
        }

        Expect(TokenType::IDENTIFIER);//type
        field.type = current_.value; 
        Advance();

        Expect(TokenType::IDENTIFIER);//name
        field.name = current_.value;
        Advance();

        Expect(TokenType::SYMBOL, "="); //=
        Advance();

        Expect(TokenType::NUMBER); //Id
        field.number = std::stoi(current_.value);
        Advance();

        if (current_.value == ";") Advance(); 
        return field;
    }

  
    Enum ParseEnum() {
        Advance(); // skipping the word 'enum'
        Expect(TokenType::IDENTIFIER);
        Enum e; //declaring enum struct e
        e.name = current_.value; //word right after enum should be name 
        Advance();

        Expect(TokenType::SYMBOL, "{"); //making sure formatting is good 
        Advance();

        while (!(current_.type == TokenType::SYMBOL && current_.value == "}")) { //Going until we hit the closing bracket
            Expect(TokenType::IDENTIFIER); //name
            std::string name = current_.value;
            Advance();

            Expect(TokenType::SYMBOL, "="); //=
            Advance();

            Expect(TokenType::NUMBER); //Id
            int value = std::stoi(current_.value);
            Advance();

            if (current_.value == ";") Advance();
            e.values.push_back({ name, value });
        }

        Advance(); // skipping the closing bracket 

        return e;
    }
};



int main() {
    // Example proto input from sent repo
    std::string proto = R"(
enum OrderSide
{
    buy = 0;
    sell = 1;
}

enum OrderType
{
    market = 0;
    limit = 1;
    stop = 2;
}

message Order
{
    int32 id = 1;
    string symbol = 2;
    OrderSide side = 3;
    OrderType type = 4;
    double price = 5;
    double volume = 6;
}

message Balance
{
    string currency = 1;
    double amount = 2;
}

message Account
{
    int32 id = 1;
    string name = 2;
    Balance wallet = 3;
    repeated Order orders = 4;
})";

    ProtoParser parser(proto);             
    ProtoFile file = parser.ParseFile();   

    // messages
    std::cout << "Messages:\n";
    for (const auto& msg : file.messages) {
        std::cout << "- " << msg.name << "\n";
        for (const auto& f : msg.fields) {
            std::cout << "  -- " << (f.repeated ? "repeated " : "") << f.type << " " << f.name << " = " << f.number << "\n";
        }
    }



    // enums
    std::cout << "\nEnums:\n";
    for (const auto& e : file.enums) {
        std::cout << "- " << e.name << "\n";
        for (const auto& val : e.values) {
            std::cout << "  -- " << val.first << " = " << val.second << "\n";
        }
    }

    return 0;
}
