/**
 * @file preprocessor_eval.cpp
 * @brief Implementation of conditional directive expression evaluation.
 */

#include "preprocessor_eval.hpp"

#include <cctype>
#include <charconv>

namespace neo {

namespace {

enum class CondTokType { Eof, Id, Str, Num, Eq, Neq, And, Or, Not, LParen, RParen };

struct CondTok {
    CondTokType type{CondTokType::Eof};
    std::string text;
    int64_t int_val{0};
};

struct ExprValue {
    bool b{false};
    int64_t i{0};
    std::string s;
    bool is_str{false};

    bool to_bool() const noexcept {
        if (is_str) return !s.empty() && s != "0" && s != "false";
        return i != 0 || b;
    }
};

bool values_equal(const ExprValue& lhs, const ExprValue& rhs) noexcept {
    if (lhs.is_str || rhs.is_str) {
        const std::string ls = lhs.is_str ? lhs.s : (lhs.b ? "true" : std::to_string(lhs.i));
        const std::string rs = rhs.is_str ? rhs.s : (rhs.b ? "true" : std::to_string(rhs.i));
        return ls == rs;
    }
    return lhs.i == rhs.i && lhs.b == rhs.b;
}

std::vector<CondTok> tokenize(std::string_view expr) {
    std::vector<CondTok> tokens;
    size_t i = 0;
    while (i < expr.size()) {
        const char c = expr[i];
        if (std::isspace(static_cast<unsigned char>(c)) || c == ';') {
            if (c == ';') break;
            i++;
        } else if (c == '=' && i + 1 < expr.size() && expr[i + 1] == '=') {
            tokens.push_back({CondTokType::Eq, "==", 0}); i += 2;
        } else if (c == '!' && i + 1 < expr.size() && expr[i + 1] == '=') {
            tokens.push_back({CondTokType::Neq, "!=", 0}); i += 2;
        } else if (c == '&' && i + 1 < expr.size() && expr[i + 1] == '&') {
            tokens.push_back({CondTokType::And, "&&", 0}); i += 2;
        } else if (c == '|' && i + 1 < expr.size() && expr[i + 1] == '|') {
            tokens.push_back({CondTokType::Or, "||", 0}); i += 2;
        } else if (c == '!') { tokens.push_back({CondTokType::Not, "!", 0}); i++;
        } else if (c == '(') { tokens.push_back({CondTokType::LParen, "(", 0}); i++;
        } else if (c == ')') { tokens.push_back({CondTokType::RParen, ")", 0}); i++;
        } else if (c == '"' || c == '\'') {
            const size_t end = expr.find(c, i + 1);
            if (end == std::string_view::npos) break;
            tokens.push_back({CondTokType::Str, std::string(expr.substr(i + 1, end - i - 1)), 0});
            i = end + 1;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t end = i;
            while (end < expr.size() && std::isdigit(static_cast<unsigned char>(expr[end]))) end++;
            int64_t val = 0;
            std::from_chars(expr.data() + i, expr.data() + end, val);
            tokens.push_back({CondTokType::Num, std::string(expr.substr(i, end - i)), val});
            i = end;
        } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t end = i;
            while (end < expr.size() && (std::isalnum(static_cast<unsigned char>(expr[end])) || expr[end] == '_')) end++;
            tokens.push_back({CondTokType::Id, std::string(expr.substr(i, end - i)), 0});
            i = end;
        } else { i++; }
    }
    tokens.push_back({CondTokType::Eof, "", 0});
    return tokens;
}

class ExprParser {
public:
    ExprParser(std::vector<CondTok> toks, const PreprocessorContext& ctx)
        : tokens_(std::move(toks)), ctx_(ctx) {}

    ExprValue parse() { return parse_or(); }
    bool failed() const noexcept { return failed_; }
    const std::string& error() const noexcept { return error_; }

private:
    const CondTok& advance() { return tokens_[pos_++]; }
    bool match(CondTokType t) {
        if (tokens_[pos_].type == t) { advance(); return true; }
        return false;
    }

