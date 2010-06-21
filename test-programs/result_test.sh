#
# Automated Testing Framework (atf)
#
# Copyright (c) 2007, 2008, 2010 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

atf_test_case result_on_stdout
result_on_stdout_head()
{
    atf_set "descr" "Tests that the test case result is printed on stdout" \
                    "by default"
}
result_on_stdout_body()
{
    srcdir="$(atf_get_srcdir)"
    for h in $(get_helpers); do
        atf_check -s eq:0 -o match:"passed" -o match:"msg" \
            -e empty "${h}" -s "${srcdir}" result_pass
        atf_check -s eq:1 -o match:"failed: Failure reason" -o match:"msg" \
            -e empty "${h}" -s "${srcdir}" result_fail
        atf_check -s eq:0 -o match:"skipped: Skipped reason" -o match:"msg" \
            -e empty "${h}" -s "${srcdir}" result_skip
    done
}

atf_test_case result_to_file
result_to_file_head()
{
    atf_set "descr" "Tests that the test case result is sent to a file if -r" \
                    "is used"
    atf_set "use.fs" "true"
}
result_to_file_body()
{
    srcdir="$(atf_get_srcdir)"
    for h in $(get_helpers); do
        atf_check -s eq:0 -o inline:"msg\n" -e empty "${h}" -s "${srcdir}" \
            -r resfile result_pass
        atf_check -o inline:"passed\n" cat resfile

        atf_check -s eq:1 -o inline:"msg\n" -e empty "${h}" -s "${srcdir}" \
            -r resfile result_fail
        atf_check -o inline:"failed: Failure reason\n" cat resfile

        atf_check -s eq:0 -o inline:"msg\n" -e empty "${h}" -s "${srcdir}" \
            -r resfile result_skip
        atf_check -o inline:"skipped: Skipped reason\n" cat resfile
    done
}

atf_test_case reason_newlines
reason_newlines_head()
{
    atf_set "descr" "Tests that newlines provided as part of status'" \
                    "reasons are handled properly"
    atf_set "use.fs" "true"
}
reason_newlines_body()
{
    for h in $(get_helpers); do
        case ${h} in
            *sh_helpers*)
                # XXX Not implemented.
                continue
                ;;
        esac

        # NO_CHECK_STYLE_BEGIN
        cat >resexp <<EOF
failed: BOGUS REASON (THE ORIGINAL HAD NEWLINES): First line<<NEWLINE>>Second line
EOF
        # NO_CHECK_STYLE_END
        atf_check -s eq:1 -o empty -e empty "${h}" -r resfile \
            -s "$(atf_get_srcdir)" result_newlines_fail
        atf_check -s eq:0 diff -u resexp resfile

        # NO_CHECK_STYLE_BEGIN
        cat >resexp <<EOF
skipped: BOGUS REASON (THE ORIGINAL HAD NEWLINES): First line<<NEWLINE>>Second line
EOF
        # NO_CHECK_STYLE_END
        atf_check -s eq:0 -o empty -e empty "${h}" -r resfile \
            -s "$(atf_get_srcdir)" result_newlines_skip
        atf_check -s eq:0 diff -u resexp resfile
    done
}

atf_init_test_cases()
{
    atf_add_test_case result_on_stdout
    atf_add_test_case result_to_file

    atf_add_test_case reason_newlines
}

# vim: syntax=sh:expandtab:shiftwidth=4:softtabstop=4
