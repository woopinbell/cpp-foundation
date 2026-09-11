#include "cppf/Formatter.hpp"

#include <cctype>

namespace cppf
{

Formatter::~Formatter()
{
}

// [INTV:ARCH] clone()은 "자기 타입을 정확히 아는 유일한 지점"이므로 기반 클래스가 대신 구현해줄 방법이
// 없다 — 파생 클래스마다 반드시 직접 정의해야 한다.
Formatter *UppercaseFormatter::clone() const
{
    return new UppercaseFormatter(*this);
}

TextBuffer UppercaseFormatter::apply(const TextBuffer &input) const
{
    TextBuffer output(input);
    std::size_t index;

    for (index = 0; index < output.size(); ++index)
    {
        // [INTV:EDGE] std::toupper는 int 인자를 받고 음수(EOF 등)를 특수 처리하므로, char를 그대로
        // 넘기면 char가 signed인 플랫폼에서 128 이상 바이트가 음수로 확장되어 미정의 동작이 된다.
        // - [TRAP] static_cast<unsigned char> 캐스팅을 빼먹으면 이식성 버그로 이어진다.
        const unsigned char byte = static_cast<unsigned char>(output.at(index));
        output.at(index) = static_cast<char>(std::toupper(byte));
    }
    return output;
}

const char *UppercaseFormatter::name() const
{
    return "upper";
}

PrefixFormatter::PrefixFormatter(const TextBuffer &prefix) : prefix_(prefix)
{
}

Formatter *PrefixFormatter::clone() const
{
    return new PrefixFormatter(*this);
}

TextBuffer PrefixFormatter::apply(const TextBuffer &input) const
{
    return prefix_ + input;
}

const char *PrefixFormatter::name() const
{
    return "prefix";
}

SuffixFormatter::SuffixFormatter(const TextBuffer &suffix) : suffix_(suffix)
{
}

Formatter *SuffixFormatter::clone() const
{
    return new SuffixFormatter(*this);
}

TextBuffer SuffixFormatter::apply(const TextBuffer &input) const
{
    return input + suffix_;
}

const char *SuffixFormatter::name() const
{
    return "suffix";
}

}
