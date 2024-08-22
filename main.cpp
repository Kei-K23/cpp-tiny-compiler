#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <functional>

// Token structure
struct Token
{
    std::string type;
    std::string value;
};

// AST Node structure
struct Node
{
    virtual ~Node() = default;
};

// Number literal
struct NumberLiteral : Node
{
    std::string value;
};

// String literal
struct StringLiteral : Node
{
    std::string value;
};

// Function call
struct CallExpression : Node
{
    std::string name;
    std::vector<std::shared_ptr<Node>> params;
};

// Expression
struct ExpressionStatement : Node
{
    std::shared_ptr<CallExpression> expression;
};

// Program
struct Program : Node
{
    std::vector<std::shared_ptr<Node>> body;
};

std::vector<Token> tokenizer(const std::string &input)
{
    size_t current = 0;
    std::vector<Token> tokens;

    while (current < input.length())
    {
        char ch = input[current];
        // Paren
        if (ch == '(')
        {
            tokens.push_back({"paren", "("});
            current++;
            continue;
        }
        if (ch == ')')
        {
            tokens.push_back({"paren", ")"});
            current++;
            continue;
        }

        // Whitespace
        if (isspace(ch))
        {
            // Skip the whitespace. Does not store in the vector
            current++;
            continue;
        }

        // Number literal
        if (isdigit(ch))
        {
            std::string value = "";
            while (isdigit(ch))
            {
                value += ch;
                // Get next character
                ch = input[++current];
            }
            tokens.push_back({"number", value});
            continue;
        }

        // String literal
        if (ch == '"')
        {
            std::string value;
            // Skip the open '"' and only store actual string block
            ch = input[++current];
            // Loop through the end '"'
            while (ch != '"')
            {
                value += ch;
                ch = input[++current];
            }
            // Skip the end '"' and only store actual string block
            current++;
            tokens.push_back({"string", value});
            continue;
        }

        // Keyword like add, subtract, concat
        if (isalpha(ch))
        {
            std::string value;
            while (isalpha(ch))
            {
                value += ch;
                ch = input[++current];
            }
            tokens.push_back({"name", value});
            continue;
        }

        throw std::runtime_error("Unknow character: " + std::string(1, ch));
    }

    return tokens;
}

// Function initialization
std::shared_ptr<Node> parseExpression(std::vector<Token> &tokens, size_t &current);

std::shared_ptr<Program> parser(std::vector<Token> &tokens)
{
    size_t current = 0;
    auto ast = std::make_shared<Program>();

    while (current < tokens.size())
    {
        ast->body.push_back(parseExpression(tokens, current));
    }

    return ast;
}

// Actual implementation for parseExpression
std::shared_ptr<Node> parseExpression(std::vector<Token> &tokens, size_t &current)
{
    Token token = tokens[current];

    // Number token
    if (token.type == "number")
    {
        current++;
        auto node = std::make_shared<NumberLiteral>();
        node->value = token.value;
        return node;
    }

    if (token.type == "string")
    {
        current++;
        auto node = std::make_shared<StringLiteral>();
        node->value = token.value;
        return node;
    }

    if (token.type == "paren" && token.value == "(")
    {
        token = tokens[++current];
        auto node = std::make_shared<CallExpression>();
        node->name = token.value;

        token = tokens[++current];
        while (token.type != "paren" || (token.type == "paren" && token.value != ")"))
        {
            node->params.push_back(parseExpression(tokens, current));
            token = tokens[current];
        }
        current++;
        return node;
    }

    throw std::runtime_error("Unexpected token type: " + token.type);
}

// Updated compiler function
void compiler(const std::string &input)
{
    auto tokens = tokenizer(input);
    auto ast = parser(tokens);
}

int main(void)
{
    compiler("(add 2 (subtract 4 2 1))");
}