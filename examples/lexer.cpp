#include <simplistic/fsm.h>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <functional>

using namespace simplistic::fsm;

static std::unordered_set<std::string> keywords = {
    "SET",
    "PRINT"
};

/*
	Sample Lexer:
	SET [identifier] = [literal];
	PRINT [identifier];
*/

namespace Tokenizer {
    class Context : public ::Context {
    public:
        explicit Context(const std::string& input);
        bool Finished();
        // Lexer input management
        char GetNextChar();
        void EmitToken(const std::string& tokenType, const std::string& value);
        void RevertOneChar();
        std::vector<std::pair<std::string, std::string>> GetTokens();
        void Finalize();

        std::vector<std::pair<std::string, std::string>> mTokens;

    private:
        std::string mInput;
        size_t mPos;
    };

    // Initial state (starting point)
    class InitialState : public IState {
    public:
        void operator()(IContext* _ctx) override;
        void OnFinishDetected(Context* ctx);
    };

    // Identifier state (e.g., keywords or variable names)
    class TokenState : public IState {
    public:
        explicit TokenState(std::string token);

        void operator()(IContext* _ctx) override;

    private:
        std::string mToken;
    };

    // Number state (handling numeric literals)
    class NumberState : public IState {
    public:
        explicit NumberState(std::string number);

        void operator()(IContext* _ctx) override;

    private:
        std::string mNumber;
    };

    Context::Context(const std::string& input)
        : mInput(input), mPos(0) {
        Apply(std::make_unique<InitialState>());
    }

    bool Context::Finished()
    {
        return (mPos < mInput.size()) == false;
    }

    char Context::GetNextChar() {
        if (mPos < mInput.size()) {
            return mInput[mPos++];
        }
        return '\0'; // EOF
    }

    void Context::EmitToken(const std::string& tokenType, const std::string& value) {
        mTokens.emplace_back(tokenType, value);
    }

    void Context::RevertOneChar() {
        if (mPos > 0) {
            --mPos;
        }
    }

    void InitialState::operator()(IContext* _ctx) {
        Context* ctx = dynamic_cast<Context*>(_ctx);
        if (ctx->Finished())
            OnFinishDetected(ctx);

        char ch = ctx->GetNextChar();
        if (isalpha(ch)) {
            ctx->Apply(std::make_unique<TokenState>(std::string(1, ch)));
        }
        else if (isdigit(ch)) {
            ctx->Apply(std::make_unique<NumberState>(std::string(1, ch)));
        }
        else if (ch == '=') {
            ctx->EmitToken("OPERATOR", std::string(1, ch));
            ctx->Apply(std::make_unique<InitialState>());
        }
        else if (ch == ';')
        {
            ctx->EmitToken("DELIMITER", std::string(1, ch));
            ctx->Apply(std::make_unique<InitialState>());
        }
        else if (isspace(ch)) {
            ctx->Apply(std::make_unique<InitialState>());
        }
        else if (ch == '\0') {
            OnFinishDetected(ctx);
        }
        else {
            std::cout << "Error: Unrecognized character '" << ch << "'" << std::endl;
        }
    }

    void InitialState::OnFinishDetected(Context* ctx)
    {
        ctx->Finalize();
        ctx->ApplyNull();
    }

    TokenState::TokenState(std::string token) : mToken(std::move(token)) {}

    void TokenState::operator()(IContext* _ctx) {
        Context* ctx = dynamic_cast<Context*>(_ctx);
        char ch = ctx->GetNextChar();
        if (isalnum(ch)) {
            mToken += ch;
            ctx->Apply(std::make_unique<TokenState>(mToken));
        }
        else {
            ctx->RevertOneChar();
            ctx->EmitToken(keywords.count(mToken) ? "KEYWORD" : "IDENTIFIER", mToken);
            ctx->Apply(std::make_unique<InitialState>());
        }
    }

    NumberState::NumberState(std::string number) : mNumber(std::move(number)) {}

    void NumberState::operator()(IContext* _ctx) {
        Context* ctx = dynamic_cast<Context*>(_ctx);
        char ch = ctx->GetNextChar();
        if (isdigit(ch)) {
            mNumber += ch;
            ctx->Apply(std::make_unique<NumberState>(mNumber));
        }
        else {
            ctx->RevertOneChar();
            ctx->EmitToken("NUMBER", mNumber);
            ctx->Apply(std::make_unique<InitialState>());
        }
    }

    std::vector<std::pair<std::string, std::string>> Context::GetTokens()
    {
        return mTokens;
    }

    void Context::Finalize()
    {
    }
}

namespace Lexer {
    class Context : public ::Context {
    public:
        using Token = std::pair<std::string, std::string>;

        Context(std::vector<Token> tokens);

        Token GetCurrentToken();

        void AdvanceToken();

        bool IsEndOfTokens() const;

    private:
        std::vector<Token> mTokens;
        size_t mCurrentTokenIndex;
    };

    class ControllerState : public IState {
        void operator()(IContext* ctx) override;
    };

    class KeywordState : public IState {
        void operator()(IContext* ctx) override;
    };

    class Apply : public IState {
    public:
        void operator()(simplistic::fsm::IContext* ctx) override;
    };

    class PrintState : public IState {
    public:
        void operator()(simplistic::fsm::IContext* ctx) override;
    };

