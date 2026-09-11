#include "cppf/TextBuffer.hpp"

#include <cstring>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace cppf
{

TextBuffer::TextBuffer() : data_(new char[1]), size_(0)
{
    data_[0] = '\0';
}

TextBuffer::TextBuffer(const char *text) : data_(0), size_(0)
{
    if (text == 0)
        text = "";
    size_ = std::strlen(text);
    data_ = new char[size_ + 1];
    std::memcpy(data_, text, size_ + 1);
}

TextBuffer::TextBuffer(const TextBuffer &other)
    : data_(new char[other.size_ + 1]), size_(other.size_)
{
    std::memcpy(data_, other.data_, size_ + 1);
}

TextBuffer::~TextBuffer()
{
    delete[] data_;
}

// [INTV:EDGE] copy-and-swap: copy(other) 생성 중 bad_alloc이 나도 *this는 손상되지 않는다.
// - [TRAP] 자기 대입(this == &other) 체크가 없어도 안전한 이유를 설명할 수 있어야 한다 — copy(other)가
//   먼저 완전한 별도 버퍼를 만들고 나서야 swap하므로, self-assignment여도 결과적으로 자기 자신의
//   복사본과 교환하는 것일 뿐 원본을 먼저 해제하지 않는다. 별도의 if (this == &other) 분기는 불필요.
TextBuffer &TextBuffer::operator=(const TextBuffer &other)
{
    TextBuffer copy(other);

    swap(copy);
    return *this;
}

// [INTV:EDGE] 덧셈 전에 오버플로 여부를 뺄셈으로 뒤집어 검사 (size_+other.size_+1을 그냥 계산하면
// size_t가 wrap-around 해서 실제보다 작은 값으로 넘어갈 수 있다).
// - [FLOW] 1. 오버플로 사전 검사 -> 2. 새 버퍼를 size_+other.size_+1로 할당 -> 3. 기존 내용과
//   추가 내용을 새 버퍼에 복사 -> 4. 복사가 다 끝난 뒤에야 delete[] data_로 옛 버퍼 해제 후 교체
// - [TRAP] "새 버퍼에 다 복사한 뒤 옛 버퍼를 지우는" 순서를 반대로 하면(먼저 delete[] data_ 등)
//   new[] 실패 시 이미 원본 데이터가 사라진 상태로 예외가 전파되어 객체가 깨진다.
TextBuffer &TextBuffer::operator+=(const TextBuffer &other)
{
    char *joined;
    std::size_t joined_size;

    if (other.size_ > std::numeric_limits<std::size_t>::max() - size_ - 1)
        throw std::length_error("text length");
    joined_size = size_ + other.size_;
    joined = new char[joined_size + 1];
    std::memcpy(joined, data_, size_);
    std::memcpy(joined + size_, other.data_, other.size_ + 1);
    delete[] data_;
    data_ = joined;
    size_ = joined_size;
    return *this;
}

std::size_t TextBuffer::size() const
{
    return size_;
}

bool TextBuffer::empty() const
{
    return size_ == 0;
}

const char *TextBuffer::c_str() const
{
    return data_;
}

char &TextBuffer::at(std::size_t index)
{
    if (index >= size_)
        throw std::out_of_range("text index");
    return data_[index];
}

// [INTV:ARCH] const/non-const 오버로드 쌍 — 호출자의 const 여부에 따라 컴파일러가 자동 선택.
// - [TRAP] const 버전에서 non-const 버전을 캐스팅으로 재사용하려는 시도(반대는 위험)는 가능하지만
//   여기서는 로직이 단순해 각각 독립 구현했다. 로직이 커지면 const_cast로 중복을 줄이는 패턴도 고려할 것.
const char &TextBuffer::at(std::size_t index) const
{
    if (index >= size_)
        throw std::out_of_range("text index");
    return data_[index];
}

void TextBuffer::swap(TextBuffer &other) throw()
{
    char *data = data_;
    const std::size_t size = size_;

    data_ = other.data_;
    size_ = other.size_;
    other.data_ = data;
    other.size_ = size;
}

TextBuffer operator+(const TextBuffer &left, const TextBuffer &right)
{
    TextBuffer result(left);

    result += right;
    return result;
}

bool operator==(const TextBuffer &left, const TextBuffer &right)
{
    return std::strcmp(left.c_str(), right.c_str()) == 0;
}

bool operator!=(const TextBuffer &left, const TextBuffer &right)
{
    return !(left == right);
}

bool operator<(const TextBuffer &left, const TextBuffer &right)
{
    return std::strcmp(left.c_str(), right.c_str()) < 0;
}

std::ostream &operator<<(std::ostream &output, const TextBuffer &value)
{
    return output << value.c_str();
}

}
