#include "cppf/Contact.hpp"

namespace
{

// [INTV:ARCH] 익명 namespace로 파일 내부 전용 헬퍼를 캡슐화 (C의 static 함수와 동등, 헤더 오염 방지).
bool validField(const std::string &value, std::size_t limit, bool allow_empty)
{
    std::string::size_type index;

    if ((!allow_empty && value.empty()) || value.size() > limit)
        return false;
    for (index = 0; index < value.size(); ++index)
    {
        // [INTV:EDGE] char의 부호 여부는 구현정의(implementation-defined)이므로 unsigned char로
        // 캐스팅하지 않으면 128 이상 바이트 값 비교가 플랫폼마다 다르게 동작할 수 있다.
        // - [TRAP] static_cast<unsigned char> 없이 char를 그대로 범위 비교하면 이식성 버그가 생긴다.
        const unsigned char byte = static_cast<unsigned char>(value[index]);
        if (byte < 32 || byte > 126)
            return false;
    }
    return true;
}

}

namespace cppf
{

Contact::Contact() : name_(), note_()
{
}

// [INTV:TRADE_OFF] 유효성 검사 실패 시 예외를 던지지 않고 "빈 이름"으로 남겨 무효 상태를 표현.
// - [FLOW] 1. validField로 name/note 검증 -> 2. 통과 시에만 멤버에 대입 -> 3. 실패 시 name_/note_는
//   기본 생성된 빈 문자열 그대로 유지
// - [TRAP] "검증 실패 시 멤버를 손대지 않는다"를 놓치기 쉽다 (예: name_만 대입하고 note_는 건너뛰는 실수).
//   실패 경로에서 어떤 멤버도 건드리지 않아야 empty()==true가 정확히 성립한다.
Contact::Contact(const std::string &name, const std::string &note)
    : name_(), note_()
{
    if (validField(name, 32, false) && validField(note, 64, true))
    {
        name_ = name;
        note_ = note;
    }
}

bool Contact::empty() const
{
    return name_.empty();
}

const std::string &Contact::name() const
{
    return name_;
}

const std::string &Contact::note() const
{
    return note_;
}

// [INTV:EDGE] std::string::swap은 버퍼 포인터만 교환하는 O(1) 연산이라 재할당/복사가 없고,
// 그래서 예외를 던지지 않는다.
// - [TRAP] Contact::swap을 name_ = other.name_; 같은 "대입"으로 재구현하면 O(n) 복사가 되어버리고
//   예외 안전성 계약(throw() / noexcept)이 깨진다. 반드시 멤버별 swap을 호출할 것.
void Contact::swap(Contact &other) throw()
{
    name_.swap(other.name_);
    note_.swap(other.note_);
}

}
