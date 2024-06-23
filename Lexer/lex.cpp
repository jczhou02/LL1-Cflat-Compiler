#include<bits/stdc++.h>
using namespace std;

/*

Global Variables

    - keywordTrie : a instance of keyword_Trie
    - symbolTable : a instance of class Symbol_Table and object type data-structure


Utils data-structures

    - keyword_node : a class data-type for keyword_Trie data-structure
    - keyword_Trie : a Trie data-structure to keywords
    - Symbol_Table : a class type data-base to store real-time declared variables and provide them id
    - Token : a class to store string-type tokens


Global Functions

    - lexical_analysis() : Lexically analyses a input-stream and returns a vectors of tokens 


Utils

    - vector_to_string() : Functions to convert vector of character's to a string


Serial lexical Analysis utils

    - getKeyword() : Looks if the Lexemme is a Keyword
    - getVariable() : Looks if the Lexemme is a Identifier
    - getParenthesis() : Looks if the lexemme is a Paranthesis
    - getPunctuation() : Looks if the lexemme is a Punctuation
    - getWhitespace() : Looks if the lexemme is a WhiteSpace
    - getRelop() : Looks if the lexemme is a Relop ~ relational operator
    - getNumber() : Looks if the lexemme is a Number


*/


class identifier
{
    public :
    int id; // to store the id of the variable
    string name; // to store the name of the variable

    identifier() // a null identifier
    {
        id=-1;
        name="\0";
    }

    identifier(int id, string name)
    {
        this->id = id;
        this->name = name;
    }
};

class Symbol_Table
{
    public :

    int current_size;
    map<int,identifier> identifiers; // map with 'key : id' and 'value : identifier type object'

    Symbol_Table()
    {
        current_size = 0; // no id's present at initialization
    }

    int insert(string var) // returns valid 'integer' if insertions is successful, -1 if not successful
    {
        bool allocate = true;

        // check if the variable is already declared of not, if not allocate position in map, if yes return -1

        for(auto node : identifiers)
        {

            if(var.compare(node.second.name)==0) // using string::str.compare()
            {allocate=false;break;}
            
        }

        if(allocate==false)
        return -1;

        int id = ++current_size;

        identifiers[id] = identifier(id,var);

        return id;
    }

    void del(int id)
    {
        identifiers.erase(id);
    }

    void pop()
    {
        identifiers.erase(current_size);
        current_size--;
    }

}SymbolTable;

string vector_to_string(vector<char> str)
{
    string to_string(str.begin(),str.end());

    return to_string;
}

class Token // class token to issue tokens - in this case RELOP tokens
{
    public :
    string type;
    string t; // represents the token string for this simple operations
    int startIndex; // Start index in the input string
    int length; // Length of the lexeme

    Token(string type, string lexeme, int startIndex, int length)
        : t(type +'('+lexeme+')'), startIndex(startIndex), length(length) {}


    // Function to get a formatted string representation of the token
    string toString() const {
        return type + "(" + t + ")";
    }

    Token()
    {
       t = "Not an identifiable Token";
    }

    Token(string token_type)   // keyword case we need to capitalize the first letter
    {
        t = token_type;
        t[0] = toupper(t[0]);
    }

    Token(string token_type ,string token_value) //  name of the operator is provided ** Good for ->  or : or
    {
        cout<<"printing a string: "<<token_value<<endl;
        t = token_type + " , " + token_value;
    }

    Token(string token_type, int token_value)
    {
        t = "< " + token_type + " , " + to_string(token_value) + " >";
    }

    Token(string token_type ,char token_value) // name of the operator is provided
    {
        cout<<"printing a char"<<token_value<<endl;
        t = token_type;
    }
    

};

class keyword_node
{
    public :

        char a;
        map<char,keyword_node*> dict;
        bool isTerminal;

        keyword_node( char a ='\0' )
        {
            this->a = a;
            isTerminal = false; // default the node is not a terminat
        }

};

