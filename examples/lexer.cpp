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
        void Handle(IContext* _ctx) override;
        void OnFinishDetected(Context* ctx);
    };

    // Identifier state (e.g., keywords or variable names)
    class TokenState : public IState {
    public:
        explicit TokenState(std::string token);

        void Handle(IContext* _ctx) override;

    private:
        std::string mToken;
    };

    // Number state (handling numeric literals)
    class NumberState : public IState {
    public:
        explicit NumberState(std::string number);

        void Handle(IContext* _ctx) override;

    private:
        std::string mNumber;
    };

    Context::Context(const std::string& input)
        : ::Context(std::make_unique<InitialState>()), mInput(input), mPos(0) {}

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

    void InitialState::Handle(IContext* _ctx) {
        Context* ctx = dynamic_cast<Context*>(_ctx);
        if (ctx->Finished())
            OnFinishDetected(ctx);

        char ch = ctx->GetNextChar();
        if (isalpha(ch)) {
            ctx->SetState(std::make_unique<TokenState>(std::string(1, ch)));
        }
        else if (isdigit(ch)) {
            ctx->SetState(std::make_unique<NumberState>(std::string(1, ch)));
        }
        else if (ch == '=') {
            ctx->EmitToken("OPERATOR", std::string(1, ch));
            ctx->SetState(std::make_unique<InitialState>());
        }
        else if (ch == ';')
        {
            ctx->EmitToken("DELIMITER", std::string(1, ch));
            ctx->SetState(std::make_unique<InitialState>());
        }
        else if (isspace(ch)) {
            ctx->SetState(std::make_unique<InitialState>());
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
        ctx->SetState(0); // NULL State!
    }

    TokenState::TokenState(std::string token) : mToken(std::move(token)) {}

    void TokenState::Handle(IContext* _ctx) {
        Context* ctx = dynamic_cast<Context*>(_ctx);
        char ch = ctx->GetNextChar();
        if (isalnum(ch)) {
            mToken += ch;
            ctx->SetState(std::make_unique<TokenState>(mToken));
        }
        else {
            ctx->RevertOneChar();
            ctx->EmitToken(keywords.count(mToken) ? "KEYWORD" : "IDENTIFIER", mToken);
            ctx->SetState(std::make_unique<InitialState>());
        }
    }

    NumberState::NumberState(std::string number) : mNumber(std::move(number)) {}

    void NumberState::Handle(IContext* _ctx) {
        Context* ctx = dynamic_cast<Context*>(_ctx);
        char ch = ctx->GetNextChar();
        if (isdigit(ch)) {
            mNumber += ch;
            ctx->SetState(std::make_unique<NumberState>(mNumber));
        }
        else {
            ctx->RevertOneChar();
            ctx->EmitToken("NUMBER", mNumber);
            ctx->SetState(std::make_unique<InitialState>());
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
        void Handle(IContext* ctx) override;
    };

    class KeywordState : public IState {
        void Handle(IContext* ctx) override;
    };

    class SetState : public IState {
    public:
        void Handle(simplistic::fsm::IContext* ctx) override;
    };

    class PrintState : public IState {
    public:
        void Handle(simplistic::fsm::IContext* ctx) override;
    };

    class IdentifierState : public IState {
    public:
        IdentifierState(std::function<void()> onPass);
        void Handle(simplistic::fsm::IContext* ctx) override;
        std::function<void()> mOnPass;
    };

    class OperatorState : public IState {
    public:
        OperatorState(std::function<void()> onPass);
        void Handle(IContext* ctx) override;
        std::function<void()> mOnPass;
    };

    class LiteralState : public IState {
    public:
        void Handle(simplistic::fsm::IContext* ctx) override;
    };

    class DelimiterState : public IState {
    public:
        void Handle(simplistic::fsm::IContext* ctx) override;
    };

    class ErrorState : public IState {
    public:
        void Handle(simplistic::fsm::IContext* ctx) override;
    };

    void SetState::Handle(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.second == "SET") {
            lexerCtx->AdvanceToken();
            lexerCtx->SetState(std::make_unique<IdentifierState>([lexerCtx] {
                lexerCtx->SetState(std::make_unique<OperatorState>([lexerCtx] {
                    lexerCtx->SetState(std::make_unique<LiteralState>());
                    }));
                }));
        }
        else {
            lexerCtx->SetState(std::make_unique<ErrorState>());
        }
    }

    void PrintState::Handle(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.second == "PRINT") {
            lexerCtx->AdvanceToken();
            lexerCtx->SetState(std::make_unique<IdentifierState>([lexerCtx] {
                lexerCtx->SetState(std::make_unique<DelimiterState>());
                }));
        }
        else {
            lexerCtx->SetState(std::make_unique<ErrorState>());
        }
    }

    IdentifierState::IdentifierState(std::function<void()> onPass)
        : mOnPass(onPass)
    {
    }

    void IdentifierState::Handle(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "IDENTIFIER") {
            lexerCtx->AdvanceToken();
            mOnPass();
        }
        else {
            lexerCtx->SetState(std::make_unique<ErrorState>());
        }
    }

    void LiteralState::Handle(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "NUMBER" || token.first == "LITERAL") {
            lexerCtx->AdvanceToken();
            lexerCtx->SetState(std::make_unique<DelimiterState>());
        }
        else {
            lexerCtx->SetState(std::make_unique<ErrorState>());
        }
    }

    void DelimiterState::Handle(IContext* ctx) {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "DELIMITER") {
            lexerCtx->AdvanceToken();
            if (lexerCtx->IsEndOfTokens()) {
                std::cout << "Lexing successful!" << std::endl;
                lexerCtx->SetState(0); // End processing
            }
            else {
                lexerCtx->SetState(std::make_unique<ControllerState>());
            }
        }
        else {
            lexerCtx->SetState(std::make_unique<ErrorState>());
        }
    }

    void ErrorState::Handle(IContext* ctx) {
        std::cerr << "Lexer Error: Unexpected token or state." << std::endl;
        // Transition to an error state or halt further processing
        ctx->SetState(0, true); // End processing
    }

    Context::Context(std::vector<Token> tokens)
        : mTokens(std::move(tokens)), mCurrentTokenIndex(0), ::Context(std::make_unique<KeywordState>()) {
        // Initialize with an appropriate state, e.g., SetState or PrintState
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

    void ControllerState::Handle(IContext* ctx)
    {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "KEYWORD")
        {
            ctx->SetState(std::make_unique<KeywordState>());
            return;
        }

        ctx->SetState(std::make_unique<ErrorState>());
    }

    void KeywordState::Handle(IContext* ctx)
    {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (keywords.count(token.second))
        {
            if(token.second == "SET")
                return ctx->SetState(std::make_unique<SetState>());
            else if (token.second == "PRINT")
                return ctx->SetState(std::make_unique<PrintState>());
        }

        ctx->SetState(std::make_unique<ErrorState>());        
    }
    OperatorState::OperatorState(std::function<void()> onPass)
        : mOnPass(onPass)
    {
    }

    void OperatorState::Handle(IContext* ctx)
    {
        auto lexerCtx = static_cast<Context*>(ctx);
        auto token = lexerCtx->GetCurrentToken();

        if (token.first == "OPERATOR")
        {
            lexerCtx->AdvanceToken();
            mOnPass();
            return;
        }

        ctx->SetState(std::make_unique<ErrorState>());
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
        tknzrCtx.Handle();
    auto allTkns = tknzrCtx.GetTokens();
    for(const auto& currTT : allTkns)
        std::cout << "[" << currTT.first << "] : [" << currTT.second << "]" << std::endl;
    Lexer::Context lxrCtx(allTkns);
    while (lxrCtx.mCurrent && !lxrCtx.IsEndOfTokens())
        lxrCtx.Handle();

}