#include "cppf/BatchEngine.hpp"

#include "cppf/RandomAccessBatch.hpp"
#include "cppf/RpnEvaluator.hpp"

#include <deque>
#include <istream>
#include <locale>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{

bool isFieldWhitespace(char value)
{
    return value == ' ' || value == '\t' || value == '\r' ||
           value == '\n' || value == '\v' || value == '\f';
}

std::string trimField(const std::string &field)
{
    std::size_t first = 0;
    std::size_t last = field.size();

    while (first < last && isFieldWhitespace(field[first]))
        ++first;
    while (last > first && isFieldWhitespace(field[last - 1]))
        --last;
    return field.substr(first, last - first);
}

bool isNameStart(char value)
{
    return (value >= 'A' && value <= 'Z') ||
           (value >= 'a' && value <= 'z');
}

bool isNameRest(char value)
{
    return isNameStart(value) || (value >= '0' && value <= '9') ||
           value == '_' || value == '-';
}

bool isValidName(const std::string &name)
{
    std::size_t index;

    if (name.empty() || !isNameStart(name[0]))
        return false;
    for (index = 1; index < name.size(); ++index)
    {
        if (!isNameRest(name[index]))
            return false;
    }
    return true;
}

// [INTV:ARCH] JobResult에는 operator<가 없다(operator==만 정의) — 정렬 기준을 별도 비교 함수로 분리해
// std::sort/RandomAccessBatch::sort에 comparator로 넘긴다.
bool resultLess(const cppf::JobResult &left,
                const cppf::JobResult &right)
{
    if (left.value() != right.value())
        return left.value() < right.value();
    return left.name() < right.name();
}

// [INTV:EDGE] '|' 구분자가 정확히 하나여야 한다는 규칙을 find()의 두 번째 인자(탐색 시작 위치)로 검증.
void parseLine(const std::string &line,
               std::string &name,
               std::string &expression)
{
    const std::size_t separator = line.find('|');

    if (separator == std::string::npos ||
        line.find('|', separator + 1) != std::string::npos)
        throw std::invalid_argument("invalid batch input");
    name = trimField(line.substr(0, separator));
    expression = trimField(line.substr(separator + 1));
    if (!isValidName(name) || expression.empty())
        throw std::invalid_argument("invalid batch input");
}

// [INTV:EDGE] std::getline 대신 문자 단위로 직접 읽는 이유: "줄바꿈 없이 파일이 끝난 마지막 줄"과
// "완전히 빈 입력"을 구분해야 하기 때문 (아래 eof()+line.empty() 조합이 그 구분을 실제로 사용한다).
// - [TRAP] get()이 false를 반환했을 때 곧바로 "파일이 끝났다"고 단정하면 안 된다. eof가 아닌 다른
//   이유(스트림 오류)로 멈췄을 수도 있으므로 반드시 !input.eof()를 별도로 검사해서 구분할 것.
bool readLine(std::istream &input, std::string &line)
{
    char value;

    line.clear();
    while (input.get(value))
    {
        if (value == '\n')
            return true;
        line.push_back(value);
    }
    if (!input.eof())
        throw std::invalid_argument("invalid batch input");
    return !line.empty();
}

}

namespace cppf
{

JobResult::JobResult() : name_(), value_(0)
{
}

JobResult::JobResult(const std::string &name, long value)
    : name_(name), value_(value)
{
}

const std::string &JobResult::name() const
{
    return name_;
}

long JobResult::value() const
{
    return value_;
}

bool operator==(const JobResult &left, const JobResult &right)
{
    return left.name() == right.name() && left.value() == right.value();
}

// [INTV:EDGE] 같은 데이터를 두 가지 내부 컨테이너(vector_batch/deque_batch)에 나란히 채워 넣고 나중에
// equal_ranges로 교차 검증 — RandomAccessBatch가 컨테이너 종류와 무관하게 일관되게 동작하는지 이
// 함수 스스로 이중 검사하는 방어적 설계.
// - [FLOW] 1. 줄 단위로 읽어 이름/RPN식 파싱 -> 2. seen 맵으로 이름 중복을 O(log n)에 검사 ->
//   3. vector_batch/deque_batch 양쪽에 동일하게 push -> 4. 둘 다 정렬 후 equal_ranges로 일치 검증
//   -> 5. 전부 성공한 뒤에야 지역 candidate를 만들어 results_와 swap
// - [TRAP] results_에 직접 하나씩 채워나가면, 중간 줄에서 파싱 실패나 중복 이름 예외가 났을 때
//   호출자가 원래 갖고 있던 results_가 이미 절반만 바뀐 채로 남는다. 반드시 별도 버퍼에서 전부
//   완성한 뒤 마지막에 한 번에 swap할 것 (copy-and-swap과 동일한 원칙).
void BatchEngine::replace(std::istream &input)
{
    RandomAccessBatch<JobResult> vector_batch;
    RandomAccessBatch<JobResult, std::deque<JobResult> > deque_batch;
    std::map<std::string, long> seen;
    std::string line;

    while (readLine(input, line))
    {
        std::string name;
        std::string expression;

        parseLine(line, name, expression);
        if (seen.find(name) != seen.end())
            throw std::invalid_argument("invalid batch input");
        const long value = RpnEvaluator::evaluate(expression);
        const JobResult result(name, value);

        seen.insert(std::make_pair(name, value));
        vector_batch.push_back(result);
        deque_batch.push_back(result);
    }
    if (vector_batch.empty())
        throw std::invalid_argument("invalid batch input");
    vector_batch.sort(resultLess);
    deque_batch.sort(resultLess);
    if (!equal_ranges(vector_batch.begin(), vector_batch.end(),
                      deque_batch.begin(), deque_batch.end()))
        throw std::logic_error("batch container disagreement");
    std::vector<JobResult> candidate(
        vector_batch.begin(), vector_batch.end());

    results_.swap(candidate);
}

const std::vector<JobResult> &BatchEngine::results() const
{
    return results_;
}

void BatchEngine::write(std::ostream &output) const
{
    std::ostringstream rendered;
    std::size_t index;

    rendered.imbue(std::locale::classic());
    for (index = 0; index < results_.size(); ++index)
        rendered << results_[index].value() << " | "
                 << results_[index].name() << '\n';
    const std::string text = rendered.str();

    output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

}