class keyword_Trie
{
    public :

        keyword_node *root;

        keyword_Trie()
        {
            root = new keyword_node(); // the root is pointing to a null object
        }

        void insert(string keyword) // inserts a string into the Trie data-structure
        {
            keyword_node* node = root;
            for (char ch : keyword) {
                if (!node->dict.count(ch)) {
                    node->dict[ch] = new keyword_node(ch);
                }
                node = node->dict[ch];
            }
            node->isTerminal = true; // Mark the end of a keyword
        }   
        
        bool nfa(string file_input, vector<Token> &tokens,int &i)
        {
            int pos = i;
            bool valid = false;
            bool loop = true;
            char data;

            vector<char> word ;

            keyword_node *temp = root;

            while(loop)
            {

                if(pos<=file_input.length())
                data = file_input[pos];
                else
                {loop = false;data='\0';}


                if(temp->isTerminal && !isalnum(data))
                {
                    valid=true;
                    Token T ;
                    T =  Token(vector_to_string(word));
                    tokens.push_back(T);
                    loop = false;
                }

                if(temp->dict.count(data)) // exist in Trie
                {
                    temp = temp->dict[data];
                    pos++;
                    word.push_back(data);
                }
                else
                {
                    loop = false;
                }
            }

            if(valid==true)
            {
                i=pos;// update i;
            }

            return valid;
        }

}keywordTrie; // keywords is a global Trie type data-structure

void initialize_keywords()
{
    // initializing all the keywords

    vector<string> keywords = {"int", "struct", "nil", "if", "else", "while", "break", "continue", "return", "new", "let", "extern", "fn"};
    for (const auto& keyword : keywords) {
        keywordTrie.insert(keyword);
    }
}

bool getKeyword(string file_input, vector<Token> &tokens,int &i)
{
    int pos = i;

    bool valid = keywordTrie.nfa(file_input,tokens,i);

    return valid;
}


bool getVariable(string file_input, vector<Token> &tokens,int &i)
{

    int pos = i;

    int state = 0;

    bool loop = true;
    bool valid = false;
    string lexeme;
    vector<char> var;

    char c;

    while(loop)
    {

        if(pos<=file_input.length())
        c = file_input[pos];
        else
        {loop = false;c='\0';}

        // we will use c++ std::'bool isalpha(char c)' , 'bool isalnum(char c)'

        switch(state)
        {

            case 0 : if(isalpha(c))
                    {
                        state=1;
                        var.push_back(c);
                        pos++;
                    }

                    else
                    {
                        loop=false;
                    }

                    break;

            case 1 : if(isalnum(c))
                    {
                        state=1;
                        var.push_back(c);
                        pos++;
                    }

                    else
                    {
                        state=2;
                        pos++;
                    }

                    break;

            case 2 : valid = true;loop=false;pos--;
                    
                    /*
                    Steps : 
                    - Issue the token
                    - Enter this int Symbol Table
                    */

                   lexeme = vector_to_string(var);
                   int id = SymbolTable.insert(lexeme);


                   if(id!=-1)
                   {
                       Token T("Id",lexeme,pos-lexeme.length(),lexeme.length());             // Token(string type, string lexeme, int startIndex, int length)
                       tokens.push_back(T);
                   }
                   else
                   {
                       Token T;
                       tokens.push_back(T); // it will return an un-identifiable Token found mean-while because token is already in symbol-table
                   }

                    break;
        }

    }

    if(valid==true)
    i=pos;

    return valid;
} 

