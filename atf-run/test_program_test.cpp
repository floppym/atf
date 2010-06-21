//
// Automated Testing Framework (atf)
//
// Copyright (c) 2010 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <fstream>
#include <iostream>

#include "atf-c++/macros.hpp"
#include "atf-c++/parser.hpp"
#include "atf-c++/text.hpp"

#include "test-program.hpp"

#include "h_lib.hpp"

namespace impl = atf::atf_run;
namespace detail = atf::atf_run::detail;

using atf::tests::vars_map;

// -------------------------------------------------------------------------
// Auxiliary functions.
// -------------------------------------------------------------------------

static
atf::fs::path
get_helper(const atf::tests::tc& tc, const char* name)
{
    return atf::fs::path(tc.get_config_var("srcdir")) / name;
}

static
void
check_property(const vars_map& props, const char* name, const char* value)
{
    const vars_map::const_iterator iter = props.find(name);
    ATF_CHECK(iter != props.end());
    ATF_CHECK_EQUAL(value, (*iter).second);
}

static
void
write_test_case_result(const char *results_path, const std::string& contents)
{
    std::ofstream results_file(results_path);
    ATF_CHECK(results_file);

    results_file << contents;
}

static
void
print_indented(const std::string& str)
{
    std::vector< std::string > ws = atf::text::split(str, "\n");
    for (std::vector< std::string >::const_iterator iter = ws.begin();
         iter != ws.end(); iter++)
        std::cout << ">>" << *iter << "<<" << std::endl;
}

// XXX Should this string handling and verbosity level be part of the
// ATF_CHECK_EQUAL macro?  It may be hard to predict sometimes that a
// string can have newlines in it, and so the error message generated
// at the moment will be bogus if there are some.
static
void
check_equal(const std::string& str, const std::string& exp)
{
    if (str != exp) {
        std::cout << "String equality check failed." << std::endl
                  << "Adding >> and << to delimit the string boundaries "
                     "below." << std::endl;
        std::cout << "GOT:" << std::endl;
        print_indented(str);
        std::cout << "EXPECTED:" << std::endl;
        print_indented(exp);
        atf_tc_fail("Constructed string differs from the expected one");
    }
}

// -------------------------------------------------------------------------
// Tests for the "tp" reader.
// -------------------------------------------------------------------------

class tp_reader : protected detail::atf_tp_reader {
    void
    got_tc(const std::string& ident,
           const std::map< std::string, std::string >& md)
    {
        std::string call = "got_tc(" + ident + ", {";
        for (std::map< std::string, std::string >::const_iterator iter =
             md.begin(); iter != md.end(); iter++) {
            if (iter != md.begin())
                call += ", ";
            call += (*iter).first + '=' + (*iter).second;
        }
        call += "})";
        m_calls.push_back(call);
    }

    void
    got_eof(void)
    {
        m_calls.push_back("got_eof()");
    }

public:
    tp_reader(std::istream& is) :
        detail::atf_tp_reader(is)
    {
    }

    void
    read(void)
    {
        atf_tp_reader::read();
    }

    std::vector< std::string > m_calls;
};

