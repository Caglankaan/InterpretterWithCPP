#include "parser.h"

std::unordered_map<TokenType, int> unordered_precedences = {{EQ,EQUALS},{NOT_EQ,EQUALS},{LT,LESSGREATER},{GT,LESSGREATER},
                                        {PLUS,SUM},{MINUS,SUM},{SLASH,PRODUCT},{ASTERISK,PRODUCT}, {LPAREN, CALL}, {LBRACKET, INDEX}};


void nextToken(Parser *p)
{
    p->curToken = p->peekToken;
    p->peekToken = nextToken(p->l);
}

void peekError(Parser *p, TokenType t)
{
    std::string err = "Expected next token to be " + t + " got " + p->peekToken.Type + " instead.";
    p->errors.push_back(err);
}

bool peekTokenIs(Parser *p, TokenType t)
{
    return p->peekToken.Type == t;
}

int peekPrecedence(Parser *p)
{
    if(unordered_precedences[p->peekToken.Type])
    {
        return unordered_precedences[p->peekToken.Type];
    }

    return LOWEST;
}

bool curTokenIs(Parser *p, TokenType t)
{
    return p->curToken.Type == t;
}

int curPrecedence(Parser *p)
{
    if(unordered_precedences[p->curToken.Type])
    {
        return unordered_precedences[p->curToken.Type];
    }

    return LOWEST;
}

bool expectPeek(Parser *p, TokenType t)
{
    if(peekTokenIs(p,t))
    {
        nextToken(p);
        return true;
    }
    else{
        peekError(p, t);
        return false;
    }
}

Node parseInteger(Parser *p)
{
    Node lit;
    lit.token = p->curToken;
    int value;
    try
    {
        value = std::stoi(p->curToken.Literal);
    }
    catch(std::invalid_argument const &e)
    {
        std::cerr << e.what() << '\n';
    }
    
    lit.Value_int = value;
    lit.node_type = "Identifier";
    lit.which_identifier = "IntegerLiteral";
    return lit;
}

void noPrefixParseFnError(Parser *p, TokenType t)
{
    p->errors.push_back("No prefix parse function for "+t+" found");
}

Node parseIdentifier(Parser *p)
{
    Node i;
    i.token = p->curToken;
    i.Value = p->curToken.Literal;
    i.node_type = "Identifier";
    return i;
}

Node parseExpression(Parser *p, int precedence)
{
    Node (*prefix)(Parser *p);
    Node defaultIdentifier;
    std::function<Node(Parser *p)> prefix_func_ptr;
    std::function<Node(Parser *p, Node *left)> infix_func_ptr;
    if(p->prefixParseFns[p->curToken.Type])
    {
        prefix_func_ptr = p->prefixParseFns[p->curToken.Type];

        defaultIdentifier = prefix_func_ptr(p);
        //defaultIdentifier.which_identifier = "PrefixExpression";
        while(!peekTokenIs(p, SEMICOLON) && precedence < peekPrecedence(p))
        {
            if(p->infixParseFns[p->peekToken.Type])
            {
                infix_func_ptr = p->infixParseFns[p->peekToken.Type];
                nextToken(p);

                Node *secondIdentifier = new Node(defaultIdentifier);

                defaultIdentifier = infix_func_ptr(p, secondIdentifier);
                //defaultIdentifier.which_identifier = "InfixExpression";
            }
            else
            {
                Node *identifier_null = new Node();
                return *identifier_null;
            }
            
        }
        return defaultIdentifier;
    }
    noPrefixParseFnError(p, p->curToken.Type);

    Node *identifier_null = new Node();
    return *identifier_null;
    
}

Node parsePrefixExpression(Parser *p)
{
    Node i;
    i.token = p->curToken;
    i.Operator = p->curToken.Type;
    
    nextToken(p);

    i.Right_identifier = new Node(parseExpression(p, PREFIX));
    i.which_identifier = "PrefixExpression";
    i.node_type = "Identifier";
    return i;
}

