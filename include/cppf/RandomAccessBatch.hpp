#ifndef CPPF_RANDOM_ACCESS_BATCH_HPP
#define CPPF_RANDOM_ACCESS_BATCH_HPP

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cppf
{

// [INTV:ARCH] 클래스 템플릿 + 기본 템플릿 인자(Container = std::vector<T>) — "어떤 컨테이너 위에 이
// 배치 인터페이스를 얹을지"를 호출부가 선택할 수 있게 하는 어댑터 설계.
// - [TRAP] 템플릿은 실제로 쓰인 각 타입 조합마다 컴파일 타임에 별도 코드가 찍혀 나온다(제네릭이 아니라
//   코드 생성). 재구현 시 "런타임에 한 벌만 존재한다"고 착각하면 바이너리 크기/컴파일 시간을 오판한다.
template <class T, class Container = std::vector<T> >
class RandomAccessBatch
{
public:
    // [INTV:TRAP] typename이 필요한 이유: Container가 아직 구체 타입으로 결정되지 않은 시점(템플릿
    // 정의 시점)에는 컴파일러가 Container::iterator를 타입으로 볼지 정적 멤버로 볼지 판단할 수 없다
    // ("의존 타입 이름"). typename을 빼먹으면 컴파일 에러가 난다.
    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;

    RandomAccessBatch() : values_()
    {
    }

    RandomAccessBatch(const RandomAccessBatch &other)
        : values_(other.values_)
    {
    }

    RandomAccessBatch &operator=(const RandomAccessBatch &other)
    {
        if (this != &other)
        {
            RandomAccessBatch copy(other);

            swap(copy);
        }
        return *this;
    }

    void push_back(const T &value)
    {
        values_.push_back(value);
    }

    std::size_t size() const
    {
        return static_cast<std::size_t>(values_.size());
    }

    bool empty() const
    {
        return values_.empty();
    }

    T &at(std::size_t index)
    {
        if (index >= values_.size())
            throw std::out_of_range("batch index");
        return values_[index];
    }

    const T &at(std::size_t index) const
    {
        if (index >= values_.size())
            throw std::out_of_range("batch index");
        return values_[index];
    }

    iterator begin()
    {
        return values_.begin();
    }

    iterator end()
    {
        return values_.end();
    }

    const_iterator begin() const
    {
        return values_.begin();
    }

    const_iterator end() const
    {
        return values_.end();
    }

    // [INTV:ARCH] 멤버 템플릿: 클래스 자체는 이미 구체화되어 있어도(RandomAccessBatch<JobResult>), 이
    // 함수 하나만 호출 시점에 별도 타입 매개변수(Compare)를 추론한다 — 자유 함수든 함수 객체든 그대로
    // 받을 수 있게 하는 설계.
    template <class Compare>
    void sort(Compare compare)
    {
        std::sort(values_.begin(), values_.end(), compare);
    }

    void swap(RandomAccessBatch &other)
    {
        values_.swap(other.values_);
    }

private:
    Container values_;
};

// [INTV:ARCH] [INTV:TRADE_OFF] 서로 다른 반복자 타입(FirstIterator/SecondIterator)을 별도 템플릿
// 인자로 분리 — 두 시퀀스가 서로 다른 컨테이너 종류(vector vs deque)여도 비교할 수 있게 한다.
// std::equal이 표준에 있지만 직접 구현한 이유는 "길이가 다르면 실패"까지 검사하기 위함 — std::equal은
// 첫 범위 길이만 기준으로 순회해 second가 더 짧으면 범위를 벗어나 읽고, 더 길면 초과분을 검사하지 않는다.
// - [TRAP] 루프는 "둘 중 하나라도 끝에 도달하면" 멈추므로, 종료 후 반드시 양쪽 다 끝에 도달했는지
//   (first==last && second==second_last) 재확인해야 한다. 이게 없으면 원소가 전부 같아도 길이가
//   다른 두 시퀀스를 "같다"고 잘못 판정한다.
template <class FirstIterator, class SecondIterator>
bool equal_ranges(FirstIterator first,
                  FirstIterator last,
                  SecondIterator second,
                  SecondIterator second_last)
{
    while (first != last && second != second_last)
    {
        if (!(*first == *second))
            return false;
        ++first;
        ++second;
    }
    return first == last && second == second_last;
}

}

#endif
