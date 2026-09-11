NAME := libcpp_foundation.a
BIN_DIR := build/bin
LIB_DIR := build/lib

CXX := c++
# `?=`: 조건부 대입으로 이 변수가 이미 값을 갖고 있으면(예: 외부에서 `make EXTRA_CXXFLAGS=...`로 호출) 그대로 두고, 없을 때만 빈 문자열로 설정한다.
# 아래 CXXFLAGS 안에서 참조되어, 사용자가 프로젝트 기본 플래그를 건드리지 않고 컴파일 옵션을 추가로 주입할 수 있게 하는 훅.
EXTRA_CXXFLAGS ?=
# `:=`: 즉시(simple) 평가 대입으로 정의 시점에 우변을 한 번 계산해서 값을 고정한다(재귀 확장 `=`와 달리, 나중에 참조하는 다른 변수가 바뀌어도 영향받지 않음).
# `override`: 커맨드라인 인자(`make CXXFLAGS=...`)로 이 변수를 재정의하려고 해도 무시하고 여기 값을 강제한다
# 즉 아래 경고/표준 플래그는 프로젝트가 항상 강제로 적용한다.
# 플래그 의미: -Wall/-Wextra/-Wpedantic -pedantic-errors는 경고 수준을 최대로 올리고 표준 위반을 에러로 취급, -Werror는 경고를 전부 에러로 승격(경고 하나라도 있으면 빌드 실패). 
# -std=c++98은 컴파일러가 받아들이는 언어 표준을 C++98로 고정(그 이후 문법/라이브러리 기능 차단).
# 나머지(-Wold-style-cast, -Wcast-qual, -Woverloaded-virtual, -Wnon-virtual-dtor, -Wc++11-extensions)는 C++ 특유의 위험한 패턴을 잡아내는 정적 분석성 경고들(C 스타일 캐스트, const 제거 캐스트, 오버라이드 실수, 가상 소멸자 누락, C++11 문법 실수 사용 등).
override CXXFLAGS := -Wall -Wextra -Werror -Wpedantic -pedantic-errors \
	-std=c++98 -Wold-style-cast -Wcast-qual -Woverloaded-virtual \
	-Wnon-virtual-dtor -Wc++11-extensions $(EXTRA_CXXFLAGS)
# 이름이 헷갈리기 쉬운데, CPPFLAGS는 "C++ 플래그"가 아니라 C/C++ "전처리기(preprocessor)" 플래그를 담는 관례적 변수명이다(-I 헤더 검색 경로, -D 매크로 정의 등).
# 실제 컴파일러 경고/표준 옵션은 위 CXXFLAGS 쪽. 여기서도 override로 커맨드라인 재정의를 막는다.
override CPPFLAGS := -Iinclude -Itests
# include/ 뿐, tests/ 내부 헤더는 포함하지 않는 별도 변수
# 라이브러리 사용자(외부 소비자)가 실제로 보게 될 공개 헤더만 가지고 컴파일되는지 검증할 때(PUBLIC_CONTRACT_BIN, test-contract) 쓰인다. 
# 내부 테스트 전용 헤더에 몰래 의존하는 코드가 없는지 걸러내기 위한 장치.
PUBLIC_CPPFLAGS := -Iinclude
# -MMD -MP: 컴파일할 때 이 소스가 include하는 헤더 목록을 .d 파일로 함께 생성하는 옵션.
# 파일 맨 아래 `-include $(DEP)`와 짝을 이루어, 헤더 하나만 바뀌어도 그걸 쓰는 .o만 정확히 재컴파일되게 하는 증분 빌드(incremental build)의 핵심 장치다.
DEPFLAGS := -MMD -MP
AR := ar
# ar(archiver)은 여러 오브젝트 파일(.o)을 하나의 정적 라이브러리(.a)로 묶는 유닉스 도구. rcs: r=파일 추가/교체, c=아카이브를 (경고 없이) 새로 생성, s=링커가 바로 쓸 수 있도록 심볼 인덱스를 기록.
ARFLAGS := rcs
RM := rm -f
RMDIR := rm -rf
MKDIR := mkdir -p

