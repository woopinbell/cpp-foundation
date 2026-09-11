#include "cppf/Factory.hpp"

namespace
{

// [INTV:ARCH] std::unique_ptr이 없던 C++98 환경에서 흔히 쓰던 자체 RAII 스코프 가드.
// create()가 반환하는 소유권 있는 Formatter*를 감싸, 스코프를 벗어나면(정상 종료든 예외든) 자동
// delete되게 한다.
// - [TRAP] 복사 생성자/대입 연산자를 선언만 하고 정의하지 않는 C++98식 non-copyable 관용구를 그대로
//   지킬 것. 복사를 허용하면 두 FormatterOwner가 같은 포인터를 각자 delete하려 해 이중 해제가 난다.
class FormatterOwner
{
public:
    explicit FormatterOwner(cppf::Formatter *formatter) : formatter_(formatter)
    {
    }

    ~FormatterOwner()
    {
        delete formatter_;
    }

    cppf::Formatter &get() const
    {
        return *formatter_;
    }

private:
    FormatterOwner(const FormatterOwner &other);
    FormatterOwner &operator=(const FormatterOwner &other);

    cppf::Formatter *formatter_;
};

}

namespace cppf
{

const char *InvalidSpecification::what() const throw()
{
    return "invalid formatter specification";
}

const char *UnknownFormatter::what() const throw()
{
    return "unknown formatter";
}

FormatterCreator::~FormatterCreator()
{
}

Formatter *DefaultFormatterCreator::create(
    const std::string &specification) const
{
    const std::string prefix_key = "prefix=";
    const std::string suffix_key = "suffix=";

    if (specification.empty())
        throw InvalidSpecification();
    if (specification == "upper")
        return new UppercaseFormatter();
    if (specification.compare(0, prefix_key.size(), prefix_key) == 0)
    {
        if (specification.size() == prefix_key.size())
            throw InvalidSpecification();
        // [INTV:ARCH] std::string -> TextBuffer(c_str())로 넘어가는 타입 경계 지점 — Formatter 계열은
        // std::string이 아닌 이 프로젝트의 자체 문자열 타입 TextBuffer를 쓰도록 설계되어 있다.
        return new PrefixFormatter(
            TextBuffer(specification.substr(prefix_key.size()).c_str()));
    }
    if (specification.compare(0, suffix_key.size(), suffix_key) == 0)
    {
        if (specification.size() == suffix_key.size())
            throw InvalidSpecification();
        return new SuffixFormatter(
            TextBuffer(specification.substr(suffix_key.size()).c_str()));
    }
    throw UnknownFormatter();
}

// [INTV:EDGE] "지역 candidate를 완전히 다 만든 뒤 마지막에 한 번에 swap"하는 트랜잭션형 구성 —
// Contact/TextBuffer의 copy-and-swap과 같은 원칙이 "여러 단계짜리 구성"으로 확장된 형태.
// - [FLOW] 1. 루프마다 create()로 원본 Formatter를 만들어 FormatterOwner에 즉시 위임(스코프 가드 시작)
//   -> 2. candidate.append()가 clone()으로 자기 몫의 복사본을 별도 소유 -> 3. 루프 반복이 끝나며
//   formatter가 스코프를 벗어나 원본을 정리 -> 4. 전부 성공한 뒤에만 target.swap(candidate)
// - [TRAP] candidate 없이 target에 바로 append하면, 중간 스펙에서 예외가 났을 때 호출자가 원래
//   갖고 있던 target이 이미 절반만 바뀐 채로 남는다. 반드시 별도 버퍼에서 완성한 뒤 교체할 것.
void PipelineBuilder::replace(FormatPipeline &target,
                              const FormatterCreator &creator,
                              const std::string *specifications,
                              std::size_t count)
{
    FormatPipeline candidate;
    std::size_t index;

    if ((specifications == 0 && count != 0) ||
        count > FormatPipeline::max_steps)
        throw InvalidSpecification();
    for (index = 0; index < count; ++index)
    {
        FormatterOwner formatter(creator.create(specifications[index]));
        candidate.append(formatter.get());
    }
    target.swap(candidate);
}

}
