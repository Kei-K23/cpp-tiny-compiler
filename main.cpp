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

void traverseNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &enter, const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &exit);

void traverseArray(
    const std::vector<std::shared_ptr<Node>> &nodes, std::shared_ptr<Node> parent, const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &enter, const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &exit)
{
    for (const auto &child : nodes)
    {
        traverseNode(child, parent, enter, exit);
    }
}

void traverseNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &enter, const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &exit)
{
    if (enter)
    {
        enter(node, parent);
    }

    if (std::dynamic_pointer_cast<Program>(node))
    {
        traverseArray(std::dynamic_pointer_cast<Program>(node)->body, node, enter, exit);
    }
    else if (std::dynamic_pointer_cast<CallExpression>(node))
    {
        traverseArray(std::dynamic_pointer_cast<CallExpression>(node)->params, node, enter, exit);
    }

    if (exit)
    {
        exit(node, parent);
    }
}

// Transformer function
std::shared_ptr<Program> transformer(std::shared_ptr<Program> ast)
{
    auto newAst = std::make_shared<Program>();

    traverseNode(ast, nullptr, [&](std::shared_ptr<Node> node, std::shared_ptr<Node> parent)
                 {
        if (auto number = std::dynamic_pointer_cast<NumberLiteral>(node)) {
            // Create a new number node
            auto newNumber = std::make_shared<NumberLiteral>();
            newNumber->value = number->value;
            
            // If there's a parent, add to its parameters; otherwise, ignore
            if (auto callExpr = std::dynamic_pointer_cast<CallExpression>(parent)) {
                callExpr->params.push_back(newNumber);
            }
        }

        if (auto stringLiteral = std::dynamic_pointer_cast<StringLiteral>(node)) {
            // Create a new string node
            auto newString = std::make_shared<StringLiteral>();
            newString->value = stringLiteral->value;

            // If there's a parent, add to its parameters; otherwise, ignore
            if (auto callExpr = std::dynamic_pointer_cast<CallExpression>(parent)) {
                callExpr->params.push_back(newString);
            }
        }

        if (auto callExpression = std::dynamic_pointer_cast<CallExpression>(node)) {
            // Create a new call expression node
            auto newCall = std::make_shared<CallExpression>();
            newCall->name = callExpression->name;

            // If the parent is null, it's a root expression, so wrap in an ExpressionStatement
            if (!parent) {
                auto expressionStmt = std::make_shared<ExpressionStatement>();
                expressionStmt->expression = newCall;
                newAst->body.push_back(expressionStmt);
            } else if (auto parentCall = std::dynamic_pointer_cast<CallExpression>(parent)) {
                parentCall->params.push_back(newCall);
            }
        } }, nullptr);

    return newAst;
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