Node parseInfixExpression(Parser *p, Node *left)
{
    Node i;
    i.token = p->curToken;
    i.Operator = p->curToken.Type;

    i.Left_identifier = left;
    
    int precedence = curPrecedence(p);
    nextToken(p);
    
    i.Right_identifier = new Node(parseExpression(p, precedence));
    i.which_identifier = "InfixExpression";
    i.node_type = "Identifier";
    return i;
}

Node parseBoolean(Parser *p)
{
    Node i;
    i.token = p->curToken;
    i.Value_bool = curTokenIs(p, TRUE);
    i.node_type = "Boolean";
    i.which_identifier = "Boolean";
    return i;
}

Node parseGroupExpression(Parser *p)
{
    nextToken(p);

    if(!expectPeek(p, RPAREN))
    {
        Node *identifier_null = new Node();
        return *identifier_null;
    }
    return parseExpression(p,LOWEST);
}

Node *parseBlockStatement(Parser *p)
{
    Node *s = new Node();
    s->token = p->curToken;

    nextToken(p);

    while(!curTokenIs(p, RBRACE) && !curTokenIs(p, END_OF_FILE))
    {
        Node *stmt = parseStatement(p); //NULL DÖNDÜ
        if(stmt != NULL)
        {
            s->Node_array.push_back(stmt);

        }
        nextToken(p);
    }
    s->node_type = "Statement";
    s->which_statement = "BlockStatement";
    return s;
}

Node parseIfExpression(Parser *p)
{
    Node *i = new Node();
    i->token = p->curToken;

    Node identifier_null;
    Node *id = &identifier_null;
    id = NULL;
    
    if(!expectPeek(p, LPAREN))
    {
        return identifier_null;
    }

    nextToken(p);
    Node *condition = new Node(parseExpression(p, LOWEST));
    i->Condition_identifier = condition;

    if(!expectPeek(p, RPAREN))
    {
        return identifier_null;
    }

    if(!expectPeek(p, LBRACE))
    {
        return identifier_null;
    }

    //Node *consequence = new Node(parseBlockStatement(p));
    i->Consequence_statement = parseBlockStatement(p);

    if(peekTokenIs(p,ELSE))
    {
        nextToken(p);

        if(!expectPeek(p, LBRACE))
        {

            return identifier_null;
        }
        i->Alternative_statement = parseBlockStatement(p);
    }
    free(id);

    i->which_identifier = "IfExpression";
    i->node_type = "Identifier";
    return *i;
}

Node parseWhileExpression(Parser *p)
{
    Node *i = new Node();
    i->token = p->curToken;

    Node identifier_null;
    Node *id = &identifier_null;
    id = NULL;
    
    if(!expectPeek(p, LPAREN))
    {
        return identifier_null;
    }

    nextToken(p);
    Node *condition = new Node(parseExpression(p, LOWEST));
    i->Condition_identifier = condition;

    if(!expectPeek(p, RPAREN))
    {
        return identifier_null;
    }

    if(!expectPeek(p, LBRACE))
    {
        return identifier_null;
    }

    free(id);
    i->Consequence_statement = parseBlockStatement(p);

    i->which_identifier = "WhileExpression";
    i->node_type = "Identifier";
    return *i;
}

std::vector<Node*> parseFunctionParameters(Parser *p)
{
    std::vector<Node *> params;

    std::vector<Node *> identifier_null;
    std::vector<Node *> *id = &identifier_null;
    id = NULL;

    if(peekTokenIs(p, RPAREN))
    {
        nextToken(p);
        return params;
    }

    nextToken(p);

    Node *i = new Node();
    i->token = p->curToken;
    i->Value = p->curToken.Literal;

    params.push_back(i);

    while(peekTokenIs(p, COMMA))
    {
        nextToken(p);
        nextToken(p);

        Node *ident = new Node();
        ident->token = p->curToken;
        ident->Value = p->curToken.Literal;

        params.push_back(ident);
    }

    if(!expectPeek(p, RPAREN))
    {
        return identifier_null;
    }

    return params;
}


