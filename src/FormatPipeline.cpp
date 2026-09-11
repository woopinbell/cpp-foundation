#include "cppf/FormatPipeline.hpp"

#include <stdexcept>

namespace cppf
{

FormatPipeline::FormatPipeline() : steps_(), size_(0)
{
    std::size_t index;

    for (index = 0; index < max_steps; ++index)
        steps_[index] = 0;
}

// [INTV:EDGE] 생성자 도중 예외가 나면 그 객체의 소멸자는 절대 호출되지 않는다는 C++ 규칙 때문에,
// 여기서 catch(...)로 직접 정리(cleanup)하지 않으면 이미 clone()된 절반의 Formatter가 누수된다.
// - [FLOW] 1. append 루프로 other의 각 step을 clone -> 2. 도중 예외 발생 시 catch(...)로 진입
//   -> 3. 지금까지 채워진 size_개를 delete로 정리 -> 4. throw;로 같은 예외를 그대로 재전파
// - [TRAP] catch 블록에서 정리만 하고 throw;를 빼먹으면 예외가 삼켜져 호출부가 실패를 알 수 없다.
//   정리 루프의 상한은 반드시 "지금까지 실제로 채운 개수(size_)"여야 하며, max_steps로 잘못 잡으면
//   아직 초기화되지 않은 슬롯을 delete하게 된다.
FormatPipeline::FormatPipeline(const FormatPipeline &other) : steps_(), size_(0)
{
    std::size_t index;

    for (index = 0; index < max_steps; ++index)
        steps_[index] = 0;
    try
    {
        for (index = 0; index < other.size_; ++index)
            append(*other.steps_[index]);
    }
    catch (...)
    {
        for (index = 0; index < size_; ++index)
            delete steps_[index];
        throw;
    }
}

FormatPipeline::~FormatPipeline()
{
    std::size_t index;

    for (index = 0; index < size_; ++index)
        delete steps_[index];
}

FormatPipeline &FormatPipeline::operator=(const FormatPipeline &other)
{
    FormatPipeline copy(other);

    swap(copy);
    return *this;
}

std::size_t FormatPipeline::size() const
{
    return size_;
}

// [INTV:ARCH] formatter.clone()은 가상 함수라서, 인자가 참조(Formatter&)로 타입 소거되어 들어와도
// 실제 가리키는 구체 타입(Prefix/Uppercase/Suffix)에 맞는 clone()이 호출된다.
// - [TRAP] 용량 체크를 clone() 호출보다 뒤로 옮기면, 이미 힙에 만든 복제본을 다시 버려야 하는
//   불필요한 할당/누수 경로가 생긴다. 용량 체크가 항상 먼저다.
void FormatPipeline::append(const Formatter &formatter)
{
    Formatter *copy;

    if (size_ == max_steps)
        throw std::length_error("pipeline capacity");
    copy = formatter.clone();
    steps_[size_] = copy;
    ++size_;
}

// [INTV:FLOW] 이전 단계의 출력이 다음 단계의 입력이 되어 누적 적용되는 파이프라인 실행.
TextBuffer FormatPipeline::apply(const TextBuffer &input) const
{
    TextBuffer result(input);
    std::size_t index;

    for (index = 0; index < size_; ++index)
        result = steps_[index]->apply(result);
    return result;
}

void FormatPipeline::swap(FormatPipeline &other) throw()
{
    std::size_t index;
    const std::size_t size = size_;

    for (index = 0; index < max_steps; ++index)
    {
        Formatter *step = steps_[index];
        steps_[index] = other.steps_[index];
        other.steps_[index] = step;
    }
    size_ = other.size_;
    other.size_ = size;
}

}
