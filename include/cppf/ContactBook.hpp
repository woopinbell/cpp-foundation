#ifndef CPPF_CONTACT_BOOK_HPP
#define CPPF_CONTACT_BOOK_HPP

#include "cppf/Contact.hpp"

#include <cstddef>
#include <iosfwd>
// [INTV:ARCH] <iosfwd>로 ostream을 전방선언만 끌어와 <iostream> 전체 의존을 피함 (컴파일 시간/헤더 결합도 감소).
// - [TRAP] write()의 실제 구현을 헤더에 inline으로 두면 결국 <iostream>이 필요해져 이 이점이 사라진다.
//   정의는 반드시 .cpp에 둘 것.

namespace cppf
{

// [INTV:ARCH] [INTV:TRADE_OFF] 동적 할당 없이 고정 크기 원형 버퍼(circular buffer)로 최근 N개만 유지.
// - [FLOW] 1. add()가 next_ 슬롯을 덮어쓰며 인덱스를 순환 -> 2. at()이 물리 인덱스<->논리 인덱스를 변환해
//   삽입 순서를 복원
// - [TRAP] std::vector 대신 고정 배열을 쓴 이유(과제의 동적 할당 금지 제약)를 놓치고 vector로 재구현하면
//   요구사항을 벗어난다. capacity를 넘는 순간 "가장 오래된 항목을 덮어쓰는" 동작도 함께 재현해야 한다.
class ContactBook
{
public:
    enum
    {
        // [INTV:TRADE_OFF] 익명 enum으로 정수 상수 정의(C++98 관례). static constexpr std::size_t가
        // 더 안전하지만, 재구현 시 타깃 표준 버전에 맞는 상수 정의 방식을 선택할 것.
        capacity = 8
    };

    ContactBook();

    void add(const Contact &contact);
    std::size_t size() const;
    const Contact &at(std::size_t logical_index) const;
    void write(std::ostream &output) const;

private:
    Contact contacts_[capacity];
    std::size_t size_;
    std::size_t next_;
};

}

#endif
