#include "cppf/RpnEvaluator.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

namespace
{

bool isDigit(char value)
{
    return value >= '0' && value <= '9';
}

// [INTV:EDGE] 부호 있는 정수의 비대칭 범위: long의 최솟값은 절댓값이 최댓값보다 1 크다(2의 보수
// 표현). 그래서 음수를 파싱할 때는 허용 한계(limit)를 1 늘려야 "-2147483648" 같은 경계값도 오버플로
// 없이 받아들일 수 있다.
// - [TRAP] limit 보정 없이 양수 최댓값만 기준으로 음수까지 검사하면, long 최솟값과 절댓값이 같은
//   입력을 정상인데도 오버플로로 잘못 거부하게 된다.
bool parseLong(const std::string &token, long &value)
{
    std::size_t index = 0;
    bool negative = false;
    unsigned long magnitude = 0;
    unsigned long limit;

    if (token.empty())
        return false;
    if (token[index] == '+' || token[index] == '-')
    {
        negative = token[index] == '-';
        ++index;
    }
    if (index == token.size())
        return false;
    limit = static_cast<unsigned long>(std::numeric_limits<long>::max());
    if (negative)
        ++limit;
    while (index < token.size())
    {
        unsigned long digit;

        if (!isDigit(token[index]))
            return false;
        digit = static_cast<unsigned long>(token[index] - '0');
        if (magnitude > (limit - digit) / 10)
            throw std::overflow_error("rpn overflow");
        magnitude = magnitude * 10 + digit;
        ++index;
    }
    if (!negative)
        value = static_cast<long>(magnitude);
    // [INTV:EDGE] magnitude == limit(양수 최댓값+1)는 정확히 long 최솟값과 절댓값이 같다는 뜻인데,
    // static_cast<long>(magnitude)로 직접 변환하면 표현 범위를 벗어나 미정의 동작이 될 수 있다.
    // - [TRAP] 이 경계 케이스를 별도 분기 없이 -static_cast<long>(magnitude)로 퉁치면, 이 값
    //   하나에서만 미정의 동작이 나는 재현하기 어려운 버그가 생긴다. numeric_limits<long>::min()을
    //   직접 대입해 안전하게 처리할 것.
    else if (magnitude == limit)
        value = std::numeric_limits<long>::min();
    else
        value = -static_cast<long>(magnitude);
    return true;
}

// [INTV:EDGE] value가 long 최솟값일 때 -value 자체가 오버플로를 일으킬 수 있어(최솟값의 부호를
// 그냥 뒤집으면 표현 범위 밖), 1을 먼저 더해 안전한 범위로 옮긴 뒤 부호를 뒤집고 마지막에 다시 1을
// 더해 절댓값을 복원하는 우회 계산 (parseLong의 limit 보정과 같은 문제를 반대 방향에서 다룬다).
unsigned long magnitudeOf(long value)
{
    if (value >= 0)
        return static_cast<unsigned long>(value);
    return static_cast<unsigned long>(-(value + 1)) + 1;
}

// [INTV:PERF] [INTV:EDGE] left+right를 먼저 계산해 사후 검사하지 않는다 — 오버플로가 나는 순간 이미
// 미정의 동작이라 그 결과로는 판단할 수 없다. 그래서 "이 덧셈을 하면 넘칠 것인가"를 뺄셈으로 뒤집어
// 연산 전에 미리 검사한다 (이 코드베이스 전반에서 반복되는 오버플로 사전검사 원칙).
long checkedAdd(long left, long right)
{
    if ((right > 0 &&
         left > std::numeric_limits<long>::max() - right) ||
        (right < 0 &&
         left < std::numeric_limits<long>::min() - right))
        throw std::overflow_error("rpn overflow");
    return left + right;
}

long checkedSubtract(long left, long right)
{
    if ((right > 0 &&
         left < std::numeric_limits<long>::min() + right) ||
        (right < 0 &&
         left > std::numeric_limits<long>::max() + right))
        throw std::overflow_error("rpn overflow");
    return left - right;
}

// [INTV:EDGE] 곱셈 오버플로 사전검사: product = left*right를 먼저 계산해 넘쳤는지 보는 대신,
// "left_magnitude가 limit/right_magnitude보다 크면 곱하는 순간 넘친다"를 나눗셈으로 미리 판단한다.
long checkedMultiply(long left, long right)
{
    const bool negative = (left < 0) != (right < 0);
    const unsigned long left_magnitude = magnitudeOf(left);
    const unsigned long right_magnitude = magnitudeOf(right);
    unsigned long limit =
        static_cast<unsigned long>(std::numeric_limits<long>::max());
    unsigned long product;

    if (left_magnitude == 0 || right_magnitude == 0)
        return 0;
    if (negative)
        ++limit;
    if (left_magnitude > limit / right_magnitude)
        throw std::overflow_error("rpn overflow");
    product = left_magnitude * right_magnitude;
    if (!negative)
        return static_cast<long>(product);
    if (product == limit)
        return std::numeric_limits<long>::min();
    return -static_cast<long>(product);
}

// [INTV:EDGE] long 최솟값을 -1로 나누면 수학적 결과의 절댓값이 표현 범위를 벗어나(최댓값보다 1 큰
// 값) 오버플로가 된다 — 나눗셈은 보통 오버플로가 안 날 것 같지만 이 조합만 예외적으로 위험하다.
// - [TRAP] 이 특수 케이스 검사를 빼먹고 left/right만 수행하면, 이 입력 조합에서만(플랫폼에 따라
//   크래시하거나 잘못된 값을 내는) 재현하기 어려운 버그가 생긴다.
long checkedDivide(long left, long right)
{
    if (right == 0)
        throw std::invalid_argument("invalid rpn expression");
    if (left == std::numeric_limits<long>::min() && right == -1)
        throw std::overflow_error("rpn overflow");
    return left / right;
}

long applyOperator(long left, long right, char operation)
{
    if (operation == '+')
        return checkedAdd(left, right);
    if (operation == '-')
        return checkedSubtract(left, right);
    if (operation == '*')
        return checkedMultiply(left, right);
    return checkedDivide(left, right);
}

bool isOperator(const std::string &token)
{
    return token.size() == 1 &&
           (token[0] == '+' || token[0] == '-' || token[0] == '*' ||
            token[0] == '/');
}

}

