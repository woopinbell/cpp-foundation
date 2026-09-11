#ifndef CPPF_CONTACT_HPP
#define CPPF_CONTACT_HPP

#include <string>

namespace cppf
{

// [INTV:ARCH] [INTV:TRADE_OFF] 생성 실패를 예외 대신 "빈 이름 상태"로 인코딩하는 값 타입.
// - [FLOW] 1. 생성자에서 유효성 검사 -> 2. 실패 시 멤버를 초기 상태(빈 문자열)로 남김 -> 3. 호출부는 empty()로만 성공 여부 확인
// - [TRAP] 예외를 던지는 설계와 섞어 쓰지 말 것: 이 클래스는 "잘못된 입력 = 조용히 무효 객체"이므로,
//   호출부에서 유효성 보장이 필요하면 반드시 empty()를 직접 체크해야 한다 (컴파일러가 강제해주지 않음).
class Contact
{
public:
    Contact();
    Contact(const std::string &name, const std::string &note);

    bool empty() const;
    const std::string &name() const;
    const std::string &note() const;

    // [INTV:EDGE] copy-and-swap 관용구의 예외 안전성 보장 근거.
    // - [TRAP] throw()는 C++98 예외 명세(현재는 noexcept로 대체, C++17에서 완전히 제거된 문법)이므로
    //   최신 표준 기준으로 재구현한다면 noexcept로 선언해야 한다. 시그니처만 보고 그대로 베끼지 말 것.
    void swap(Contact &other) throw();

private:
    std::string name_;
    std::string note_;
};

}

#endif
