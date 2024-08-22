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
