#ifndef CPPF_FORMAT_PIPELINE_HPP
#define CPPF_FORMAT_PIPELINE_HPP

#include "cppf/Formatter.hpp"

#include <cstddef>

namespace cppf
{

// [INTV:ARCH] [INTV:TRADE_OFF] Formatter*의 고정 크기 배열로 다형적 요소들의 실행 순서를 조립하는
// Chain 파이프라인. 추상 타입은 값으로 저장할 수 없어(크기 미상) 포인터 소유 구조가 강제된다.
// - [TRAP] 포인터 배열을 얕게 복사(컴파일러 기본 복사 생성자)하면 두 파이프라인이 같은 Formatter를
//   가리키게 되어 이중 delete로 이어진다. 복사/대입/소멸을 전부 clone() 기반으로 직접 정의해야 한다.
class FormatPipeline
{
public:
    enum
    {
        max_steps = 8
    };

    FormatPipeline();
    FormatPipeline(const FormatPipeline &other);
    ~FormatPipeline();
    FormatPipeline &operator=(const FormatPipeline &other);

    std::size_t size() const;
    void append(const Formatter &formatter);
    TextBuffer apply(const TextBuffer &input) const;
    void swap(FormatPipeline &other) throw();

private:
    Formatter *steps_[max_steps];
    std::size_t size_;
};

}

#endif
