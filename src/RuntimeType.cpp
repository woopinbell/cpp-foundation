#include "cppf/RuntimeType.hpp"

#include <typeinfo>

namespace cppf
{

RuntimeBase::RuntimeBase()
{
}

RuntimeBase::~RuntimeBase()
{
}

RuntimeBase *RuntimeInspector::create(RuntimeKind kind)
{
    if (kind == runtime_a)
        return new RuntimeA();
    if (kind == runtime_b)
        return new RuntimeB();
    if (kind == runtime_c)
        return new RuntimeC();
    return 0;
}

// [INTV:EDGE] dynamic_cast(RTTI)가 동작하려면 대상 타입이 다형적(가상 함수 최소 1개)이어야 한다 —
// 이 클래스가 protected 생성자로 직접 인스턴스화를 막으면서도 RTTI 대상이 될 수 있는 건 virtual
// 소멸자가 그 조건을 충족시키기 때문이다.
// - [TRAP] 소멸자를 virtual로 선언하지 않고 재구현하면 dynamic_cast 자체가 컴파일 에러가 난다
//   (non-polymorphic 타입을 대상으로 한 dynamic_cast는 표준에서 금지).
RuntimeKind RuntimeInspector::identify(const RuntimeBase *value)
{
    if (dynamic_cast<const RuntimeA *>(value) != 0)
        return runtime_a;
    if (dynamic_cast<const RuntimeB *>(value) != 0)
        return runtime_b;
    if (dynamic_cast<const RuntimeC *>(value) != 0)
        return runtime_c;
    return runtime_unknown;
}

// [INTV:EDGE] 참조 대상 dynamic_cast는 실패 시 null을 돌려줄 방법이 없어(참조는 항상 유효한 대상을
// 가리켜야 함) std::bad_cast를 던진다. 그래서 포인터 버전과 달리 try/catch로 성공 여부를 감지한다.
// - [FLOW] 1. 캐스팅 시도 -> 2. 성공하면 결과를 변수에 바인딩하고 kind 반환 -> 3. 실패하면
//   catch(bad_cast)로 넘어가 다음 후보 타입을 시도
// - [TRAP] static_cast<void>(matched)를 빼면 "선언만 하고 쓰지 않은 변수" 경고가 남는다.
//   dynamic_cast의 목적이 캐스팅 성공 여부 확인 자체이지 matched의 값 사용이 아니라는 점이 핵심.
RuntimeKind RuntimeInspector::identify(const RuntimeBase &value)
{
    try
    {
        const RuntimeA &matched = dynamic_cast<const RuntimeA &>(value);
        static_cast<void>(matched);
        return runtime_a;
    }
    catch (const std::bad_cast &)
    {
    }
    try
    {
        const RuntimeB &matched = dynamic_cast<const RuntimeB &>(value);
        static_cast<void>(matched);
        return runtime_b;
    }
    catch (const std::bad_cast &)
    {
    }
    try
    {
        const RuntimeC &matched = dynamic_cast<const RuntimeC &>(value);
        static_cast<void>(matched);
        return runtime_c;
    }
    catch (const std::bad_cast &)
    {
    }
    return runtime_unknown;
}

const char *RuntimeInspector::name(RuntimeKind kind)
{
    if (kind == runtime_a)
        return "A";
    if (kind == runtime_b)
        return "B";
    if (kind == runtime_c)
        return "C";
    return "unknown";
}

}
