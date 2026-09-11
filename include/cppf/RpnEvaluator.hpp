#ifndef CPPF_RPN_EVALUATOR_HPP
#define CPPF_RPN_EVALUATOR_HPP

#include <string>

namespace cppf
{

// [INTV:ARCH] RPN(후위 표기법)은 스택 하나로 왼쪽에서 오른쪽으로 훑으며 계산 가능 — evaluate()의
// 구현이 스택 기반인 근거.
class RpnEvaluator
{
public:
    static long evaluate(const std::string &expression);

private:
    RpnEvaluator();
    RpnEvaluator(const RpnEvaluator &other);
    RpnEvaluator &operator=(const RpnEvaluator &other);
};

}

#endif