//--------------------------------------------------------
/*
bool getVariable(string file_input, vector<Token>& tokens, int& i) {
    if (!isalpha(file_input[i]) && file_input[i] != '_') return false; // Identifiers must start with a letter or underscore

    int start = i;
    while (i < file_input.length() && (isalnum(file_input[i]) || file_input[i] == '_')) {
        i++;
    }

    string identifier = file_input.substr(start, i - start);
    tokens.push_back(Token("Id", identifier, start, i - start));
    return true;
}

bool getNumber(string file_input, vector<Token>& tokens, int& i) {
    if (!isdigit(file_input[i])) return false; // Numbers must start with a digit

    int start = i;
    while (i < file_input.length() && isdigit(file_input[i])) {
        i++;
    }

    string number = file_input.substr(start, i - start);
    tokens.push_back(Token("Num", number, start, i - start));
    return true;
} */
//--------------------------------------------------------




bool getArithmetic(string file_input, vector<Token> &tokens,int &i)
{
    bool valid = false;

    char c = file_input[i];

    Token T;

    switch(c)
    {
        case '+' : valid=true;
                    T = Token("ARITH",c);
                    tokens.push_back(T);

                    break;

        case '-' : valid=true;
                    T = Token("ARITH",c);
                    tokens.push_back(T);

                    break;

        case '*' : valid=true;
                    T = Token("ARITH",c);
                    tokens.push_back(T);

                    break;

        case '/' : valid=true;
                    T = Token("ARITH",c);
                    tokens.push_back(T);

                    break;
    }

    if(valid==true)
    i++;

    return valid;
} 

bool getParenthesis(string file_input, vector<Token> &tokens,int &i)
{
    bool valid = false;

    char c = file_input[i];

    Token T;

    switch(c)
    {
        case '(' : valid=true;
                    T = Token("OpenParen",c);
                    tokens.push_back(T);

                    break;

        case ')' : valid=true;
                    T = Token("CloseParen",c);
                    tokens.push_back(T);

                    break;

        case '{' : valid=true;
                    T = Token("OpenBrace",c);
                    tokens.push_back(T);

                    break;

        case '}' : valid=true;
                    T = Token("CloseBrace",c);
                    tokens.push_back(T);

                    break;

        case '[' : valid=true;
                    T = Token("OpenBracket",c);
                    tokens.push_back(T);

                    break;

        case ']' : valid=true;
                    T = Token("CloseBracket",c);
                    tokens.push_back(T);

                    break;
    }

    if(valid==true)
    i++;

    return valid;
}

/*
bool getPunctuation(string file_input, vector<Token> &tokens,int &i)
{
    bool valid = false;

    char c = file_input[i];

    Token T;

    switch(c)
    {
        case ';' : valid=true;
                    T = Token("PUNC",c);
                    tokens.push_back(T);

                    break;

        case '"' : valid=true;
                    T = Token("PUNC",c);
                    tokens.push_back(T);

                    break;

        case '\'' : valid=true;
                    T = Token("PUNC",c);
                    tokens.push_back(T);

                    break;

    }

    if(valid==true)
    i++;

    return valid;
} */


bool getPunctuation(string file_input, vector<Token> &tokens, int &i) {
    map<char, string> punctuationMap = {
        {':', "Colon"},
        {';', "Semicolon"},
        {',', "Comma"},
        // Add other punctuations as needed
    };

    char currentChar = file_input[i];
    auto it = punctuationMap.find(currentChar);
    if (it != punctuationMap.end()) {
        tokens.push_back(Token(it->second));
        i++;  // Move past the character
        return true;
    }
    return false;
}


bool getWhitespace(string file_input, vector<Token> &tokens,int &i)
{
    int pos = i;

    int state = 0;

    bool loop = true;
    bool valid = false;

    char c;

    while(loop)
    {
        if(pos<=file_input.length())
        c = file_input[pos];
        else
        {loop = false;c='\0';}

        // we will call the c++ std::isspace(char ch) to check if its a white-space or not
        
        switch(state)
        {
        
        case 0 : if(isspace(c))
                    {
                        state = 1;
                        pos++;
                    }

                    else
                    {
                        loop = valid = false;
                    }

                    break;

        case 1 : if(isspace(c))
                    {
                        pos++;
                    }

                    else
                    {
                        state=2;
                        pos++;
                    }

                    break;

        case 2 : valid = true; loop=false; pos--;
                    break;

        }


    }

    if(valid==true)
    i=pos;

    return valid;
} 

