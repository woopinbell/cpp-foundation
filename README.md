# CPP Foundation

![Language](https://img.shields.io/badge/language-C%2B%2B98-blue?logo=cplusplus&logoColor=white)
![Build](https://img.shields.io/badge/build-Make-lightgrey)

`cpp-foundation`은 42 `CPP00`부터 `CPP09`까지의 과제를 하나의 C++98 기반 학습 프로젝트로 확장한 정적 라이브러리와 명령행 예제 모음입니다. 객체 수명, 복사 의미론, 다형성, 변환 경계, 템플릿과 예외 안전성을 공개 API와 실행 프로그램으로 검증합니다.

## 구성

- `include/`: 공개 헤더
- `src/`: 정적 라이브러리 구현
- `apps/`: 공개 API를 사용하는 예제 프로그램
- `tests/`: 단위, 통합, 실패 경계와 공개 계약 테스트

## 빌드

```sh
make
```

결과물은 다음 위치에 생성됩니다.

```text
build/lib/libcpp_foundation.a
build/bin/ex00_contact_book
build/bin/ex01_text_buffer
...
build/obj/
```

C++98 경고 계약과 공개 헤더 경로는 Makefile에 정의되어 있습니다. `CXX`와 `EXTRA_CXXFLAGS`로 컴파일러 또는 추가 플래그를 지정할 수 있습니다.

## 테스트

전체 기능 검증은 다음 명령으로 실행합니다.

```sh
make test
```

개별 검증이 필요하면 다음 타깃을 사용할 수 있습니다.

```sh
make test-unit
make failure-test
make test-contract
make test-integration
make test-property
make test-sanitize
make check
```

테스트는 정상 동작뿐 아니라 공개 헤더 소비, archive 계약, 할당 실패, 복사/팩토리/pipeline 실패, 결정성 및 플랫폼 의존성을 확인합니다.

## 예제

예제는 모두 `build/bin/` 아래에 생성됩니다. 예를 들어 연락처 예제는 다음과 같이 실행할 수 있습니다.

```sh
./build/bin/ex00_contact_book < tests/fixtures/contact-session.in
```

## 정리

```sh
make clean  # build/ 및 호환용 bin/ 산출물 삭제
make fclean # clean과 동일
make re     # fclean 후 전체 재빌드
```

생성된 실행 파일, object, archive와 테스트 산출물은 저장소에 포함하지 않습니다.