    ExprValue parse_primary() {
        if (match(CondTokType::Not)) return {!parse_primary().to_bool(), 0, "", false};
        if (match(CondTokType::LParen)) {
            ExprValue v = parse_or();
            if (!match(CondTokType::RParen)) { failed_ = true; error_ = "Expected ')'"; }
            return v;
        }
        if (tokens_[pos_].type == CondTokType::Id && tokens_[pos_].text == "defined") {
            advance();
            const bool paren = match(CondTokType::LParen);
            if (tokens_[pos_].type != CondTokType::Id) { failed_ = true; error_ = "Expected identifier"; return {}; }
            const bool def = ctx_.symbols.find(advance().text) != ctx_.symbols.end();
            if (paren && !match(CondTokType::RParen)) { failed_ = true; error_ = "Expected ')'"; }
            return {def, def ? 1 : 0, def ? "1" : "0", false};
        }
        if (tokens_[pos_].type == CondTokType::Id) {
            const std::string id = advance().text;
            if (id == "true") return {true, 1, "true", false};
            if (id == "false") return {false, 0, "false", false};
            const auto it = ctx_.symbols.find(id);
            if (it != ctx_.symbols.end()) return {it->second != "0" && !it->second.empty(), 0, it->second, true};
            return {false, 0, "", true};
        }
        if (tokens_[pos_].type == CondTokType::Str) return {false, 0, advance().text, true};
        if (tokens_[pos_].type == CondTokType::Num) return {advance().int_val != 0, advance().int_val, "", false};
        failed_ = true; error_ = "Unexpected token in expression"; return {};
    }

    ExprValue parse_comp() {
        ExprValue l = parse_primary();
        if (match(CondTokType::Eq)) return {values_equal(l, parse_primary()), 0, "", false};
        if (match(CondTokType::Neq)) return {!values_equal(l, parse_primary()), 0, "", false};
        return l;
    }

    ExprValue parse_and() {
        ExprValue l = parse_comp();
        while (match(CondTokType::And)) l = {l.to_bool() && parse_comp().to_bool(), 0, "", false};
        return l;
    }

    ExprValue parse_or() {
        ExprValue l = parse_and();
        while (match(CondTokType::Or)) l = {l.to_bool() || parse_and().to_bool(), 0, "", false};
        return l;
    }

    std::vector<CondTok> tokens_;
    const PreprocessorContext& ctx_;
    size_t pos_{0};
    bool failed_{false};
    std::string error_;
};

std::string trim_symbol(std::string_view spec) {
    size_t s = 0;
    while (s < spec.size() && std::isspace(static_cast<unsigned char>(spec[s]))) s++;
    size_t e = spec.size();
    while (e > s && (std::isspace(static_cast<unsigned char>(spec[e - 1])) || spec[e - 1] == ';')) e--;
    return std::string(spec.substr(s, e - s));
}

} // namespace

ConditionEvaluator::ConditionEvaluator(const PreprocessorContext& context) noexcept
    : context_(context) {}

EvalResult ConditionEvaluator::evaluate(std::string_view expression) {
    auto tokens = tokenize(expression);
    ExprParser parser(std::move(tokens), context_);
    ExprValue res = parser.parse();
    if (parser.failed()) return {false, false, parser.error()};
    return {true, res.to_bool(), ""};
}

EvalResult ConditionEvaluator::evaluate_ifndef(std::string_view symbol_spec) {
    const std::string sym = trim_symbol(symbol_spec);
    if (sym.empty()) return {false, false, "Missing symbol for @IFNDEF"};
    return {true, context_.symbols.find(sym) == context_.symbols.end(), ""};
}

EvalResult ConditionEvaluator::evaluate_ifdef(std::string_view symbol_spec) {
    const std::string sym = trim_symbol(symbol_spec);
    if (sym.empty()) return {false, false, "Missing symbol for @IFDEF"};
    return {true, context_.symbols.find(sym) != context_.symbols.end(), ""};
}

} // namespace neo
