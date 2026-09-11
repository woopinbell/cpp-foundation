#include "ScalarLiteral.hpp"

#include <limits>
#include <locale>
#include <sstream>

namespace
{

bool isDigit(char value)
{
    return value >= '0' && value <= '9';
}

bool isWhitespace(char value)
{
    return value == ' ' || value == '\t' || value == '\n' ||
           value == '\v' || value == '\f' || value == '\r';
}

// [INTV:EDGE] 널 바이트, 8비트 확장 문자(127 초과), 공백을 여기서 조기에 거부해 이후 파싱 로직이
// 순수 ASCII 인쇄 문자만 다룬다고 가정할 수 있게 만든다.
// - [TRAP] 이 검사를 파싱 로직 뒤로 미루면, 뒤쪽의 문자별 비교(isDigit 등)가 8비트 문자에 대해
//   구현정의 동작을 일으킬 수 있다. 반드시 진입점에서 가장 먼저 걸러낼 것.
void rejectInvalidBytes(const std::string &text)
{
    std::size_t index;

    if (text.empty())
        throw cppf::scalar_detail::ScalarParseError();
    for (index = 0; index < text.size(); ++index)
    {
        const unsigned char value =
            static_cast<unsigned char>(text[index]);

        if (value == 0 || value > 127 || isWhitespace(text[index]))
            throw cppf::scalar_detail::ScalarParseError();
    }
}

// [INTV:ARCH] std::numeric_limits<double>::quiet_NaN()/infinity(): 0.0/0.0 같은 직접 계산 대신
// 표준이 제공하는 함수로 IEEE 754 특수값을 이식성 있게 얻는다.
cppf::scalar_detail::ScalarLiteral makeSpecial(
    cppf::scalar_detail::LiteralKind kind,
    bool float_suffix)
{
    cppf::scalar_detail::ScalarLiteral literal;

    literal.kind = kind;
    literal.float_suffix = float_suffix;
    literal.negative_zero = false;
    if (kind == cppf::scalar_detail::literal_nan)
        literal.value = std::numeric_limits<double>::quiet_NaN();
    else if (kind == cppf::scalar_detail::literal_negative_infinity)
        literal.value = -std::numeric_limits<double>::infinity();
    else
        literal.value = std::numeric_limits<double>::infinity();
    return literal;
}

bool allMantissaDigitsAreZero(const std::string &text)
{
    std::size_t index = 0;

    if (text[index] == '+' || text[index] == '-')
        ++index;
    while (index < text.size() && text[index] != 'e' &&
           text[index] != 'E' && text[index] != 'f')
    {
        if (isDigit(text[index]) && text[index] != '0')
            return false;
        ++index;
    }
    return true;
}

void validateFiniteGrammar(const std::string &text, bool &float_suffix)
{
    std::size_t index = 0;
    std::size_t integer_digits = 0;
    std::size_t fraction_digits = 0;
    bool has_point = false;
    bool has_exponent = false;

    if (text[index] == '+' || text[index] == '-')
        ++index;
    while (index < text.size() && isDigit(text[index]))
    {
        ++integer_digits;
        ++index;
    }
    if (index < text.size() && text[index] == '.')
    {
        has_point = true;
        ++index;
        while (index < text.size() && isDigit(text[index]))
        {
            ++fraction_digits;
            ++index;
        }
    }
    if (integer_digits == 0 && fraction_digits == 0)
        throw cppf::scalar_detail::ScalarParseError();
    if (index < text.size() && (text[index] == 'e' || text[index] == 'E'))
    {
        std::size_t exponent_digits = 0;

        has_exponent = true;
        ++index;
        if (index < text.size() &&
            (text[index] == '+' || text[index] == '-'))
            ++index;
        while (index < text.size() && isDigit(text[index]))
        {
            ++exponent_digits;
            ++index;
        }
        if (exponent_digits == 0)
            throw cppf::scalar_detail::ScalarParseError();
    }
    float_suffix = false;
    if (index < text.size() && text[index] == 'f')
    {
        float_suffix = true;
        ++index;
    }
    if (index != text.size() || (float_suffix && !has_point && !has_exponent))
        throw cppf::scalar_detail::ScalarParseError();
}

// [INTV:EDGE] istringstream + imbue(classic)로 strtod 대신 C++ 스트림 파싱을 쓰되, 로케일이 소수점
// 문자를 바꿔버리는 걸 막는다. input.fail() || !input.eof()로 "부분만 파싱되고 남은 문자가 있는" 입력을
// 걸러내고, value != value로 NaN을 검출한다(NaN은 IEEE 754상 자기 자신과도 같지 않은 유일한 값).
// - [TRAP] !input.eof() 체크를 빼먹으면 "12abc" 같은 입력이 "12"까지만 파싱되고 성공으로 처리되는
//   버그가 생긴다. operator>>는 뒤에 문자가 남아 있어도 fail 플래그를 세우지 않는다.
double extractFiniteValue(const std::string &text, bool float_suffix)
{
    const std::string number =
        float_suffix ? text.substr(0, text.size() - 1) : text;
    std::istringstream input(number);
    double value;

    input.imbue(std::locale::classic());
    input >> value;
    if (input.fail() || !input.eof() || value != value ||
        value > std::numeric_limits<double>::max() ||
        value < -std::numeric_limits<double>::max())
        throw cppf::scalar_detail::ScalarParseError();
    return value;
}

}

namespace cppf
{
namespace scalar_detail
{

ScalarLiteral parseScalarLiteral(const std::string &text)
{
    ScalarLiteral literal;
    bool float_suffix;
    bool all_zero;

    rejectInvalidBytes(text);
    if (text == "nan" || text == "nanf")
        return makeSpecial(literal_nan, text == "nanf");
    if (text == "+inf" || text == "+inff")
        return makeSpecial(literal_positive_infinity, text == "+inff");
    if (text == "-inf" || text == "-inff")
        return makeSpecial(literal_negative_infinity, text == "-inff");
    if (text.size() == 1 && !isDigit(text[0]) &&
        static_cast<unsigned char>(text[0]) >= 33)
    {
        literal.kind = literal_character;
        literal.value = static_cast<unsigned char>(text[0]);
        literal.float_suffix = false;
        literal.negative_zero = false;
        return literal;
    }
    validateFiniteGrammar(text, float_suffix);
    all_zero = allMantissaDigitsAreZero(text);
    literal.kind = literal_finite;
    literal.float_suffix = float_suffix;
    literal.negative_zero = text[0] == '-' && all_zero;
    if (all_zero)
        literal.value = literal.negative_zero ? -0.0 : 0.0;
    else
    {
        literal.value = extractFiniteValue(text, float_suffix);
        if (literal.value == 0.0)
            throw ScalarParseError();
    }
    return literal;
}

}
}
