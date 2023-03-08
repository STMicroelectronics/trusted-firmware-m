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
from typing import List
from jinja2 import Environment, FileSystemLoader, select_autoescape
from git import Repo

cwd_path = os.path.abspath(os.getcwd())

ENV = Environment(
        loader = FileSystemLoader(cwd_path + '/template'),
        autoescape = select_autoescape(['html', 'xml']),
        lstrip_blocks = True,
        trim_blocks = True,
        keep_trailing_newline = True
    )

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

    def read(self):
        self.f_rst.seek(0)
        return self.f_rst.read()

def issue_known():

    return []

def issue_fixed():

    print("Found issue fixed in bugzilla between 2 commits:")
    g_brch = input('\tEnter gerrit branch: ')
    s_sha1 = input('\tEnter start sha1,branch or tag: ')
    e_sha1 = input('\tEnter end sha1,branch or tag: ')

    bz_list = stm32_bzlist.main(["-b", g_brch, "-s", s_sha1, "-e", e_sha1, "-c"])

    return  bz_list.bugs

def create_tag(args):

    print("tag name:{}".format(args.tag))
    repo = Repo(os.getcwd(), search_parent_directories=True)

    changelog_rst = args.release + "/changelog.rst"

    with open(changelog_rst) as fp:
        chglog_file = fp.read()

    feature_pattern = "(?P<FEATURES>New major features(.|\n)*)(?=Tested platforms)"
    feature_log = re.search(feature_pattern, chglog_file, re.DOTALL).group('FEATURES')

    tests_platform = []

    for test_rst in os.listdir(args.release):
        if test_rst.endswith('_test.rst'):
            with open(os.path.join(args.release,test_rst)) as fp:
                tests_platform.append(fp.read())

    tag_rst = RstHelper(os.path.join(args.release, "tag_" + args.tag + ".rst"))

    tag_var = {}
    tag_var['tag_tittle'] = tag_rst.to_title(args.tag, '=')
    tag_var['branch'] = repo.active_branch
    tag_var['tag'] = args.tag
    tag_var['sha1'] = repo.head.object.hexsha
    tag_var['doc_release'] = os.path.basename(args.release)
    tag_var['features_log'] = feature_log
    tag_var['tests_platform'] = tests_platform
    tag_var['k_issues'] = issue_known()
    tag_var['f_issues'] = issue_fixed()

    tag_template = ENV.get_template('tag.rst.template')
    tag_rst.write(tag_template.render(tag_var))

    repo.create_tag(args.tag, message=tag_rst.read())

def main():
    parser = argparse.ArgumentParser(description="Tag generator for stm tfm")

    # Add the arguments
    parser.add_argument('-t', '--tag', metavar='NAME', help='tag name', required=True)
    parser.add_argument('-r', '--release', metavar='PATH', help='path to doc release', required=True)

    args = parser.parse_args()

    if os.path.isdir(args.release) is False:
        raise Exception("No such path:{}".format(release))

    create_tag(args)

if __name__ == "__main__":
    # execute only if run as a script
    main()