namespace cppf
{

// [INTV:FLOW] 표준 스택 어댑터(std::stack) 대신 std::vector를 직접 써서 back()/pop_back()만으로
// 스택 동작을 구현.
// - [FLOW] 1. 공백 기준 토큰 분리 -> 2. 숫자면 push -> 3. 연산자면 스택에서 두 개를 pop(오른쪽 먼저,
//   그다음 왼쪽) 후 연산 결과를 다시 push -> 4. 끝까지 처리한 뒤 스택에 정확히 하나만 남으면 그 값이 결과
// - [TRAP] pop 순서를 뒤집으면(왼쪽 먼저) 뺄셈/나눗셈처럼 교환법칙이 성립하지 않는 연산에서 결과가
//   틀린다 — RPN에서는 먼저 push된 값이 왼쪽 피연산자, 나중에 push된 값이 오른쪽 피연산자다
//   ("3 4 -"는 3-4를 뜻한다).
long RpnEvaluator::evaluate(const std::string &expression)
{
    std::vector<long> stack;
    std::size_t index = 0;

    while (index < expression.size())
    {
        std::size_t start;
        long value;

        while (index < expression.size() && expression[index] == ' ')
            ++index;
        if (index == expression.size())
            break;
        start = index;
        while (index < expression.size() && expression[index] != ' ')
            ++index;
        const std::string token = expression.substr(start, index - start);

        if (isOperator(token))
        {
            long right;
            long left;

            if (stack.size() < 2)
                throw std::invalid_argument("invalid rpn expression");
            right = stack.back();
            stack.pop_back();
            left = stack.back();
            stack.pop_back();
            stack.push_back(applyOperator(left, right, token[0]));
        }
        else if (parseLong(token, value))
            stack.push_back(value);
        else
            throw std::invalid_argument("invalid rpn expression");
    }
    // [INTV:EDGE] 계산 종료 후 스택에 값이 정확히 하나만 남지 않으면(0개 또는 여러 개) 식이
    // 문법적으로 잘못됐다는 뜻 — 정상 RPN 수식은 항상 결과값 하나만 남긴다.
    if (stack.size() != 1)
        throw std::invalid_argument("invalid rpn expression");
    return stack.back();
}

}
