/**
 * @file preprocessor_eval.hpp
 * @brief Expression evaluator for conditional preprocessor directives.
 */

#pragma once

#include "neo/preprocessor.hpp"

#include <string>
#include <string_view>

namespace neo {

/**
 * @brief Result of evaluating a conditional expression.
 */
struct EvalResult {
    bool success{false};
    bool value{false};
    std::string error;
};

/**
 * @brief Evaluator for @IF, @IFNDEF, and @IFDEF expressions.
 */
class ConditionEvaluator {
public:
    /**
     * @brief Constructs evaluator referencing preprocessor context.
     * @param context Preprocessor context providing symbols and environment.
     */
    explicit ConditionEvaluator(const PreprocessorContext& context) noexcept;

    /**
     * @brief Evaluates an arbitrary boolean expression.
     * @param expression String view of expression after directive.
     * @return EvalResult with evaluation success and boolean outcome.
     */
    [[nodiscard]] EvalResult evaluate(std::string_view expression);

    /**
     * @brief Evaluates an @IFNDEF symbol test.
     * @param symbol_spec Raw symbol argument.
     * @return EvalResult with true if undefined.
     */
    [[nodiscard]] EvalResult evaluate_ifndef(std::string_view symbol_spec);

    /**
     * @brief Evaluates an @IFDEF symbol test.
     * @param symbol_spec Raw symbol argument.
     * @return EvalResult with true if defined.
     */
    [[nodiscard]] EvalResult evaluate_ifdef(std::string_view symbol_spec);

private:
    const PreprocessorContext& context_;
};

} // namespace neo