Node parseFunctionLiteral(Parser *p)
{
    Node i ;
    i.token = p->curToken;

    Node identifier_null;
    Node *id = &identifier_null;
    id = NULL;

    if(!expectPeek(p,LPAREN))
    {
        return identifier_null;
    }

    i.Node_array = parseFunctionParameters(p);

    if(!expectPeek(p, LBRACE))
    {
        return identifier_null;
    }
    i.Body_statement = parseBlockStatement(p);
    i.which_identifier = "FunctionLiteral";
    i.node_type = "Identifier";
    free(id);
    return i;
}

Node parseCallExpression(Parser *p, Node *function)
{
    Node *i = new Node();
    i->token = p->curToken;
    i->Function_identifier = function;
    i->Node_array = parseExpressionList(p, RPAREN);
    i->which_identifier = "CallExpression";
    i->node_type = "Identifier";
    return *i;
}

std::vector<Node *> parseCallArguments(Parser *p)
{
    std::vector<Node *> args;

    std::vector<Node *> identifier_null;
    std::vector<Node *> *id = &identifier_null;
    id = NULL;

    if(peekTokenIs(p, RPAREN))
    {
        nextToken(p);
        return args;
    }

    nextToken(p);
    Node *identifier = new Node(parseExpression(p,LOWEST));
    args.push_back(identifier);

    while(peekTokenIs(p, COMMA))
    {
        nextToken(p);
        nextToken(p);
        Node *i = new Node(parseExpression(p,LOWEST));
        args.push_back(i);
    }

    if(!expectPeek(p, RPAREN))
    {
        return identifier_null;
    }

    return args;
}
std::vector<Node *> parseExpressionList(Parser *p, TokenType end)
{
    std::vector<Node *> array;
    std::vector<Node *> array_null;
    std::vector<Node *> *id = &array_null;
    id = NULL;
    if(peekTokenIs(p, end))
    {
        nextToken(p);
        return array;
    }
    nextToken(p);
    Node *newNode = new Node(parseExpression(p,LOWEST));
    array.push_back(newNode);

    while(peekTokenIs(p, COMMA))
    {
        nextToken(p);
        nextToken(p);
        Node *newNode = new Node(parseExpression(p,LOWEST));    
        array.push_back(newNode);
    }

    if(!expectPeek(p, end))
    {
        return array_null;
    }

    return array;
}

Node parseIndexExpression(Parser *p, Node *left)
{
    Node index_null;
    Node *id = &index_null;
    id = NULL;


    Node *index = new Node();
    index->token = p->curToken;
    index->Left_index = left;
    index->node_type = "Identifier";
    index->which_identifier = "IndexExpression";
    
    nextToken(p);

    index->Index = new Node(parseExpression(p, LOWEST));
    //index->Index->which_identifier = "IndexExpression";

    if(!expectPeek(p, RBRACKET))
    {
        return index_null;
    }
    
    return *index;
}

Node parseArrayLiteral(Parser *p)
{
    Node *array = new Node();
    array->token = p->curToken;
    array->node_type = "Identifier";
    array->which_identifier = "ArrayLiteral";
    array->Node_array = parseExpressionList(p, RBRACKET);

    return *array;
}

Node parseHashLiteral(Parser *p)
{
    Node *hash = new Node();
    hash->token = p->curToken;
    hash->node_type = "Identifier";
    hash->which_identifier = "HashLiteral";

    Node hash_null;
    Node *id = &hash_null;
    id = NULL;

    
    while(!peekTokenIs(p, RBRACE))
    {
        nextToken(p);
        Node *key = new Node(parseExpression(p, LOWEST));

        if(!expectPeek(p, COLON))
        {
            return hash_null;
        }
        nextToken(p);
        Node *value = new Node(parseExpression(p, LOWEST));
        hash->Pairs[key] = value;

        if(!peekTokenIs(p, RBRACE) && !expectPeek(p, COMMA))
            return hash_null;
    }

    if(!expectPeek(p, RBRACE))
    {
        return hash_null;
    }

    return *hash;
}

