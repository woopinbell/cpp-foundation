#include "cppf/Serializer.hpp"

namespace cppf
{

Payload::Payload(unsigned long id_value, const std::string &label_value)
    : id(id_value), label(label_value)
{
}

// [INTV:EDGE] reinterpret_cast로 포인터의 비트 패턴을 그대로 정수로 재해석 — 값의 의미를 바꾸지 않는
// 가장 위험한 캐스팅이라 저수준 왕복 변환에만 제한적으로 써야 한다.
// - [TRAP] 결과값(raw_type)은 유효한 산술 정수가 아니라 "포인터를 정수로 표현한 것"일 뿐이다.
//   여기에 덧셈/뺄셈 등 산술 연산을 하면 deserialize()가 원래 주소를 복원하지 못한다.
Serializer::raw_type Serializer::serialize(Payload *payload)
{
    if (payload == 0)
        return static_cast<raw_type>(0);
    return reinterpret_cast<raw_type>(payload);
}

// [INTV:EDGE] serialize()의 역방향 reinterpret_cast. 이 왕복이 안전하려면 raw가 반드시 serialize()가
// 실제로 만들어낸 값이어야 한다.
// - [TRAP] 임의의 정수(사용자가 직접 만든 값 등)를 deserialize()에 넘기면 미정의 동작이다.
//   이 두 함수는 항상 쌍으로, 원본 포인터가 아직 살아있는 동안에만 안전하게 쓸 수 있다.
Payload *Serializer::deserialize(raw_type raw)
{
    if (raw == static_cast<raw_type>(0))
        return 0;
    return reinterpret_cast<Payload *>(raw);
}

}
