#ifndef CPP_FOUNDATION_SCALAR_LITERAL_HPP
#define CPP_FOUNDATION_SCALAR_LITERAL_HPP

#include <string>

// [INTV:ARCH] 중첩 네임스페이스로 "ScalarConverter 구현을 위한 내부 세부사항"임을 표시 — 완전히
// 숨기는 익명 namespace와 달리, 같은 라이브러리 내 다른 번역 단위(ScalarConverter.cpp)에서는
// 명시적으로 참조할 수 있도록 열어둔다.
namespace cppf
{
namespace scalar_detail
{

enum LiteralKind
{
    literal_character,
    literal_finite,
    literal_nan,
    literal_positive_infinity,
    literal_negative_infinity
};

// [INTV:ARCH] struct(전부 public, 순수 데이터 전달) vs class(불변조건/캡슐화가 있는 타입) 구분을
// 이 코드베이스의 관례로 사용.
struct ScalarLiteral
{
    LiteralKind kind;
    double value;
    bool float_suffix;
    bool negative_zero;
};

// [INTV:ARCH] 멤버도 상속도 없는 빈 클래스 — 값이 아니라 "타입 자체"가 신호인 마커 예외. std::exception을
// 상속하지 않은 이유는 이 타입이 scalar_detail 내부에서만 잡히고 곧바로 공개 예외(InvalidScalar)로
// 변환되어 다시 던져지는 내부 전용 신호이기 때문 (ScalarConverter.cpp의 catch/throw 참고).
class ScalarParseError
{
};

ScalarLiteral parseScalarLiteral(const std::string &text);

}
}

#endif
