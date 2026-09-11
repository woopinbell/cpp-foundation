#include "cppf/ContactBook.hpp"

#include <ostream>
#include <stdexcept>

namespace cppf
{

ContactBook::ContactBook() : contacts_(), size_(0), next_(0)
{
}

// [INTV:EDGE] copy-and-swap으로 강한 예외 안전성을 확보: 대입 실패 시에도 기존 슬롯은 손상되지 않는다.
// - [FLOW] 1. 지역 replacement에 값 복사(예외 발생 가능 지점) -> 2. 복사가 끝난 뒤에만 swap(예외 없음)
//   -> 3. next_/size_ 갱신
// - [TRAP] contacts_[next_] = contact; 로 직접 대입하면 복사 도중 예외가 나는 순간 기존 슬롯이 이미
//   일부 변경된 채로 남을 수 있다. 반드시 "복사 -> swap" 순서를 지킬 것 (순서를 바꾸면 안전성 보장이 무너진다).
void ContactBook::add(const Contact &contact)
{
    Contact replacement;

    if (contact.empty())
        return;
    replacement = contact;
    contacts_[next_].swap(replacement);
    next_ = (next_ + 1) % capacity;
    if (size_ < capacity)
        ++size_;
}

std::size_t ContactBook::size() const
{
    return size_;
}

// [INTV:FLOW] 물리 인덱스(next_ 기준 원형 배열)와 논리 인덱스(삽입 순서, 0=가장 오래된 것) 간 변환.
// - [FLOW] 1. size_==capacity면 next_가 가장 오래된 슬롯이므로 first=next_ -> 2. 아직 안 찼으면 0번이
//   곧 가장 오래된 것이므로 first=0 -> 3. (first + logical_index) % capacity로 최종 물리 인덱스 계산
// - [TRAP] "꽉 찬 상태"와 "안 찬 상태"를 하나의 공식으로 퉁치면 버그가 난다. 아직 안 찼을 때 next_ 기준으로
//   계산하면 엉뚱한 슬롯을 가리키므로 두 케이스를 반드시 분기해서 first를 정할 것.
const Contact &ContactBook::at(std::size_t logical_index) const
{
    std::size_t first;

    if (logical_index >= size_)
        throw std::out_of_range("contact index");
    first = size_ == capacity ? next_ : 0;
    return contacts_[(first + logical_index) % capacity];
}

void ContactBook::write(std::ostream &output) const
{
    std::size_t index;

    for (index = 0; index < size_; ++index)
    {
        const Contact &contact = at(index);
        output << index << '|' << contact.name() << '|' << contact.note() << '\n';
    }
}

}