ATF_TEST_CASE_WITHOUT_HEAD(tp_1);
ATF_TEST_CASE_BODY(tp_1)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test_case_1\n"
        "\n"
        "ident: test_case_2\n"
        "\n"
        "ident: test_case_3\n"
    ;

    const char* exp_calls[] = {
        "got_tc(test_case_1, {ident=test_case_1})",
        "got_tc(test_case_2, {ident=test_case_2})",
        "got_tc(test_case_3, {ident=test_case_3})",
        "got_eof()",
        NULL
    };

    const char* exp_errors[] = {
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_2);
ATF_TEST_CASE_BODY(tp_2)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test_case_1\n"
        "descr: This is the description\n"
        "timeout: 300\n"
        "\n"
        "ident: test_case_2\n"
        "\n"
        "ident: test_case_3\n"
        "X-prop1: A custom property\n"
        "descr: Third test case\n"
    ;

    // NO_CHECK_STYLE_BEGIN
    const char* exp_calls[] = {
        "got_tc(test_case_1, {descr=This is the description, ident=test_case_1, timeout=300})",
        "got_tc(test_case_2, {ident=test_case_2})",
        "got_tc(test_case_3, {X-prop1=A custom property, descr=Third test case, ident=test_case_3})",
        "got_eof()",
        NULL
    };
    // NO_CHECK_STYLE_END

    const char* exp_errors[] = {
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_3);
ATF_TEST_CASE_BODY(tp_3)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: single_test\n"
        "descr: Some description\n"
        "timeout: 300\n"
        "require.arch: thearch\n"
        "require.config: foo-bar\n"
        "require.machine: themachine\n"
        "require.progs: /bin/cp mv\n"
        "require.user: root\n"
    ;

    // NO_CHECK_STYLE_BEGIN
    const char* exp_calls[] = {
        "got_tc(single_test, {descr=Some description, ident=single_test, require.arch=thearch, require.config=foo-bar, require.machine=themachine, require.progs=/bin/cp mv, require.user=root, timeout=300})",
        "got_eof()",
        NULL
    };
    // NO_CHECK_STYLE_END

    const char* exp_errors[] = {
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_4);
ATF_TEST_CASE_BODY(tp_4)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident:   single_test    \n"
        "descr:      Some description	\n"
    ;

    const char* exp_calls[] = {
        "got_tc(single_test, {descr=Some description, ident=single_test})",
        "got_eof()",
        NULL
    };

    const char* exp_errors[] = {
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_50);
ATF_TEST_CASE_BODY(tp_50)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "3: Unexpected token `<<EOF>>'; expected property name",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_51);
ATF_TEST_CASE_BODY(tp_51)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "\n"
        "\n"
        "\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "3: Unexpected token `<<NEWLINE>>'; expected property name",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_52);
ATF_TEST_CASE_BODY(tp_52)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test1\n"
        "ident: test2\n"
    ;

    const char* exp_calls[] = {
        "got_tc(test1, {ident=test1})",
        "got_eof()",
        NULL
    };

    const char* exp_errors[] = {
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_53);
ATF_TEST_CASE_BODY(tp_53)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "descr: Out of order\n"
        "ident: test1\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "3: First property of a test case must be 'ident'",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_54);
ATF_TEST_CASE_BODY(tp_54)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident:\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "3: The value for 'ident' cannot be empty",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_55);
ATF_TEST_CASE_BODY(tp_55)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: +*,\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "3: The identifier must match ^[_A-Za-z0-9]+$; was '+*,'",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_56);
ATF_TEST_CASE_BODY(tp_56)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test\n"
        "timeout: hello\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "4: The timeout property requires an integer value",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_57);
ATF_TEST_CASE_BODY(tp_57)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test\n"
        "unknown: property\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "4: Unknown property 'unknown'",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_58);
ATF_TEST_CASE_BODY(tp_58)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "ident: test\n"
        "X-foo:\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "4: The value for 'X-foo' cannot be empty",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

ATF_TEST_CASE_WITHOUT_HEAD(tp_59);
ATF_TEST_CASE_BODY(tp_59)
{
    const char* input =
        "Content-Type: application/X-atf-tp; version=\"1\"\n"
        "\n"
        "\n"
        "ident: test\n"
        "timeout: 300\n"
    ;

    const char* exp_calls[] = {
        NULL
    };

    const char* exp_errors[] = {
        "3: Unexpected token `<<NEWLINE>>'; expected property name",
        NULL
    };

    do_parser_test< tp_reader >(input, exp_calls, exp_errors);
}

// -------------------------------------------------------------------------
// Tests for the "tps" writer.
// -------------------------------------------------------------------------

ATF_TEST_CASE(atf_tps_writer);
ATF_TEST_CASE_HEAD(atf_tps_writer)
{
    set_md_var("descr", "Verifies the application/X-atf-tps writer");
}
ATF_TEST_CASE_BODY(atf_tps_writer)
{
    std::ostringstream expss;
    std::ostringstream ss;

#define RESET \
    expss.str(""); \
    ss.str("")

#define CHECK \
    check_equal(ss.str(), expss.str())

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.info("foo", "bar");
        expss << "info: foo, bar" << std::endl;
        CHECK;

        w.info("baz", "second info");
        expss << "info: baz, second info" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(0);
        expss << "tps-count: 0" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(123);
        expss << "tps-count: 123" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(2);
        expss << "tps-count: 2" << std::endl;
        CHECK;

        w.start_tp("foo", 0);
        expss << "tp-start: foo, 0" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: foo" << std::endl;
        CHECK;

        w.start_tp("bar", 0);
        expss << "tp-start: bar, 0" << std::endl;
        CHECK;

        w.end_tp("failed program");
        expss << "tp-end: bar, failed program" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("foo", 1);
        expss << "tp-start: foo, 1" << std::endl;
        CHECK;

        w.start_tc("brokentc");
        expss << "tc-start: brokentc" << std::endl;
        CHECK;

        w.end_tp("aborted");
        expss << "tp-end: foo, aborted" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("thetp", 3);
        expss << "tp-start: thetp, 3" << std::endl;
        CHECK;

        w.start_tc("passtc");
        expss << "tc-start: passtc" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::passed_state));
        expss << "tc-end: passtc, passed" << std::endl;
        CHECK;

        w.start_tc("failtc");
        expss << "tc-start: failtc" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::failed_state,
                                 "The reason"));
        expss << "tc-end: failtc, failed, The reason" << std::endl;
        CHECK;

        w.start_tc("skiptc");
        expss << "tc-start: skiptc" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::skipped_state,
                                 "The reason"));
        expss << "tc-end: skiptc, skipped, The reason" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: thetp" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("thetp", 1);
        expss << "tp-start: thetp, 1" << std::endl;
        CHECK;

        w.start_tc("thetc");
        expss << "tc-start: thetc" << std::endl;
        CHECK;

        w.stdout_tc("a line");
        expss << "tc-so:a line" << std::endl;
        CHECK;

        w.stdout_tc("another line");
        expss << "tc-so:another line" << std::endl;
        CHECK;

        w.stderr_tc("an error message");
        expss << "tc-se:an error message" << std::endl;
        CHECK;

        w.end_tc(atf::tests::tcr(atf::tests::tcr::passed_state));
        expss << "tc-end: thetc, passed" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: thetp" << std::endl;
        CHECK;
    }

    {
        RESET;

        impl::atf_tps_writer w(ss);
        expss << "Content-Type: application/X-atf-tps; version=\"2\""
              << std::endl << std::endl;
        CHECK;

        w.ntps(1);
        expss << "tps-count: 1" << std::endl;
        CHECK;

        w.start_tp("thetp", 0);
        expss << "tp-start: thetp, 0" << std::endl;
        CHECK;

        w.end_tp("");
        expss << "tp-end: thetp" << std::endl;
        CHECK;

        w.info("foo", "bar");
        expss << "info: foo, bar" << std::endl;
        CHECK;

        w.info("baz", "second value");
        expss << "info: baz, second value" << std::endl;
        CHECK;
    }