/*
bool getRelop(string file_input, vector<Token> &tokens,int &i)
{
    int pos = i ; // saving the current position of the 'i' input stream pointer

    int state = 0; // state is initialized to 0

    bool loop = true; // while we have to loop
    bool valid = false; // valid = True when something is found

    char c;

    while(loop)
    {
        
        if(pos<=file_input.length())
        c = file_input[pos];
        else
        {loop = false;c='\0';}

        switch(state)
        {

        case 0 : if(c=='<')
                {
                    pos++;
                    state = 1;
                }

                else if(c=='=')
                {
                    pos++;
                    state = 5;
                }

                else if(c=='>')
                {
                    pos++;
                    state = 6;
                }

                else
                {
                    valid = false; // the valid turns to False
                    loop = false;
                } 

                break;


        case 1 :if(c=='=')
                {
                    pos++;
                    state = 2;
                }

                else if(c=='>')
                {
                    pos++;
                    state = 3;
                }

                else
                {
                    pos++;
                    state = 4;
                } 

                break;


    
        case 2 :{Token T("RELOP","LE");
                tokens.push_back(T);

                valid = true;
                loop = false;

                break;}


        case 3 : {Token T("RELOP","NE");
                tokens.push_back(T);

                valid = true;
                loop = false;

                break;
                }


        case 4 : {Token T("RELOP","LT");
                tokens.push_back(T);

                valid = true;
                loop = false;

                break;}


        case 5 : {Token T("RELOP","EQ");
                tokens.push_back(T);
 
                valid = true;
                loop = false;

                break;}


        case 6 : if(c=='=')
                {
                    pos++;
                    state = 7;
                }

                else
                {
                    pos++;
                    state = 8;
                } 

                break;


        case 7 : {Token T("RELOP","GE");
                tokens.push_back(T);

                valid = true;
                loop = false;

                break;}


        case 8 :{ Token T("RELOP","GT");
                tokens.push_back(T);

                pos--;
                valid = true;
                loop = false;

                break;}

        // End of the loop and the switch conditional statements
    }
    }

    if(valid==true)
    i = pos; // update pointer to the input in stream

    return valid; // either true or false
} */



