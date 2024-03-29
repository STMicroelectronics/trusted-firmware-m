#!/usr/bin/env python
#
# GPL HEADER START
#
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 only,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License version 2 for more details (a copy is included
# in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU General Public License
# version 2 along with this program; If not, see
# http://www.gnu.org/licenses/gpl-2.0.html
#
# GPL HEADER END
#
"""
Git gerrit bugzila tracing
~~~~~~ ~~~~~~~~~~ ~~~~~~~~

* find commit & changeid between 2 sha1.
* convert to gerrit id.
* find bugzilla list of gerrit id.
"""
import argparse
import bugzilla
import base64
import fnmatch
import logging
import json
import os
import requests
import subprocess
import time
import urllib
from subprocess import check_output

requests.packages.urllib3.disable_warnings()

cwd_path = os.path.abspath(os.getcwd())

def _getenv_list(key, default=None, sep=':'):
    """
    'PATH' => ['/bin', '/usr/bin', ...]
    """
    value = os.getenv(key)
    if value is None:
        return default
    else:
        return value.split(sep)

GERRIT_HOST = os.getenv('GERRIT_HOST', 'gerrit.st.com')
GERRIT_PROJECT = os.getenv('GERRIT_PROJECT', 'mpu/oe/st/TF-M/trusted-firmware-m')
GERRIT_BRANCH = os.getenv('GERRIT_BRANCH', 'stm32mp25-dev')

BUGZILLA_HOST = os.getenv('BUGZILLA_HOST', 'intbugzilla.st.com')
BUGZILLA_PRODUCT = os.getenv('BUGZILLA_PRODUCT', 'STM32MP25')
BUGZILLA_SUB_SYS = os.getenv('BUGZILLA_SUB_SYS', 'MPU_TFM')


ST_AUTH_PATH = os.getenv('ST_AUTH_PATH', '/local/home/frq09524/.st/AUTH')

# GERRIT_AUTH should contain a single JSON dictionary of the form:
# {
#     "review.example.com": {
#         "gerrit/http": {
#             "username": "example-checkpatch",
#             "password": "1234"
#         }
#     }
#     ...
# }

class BugZilla(object):
    """
    * group bugzilla request
    """
    def __init__(self, host, sub_sys):
        self.host = host
        self.sub_sys = sub_sys
        with open(ST_AUTH_PATH) as auth_file:
            auth = json.load(auth_file)
            self.api_key = auth[BUGZILLA_HOST]['bugzilla/https']['api_key']

        self.url = 'https://' + self.host + '/rest/'
        self.bzapi = bugzilla.Bugzilla(url=self.url, api_key=self.api_key)

    def get_buglist(self, commit_revision, cgi_link=False):
        """
        * &include_fields=id,summary,status,cf_description&f1=cf_commit_revision&o1=anywordssubstr&cf_subsystem=MPU_TFM&v1=<commit_revision>
        """
        if not commit_revision:
            commit_str = "000000"
        else:
            commit_str = " ".join(str(e) for e in commit_revision)

        include_fields = "id,summary,cf_description,status"

        terms = [{'include_fields': include_fields},
                 {'f1': 'cf_commit_revision'},
                 {'o1': 'anywordssubstr'},
                 {'cf_subsystem': self.sub_sys},
                 {'query_format': 'advanced'},
                 {'v1': commit_str}]

        if cgi_link:
            query=''
            for d in terms:
                query ='{}&{}={}'.format(query,
                                         urllib.parse.quote(list(d.keys())[0]),
                                         urllib.parse.quote(list(d.values())[0]))

            logging.info("buglist: https://{}/buglist.cgi?{}".format(self.host, query))

        return self.bzapi.search_bugs(terms)

class Gerrit(object):
    """
     * group gerrit request
    """
    def __init__(self, host, project, branch):
        self.request_timeout = 60
        self.host = host
        self.project = project
        self.branch = branch

        with open(ST_AUTH_PATH) as auth_file:
            auth = json.load(auth_file)
            username = auth[GERRIT_HOST]['gerrit/https']['username']
            password = auth[GERRIT_HOST]['gerrit/https']['password']

        self.gerrit_auth = requests.auth.HTTPBasicAuth(username, password)

    def _debug(self, msg, *args):
        """_"""
        self.logger.debug(msg, *args)

    def _error(self, msg, *args):
        """_"""
        self.logger.error(msg, *args)

    def _url(self, path):
        """_"""
        return 'https://' + self.host + '/a' + path

    def _get(self, path):
        """
        * GET path return Response.
        """
        url = self._url(path)
        try:
            res = requests.get(url, auth=self.gerrit_auth,
                               timeout=self.request_timeout,
                               verify=False)

        except Exception as exc:
            self._error("cannot GET '%s': exception = %s", url, str(exc))
            return None

        if res.status_code != requests.codes.ok:
            #self._debug("cannot GET '%s': reason = %s, status_code = %d", url, res.reason, res.status_code)
            return None

        return res

    def get_change(self, change_id):
        """
        * GET a list of ChangeInfo()s for all changes matching query.
        """
        path = ('/changes/' + '%s~%s~%s' %
                (urllib.parse.quote_plus(self.project),
                 urllib.parse.quote_plus(self.branch),
                 urllib.parse.quote_plus(change_id)))

        res = self._get(path)

        if res is None:
            return None

        # Gerrit uses " )]}'" to guard against XSSI.
        gerrit_response = res.content.decode('utf-8').lstrip(')]}\'[').strip()
        return json.loads(gerrit_response)

    def get_gerrit_id(self, change_list):
        """
        * parameter: change_id list
        * return gerrit id list
        """
        gerrit_id = []
        for ch_id in change_list:
            change_info = self.get_change(ch_id)

            if change_info is None:
                continue

            gerrit_id.append(change_info.get('_number'))

        return gerrit_id

def parse_ch_id(log):
    """
     * git.
    """
    ch_id_list = []

    for line in log.splitlines():
        ch_id = line.split("Change-Id:")
        if len(ch_id) == 2:
            ch_id_list.append(ch_id[1].strip())

    return ch_id_list

def main(args):
    """_"""
    logging.basicConfig(format='%(asctime)s %(message)s', level=logging.INFO)
#    logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)

    parser = argparse.ArgumentParser(description="release issues tracing")

    # Add the arguments
    parser.add_argument('-p', '--prj_path', metavar='PPATH', help='project path')
    parser.add_argument('-b', '--gerrit_branch', metavar='BRANCH', help='gerrit branch name')
    parser.add_argument('-s', '--start', metavar='START', help='start sha1,tag')
    parser.add_argument('-e', '--end', metavar='END', help='stop sha1,tag')
    parser.add_argument('--ex_start', metavar='EX_START', help='exclude start sha1,tag')
    parser.add_argument('--ex_stop', metavar='EX_STOP', help='exclude stop sha1,tag')
    parser.add_argument('-c', '--cgi_link', help='view cgi bugzilla list', action='store_true')

    args = parser.parse_args(args)

    if args.prj_path is None:
        args.prj_path = cwd_path

    if args.gerrit_branch is None:
        args.gerrit_branch = GERRIT_BRANCH

    logging.debug("use {} git repositorie".format(args.prj_path))

    git_log="git log " + args.start + ".." + args.end
    log = check_output([git_log], shell=True, cwd=args.prj_path).decode('UTF-8').strip()
    ch_ids = parse_ch_id(log)

    logging.info("waiting gerrit id request...")
    logging.info("gerrit project:{} branch:{}".format(GERRIT_PROJECT, args.gerrit_branch))

    gerrit = Gerrit(GERRIT_HOST, GERRIT_PROJECT, args.gerrit_branch)
    gerrit_id = gerrit.get_gerrit_id(ch_ids)
    logging.debug("gerrit_id:{}".format(gerrit_id))

    logging.info("waiting bugzilla request...")
    bugtool = BugZilla(BUGZILLA_HOST, BUGZILLA_SUB_SYS)
    blist = bugtool.get_buglist(gerrit_id, args.cgi_link)
    logging.debug("nb bz found:{}".format(len(blist.bugs)))

    return blist

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