#undef CHECK
#undef RESET
}

// -------------------------------------------------------------------------
// Tests for the free functions.
// -------------------------------------------------------------------------

ATF_TEST_CASE(get_metadata_bad);
ATF_TEST_CASE_HEAD(get_metadata_bad) {}
ATF_TEST_CASE_BODY(get_metadata_bad) {
    const atf::fs::path executable = get_helper(*this, "bad_metadata_helper");
    ATF_CHECK_THROW(atf::parser::parse_errors,
                    impl::get_metadata(executable, vars_map()));
}

ATF_TEST_CASE(get_metadata_zero_tcs);
ATF_TEST_CASE_HEAD(get_metadata_zero_tcs) {}
ATF_TEST_CASE_BODY(get_metadata_zero_tcs) {
    const atf::fs::path executable = get_helper(*this, "zero_tcs_helper");
    ATF_CHECK_THROW(atf::parser::parse_errors,
                    impl::get_metadata(executable, vars_map()));
}

ATF_TEST_CASE(get_metadata_several_tcs);
ATF_TEST_CASE_HEAD(get_metadata_several_tcs) {}
ATF_TEST_CASE_BODY(get_metadata_several_tcs) {
    const atf::fs::path executable = get_helper(*this, "several_tcs_helper");
    const impl::metadata md = impl::get_metadata(executable, vars_map());
    ATF_CHECK_EQUAL(3, md.test_cases.size());

    {
        const impl::test_cases_map::const_iterator iter =
            md.test_cases.find("first");
        ATF_CHECK(iter != md.test_cases.end());

        ATF_CHECK_EQUAL(5, (*iter).second.size());
        check_property((*iter).second, "descr", "Description 1");
        check_property((*iter).second, "has.cleanup", "false");
        check_property((*iter).second, "ident", "first");
        check_property((*iter).second, "timeout", "300");
        check_property((*iter).second, "use.fs", "false");
    }

    {
        const impl::test_cases_map::const_iterator iter =
            md.test_cases.find("second");
        ATF_CHECK(iter != md.test_cases.end());

        ATF_CHECK_EQUAL(6, (*iter).second.size());
        check_property((*iter).second, "descr", "Description 2");
        check_property((*iter).second, "has.cleanup", "true");
        check_property((*iter).second, "ident", "second");
        check_property((*iter).second, "timeout", "500");
        check_property((*iter).second, "use.fs", "true");
        check_property((*iter).second, "X-property", "Custom property");
    }

    {
        const impl::test_cases_map::const_iterator iter =
            md.test_cases.find("third");
        ATF_CHECK(iter != md.test_cases.end());

        ATF_CHECK_EQUAL(4, (*iter).second.size());
        check_property((*iter).second, "has.cleanup", "false");
        check_property((*iter).second, "ident", "third");
        check_property((*iter).second, "timeout", "300");
        check_property((*iter).second, "use.fs", "false");
    }
}

ATF_TEST_CASE(read_test_case_result_no_file);
ATF_TEST_CASE_HEAD(read_test_case_result_no_file) {}
ATF_TEST_CASE_BODY(read_test_case_result_no_file) {
    ATF_CHECK_THROW(std::runtime_error,
                    impl::read_test_case_result(atf::fs::path("resfile")));
}

