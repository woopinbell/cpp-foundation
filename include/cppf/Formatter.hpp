#ifndef CPPF_FORMATTER_HPP
#define CPPF_FORMATTER_HPP

#include "cppf/TextBuffer.hpp"

namespace cppf
{

// [INTV:ARCH] Prototype 패턴: 추상 인터페이스 + clone()으로 "타입을 모르는 채로 다형적 복사"를 지원.
// - [FLOW] 1. Formatter*로만 다뤄지는 지점에서 clone() 호출 -> 2. 가상 디스패치로 실제 파생 타입의
//   clone()이 실행 -> 3. 힙에 같은 타입의 새 객체가 만들어져 소유권이 호출자에게 넘어감
// - [TRAP] 소멸자를 virtual로 선언하지 않으면 Formatter*로 delete할 때 파생 클래스 소멸자가 호출되지
//   않아 슬라이싱/누수로 이어진다. 다형적으로 쓰이는 기반 클래스는 소멸자를 반드시 virtual로 선언할 것.
class Formatter
{
public:
    virtual ~Formatter();

    virtual Formatter *clone() const = 0;
    virtual TextBuffer apply(const TextBuffer &input) const = 0;
    virtual const char *name() const = 0;
};

class UppercaseFormatter : public Formatter
{
public:
    // [INTV:TRAP] virtual 재기입은 문법적으로 불필요(이미 기반 클래스에서 상속됨)하지만 이 프로젝트는
    // C++98 관례를 따른다. C++11 이후로 재구현한다면 override를 붙여 "오버라이드 의도"를 컴파일러가
    // 검증하게 하는 편이 더 안전하다 (시그니처가 미묘하게 어긋나면 override는 컴파일 에러를 낸다).
    virtual Formatter *clone() const;
    virtual TextBuffer apply(const TextBuffer &input) const;
    virtual const char *name() const;
};

class PrefixFormatter : public Formatter
{
public:
    explicit PrefixFormatter(const TextBuffer &prefix);

    virtual Formatter *clone() const;
    virtual TextBuffer apply(const TextBuffer &input) const;
    virtual const char *name() const;

private:
    TextBuffer prefix_;
};

class SuffixFormatter : public Formatter
{
public:
    explicit SuffixFormatter(const TextBuffer &suffix);

    virtual Formatter *clone() const;
    virtual TextBuffer apply(const TextBuffer &input) const;
    virtual const char *name() const;

private:
    TextBuffer suffix_;
};

}

#endif