bool getNumber(string file_input, vector<Token> &tokens,int &i)
{
    int pos = i ; // saving the current position of the 'i' input stream pointer

    int state = 0; // state is initialized to 0

    bool loop = true; // while we have to loop
    bool valid = false; // valid = True when something is found

    vector<char> number_string;
    string lexeme;

    char c ;

    while(loop)
    {
        if(pos<=file_input.length())
        c = file_input[pos];
        else
        {loop = false;c='\0';}

        switch(state)
        {

        case 0 : if(isdigit(c))
                {
                    pos++;
                    state = 1;
                    number_string.push_back(c);
                }

                else
                {
                    valid = false; // the valid turns to False
                    loop = false;
                } 

                break;


        case 1 :if(isdigit(c))
                {
                    pos++;
                    state = 1;
                    number_string.push_back(c);
                }

                else if(c=='.')
                {
                    pos++;
                    state = 3;
                    number_string.push_back(c);
                }

                else if(c=='e')
                {
                    pos++;
                    state = 6;
                    number_string.push_back(c);
                }

                else
                {
                    pos++;
                    state = 2;
                } 

                break;


        case 2 :{ Token T; // put the number into the token
                lexeme = vector_to_string(number_string);
                T = Token("Num",lexeme,pos-lexeme.length(),lexeme.length());

                tokens.push_back(T);

                pos--;
                valid = true;
                loop = false;

                break;}


         case 3 :if(isdigit(c))
                {
                    pos++;
                    state = 4;
                    number_string.push_back(c);
                }

                else
                {
                    valid = false; // the valid turns to False
                    loop = false;
                } 

                break;


        case 4 :if(isdigit(c))
                {
                    pos++;
                    state = 4;
                    number_string.push_back(c);
                }

                else if(c=='e')
                {
                    pos++;
                    state = 6;
                    number_string.push_back(c);
                }

                else
                {
                    pos++;
                    state = 5;
                } 

                break;


        case 5 : { Token T; // put the number into the token

                T = Token("NUM",vector_to_string(number_string));

                tokens.push_back(T);

                pos--;
                valid = true;
                loop = false;

                break;}


        case 6 :if(isdigit(c))
                {
                    pos++;
                    state = 8;
                    number_string.push_back(c);
                }

                else if(c=='+' || c=='-')
                {
                    pos++;
                    state = 7;
                    number_string.push_back(c);
                }

                else
                {
                    valid = false; // the valid turns to False
                    loop = false;
                } 

                break;


        case 7 :if(isdigit(c))
                {
                    pos++;
                    state = 8;
                    number_string.push_back(c);
                }

                else
                {
                    valid = false; // the valid turns to False
                    loop = false;
                } 

                break;


        case 8 :if(isdigit(c))
                {
                    pos++;
                    state = 8;
                    number_string.push_back(c);
                }

                else
                {
                    pos++;
                    state = 9;
                } 

                break;


        case 9 : { Token T; // put the number into the token
                lexeme = vector_to_string(number_string);
                T = Token("Num",lexeme,pos-lexeme.length(),lexeme.length());

                tokens.push_back(T);

                pos--;
                valid = true;
                loop = false;

                break;}

        // End of the loop and the switch conditional statements
    }
    }

    if(valid==true)
    i = pos; // update pointer to the input in stream

    return valid; // either true or false
} 

bool getRelop(string file_input, vector<Token>& tokens, int& i) {
    int pos = i;
    bool matched = false;

    // Handle two-character relational operators first
    while (pos + 1 < file_input.length()) {
        string twoCharOp = file_input.substr(pos, 2);
        if (twoCharOp == ">=" || twoCharOp == "==" || twoCharOp == "!=") {
            tokens.push_back(Token(twoCharOp == "!=" ? "NotEq" : (twoCharOp == "==" ? "Equal" : "Gte")));
            pos += 2;
            matched = true;
            continue;
        }

        char c = file_input[pos];
        if (c == '>') {
            tokens.push_back(Token("Gt"));
            pos++;
            matched = true;
        } else {
            break; // No more relational operators
        }
    }

    if (matched) {
        i = pos;
        return true;
    }

    return false;
}

// bool getOperatorsAndSpecialChars(string file_input, vector<Token>& tokens, int& i) {
//     static map<string, string> operatorsMap = {
//         {":", "Colon"}, {"->", "Arrow"}, {"<", "Lt"}, {"<=", "Lte"}, {"==", "Equal"}, {"!=", "NotEq"},
//         {">", "Gt"}, {">=", "Gte"}, {"=", "Gets"}, {"+", "Plus"}, {"-", "Dash"},
//         {"*", "Star"}, {"/", "Slash"}, {"(", "OpenParen"}, {")", "CloseParen"},
//         {"{", "OpenBrace"}, {"}", "CloseBrace"}, {"[", "OpenBracket"}, {"]", "CloseBracket"},
//         {"&", "Address"}, {".","Dot"}, {"_","Underscore"}
//     };

//     int pos = i;
//     while (pos < file_input.length()) {
//         string longestMatch;
//         for (int len = min((size_t)3, file_input.length() - pos); len > 0; len--) {
//             string sub = file_input.substr(pos, len);
//             if (operatorsMap.find(sub) != operatorsMap.end()) {
//                 longestMatch = sub;
//                 break;
//             }
//         }
        
