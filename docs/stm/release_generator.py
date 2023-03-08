#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# SPDX-License-Identifier: BSD-3-Clause
#
# Author: Ludovic.barre@st.com
# -----------------------------------------------------------------------------
import os
import re
import argparse
import stm32_bzlist
from subprocess import check_output
from dataclasses import dataclass, field
from typing import List
from jinja2 import Environment, FileSystemLoader, select_autoescape

cwd_path = os.path.abspath(os.getcwd())

ENV = Environment(
        loader = FileSystemLoader(cwd_path + '/template'),
        autoescape = select_autoescape(['html', 'xml']),
        lstrip_blocks = True,
        trim_blocks = True,
        keep_trailing_newline = True
    )

@dataclass
class Test:
    name: str
    desc: str
    res: str

@dataclass
class Testsuite:
    name: str
    test_suite: List[Test] = field(default_factory=lambda : [])

def tfm_log2testsuite(log):

    sum_pattern = re.compile(r"Test suite \'(?P<DESC>.*?)\s*\((?P<NAME>\w+)\)\'\s+has\s+(?P<RES>\w+)")
    testsuites_pattern = re.compile(r"(?:Running Test Suite.*?TESTSUITE PASSED!)", re.DOTALL)
    testsuite_pattern = re.compile(r"Running Test Suite (?P<DESC>.*?)\s*\((?P<NAME>\w+)\)...")
    test_pattern = re.compile(r"> Executing \'(?P<NAME>\w+)\'[\s|\w|>]*Description.*\'(?P<DESC>.*)\'[\s|\w|>]*TEST.*- (?P<RES>\w+)!")
    testsuites = []

    summary = Testsuite("Summary")

    for m in re.finditer(sum_pattern, log[0]):
        summary.test_suite += [Test(m.group('NAME'), m.group('DESC'), m.group('RES'))]

    testsuites += [summary]

    for ts in re.findall(testsuites_pattern, log[0]):
        suite =  re.search(testsuite_pattern, ts)
        tsuite = Testsuite(suite.group('NAME'))

        for t in re.finditer(test_pattern, ts):
            tsuite.test_suite += [Test(t.group('NAME'), t.group('DESC'), t.group('RES'))]

        testsuites += [tsuite]

    return testsuites

class RstHelper(object):
    """
    Helper class to open, write the rst file, convert string to title...
    """
    def __init__(self, outfile):
        self.outfile = outfile
        self.f_rst = open(outfile, "w+")

    def to_title(self, title, marker='#'):
        return title + '\n' + marker * len(title) + '\n'

    def write(self,contents):
        self.f_rst.write(contents)

def create_release_test(logfile, release_path):
    rst_tests_var = {}

    if logfile:
        model_pattern ="welcome to (?P<MODEL>.*)"
        s_testsuites_pattern = "(?:Execute test suites for the Secure area.*?End of Secure test suites)"
        ns_testsuites_pattern = "(?:Execute test suites for the Non-secure area.*?End of Non-secure test suites)"

        with open(logfile) as fp:
            log = fp.read()

        platform = re.search(model_pattern, log).group('MODEL').replace(" ","_")
        log_s = re.findall(s_testsuites_pattern, log, re.DOTALL)
        log_ns = re.findall(ns_testsuites_pattern, log, re.DOTALL)

        rst_tests_var['s_testsuites'] = tfm_log2testsuite(log_s)
        rst_tests_var['ns_testsuites'] = tfm_log2testsuite(log_ns)

    else:
        platform = "no_board"

    print("logfile:{} release path:{} platform:{}".format(logfile, release_path, platform))

    test_template = ENV.get_template('platform_tests.rst.template')
    test_rst = RstHelper(release_path + "/" + platform + "_test.rst")

    rst_tests_var['platform_title'] = test_rst.to_title(platform, '"')
    test_rst.write(test_template.render(rst_tests_var))

def create_changelog(release_name, release_path):

    chglog_template = ENV.get_template('changelog.rst.template')
    chglog_rst = RstHelper(release_path + "/" + "/changelog.rst")

    rst_chglog_var = {}
    rst_chglog_var['release_title'] = chglog_rst.to_title(release_name, '=')
    rst_chglog_var['release_summary'] = ""
    rst_chglog_var['release_chglog'] = ""

    chglog_rst.write(chglog_template.render(rst_chglog_var))

def create_issue(args, release_path):

    issue_template = ENV.get_template('issues.rst.template')
    issue_rst = RstHelper(release_path + "/" + "/issues.rst")

    rst_issues_var = {}
    rst_issues_var['k_issues'] = []

    issue_rst.write(issue_template.render(rst_issues_var))

def create_issue_fixed(args, release_path):

    fixed_template = ENV.get_template('fixed.rst.template')
    fixed_rst = RstHelper(release_path + "/" + "/fixed.rst")

    print("Found issue fixed in bugzilla between 2 commits:")
    g_brch = input('\tEnter gerrit branch:')
    s_sha1 = input('\tEnter start sha1,branch or tag:')
    e_sha1 = input('\tEnter end sha1,branch or tag:')

    bz_list = stm32_bzlist.main(["-b", g_brch, "-s", s_sha1, "-e", e_sha1, "-c"])

    rst_fixed_var = {}
    rst_fixed_var['f_issues'] = bz_list.bugs

    fixed_rst.write(fixed_template.render(rst_fixed_var))

def git_find_tag():
    tag = check_output(["git", "describe", "--tags"], cwd=cwd_path).decode('UTF-8').strip()

    pattern = re.compile(r'(TF-Mv|v)'
                         r'(?P<VER_MAJ>\d{1,2}).(?P<VER_MIN>\d{1,2}).?(?P<VER_HOT>\d{0,2})'
                         r'-?(?P<VER_PLAT>\w+)?-?(?P<VER_R>r\d)?-?(?P<RC>rc\d)?'
                         r'-?(?P<PATCH_NO>\d+)?-?(?P<GIT_HASH>[a-g0-9]+)?')

    if pattern.match(tag):
        return tag

    return None

def create_release(args, outputdir):
    release_name = check_output(["git", "describe", "--tags"], cwd=cwd_path).decode('UTF-8').strip()

    release_name = args.release
    if release_name is None:
        release_name = git_find_tag()
        if release_name is None:
            raise Exception("No tag release defined in parameter or git")

    print("release name:{}".format(release_name))
    release_path = outputdir + "/" + release_name

    if os.path.isdir(release_path) is False:
        os.mkdir(release_path)

    create_changelog(release_name, release_path)

    if args.logfile:
        for logfile in args.logfile:
            create_release_test(logfile, release_path)
    else:
        create_release_test("", release_path)

    create_issue(args, release_path)
    create_issue_fixed(args, release_path)

def main():
    parser = argparse.ArgumentParser(description="Release generator for stm tfm")

    # Add the arguments
    parser.add_argument('-r', '--release', metavar='NAME', help='release name, else find the close tag')
    parser.add_argument('-l', '--logfile', metavar='FILE', help='path to logfile', action='append')
    parser.add_argument('-o', '--outdir', metavar='FILE', help='output directory to store release')

    args = parser.parse_args()

    release_name = check_output(["git", "describe", "--tags"], cwd=cwd_path).decode('UTF-8').strip()

    outdir = args.outdir

    if outdir is None:
        outdir = cwd_path + "/releases"
        print("Use {} path to store the releases".format(outdir))

    if os.path.isdir(outdir) is False:
        raise Exception("No such directory:{}".format(outdir))

    create_release(args, outdir)


if __name__ == "__main__":
    # execute only if run as a script
    main()
