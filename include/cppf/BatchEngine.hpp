#ifndef CPPF_BATCH_ENGINE_HPP
#define CPPF_BATCH_ENGINE_HPP

#include <iosfwd>
#include <string>
#include <vector>

namespace cppf
{

class JobResult
{
public:
    JobResult();
    JobResult(const std::string &name, long value);

    const std::string &name() const;
    long value() const;

private:
    std::string name_;
    long value_;
};

bool operator==(const JobResult &left, const JobResult &right);

class BatchEngine
{
public:
    // [INTV:ARCH] 메서드명이 "parse"/"read"가 아니라 "replace"인 이유: 파싱 도중 실패하면 기존
    // results_는 전혀 건드리지 않고, 전부 성공했을 때만 한 번에 교체한다는 트랜잭션 의미를 이름에 담았다.
    void replace(std::istream &input);
    const std::vector<JobResult> &results() const;
    void write(std::ostream &output) const;

private:
    std::vector<JobResult> results_;
};

}

#endif