//         if (!longestMatch.empty()) {
//             tokens.push_back(Token(operatorsMap[longestMatch]));
//             pos += longestMatch.length();
//         } else {
//             break; // No more operators at this point
//         }
//     }

//     if (pos > i) {
//         i = pos;
//         return true;
//     }

//     return false;
// }




bool skipComments(const string& file_input, int& i) {
    if (i + 1 < file_input.length() && file_input[i] == '/') {
        if (file_input[i + 1] == '/') {
            // Skip C++ style comments
            i += 2; // Move past "//"
            while (i < file_input.length() && file_input[i] != '\n') i++;
            return true;
        } else if (file_input[i + 1] == '*') {
            // Skip C style comments
            i += 2; // Move past "/*"
            while (i + 1 < file_input.length() && !(file_input[i] == '*' && file_input[i + 1] == '/')) i++;
            if (i + 1 < file_input.length()) i += 2; // Move past "*/"
            return true;
        }
    }
    return false;
}


bool skipWhitespace(const string& file_input, int& i) {
    bool found = false;
    while (i < file_input.length() && isspace(file_input[i])) {
        i++;
        found = true;
    }
    return found;
}

void skipCppComment(const string& file_input, int& i) {
    // This function skips C++ style single line comments.
    if (i + 1 < file_input.length() && file_input[i] == '/' && file_input[i + 1] == '/') {
        i += 2; // Skip the "//"
        while (i < file_input.length() && file_input[i] != '\n') {
            i++; // Skip until the end of the line
        }
    }
}

void skipCComment(const string& file_input, int& i) {
    if (i + 1 < file_input.length() && file_input[i] == '/' && file_input[i + 1] == '*') {
        i += 2; // Skip the "/*"
        while (i + 1 < file_input.length()) {
            if (file_input[i] == '*' && file_input[i + 1] == '/') {
                i += 2; // Skip the "*/"
                return;  // Comment closed properly
            }
            i++;
        }
        // If the loop exits and no closing "*/" has been found, we are at the end of the file
        // This handles unclosed comments by simply ending the parsing of this comment.
    }
}









bool getOperatorsAndSpecialChars(string file_input, vector<Token>& tokens, int& i) {
    static map<string, string> operatorsMap = {
        {":", "Colon"}, {"->", "Arrow"}, {"<", "Lt"}, {"<=", "Lte"}, {"==", "Equal"}, {"!=", "NotEq"},
        {">", "Gt"}, {">=", "Gte"}, {"=", "Gets"}, {"+", "Plus"}, {"-", "Dash"},
        {"*", "Star"}, {"/", "Slash"}, {"(", "OpenParen"}, {")", "CloseParen"},
        {"{", "OpenBrace"}, {"}", "CloseBrace"}, {"[", "OpenBracket"}, {"]", "CloseBracket"},
        {"&", "Address"}, {".", "Dot"},{"_","Underscore"}
    };

    bool found = false;
    while (i < file_input.length()) {
        if (file_input[i] == '/' && (i + 1 < file_input.length()) && (file_input[i + 1] == '/' || file_input[i + 1] == '*')) {
            // Immediately check for comments if '/' is found
            if (skipComments(file_input, i)) return true; // Skip the entire comment
        }

        string longestMatch;
        for (int len = min((size_t)3, file_input.length() - i); len > 0; len--) {
            string sub = file_input.substr(i, len);
            if (operatorsMap.find(sub) != operatorsMap.end()) {
                longestMatch = sub;
                break;
            }
        }
        
        if (!longestMatch.empty()) {
            tokens.push_back(Token(operatorsMap[longestMatch]));
            i += longestMatch.length();
            found = true;
        } else {
            break; // No more operators at this point
        }
    }

    return found;
}