Node parseStringLiteral(Parser *p)
{
    Node *str = new Node();
    str->token = p->curToken;
    str->Value_string = p->curToken.Literal;
    str->node_type = "Identifier";
    str->which_identifier = "StringLiteral";

    return *str;
}

Parser *New(Lexer *l)
{
    std::vector<std::string> empty_errors_arr;
    Parser *p = new Parser();
    p->l = l;
    p->errors = empty_errors_arr;

    Node (*parseIdentifierPtr)(Parser *p);
    Node (*parseIntegerPtr)(Parser *p);
    Node (*parsePrefixExpressionPtr)(Parser *p);
    Node (*parseBooleanPtr)(Parser *p);
    Node (*parseInfixExpressionPtr)(Parser *p, Node *left);
    Node (*parseGroupExpressionPtr)(Parser *p);
    Node (*parseIfExpressionPtr)(Parser *p);
    Node (*parseWhileExpressionPtr)(Parser *p);
    Node (*parseFunctionLiteralPtr)(Parser *p);
    Node (*parseCallExpressionPtr)(Parser *p, Node *function);
    Node (*parseStringLiteralPtr)(Parser *p);
    Node (*parseArrayLiteralPtr)(Parser *p);
    Node (*parseIndexExpressionPtr)(Parser *p, Node *left);
    Node (*parseHashLiteralPtr)(Parser *p);

    parseIdentifierPtr = &parseIdentifier;
    parseIntegerPtr = &parseInteger;
    parsePrefixExpressionPtr = &parsePrefixExpression;
    parseInfixExpressionPtr = &parseInfixExpression;
    parseBooleanPtr = &parseBoolean;
    parseGroupExpressionPtr = &parseGroupExpression;
    parseIfExpressionPtr =  &parseIfExpression;
    parseWhileExpressionPtr =  &parseWhileExpression;
    parseFunctionLiteralPtr = &parseFunctionLiteral;
    parseCallExpressionPtr = &parseCallExpression;
    parseStringLiteralPtr = &parseStringLiteral;
    parseArrayLiteralPtr = &parseArrayLiteral;
    parseIndexExpressionPtr = &parseIndexExpression;
    parseHashLiteralPtr = &parseHashLiteral;

	registerPrefix(p,IDENT, parseIdentifierPtr);
	registerPrefix(p,INT, parseIntegerPtr);
	registerPrefix(p,BANG, parsePrefixExpressionPtr);
	registerPrefix(p,MINUS, parsePrefixExpressionPtr);
	registerPrefix(p,PLUS, parsePrefixExpressionPtr);
	registerPrefix(p,TRUE, parseBooleanPtr);
	registerPrefix(p,FALSE, parseBooleanPtr);
    registerPrefix(p, LPAREN, parseGroupExpressionPtr);
    registerPrefix(p, IF, parseIfExpressionPtr);
    registerPrefix(p, WHILE, parseWhileExpressionPtr);
    registerPrefix(p, FUNCTION, parseFunctionLiteralPtr);
    registerPrefix(p, STRING, parseStringLiteralPtr);
    registerPrefix(p, LBRACKET, parseArrayLiteralPtr);
    registerPrefix(p, LBRACE, parseHashLiteralPtr);
    

    registerInfix(p, PLUS, parseInfixExpressionPtr);
    registerInfix(p, MINUS, parseInfixExpressionPtr);
    registerInfix(p, SLASH, parseInfixExpressionPtr);
    registerInfix(p, ASTERISK, parseInfixExpressionPtr);
    registerInfix(p, EQ, parseInfixExpressionPtr);
    registerInfix(p, NOT_EQ, parseInfixExpressionPtr);
    registerInfix(p, LT, parseInfixExpressionPtr);
    registerInfix(p, GT, parseInfixExpressionPtr);
    registerInfix(p, LPAREN, parseCallExpressionPtr);
    registerInfix(p, LBRACKET, parseIndexExpressionPtr);


    nextToken(p);
    nextToken(p);

    return p;
}

