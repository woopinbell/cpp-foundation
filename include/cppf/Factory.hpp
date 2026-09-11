#ifndef CPPF_FACTORY_HPP
#define CPPF_FACTORY_HPP

#include "cppf/FormatPipeline.hpp"

#include <cstddef>
#include <exception>
#include <string>

namespace cppf
{

// [INTV:ARCH] std::exception을 상속해 표준 예외 계층에 편입 -> 호출부가 catch (const std::exception&)
// 하나로 이 커스텀 예외와 표준 라이브러리 예외(std::out_of_range 등)를 한 지점에서 함께 처리할 수 있다.
class InvalidSpecification : public std::exception
{
public:
    virtual const char *what() const throw();
};

class UnknownFormatter : public std::exception
{
public:
    virtual const char *what() const throw();
};

class FormatterCreator
{
public:
    virtual ~FormatterCreator();
    virtual Formatter *create(const std::string &specification) const = 0;
};

class DefaultFormatterCreator : public FormatterCreator
{
public:
    virtual Formatter *create(const std::string &specification) const;
};

// [INTV:ARCH] 상태 없는 static 유틸리티 클래스 (Factory + Builder 역할을 겸함).
class PipelineBuilder
{
public:
    static void replace(FormatPipeline &target,
                        const FormatterCreator &creator,
                        const std::string *specifications,
                        std::size_t count);

private:
    // [INTV:TRAP] private 생성자로 인스턴스화 자체를 컴파일 타임에 차단. 이걸 빼먹고 public 기본
    // 생성자를 남겨두면 "PipelineBuilder builder;" 같은 무의미한 객체 생성이 그냥 통과해버린다.
    PipelineBuilder();
};

}

#endif
