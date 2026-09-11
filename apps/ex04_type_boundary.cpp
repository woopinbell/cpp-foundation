#include "cppf/ScalarConverter.hpp"
#include "cppf/RuntimeType.hpp"
#include "cppf/Serializer.hpp"

#include <iostream>
#include <limits>
#include <string>

namespace
{

bool parseRuntimeKind(const std::string &text, cppf::RuntimeKind &kind)
{
    if (text == "A")
        kind = cppf::runtime_a;
    else if (text == "B")
        kind = cppf::runtime_b;
    else if (text == "C")
        kind = cppf::runtime_c;
    else
        return false;
    return true;
}

// [INTV:EDGE] 자릿수를 누적하기 전에 "이번 자리를 더하면 최댓값을 넘는가"를 뺄셈/나눗셈으로 미리
// 뒤집어 검사하는 오버플로 방어 (ScalarConverter의 canProjectInt와 같은 취지).
// - [TRAP] value * 10 + digit을 먼저 계산하고 나서 오버플로 여부를 확인하려 하면 이미 wrap-around가
//   일어난 뒤라 늦다. 반드시 연산 전에 역산으로 상한을 검사할 것.
bool parsePayloadId(const std::string &text, unsigned long &value)
{
    std::size_t index;

    if (text.empty())
        return false;
    value = 0;
    for (index = 0; index < text.size(); ++index)
    {
        unsigned long digit;

        if (text[index] < '0' || text[index] > '9')
            return false;
        digit = static_cast<unsigned long>(text[index] - '0');
        if (value > (std::numeric_limits<unsigned long>::max() - digit) /
                        10)
            return false;
        value = value * 10 + digit;
    }
    return true;
}

int runScalar(const char *literal)
{
    try
    {
        cppf::ScalarConverter::write(literal, std::cout);
    }
    // [INTV:ARCH] std::exception이 아니라 구체 타입 InvalidScalar로 좁혀서 잡는다 — 이 함수가
    // 처리하려는 건 스칼라 변환 실패뿐이고, 다른 종류의 std::exception(예: bad_alloc)까지 여기서
    // 삼키지 않기 위한 의도적 구분.
    catch (const cppf::InvalidScalar &error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
    return 0;
}

int runRuntime(const char *name)
{
    cppf::RuntimeKind kind;

    if (!parseRuntimeKind(name, kind))
    {
        std::cerr << "unknown runtime kind" << std::endl;
        return 1;
    }
    cppf::RuntimeBase *value = cppf::RuntimeInspector::create(kind);
    // [INTV:EDGE] 같은 객체를 포인터/참조 두 가지 경로로 identify() — RuntimeType.cpp에서 두 오버로드가
    // 서로 다른 실패 감지 방식(null 체크 vs try/catch)을 쓴다는 걸 나란히 검증하는 구성. 같은 객체를
    // 가리키므로 pointer_kind와 reference_kind는 항상 같아야 한다.
    const cppf::RuntimeKind pointer_kind =
        cppf::RuntimeInspector::identify(value);
    const cppf::RuntimeKind reference_kind =
        cppf::RuntimeInspector::identify(*value);

    delete value;
    std::cout << "pointer: " << cppf::RuntimeInspector::name(pointer_kind)
              << '\n';
    std::cout << "reference: "
              << cppf::RuntimeInspector::name(reference_kind) << '\n';
    return 0;
}

int runAddress(const char *id_text, const char *label)
{
    unsigned long id;

    if (!parsePayloadId(id_text, id))
    {
        std::cerr << "invalid payload id" << std::endl;
        return 1;
    }
    cppf::Payload payload(id, label);
    // [INTV:EDGE] &payload를 serialize()로 정수화했다가 deserialize()로 다시 포인터로 되돌리는 왕복
    // 검증. payload가 스택 지역 변수라 이 함수가 끝나기 전까지는 유효하므로 왕복이 안전하다.
    // - [TRAP] 이 왕복은 원본 포인터가 아직 살아있는 스코프 안에서만 안전하다. payload가 이미 소멸된
    //   뒤 deserialize한 토큰을 역참조하면 미정의 동작(dangling pointer)이다.
    const cppf::Serializer::raw_type token =
        cppf::Serializer::serialize(&payload);
    cppf::Payload *recovered = cppf::Serializer::deserialize(token);

    std::cout << "token: " << (token == 0 ? "zero" : "nonzero") << '\n';
    std::cout << "same: " << (recovered == &payload ? "yes" : "no")
              << '\n';
    std::cout << "id: " << recovered->id << '\n';
    std::cout << "label: " << recovered->label << '\n';
    return 0;
}

}

int main(int argument_count, char **arguments)
{
    if (argument_count >= 2 && std::string(arguments[1]) == "scalar")
    {
        if (argument_count != 3)
        {
            std::cerr << "usage: ex04_type_boundary scalar LITERAL"
                      << std::endl;
            return 1;
        }
        return runScalar(arguments[2]);
    }
    if (argument_count >= 2 && std::string(arguments[1]) == "runtime")
    {
        if (argument_count != 3)
        {
            std::cerr << "usage: ex04_type_boundary runtime A|B|C"
                      << std::endl;
            return 1;
        }
        return runRuntime(arguments[2]);
    }
    if (argument_count >= 2 && std::string(arguments[1]) == "address")
    {
        if (argument_count != 4)
        {
            std::cerr << "usage: ex04_type_boundary address ID LABEL"
                      << std::endl;
            return 1;
        }
        return runAddress(arguments[2], arguments[3]);
    }
    std::cerr << "usage: ex04_type_boundary MODE ..." << std::endl;
    return 1;
}