std::vector<std::string> Errors(Parser *p)
{
    return p->errors;
}

Node *parseReturnStatement(Parser *p)
{
    Node *stmt = new Node();
    stmt->token = p->curToken;

    nextToken(p);
    stmt->ReturnValue_identifier = new Node(parseExpression(p, LOWEST));

    if(peekTokenIs(p, SEMICOLON))
    {
        nextToken(p);
    }

    stmt->which_statement = "ReturnStatement";
    stmt->node_type = "Statement";
    return stmt;
}



Node *parseExpressionStatement(Parser *p)
{
    Node *stmt = new Node();
    stmt->token = p->curToken;
    Node *i = new Node(parseExpression(p,LOWEST));
    stmt->Expression_identifier = i;
    stmt->Expression_identifier->node_type = "Identifier";

    if(peekTokenIs(p, SEMICOLON))
    {
        nextToken(p);
    }

    stmt->which_statement = "ExpressionStatement";
    stmt->node_type = "Statement";
    return stmt;
}


Node *parseLetStatement(Parser *p)
{
    Node *stmt = new Node();
    stmt->token = p->curToken;
    Node statement_null;
    Node *st = &statement_null;
    st = NULL;

    if(!expectPeek(p, IDENT))
    {
        return st;
    }

    stmt->Name_identifier = new Node();
    stmt->Name_identifier->Value = p->curToken.Literal;
    stmt->Name_identifier->token = p->curToken;

    if(!expectPeek(p,ASSIGN))
    {
        return st;
    }

    nextToken(p);

    stmt->Value_identifier = new Node(parseExpression(p,LOWEST));

    if(peekTokenIs(p, SEMICOLON))
    {
        nextToken(p);
    }

    return stmt;
}

Node *parseStatement(Parser *p)
{
    if(p->curToken.Type == LET)
    {
        Node *stmt = new Node();
        stmt = parseLetStatement(p);
        if(stmt != NULL)
        {
            stmt->which_statement = "LetStatement";
            stmt->node_type = "Statement";
        }
        return stmt;
    }
    else if(p->curToken.Type == RETURN)
    {
        Node *stmt = new Node();
        stmt = parseReturnStatement(p);
        if(stmt != NULL)
        {
            stmt->which_statement = "ReturnStatement";
            stmt->node_type = "Statement";
        }

        return stmt;
    }
    else if(p->curToken.Type == WHILE)
    {
        Node *stmt = new Node();
        stmt = parseExpressionStatement(p);
        if(stmt != NULL)
        {
            stmt->which_statement = "WhileStatement";
            stmt->node_type = "Statement";
        }
        return stmt;
    }
    else if(p->curToken.Type == BREAK)
    {
        Node *stmt = new Node();
        stmt->token = p->curToken;
        if(peekTokenIs(p, SEMICOLON))
        {
            nextToken(p);
        }
        stmt->which_statement = "BreakStatement";
        stmt->node_type = "Statement";
        return stmt;
    }
    else
    {
        Node *stmt = new Node();
        stmt = parseExpressionStatement(p);
        if(stmt != NULL)
        {
            stmt->which_statement = "ExpressionStatement";
            stmt->node_type = "Statement";
        }
       

        return stmt;
    }
}

Node *ParseProgram(Parser *p)
{  
    Node *program = new Node();
    
    while(p->curToken.Type != END_OF_FILE)
    {
        Node *stmt = parseStatement(p);
        if(stmt != NULL)
        {
            program->Node_array.push_back(stmt);
        }
        free(stmt);

        nextToken(p);
    }
    program->node_type = "Program";
    return program;
}

void registerPrefix(Parser *p, TokenType tokenType, std::function<Node(Parser *p)> fn)
{
    p->prefixParseFns[tokenType] = fn;
}
void registerInfix(Parser *p, TokenType tokenType, std::function<Node(Parser *p, Node *left)> fn)
{
    p->infixParseFns[tokenType] = fn;
}