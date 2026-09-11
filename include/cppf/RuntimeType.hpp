#ifndef CPPF_RUNTIME_TYPE_HPP
#define CPPF_RUNTIME_TYPE_HPP

namespace cppf
{

enum RuntimeKind
{
    runtime_a,
    runtime_b,
    runtime_c,
    runtime_unknown
};

// [INTV:ARCH] 순수 가상 함수 없이 protected 생성자만으로 "직접 인스턴스화 금지, 파생 클래스를 통해서만
// 생성 가능"이라는 추상 클래스 의도를 표현하는 대안적 관용구.
class RuntimeBase
{
public:
    virtual ~RuntimeBase();

protected:
    RuntimeBase();
};

// [INTV:ARCH] 본문이 빈 파생 클래스들 — 데이터/동작 추가가 목적이 아니라 dynamic_cast/RTTI로
// 구분 가능한 "서로 다른 타입 그 자체"가 되는 것이 목적이다.
class RuntimeA : public RuntimeBase
{
};

class RuntimeB : public RuntimeBase
{
};

class RuntimeC : public RuntimeBase
{
};

class RuntimeInspector
{
public:
    static RuntimeBase *create(RuntimeKind kind);
    static RuntimeKind identify(const RuntimeBase *value);
    // [INTV:EDGE] dynamic_cast는 대상이 포인터냐 참조냐에 따라 실패 시 동작이 다르다
    // (포인터: null 반환 / 참조: std::bad_cast 예외). 그래서 identify()가 두 오버로드로 나뉘어 있고,
    // 각각 다른 실패 감지 코드(null 체크 vs try/catch)를 쓴다.
    // - [TRAP] 참조 버전을 포인터 버전처럼 "== 0" 비교로 재구현할 수 없다(참조는 null이 될 수 없다).
    //   캐스팅 종류에 맞는 실패 감지 방식을 반드시 구분해서 쓸 것.
    static RuntimeKind identify(const RuntimeBase &value);
    static const char *name(RuntimeKind kind);

private:
    RuntimeInspector();
    RuntimeInspector(const RuntimeInspector &other);
    RuntimeInspector &operator=(const RuntimeInspector &other);
};

}

#endif
