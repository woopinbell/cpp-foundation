#include "cppf/ScalarConverter.hpp"

#include "ScalarLiteral.hpp"

#include <limits>
#include <locale>
#include <ostream>
#include <sstream>

namespace
{

bool isValue(const cppf::scalar_detail::ScalarLiteral &literal)
{
    return literal.kind == cppf::scalar_detail::literal_character ||
           literal.kind == cppf::scalar_detail::literal_finite;
}

bool canProjectChar(const cppf::scalar_detail::ScalarLiteral &literal)
{
    return isValue(literal) && literal.value > -1.0 &&
           literal.value < 128.0;
}

// [INTV:EDGE] int 최솟값/최댓값을 그대로 비교하지 않고 -1.0/+1.0을 더한 double로 비교하는 이유:
// literal.value가 double이라서 int 최댓값 근처의 큰 정수는 double로 정확히 표현되지 못하고 반올림될
// 수 있다. 경계를 살짝 밖으로 넓혀 "double 반올림 때문에 실제로는 범위 안인데 밀려나는" 오차를 흡수한다.
// - [TRAP] std::numeric_limits<int>::min()/max()를 여유값 없이 그대로 double 비교에 쓰면, 경계값
//   근처 입력에서 유효한 int를 "impossible"로 잘못 판정하는 off-by-epsilon 버그가 난다.
bool canProjectInt(const cppf::scalar_detail::ScalarLiteral &literal)
{
    const double lower =
        static_cast<double>(std::numeric_limits<int>::min()) - 1.0;
    const double upper =
        static_cast<double>(std::numeric_limits<int>::max()) + 1.0;

    return isValue(literal) && literal.value > lower &&
           literal.value < upper;
}

std::string quotedCharacter(int value)
{
    if (value == '\'')
        return "'\\''";
    if (value == '\\')
        return "'\\\\'";
    return std::string("'") + static_cast<char>(value) + "'";
}

void writeCharacter(const cppf::scalar_detail::ScalarLiteral &literal,
                    std::ostream &output)
{
    output << "char: ";
    if (!canProjectChar(literal))
        output << "impossible";
    else
    {
        const int value = static_cast<int>(literal.value);

        if (value < 32 || value > 126)
            output << "Non displayable";
        else
            output << quotedCharacter(value);
    }
    output << '\n';
}

void writeInteger(const cppf::scalar_detail::ScalarLiteral &literal,
                  std::ostream &output)
{
    output << "int: ";
    if (!canProjectInt(literal))
        output << "impossible";
    else
        output << static_cast<int>(literal.value);
    output << '\n';
}

// [INTV:EDGE] 로케일을 classic()으로 고정해 지역화된 소수점 표기(예: '.'대신 ',')가 섞이는 걸 막고,
// digits10(표준이 보장하는 "왕복 손실 없는 최소 십진 자릿수")으로 정밀도를 맞춘다.
// - [TRAP] precision을 임의로 크게 잡으면 부동소수점 이진 표현 오차가 그대로 드러나 지저분한 숫자가
//   출력되고, 너무 작게 잡으면 값이 실제로 손실된다. digits10을 그대로 쓰는 게 표준적 해법이다.
std::string finiteNumber(double value, bool as_float, bool negative_zero)
{
    std::ostringstream output;
    std::string result;

    output.imbue(std::locale::classic());
    output.precision(as_float ? std::numeric_limits<float>::digits10
                              : std::numeric_limits<double>::digits10);
    if (value == 0.0 && negative_zero)
        result = "-0";
    else if (as_float)
        output << static_cast<float>(value);
    else
        output << value;
    if (result.empty())
        result = output.str();
    if (result.find('.') == std::string::npos &&
        result.find('e') == std::string::npos &&
        result.find('E') == std::string::npos)
        // [INTV:TRADE_OFF] "1"처럼 정수로 보이는 출력에 ".0"을 붙여 float/double 리터럴임을 시각적으로
        // 구분 — C++에서 1과 1.0f가 다른 타입이라는 걸 출력 형식에도 반영한다.
        result += ".0";
    return result;
}

// [INTV:EDGE] double -> float 변환 시 정밀도 손실로 아주 작은 0이 아닌 값이 0.0f로 내려앉는
// (underflow) 경우를 걸러낸다. "원래 0이었던 경우"는 허용하고 "0이 아니었는데 변환 후 0이 된 경우"만
// 실패로 처리한다.
// - [TRAP] literal.value == 0.0 조건 없이 value != 0.0f만 검사하면, 원래 정말 0.0이었던 정상 케이스도
//   같은 검사식에 걸려 의도가 뒤집힌다. 두 조건을 OR로 분리해야 한다.
bool canProjectFloat(const cppf::scalar_detail::ScalarLiteral &literal)
{
    const double maximum = std::numeric_limits<float>::max();
    float value;

    if (!isValue(literal) || literal.value < -maximum ||
        literal.value > maximum)
        return false;
    value = static_cast<float>(literal.value);
    return literal.value == 0.0 || value != 0.0f;
}

void writeFloating(const cppf::scalar_detail::ScalarLiteral &literal,
                   std::ostream &output)
{
    output << "float: ";
    if (literal.kind == cppf::scalar_detail::literal_nan)
        output << "nanf";
    else if (literal.kind ==
             cppf::scalar_detail::literal_positive_infinity)
        output << "+inff";
    else if (literal.kind ==
             cppf::scalar_detail::literal_negative_infinity)
        output << "-inff";
    else if (!canProjectFloat(literal))
        output << "impossible";
    else
        output << finiteNumber(literal.value, true, literal.negative_zero)
               << 'f';
    output << '\n';
}

void writeDouble(const cppf::scalar_detail::ScalarLiteral &literal,
                 std::ostream &output)
{
    output << "double: ";
    if (literal.kind == cppf::scalar_detail::literal_nan)
        output << "nan";
    else if (literal.kind ==
             cppf::scalar_detail::literal_positive_infinity)
        output << "+inf";
    else if (literal.kind ==
             cppf::scalar_detail::literal_negative_infinity)
        output << "-inf";
    else
        output << finiteNumber(literal.value, false, literal.negative_zero);
    output << '\n';
}

}

namespace cppf
{

const char *InvalidScalar::what() const throw()
{
    return "invalid scalar literal";
}

// [INTV:ARCH] 내부 전용 예외(ScalarParseError)를 잡아 공개 예외(InvalidScalar)로 바꿔 다시 던지는
// 캡슐화 경계 — scalar_detail 네임스페이스의 세부 구현 타입이 공개 인터페이스 밖으로 새어나가지 않게 한다.
// - [TRAP] catch/rethrow 없이 ScalarParseError를 그대로 전파시키면, 호출부가 ScalarConverter의 공개
//   계약에 없는 내부 타입을 직접 catch해야 하는 상황이 생겨 캡슐화가 깨진다.
void ScalarConverter::write(const std::string &text, std::ostream &output)
{
    scalar_detail::ScalarLiteral literal;

    try
    {
        literal = scalar_detail::parseScalarLiteral(text);
    }
    catch (const scalar_detail::ScalarParseError &)
    {
        throw InvalidScalar();
    }
    std::ostringstream rendered;

    rendered.imbue(std::locale::classic());
    writeCharacter(literal, rendered);
    writeInteger(literal, rendered);
    writeFloating(literal, rendered);
    writeDouble(literal, rendered);
    const std::string result = rendered.str();

    output.write(result.data(), static_cast<std::streamsize>(result.size()));
}

}
