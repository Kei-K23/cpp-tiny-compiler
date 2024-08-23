
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

// AST Node structures
struct Node
{
    virtual ~Node() = default;
};

struct NumberLiteral : Node
{
    std::string value;
};

struct StringLiteral : Node
{
    std::string value;
};

struct CallExpression : Node
{
    std::string name;
    std::vector<std::shared_ptr<Node>> params;
};

struct ExpressionStatement : Node
{
    std::shared_ptr<CallExpression> expression;
};

struct Program : Node
{
    std::vector<std::shared_ptr<Node>> body;
};

// Tokenizer function
std::vector<Token> tokenizer(const std::string &input)
{
    size_t current = 0;
    std::vector<Token> tokens;

    while (current < input.length())
    {
        char ch = input[current];

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

        if (isspace(ch))
        {
            current++;
            continue;
        }

        if (isdigit(ch))
        {
            std::string value = "";
            while (isdigit(ch))
            {
                value += ch;
                ch = input[++current];
            }
            tokens.push_back({"number", value});
            continue;
        }

        if (ch == '"')
        {
            std::string value = "";
            ch = input[++current];
            while (ch != '"')
            {
                value += ch;
                ch = input[++current];
            }
            current++;
            tokens.push_back({"string", value});
            continue;
        }

        if (isalpha(ch))
        {
            std::string value = "";
            while (isalpha(ch))
            {
                value += ch;
                ch = input[++current];
            }
            tokens.push_back({"name", value});
            continue;
        }

        throw std::runtime_error("Unknown character: " + std::string(1, ch));
    }

    return tokens;
}

// Parser function
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

std::shared_ptr<Node> parseExpression(std::vector<Token> &tokens, size_t &current)
{
    Token token = tokens[current];

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

// Traverser function
void traverseNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent,
                  const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &enter,
                  const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &exit);

void traverseArray(const std::vector<std::shared_ptr<Node>> &nodes, std::shared_ptr<Node> parent,
                   const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &enter,
                   const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &exit)
{
    for (const auto &child : nodes)
    {
        traverseNode(child, parent, enter, exit);
    }
}

void traverseNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent,
                  const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &enter,
                  const std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> &exit)
{
    if (enter)
        enter(node, parent);

    if (std::dynamic_pointer_cast<Program>(node))
    {
        traverseArray(std::dynamic_pointer_cast<Program>(node)->body, node, enter, exit);
    }
    else if (std::dynamic_pointer_cast<CallExpression>(node))
    {
        traverseArray(std::dynamic_pointer_cast<CallExpression>(node)->params, node, enter, exit);
    }

    if (exit)
        exit(node, parent);
}

// Transformer function
std::shared_ptr<Program> transformer(std::shared_ptr<Program> ast)
{
    auto newAst = std::make_shared<Program>();

    traverseNode(ast, nullptr, [&](std::shared_ptr<Node> node, std::shared_ptr<Node> parent)
                 {
        if (std::dynamic_pointer_cast<NumberLiteral>(node)) {
            newAst->body.push_back(std::make_shared<NumberLiteral>(*std::dynamic_pointer_cast<NumberLiteral>(node)));
        }

        if (std::dynamic_pointer_cast<StringLiteral>(node)) {
            newAst->body.push_back(std::make_shared<StringLiteral>(*std::dynamic_pointer_cast<StringLiteral>(node)));
        }

        if (std::dynamic_pointer_cast<CallExpression>(node)) {
            auto expression = std::make_shared<CallExpression>();
            expression->name = std::dynamic_pointer_cast<CallExpression>(node)->name;

            for (auto &param : std::dynamic_pointer_cast<CallExpression>(node)->params) {
                expression->params.push_back(param);
            }

            if (parent == nullptr) {
                auto expressionStmt = std::make_shared<ExpressionStatement>();
                expressionStmt->expression = expression;
                newAst->body.push_back(expressionStmt);
            } else {
                newAst->body.push_back(expression);
            }
        } }, nullptr);

    return newAst;
}

// Code generator function
std::string codeGenerator(std::shared_ptr<Node> node)
{
    if (auto number = std::dynamic_pointer_cast<NumberLiteral>(node))
    {
        return number->value;
    }
    else if (auto str = std::dynamic_pointer_cast<StringLiteral>(node))
    {
        return "\"" + str->value + "\"";
    }
    else if (auto call = std::dynamic_pointer_cast<CallExpression>(node))
    {
        std::string args;
        for (size_t i = 0; i < call->params.size(); i++)
        {
            args += codeGenerator(call->params[i]);
            if (i < call->params.size() - 1)
                args += ", ";
        }
        return call->name + "(" + args + ")";
    }
    else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(node))
    {
        return codeGenerator(exprStmt->expression) + ";";
    }

    throw std::runtime_error("Unknown node type");
}

std::string codeGenerator(std::shared_ptr<Program> node)
{
    std::string code;
    for (const auto &n : node->body)
    {
        code += codeGenerator(n) + "\n";
    }
    return code;
}

// Compiler function
std::string compiler(const std::string &input)
{
    auto tokens = tokenizer(input);
    auto ast = parser(tokens);
    auto newAst = transformer(ast);
    return codeGenerator(newAst);
}

int main()
{
    std::string input = "(add 2 (subtract 4 2))";
    std::string output = compiler(input);
    std::cout << output << std::endl;
    return 0;
}
