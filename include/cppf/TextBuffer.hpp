#ifndef CPPF_TEXT_BUFFER_HPP
#define CPPF_TEXT_BUFFER_HPP

#include <cstddef>
#include <iosfwd>

namespace cppf
{

// [INTV:ARCH] [INTV:EDGE] char*로 힙 버퍼를 직접 소유하는 값 타입 -> Rule of Three 전체(소멸자/복사
// 생성자/대입 연산자)를 직접 정의해야 하는 대표 사례.
// - [TRAP] 재구현 시 이 셋 중 하나라도 컴파일러 기본 생성본(얕은 복사)에 맡기면, 복사본과 원본이
//   같은 data_를 가리키게 되어 이중 delete[]나 use-after-free로 이어진다. 반드시 셋을 함께 정의할 것.
class TextBuffer
{
public:
    TextBuffer();
    // [INTV:TRADE_OFF] 단일 인자 생성자에 explicit을 붙여 "const char* -> TextBuffer" 암시적 변환을 차단.
    // - [TRAP] explicit을 빼먹고 재구현하면 컴파일은 되지만, 의도치 않은 임시 객체 생성(예: 비교
    //   연산에서 문자열 리터럴이 조용히 TextBuffer로 변환)이 생겨 디버깅하기 어려운 동작을 유발한다.
    explicit TextBuffer(const char *text);
    TextBuffer(const TextBuffer &other);
    ~TextBuffer();

    TextBuffer &operator=(const TextBuffer &other);
    TextBuffer &operator+=(const TextBuffer &other);

    std::size_t size() const;
    bool empty() const;
    const char *c_str() const;
    char &at(std::size_t index);
    const char &at(std::size_t index) const;
    void swap(TextBuffer &other) throw();

private:
    // [INTV:ARCH] std::string 대신 char*+size_t로 직접 구현 (과제의 표준 컨테이너 사용 제한).
    // - [FLOW] 1. 생성 시 size_+1바이트 힙 할당(널 종결 포함) -> 2. 복사/연결 시 항상 "새 버퍼를 할당하고
    //   내용을 복사한 뒤 마지막에 교체"하는 순서를 지켜 강한 예외 안전성 유지 -> 3. 소멸자에서 delete[]
    char *data_;
    std::size_t size_;
};

// [INTV:ARCH] 이항 연산자를 멤버가 아닌 자유 함수로 둔 이유.
// - [TRAP] operator<<는 좌변이 std::ostream&(이 클래스가 아님)이라 애초에 멤버로 정의할 수 없다.
//   ==/</+는 멤버로도 만들 수 있지만, 좌우 피연산자를 비대칭적으로 다루게 되는(암시적 변환이 좌변에만
//   적용) 문제를 피하려고 자유 함수로 통일한다.
TextBuffer operator+(const TextBuffer &left, const TextBuffer &right);
bool operator==(const TextBuffer &left, const TextBuffer &right);
bool operator!=(const TextBuffer &left, const TextBuffer &right);
bool operator<(const TextBuffer &left, const TextBuffer &right);
std::ostream &operator<<(std::ostream &output, const TextBuffer &value);

}

#endif