ATF_TEST_CASE(read_test_case_result_empty_file);
ATF_TEST_CASE_HEAD(read_test_case_result_empty_file) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_test_case_result_empty_file) {
    write_test_case_result("resfile", "");
    ATF_CHECK_THROW(std::runtime_error,
                    impl::read_test_case_result(atf::fs::path("resfile")));
}

ATF_TEST_CASE(read_test_case_result_invalid);
ATF_TEST_CASE_HEAD(read_test_case_result_invalid) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_test_case_result_invalid) {
    write_test_case_result("resfile", "passed: hello\n");
    ATF_CHECK_THROW(std::runtime_error,
                    impl::read_test_case_result(atf::fs::path("resfile")));
}

ATF_TEST_CASE(read_test_case_result_passed);
ATF_TEST_CASE_HEAD(read_test_case_result_passed) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_test_case_result_passed) {
    write_test_case_result("resfile", "passed\n");
    const atf::tests::tcr tcr = impl::read_test_case_result(atf::fs::path(
        "resfile"));
    ATF_CHECK_EQUAL(atf::tests::tcr::passed_state, tcr.get_state());
}

ATF_TEST_CASE(read_test_case_result_failed);
ATF_TEST_CASE_HEAD(read_test_case_result_failed) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_test_case_result_failed) {
    write_test_case_result("resfile", "failed: foo bar\n");
    const atf::tests::tcr tcr = impl::read_test_case_result(atf::fs::path(
        "resfile"));
    ATF_CHECK_EQUAL(atf::tests::tcr::failed_state, tcr.get_state());
    ATF_CHECK_EQUAL("foo bar", tcr.get_reason());
}

ATF_TEST_CASE(read_test_case_result_skipped);
ATF_TEST_CASE_HEAD(read_test_case_result_skipped) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_test_case_result_skipped) {
    write_test_case_result("resfile", "skipped: baz bar\n");
    const atf::tests::tcr tcr = impl::read_test_case_result(atf::fs::path(
        "resfile"));
    ATF_CHECK_EQUAL(atf::tests::tcr::skipped_state, tcr.get_state());
    ATF_CHECK_EQUAL("baz bar", tcr.get_reason());
}

ATF_TEST_CASE(read_test_case_result_multiline);
ATF_TEST_CASE_HEAD(read_test_case_result_multiline) {
    set_md_var("use.fs", "true");
}
ATF_TEST_CASE_BODY(read_test_case_result_multiline) {
    write_test_case_result("resfile", "skipped: foo\nbar\n");
    const atf::tests::tcr tcr = impl::read_test_case_result(atf::fs::path(
        "resfile"));
    ATF_CHECK_EQUAL(atf::tests::tcr::skipped_state, tcr.get_state());
    ATF_CHECK_EQUAL("foo<<NEWLINE UNEXPECTED>>bar", tcr.get_reason());
}

// -------------------------------------------------------------------------
// Main.
// -------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, tp_1);
    ATF_ADD_TEST_CASE(tcs, tp_2);
    ATF_ADD_TEST_CASE(tcs, tp_3);
    ATF_ADD_TEST_CASE(tcs, tp_4);
    ATF_ADD_TEST_CASE(tcs, tp_50);
    ATF_ADD_TEST_CASE(tcs, tp_51);
    ATF_ADD_TEST_CASE(tcs, tp_52);
    ATF_ADD_TEST_CASE(tcs, tp_53);
    ATF_ADD_TEST_CASE(tcs, tp_54);
    ATF_ADD_TEST_CASE(tcs, tp_55);
    ATF_ADD_TEST_CASE(tcs, tp_56);
    ATF_ADD_TEST_CASE(tcs, tp_57);
    ATF_ADD_TEST_CASE(tcs, tp_58);
    ATF_ADD_TEST_CASE(tcs, tp_59);

    ATF_ADD_TEST_CASE(tcs, atf_tps_writer);

    ATF_ADD_TEST_CASE(tcs, get_metadata_bad);
    ATF_ADD_TEST_CASE(tcs, get_metadata_zero_tcs);
    ATF_ADD_TEST_CASE(tcs, get_metadata_several_tcs);

    ATF_ADD_TEST_CASE(tcs, read_test_case_result_no_file);
    ATF_ADD_TEST_CASE(tcs, read_test_case_result_empty_file);
    ATF_ADD_TEST_CASE(tcs, read_test_case_result_multiline);
    ATF_ADD_TEST_CASE(tcs, read_test_case_result_invalid);
    ATF_ADD_TEST_CASE(tcs, read_test_case_result_passed);
    ATF_ADD_TEST_CASE(tcs, read_test_case_result_failed);
    ATF_ADD_TEST_CASE(tcs, read_test_case_result_skipped);

    // TODO: Add tests for run_test_case once all the missing functionality
    // is implemented.
}