# $(wildcard ...): Make 내장 함수
# 글롭 패턴에 실제로 매칭되는 파일 목록을 파일시스템에서 읽어온다. 
# $(sort ...): 정렬 + 중복 제거.
# 여기서 sort를 쓰는 이유는 단순 알파벳 정렬이 아니라, 파일시스템마다 wildcard가 돌려주는 순서가 다를 수 있어(OS/파일시스템 의존적) 이를 고정시켜 빌드 결과가 항상 같은 순서로 나오게(결정론적 빌드) 하기 위함.
SRC := $(sort $(wildcard src/*.cpp))
# `$(VAR:pattern=replacement)`: Make의 치환 참조(substitution reference) 문법
# VAR에 담긴 각 경로를 pattern에서 replacement 형태로 1:1 변환한다. 
# 여기서는 src/foo.cpp 형태를 build/obj/foo.o로 매핑해서, SRC의 각 소스파일에 대응하는 오브젝트 파일 경로 목록을 만든다.
OBJ := $(SRC:src/%.cpp=build/obj/%.o)
DEP := $(OBJ:.o=.d)

APP_SRC := $(sort $(wildcard apps/*.cpp))
APP_BIN := $(APP_SRC:apps/%.cpp=$(BIN_DIR)/%)

TEST_SRC := $(sort $(wildcard tests/test_*.cpp))
TEST_SUPPORT_SRC := tests/support/TestFormatter.cpp
TEST_BIN := build/tests/unit
# 아래 FAILURE_BIN 계열은 모두 "강제로 메모리 할당을 실패시켰을 때 코드가 예외 안전(exception-safe)하게 동작하는가"를 검증하는 테스트 바이너리들이다.
# 같이 링크되는 tests/support/FailingNew.cpp가 전역 operator new를 오버라이드해서 의도적으로 할당 실패(bad_alloc)를 재현시키는 역할을 하는 것이며 C++에서만 의미있는 도메인 개념.
FAILURE_BIN := build/tests/buffer_failure
FAILURE_SRC := tests/failure/test_buffer_failure.cpp \
	tests/support/FailingNew.cpp
FACTORY_FAILURE_BIN := build/tests/factory_failure
FACTORY_FAILURE_SRC := tests/failure/test_factory_failure.cpp \
	tests/support/FailingNew.cpp
BATCH_FAILURE_BIN := build/tests/batch_failure
BATCH_FAILURE_SRC := tests/failure/test_batch_failure.cpp \
	tests/support/FailingNew.cpp
PIPELINE_FAILURE_BIN := build/tests/pipeline_failure
PIPELINE_FAILURE_SRC := tests/failure/test_pipeline_failure.cpp \
	tests/support/TestFormatter.cpp
CONTACT_FAILURE_BIN := build/tests/contact_failure
CONTACT_FAILURE_SRC := tests/failure/test_contact_failure.cpp \
	tests/support/FailingNew.cpp
NO_ELIDE_BIN := build/tests/unit_no_elide
PUBLIC_CONTRACT_BIN := build/tests/public_contract
PUBLIC_CONTRACT_SRC := tests/integration/test_public_contract.cpp
PROPERTY_BIN := build/tests/boundary_properties
PROPERTY_SRC := tests/property/test_boundary_properties.cpp
DATA_MODEL_BIN := build/tests/data_model
DATA_MODEL_SRC := tests/portability/test_data_model.cpp
ASAN_BIN := build/tests/unit_asan
UBSAN_BIN := build/tests/unit_ubsan
# ASan(AddressSanitizer)/UBSan(UndefinedBehaviorSanitizer): 컴파일러가 코드에 런타임 계측을 삽입해 각각 메모리 오류(버퍼 오버런, use-after-free, 메모리 누수)와 정의되지 않은 동작(정수 오버플로우, 정렬되지 않은 포인터 접근 등)을 실행 중에 잡아내는 도구.
# -O1: 계측 오버헤드와 스택트레이스 정확도의 균형을 맞춘 최적화 단계.
# -fno-omit-frame-pointer: 크래시 시 스택트레이스를 정확히 남기기 위해 프레임 포인터 생략 최적화를 끔.
ASAN_FLAGS := -O1 -fsanitize=address -fno-omit-frame-pointer -g
UBSAN_FLAGS := -O1 -fsanitize=undefined -fno-sanitize-recover=all \
	-fno-omit-frame-pointer -g
# check-dependencies에서 "실제 배포되는 산출물"로 취급할 바이너리 목록
# 배포용 앱 실행파일들 + 공개 API 계약을 검증하는 바이너리를 묶어서, 이 산출물들의 동적 라이브러리 의존성을 점검하는 데 쓰인다.
RELEASE_BIN := $(APP_BIN) $(PUBLIC_CONTRACT_BIN)

# .PHONY: 아래 나열된 이름들이 실제 파일이 아니라 "명령어"임을 Make에 선언하는 특수 타겟.
# 이게 없으면, 만약 작업 디렉터리에 우연히 "test" 같은 이름의 파일이 생기면 Make가 "이미 최신 상태"라고 착각해서 해당 명령을 건너뛸 수 있다.
.PHONY: all test-unit failure-test test-no-elide test-contract \
	test-integration test-consumer test-asan test-ubsan test-sanitize \
	test-leak check-archive check-data-model check-build \
	check-dependencies check-determinism test-property check-portable \
	check-platform test check clean fclean re

# 파일에서 가장 먼저 나오는 타겟이라 `make`를 인자 없이 실행했을 때 기본으로 이 타겟이 실행된다(Make의 관용 규칙)
# 라이브러리와 앱 바이너리들을 만든다.
all: $(LIB_DIR)/$(NAME) $(APP_BIN)

# 타겟: 의존성 형태의 규칙(rule) 
# $(OBJ) 목록의 오브젝트 파일 중 하나라도 이 아카이브보다 최신이면 아래 레시피가 실행된다.
$(LIB_DIR)/$(NAME): $(OBJ)
	@$(MKDIR) $(dir $@)
	$(RM) $@
	$(AR) $(ARFLAGS) $@ $(OBJ)

# 패턴 규칙(pattern rule): `%`는 와일드카드로, src/ 아래 임의의 .cpp  파일 하나하나에 대해 build/obj/ 밑에 대응하는 .o를 만드는 규칙을 한 줄로 표현한다(파일마다 규칙을 따로 안 써도 됨). 
# 자동 변수: `$@`=현재 타겟 경로(만들어질 .o), `$<`=규칙의 첫 번째 의존성(매칭된 .cpp), `$(dir $@)`=그 함수로 $@에서 디렉터리 부분만 추출해 미리 생성. `-c`는 링크 없이 컴파일만 해서 오브젝트 파일까지만 만든다.
build/obj/%.o: src/%.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR)/%: apps/%.cpp $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LIB_DIR)/$(NAME) -o $@

$(TEST_BIN): $(TEST_SRC) $(TEST_SUPPORT_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_SRC) $(TEST_SUPPORT_SRC) \
		$(LIB_DIR)/$(NAME) -o $@

# test-* 타겟들의 공통 패턴: 먼저 바이너리를 빌드(의존성)한 뒤, 그냥 실행만 한다. 이 패턴이 아래에서 여러 번 반복된다.
test-unit: $(TEST_BIN)
	./$(TEST_BIN)

$(FAILURE_BIN): $(FAILURE_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(FAILURE_SRC) $(LIB_DIR)/$(NAME) -o $@

$(FACTORY_FAILURE_BIN): $(FACTORY_FAILURE_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(FACTORY_FAILURE_SRC) $(LIB_DIR)/$(NAME) -o $@

$(BATCH_FAILURE_BIN): $(BATCH_FAILURE_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(BATCH_FAILURE_SRC) $(LIB_DIR)/$(NAME) -o $@

$(PIPELINE_FAILURE_BIN): $(PIPELINE_FAILURE_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PIPELINE_FAILURE_SRC) $(LIB_DIR)/$(NAME) -o $@

$(CONTACT_FAILURE_BIN): $(CONTACT_FAILURE_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CONTACT_FAILURE_SRC) $(LIB_DIR)/$(NAME) -o $@

failure-test: $(FAILURE_BIN) $(FACTORY_FAILURE_BIN) $(BATCH_FAILURE_BIN) \
	$(PIPELINE_FAILURE_BIN) $(CONTACT_FAILURE_BIN)
	./$(FAILURE_BIN)
	./$(FACTORY_FAILURE_BIN)
	./$(BATCH_FAILURE_BIN)
	./$(PIPELINE_FAILURE_BIN)
	./$(CONTACT_FAILURE_BIN)

# -fno-elide-constructors: 컴파일러의 복사 생략(copy elision/RVO) 최적화를 강제로 끄는 플래그.
# 이 최적화가 켜져 있으면 임시 객체의 복사/이동 생성자 호출 자체가 사라져 버그(부작용 있는 복사 생성자, 호출 횟수에 의존하는 로직 등)가 테스트에서 드러나지 않을 수 있어서, 같은 테스트 코드를 이 플래그로 한 번 더 빌드해 별도 바이너리로 검증한다.
$(NO_ELIDE_BIN): $(TEST_SRC) $(TEST_SUPPORT_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fno-elide-constructors \
		$(TEST_SRC) $(TEST_SUPPORT_SRC) $(LIB_DIR)/$(NAME) -o $@

test-no-elide: $(NO_ELIDE_BIN)
	./$(NO_ELIDE_BIN)

# 여기서는 CPPFLAGS 대신 PUBLIC_CPPFLAGS(-Iinclude만, -Itests 없음)를 써서, 외부 소비자가 실제로 보게 될 공개 헤더만으로 이 테스트가 컴파일되는지(내부 전용 헤더에 몰래 의존하지 않는지) 검증한다.
$(PUBLIC_CONTRACT_BIN): $(PUBLIC_CONTRACT_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) $(PUBLIC_CONTRACT_SRC) \
		$(LIB_DIR)/$(NAME) -o $@

test-contract:
# -fsyntax-only: 실제 오브젝트 파일이나 바이너리를 만들지 않고 문법/타입 체크만 빠르게 수행하는 컴파일러 옵션.
# 아래 각 *_headers.cpp가 단독으로(추가 소스 없이) 문제없이 컴파일되는지, 즉 공개 헤더들이 자기 완결적(self-contained)인지 확인하는 용도.
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/public_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/contact_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/text_buffer_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/format_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/factory_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/scalar_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/runtime_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/serializer_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/template_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/rpn_headers.cpp
	$(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/batch_headers.cpp
# 아래부터는 반대로 "컴파일이 실패해야 정상"인 음성 테스트(negative test)다.
# 맨 앞 `!`는 셸의 논리 부정, 뒤 명령의 종료 코드를 뒤집는다.
# (컴파일 성공(exit 0)하면 오히려 실패로 처리). 
# private 멤버 접근이나 const 위반처럼, 의도적으로 컴파일 에러가 나야 하는 코드가 실제로 에러를 내는지 검증한다.
# `@`는 명령 자체를 에코하지 않는다는 뜻이고, `>/dev/null 2>&1`은 (예상된) 컴파일 에러 메시지를 화면에 안 뿌리고 버린다.
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/contact_private_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/formatter_abstract_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/runtime_inspector_private_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/runtime_unrelated_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/runtime_integer_kind_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/serializer_private_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/serializer_const_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/rpn_evaluator_private_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/batch_results_mutation_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/contact_book_const_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/text_buffer_const_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/text_buffer_storage_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/text_buffer_implicit_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/formatter_creator_abstract_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/pipeline_builder_private_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/scalar_converter_private_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/runtime_base_constructor_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/template_const_iterator_fail.cpp >/dev/null 2>&1
	@! $(CXX) $(PUBLIC_CPPFLAGS) $(CXXFLAGS) -fsyntax-only \
		tests/compile/template_list_sort_fail.cpp >/dev/null 2>&1

test-consumer: $(LIB_DIR)/$(NAME)
# $(abspath ...): Make 내장 함수, 상대경로를 절대경로로 바꿔준다.
# 외부 셸 스크립트에 넘길 때 현재 작업 디렉터리가 무엇이든 라이브러리 위치를 정확히 알려주기 위함.
	sh tests/check_external_consumer.sh "$(CXX)" "$(abspath $(LIB_DIR)/$(NAME))"

# phony 타겟(test-consumer)을 다른 phony 타겟의 의존성으로 나열하는 것도 가능
# Make가 먼저 test-consumer의 레시피를 실행한 뒤, 이어서 아래 레시피를 실행하는 식으로 타겟들을 체이닝할 수 있다.
test-integration: $(APP_BIN) $(PUBLIC_CONTRACT_BIN) test-consumer
	sh tests/check_cli.sh
	./$(PUBLIC_CONTRACT_BIN)

$(PROPERTY_BIN): $(PROPERTY_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PROPERTY_SRC) $(LIB_DIR)/$(NAME) -o $@

# property-based testing(속성 기반 테스트): 개별 예시 값이 아니라 무작위/다양한 입력을 대량으로 넣어 "항상 성립해야 하는 불변식"을 검증하는 기법
# 때로 오래 걸리거나 멈출 수 있어 아래처럼 타임아웃 래퍼(run_with_timeout.sh)로 감싸서 30초 넘으면 강제 종료시킨다.
test-property: $(PROPERTY_BIN)
	sh tests/run_with_timeout.sh 30 ./$(PROPERTY_BIN)

# 여기서는 $(LIB_DIR)/$(NAME)(아카이브)를 링크하지 않고 $(SRC)를 직접 나열해서 라이브러리 소스 자체를 이 바이너리 안에 다시 컴파일해 넣는다.
# 산타이저 계측이 라이브러리 내부 코드에까지 적용되려면 라이브러리도 같은 sanitize 플래그로 다시 컴파일돼야 하기 때문 (이미 만들어진 일반 .a를 그냥 링크하면 그 안의 코드는 계측되지 않음).
# $(LIB_DIR)/$(NAME)는 순서 보장용 의존성으로만 남아 있다.
$(ASAN_BIN): $(SRC) $(TEST_SRC) $(TEST_SUPPORT_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ASAN_FLAGS) \
		$(SRC) $(TEST_SRC) $(TEST_SUPPORT_SRC) -o $@

$(UBSAN_BIN): $(SRC) $(TEST_SRC) $(TEST_SUPPORT_SRC) $(LIB_DIR)/$(NAME)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(UBSAN_FLAGS) \
		$(SRC) $(TEST_SRC) $(TEST_SUPPORT_SRC) -o $@

# `env VAR=value cmd`: 셸 관용구 - 이 프로세스 실행에만 한정해서 환경변수를 설정한다. ASAN_OPTIONS은 AddressSanitizer 런타임 동작을 조정하는 환경변수
# detect_leaks=1(메모리 누수도 함께 검사), halt_on_error=1(에러 발견 즉시 중단).
test-asan: $(ASAN_BIN)
	sh tests/run_with_timeout.sh 120 env \
		ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./$(ASAN_BIN)

# UBSAN_OPTIONS도 위와 같은 방식 - halt_on_error=1(발견 즉시 중단), print_stacktrace=1(문제 지점의 콜스택을 출력).
test-ubsan: $(UBSAN_BIN)
	sh tests/run_with_timeout.sh 120 env \
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		./$(UBSAN_BIN)

# $(MAKE): 재귀적 make 호출(recursive make), 단순히 다른 타겟을 의존성으로 나열하는 것과 달리, 각 줄이 독립적인 make 프로세스를 새로 실행한다.
# 이렇게 하면 이미 최신 상태라 실행을 건너뛰는 일 없이 ubsan/asan 테스트가 매번 확실히 실행된다.
test-sanitize:
	$(MAKE) test-ubsan
	$(MAKE) test-asan

test-leak: $(TEST_BIN) $(NO_ELIDE_BIN) $(PUBLIC_CONTRACT_BIN)
	sh tests/check_leaks.sh $(TEST_BIN) $(NO_ELIDE_BIN) \
		$(PUBLIC_CONTRACT_BIN)

check-archive: $(LIB_DIR)/$(NAME)
	sh tests/check_archive.sh $(LIB_DIR)/$(NAME)

check-dependencies: $(RELEASE_BIN)
	sh tests/check_dependencies.sh $(RELEASE_BIN)

# LC_ALL/LANG/TZ를 고정값(C 로케일, UTC)으로 지정해 로케일이나 시간대에 따라 출력이 달라지는 걸 막고, 같은 스크립트를 두 번 실행해 두 결과가 완전히 같은지(=결정론적으로 동작하는지) 비교하려는 의도(실제 비교 로직은 check_cli.sh 안에 있을 것).
check-determinism: $(APP_BIN)
	LC_ALL=C LANG=C TZ=UTC sh tests/check_cli.sh
	LC_ALL=C LANG=C TZ=UTC sh tests/check_cli.sh

test: test-unit failure-test test-no-elide test-contract test-integration \
	test-property

$(DATA_MODEL_BIN): $(DATA_MODEL_SRC)
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DATA_MODEL_SRC) -o $@

check-data-model: $(DATA_MODEL_BIN)
	./$(DATA_MODEL_BIN)

check-build:
# git diff --check: Git 명령 - 커밋되지 않은 변경분에 trailing whitespace나 탭/스페이스 혼용 같은 공백 문제가 있는지 검사한다.
	git diff --check
	$(MAKE) fclean
	$(MAKE) all
	$(MAKE) test
	$(MAKE) check-determinism
	$(MAKE) check-data-model
# `-q`(--question) 플래그: 실제로는 아무것도 빌드하지 않고, "할 일이 남아있는가?"만 종료 코드로 알려준다(할 일 없으면 0).
# 방금 `make all`을 했으니 다시 `-q all`을 물었을 때 "더 할 일 없음"이어야 정상
# 빌드가 불필요한 재빌드 없이 결정론적으로 끝나는지 확인하는 회귀 검증.
	$(MAKE) -q all

check-portable:
	$(MAKE) check-build
	$(MAKE) test-ubsan

check-platform:
	$(MAKE) check-archive
	$(MAKE) check-dependencies
	$(MAKE) test-leak

check:
	$(MAKE) check-portable
	$(MAKE) check-platform

clean:
	$(RMDIR) build bin

# 레시피(실행할 명령)가 없이 의존성만 있는 타겟, 다른 타겟의 별칭 (alias)처럼 동작한다.
# 즉 fclean은 지금 당장은 clean과 동일하게 동작.
fclean: clean

re: fclean all

# `-include`: 일반 `include`와 달리 대상 파일이 없어도 에러 없이 조용히 무시한다(맨 처음 빌드 시에는 .d 파일들이 아직 존재하지 않으므로 필요).
# DEP의 각 .d 파일에는 위쪽 DEPFLAGS(-MMD -MP)로 컴파일러가 자동 생성한 "이 .o가 어떤 헤더들에 의존하는지" 규칙이 들어있다.
# 이걸 Makefile에 합쳐 넣으면, 헤더 파일 하나만 바뀌어도 Make가 그 헤더를 쓰는 .o만 정확히 찾아내 재컴파일한다.
-include $(DEP)