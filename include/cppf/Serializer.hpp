#ifndef CPPF_SERIALIZER_HPP
#define CPPF_SERIALIZER_HPP

#include <stdint.h>
#include <string>

namespace cppf
{

struct Payload
{
    Payload(unsigned long id_value, const std::string &label_value);

    unsigned long id;
    std::string label;
};

// [INTV:ARCH] 정적 유틸리티 클래스 (인스턴스화/복사 차단 관용구, PipelineBuilder/ScalarConverter와 동일).
class Serializer
{
public:
    // [INTV:ARCH] uintptr_t: "포인터 하나를 정보 손실 없이 담을 수 있음"을 표준이 보장하는 정수 타입.
    // - [TRAP] unsigned long으로 직접 재구현하면, 포인터 크기가 unsigned long보다 큰 플랫폼(LLP64 등)
    //   에서 상위 비트가 잘려나가는 이식성 버그가 생긴다.
    typedef uintptr_t raw_type;

    static raw_type serialize(Payload *payload);
    static Payload *deserialize(raw_type raw);

private:
    Serializer();
    Serializer(const Serializer &other);
    Serializer &operator=(const Serializer &other);
};

}

#endif