    class IdentifierState : public IState {
    public:
        IdentifierState(std::function<void()> onPass);
        void operator()(simplistic::fsm::IContext* ctx) override;
        std::function<void()> mOnPass;
    };

    class OperatorState : public IState {
    public:
        OperatorState(std::function<void()> onPass);
        void operator()(IContext* ctx) override;
        std::function<void()> mOnPass;
    };

    class LiteralState : public IState {
    public:
        void operator()(simplistic::fsm::IContext* ctx) override;
    };

    class DelimiterState : public IState {
    public:
        void operator()(simplistic::fsm::IContext* ctx) override;
    };

    class ErrorState : public IState {
    public:
        void operator()(simplistic::fsm::IContext* ctx) override;
    };

    void Apply::operator()(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.second == "SET") {
            lexerCtx->AdvanceToken();
            ctx->Apply(std::make_unique<IdentifierState>([ctx] {
                ctx->Apply(std::make_unique<OperatorState>([ctx] {
                    ctx->Apply(std::make_unique<LiteralState>());
                    }));
                }));
        }
        else {
            ctx->Apply(std::make_unique<ErrorState>());
        }
    }

    void PrintState::operator()(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.second == "PRINT") {
            lexerCtx->AdvanceToken();
            ctx->Apply(std::make_unique<IdentifierState>([ctx] {
                ctx->Apply(std::make_unique<DelimiterState>());
                }));
        }
        else {
            ctx->Apply(std::make_unique<ErrorState>());
        }
    }

    IdentifierState::IdentifierState(std::function<void()> onPass)
        : mOnPass(onPass)
    {
    }

    void IdentifierState::operator()(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "IDENTIFIER") {
            lexerCtx->AdvanceToken();
            mOnPass();
        }
        else {
            ctx->Apply(std::make_unique<ErrorState>());
        }
    }

    void LiteralState::operator()(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "NUMBER" || token.first == "LITERAL") {
            lexerCtx->AdvanceToken();
            ctx->Apply(std::make_unique<DelimiterState>());
        }
        else {
            ctx->Apply(std::make_unique<ErrorState>());
        }
    }

    void DelimiterState::operator()(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "DELIMITER") {
            lexerCtx->AdvanceToken();
            if (lexerCtx->IsEndOfTokens()) {
                std::cout << "Lexing successful!" << std::endl;
                lexerCtx->ApplyNull(); // End processing
            }
            else {
                ctx->Apply(std::make_unique<ControllerState>());
            }
        }
        else {
            ctx->Apply(std::make_unique<ErrorState>());
        }
    }

    void ErrorState::operator()(IContext* ctx) {
        std::cerr << "Lexer Error: Unexpected token or state." << std::endl;
        // Transition to an error state or halt further processing
        ctx->Apply(0, true); // End processing
    }

    Context::Context(std::vector<Token> tokens)
        : mTokens(std::move(tokens)), mCurrentTokenIndex(0) {
        // Initialize with an appropriate state, e.g., Apply or PrintState
        IContext::Apply(std::make_unique<KeywordState>());
    }
    Context::Token Context::GetCurrentToken() {
        if (mCurrentTokenIndex < mTokens.size()) {
            return mTokens[mCurrentTokenIndex];
        }
        return { "", "" }; // Return empty token if out of range
    }

    void Context::AdvanceToken() {
        if (mCurrentTokenIndex < mTokens.size()) {
            ++mCurrentTokenIndex;
        }
    }
    
    bool Context::IsEndOfTokens() const {
        return mCurrentTokenIndex >= mTokens.size();
    }

    // Inherited via IState

    void ControllerState::operator()(IContext* ctx)
    {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "KEYWORD")
        {
            ctx->Apply(std::make_unique<KeywordState>());
            return;
        }

        ctx->Apply(std::make_unique<ErrorState>());
    }

    void KeywordState::operator()(IContext* ctx)
    {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (keywords.count(token.second))
        {
            if(token.second == "SET")
                return ctx->Apply(std::make_unique<Apply>());
            else if (token.second == "PRINT")
                return ctx->Apply(std::make_unique<PrintState>());
        }

        ctx->Apply(std::make_unique<ErrorState>());        
    }
    OperatorState::OperatorState(std::function<void()> onPass)
        : mOnPass(onPass)
    {
    }

    void OperatorState::operator()(IContext* ctx)
    {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "OPERATOR")
        {
            lexerCtx->AdvanceToken();
            mOnPass();
            return;
        }

        ctx->Apply(std::make_unique<ErrorState>());
    }
}

int main()
{
    Tokenizer::Context tknzrCtx(R"(
        SET aa1 = 24241;
        SET aa2 = 12;SET aa10 = 55;
        SET aa3 = 421;
        SET aa4 = 1424;
        SET aa5 = 1;
        SET aa6 = 24241;
        PRINT aa6;    
        PRINT aa3; PRINT aa6;   
        PRINT aa2;    
    )");
    while (!tknzrCtx.Finished())
        tknzrCtx.operator()();
    auto allTkns = tknzrCtx.GetTokens();
    for(const auto& currTT : allTkns)
        std::cout << "[" << currTT.first << "] : [" << currTT.second << "]" << std::endl;
    Lexer::Context lxrCtx(allTkns);
    while (Context::GetState(lxrCtx.mCurrent) && !lxrCtx.IsEndOfTokens())
        lxrCtx.operator()();

}