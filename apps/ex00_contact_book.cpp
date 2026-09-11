#include "cppf/Contact.hpp"
#include "cppf/ContactBook.hpp"

#include <iostream>
#include <string>

namespace
{

bool addContact(cppf::ContactBook &book, const std::string &payload)
{
    const std::string::size_type separator = payload.find('|');
    cppf::Contact contact;

    if (separator == std::string::npos)
        return false;
    contact = cppf::Contact(payload.substr(0, separator),
                            payload.substr(separator + 1));
    // [INTV:EDGE] 이 파일은 "Contact 생성자가 검증 실패 시 예외 대신 빈 이름 상태를 만든다"는
    // Contact.cpp 쪽 계약에 암묵적으로 의존한다.
    // - [TRAP] 이 파일만 보고 재구현하면 왜 try/catch 없이 empty()만 검사하는지 알 수 없다.
    //   Contact의 무효 상태 인코딩 전략을 먼저 확인하지 않으면 "예외를 던지는 유효성 검사"로
    //   잘못 재구현하기 쉽다.
    if (contact.empty())
        return false;
    book.add(contact);
    return true;
}

}

int main()
{
    cppf::ContactBook book;
    std::string line;

    while (std::getline(std::cin, line))
    {
        if (line.compare(0, 4, "ADD ") == 0)
            // [INTV:PERF] line.substr(0,4) == "ADD "와 결과는 같지만, compare(pos, len, str)는
            // 비교용 임시 std::string을 만들지 않는다 (substr은 항상 새로 할당/복사한다).
            std::cout << (addContact(book, line.substr(4)) ? "ok\n" : "error\n");
        else if (line == "LIST")
            book.write(std::cout);
        else if (line == "QUIT")
            return 0;
        else
            std::cout << "error\n";
    }
    return 0;
}