void serial_lexical_analysis(const string& file_input, vector<Token>& tokens) {
    int i = 0; // index where the file pointer is at the start

    while (i < file_input.length()) {
        // Skip whitespace and comments before attempting to read the next token
        skipWhitespace(file_input, i); // Skip any leading whitespace

        if (i + 1 < file_input.length() && file_input[i] == '/') {
            if (file_input[i + 1] == '*') {
                skipCComment(file_input, i);  // This function needs to handle EOF properly
                continue;
            } else if (file_input[i + 1] == '/') {
                skipCppComment(file_input, i);
                continue;
            }
        }

        if (getOperatorsAndSpecialChars(file_input, tokens, i)) continue;
        if (getKeyword(file_input, tokens, i)) continue;
        if (getVariable(file_input, tokens, i)) continue;
        if (getNumber(file_input, tokens, i)) continue;
        if (getParenthesis(file_input, tokens, i)) continue;
        if (getPunctuation(file_input, tokens, i)) continue;

        // If no recognized token is found
        if (i < file_input.length()) {
            tokens.push_back(Token("Error"));
            i++; // Increment to avoid getting stuck on the same character
        }
    }
}



void parallel_lexical_analysis(string file_input, vector<Token> &tokens)
{
    int i = 0; // index where the file pointer is at the start - acts like string pointer

    while(file_input[i]!='\0' && i<=file_input.length()) // till the input stream does not end
    {
        int final_pos[8] = {0,0,0,0,0,0,0,0}; // Order : keyword, Variable, Number, Relop, Arithmetic, Parenthesis, Punctuation, Whitespace

        vector<Token> temp_tokens; // will be deleted as the loop iterates

        for(int k=0;k<8;k++)
        {
            final_pos[k] = i;

            switch(k)
            {
                case 0 : getKeyword(file_input,temp_tokens,final_pos[0]); break;

                case 1 : getVariable(file_input,temp_tokens,final_pos[1]); break;

                case 2 : getNumber(file_input,temp_tokens,final_pos[2]); break;

                case 3 : getRelop(file_input,temp_tokens,final_pos[3]); break;

                case 4 : getArithmetic(file_input,temp_tokens,final_pos[4]); break;

                case 5 : getParenthesis(file_input,temp_tokens,final_pos[5]); break;

                case 6 : getPunctuation(file_input,temp_tokens,final_pos[6]); break;

                case 7 : getWhitespace(file_input,temp_tokens,final_pos[7]); break;
            }
        }


        // see what to call finally

        int max_index = -1;
        int max_pos = i;


        for(int k=0;k<8;k++)
        {
            if(final_pos[k]>max_pos)
            {
                max_index = k;
                max_pos = final_pos[k];
            }
        }


        if(max_index == -1) // i.e no token is issued
        {
            Token T; // not an identifiabel token
            tokens.push_back(T);
            i++; // increment i
        }
        else
        {

            // A Token will surely be issued

            // correcting the Symbol Table

            if(final_pos[1]>i)
            SymbolTable.pop(); // popping the identifier if issued from the symbol table

            // Now issuing the original token by using the max_index

            switch(max_index)
            {
                case 0 : getKeyword(file_input,tokens,i); break;

                case 1 : getVariable(file_input,tokens,i);break;

                case 2 : getNumber(file_input,tokens,i); break;

                case 3 : getRelop(file_input,tokens,i); break;

                case 4 : getArithmetic(file_input,tokens,i); break;

                case 5 : getParenthesis(file_input,tokens,i); break;

                case 6 : getPunctuation(file_input,tokens,i); break;

                case 7 : getWhitespace(file_input,tokens,i); break;
            }

        }
        
    }

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    // Read the entire content of the file into a single string
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Initialize necessary structures
    initialize_keywords();  

    vector<Token> tokens;
    serial_lexical_analysis(content, tokens);  // Pass the entire content for analysis

    // Display tokens
    for (const auto& token : tokens) {
        std::cout << token.t << std::endl;
    }

    return 0;
}
