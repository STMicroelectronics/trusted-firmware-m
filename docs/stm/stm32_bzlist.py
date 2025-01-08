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

TFM_STM_TOOLS_ENV = os.getenv('TFM_STM_TOOLS_ENV', cwd_path + '/tools_env.json')

# GERRIT & BUGZILLA AUTH should contain a single JSON dictionary of the form:
# {
#     "review.example.com": {
#         "gerrit/http": {
#             "username": "example-checkpatch",
#             "password": "1234"
#         }
#     },
#     "intbugzilla.st.com": {
#		"bugzilla/https": {
#			"api_key": "kdjskdjskdj"
#		}
#	}
#	...
# }
#
#

class BugZilla(object):
    """
    * group bugzilla request
    """
    def __init__(self, host, interface, auth_file):
        self.host = host

        with open(auth_file) as a_file:
            auth = json.load(a_file)
            self.api_key = auth[host][interface]['api_key']

        self.url = 'https://' + self.host + '/rest/'
        self.bzapi = bugzilla.Bugzilla(url=self.url, api_key=self.api_key)

    def get_buglist(self, commit_revision, sub_sys, cgi_link=False):
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
                 {'cf_subsystem': sub_sys},
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
    def __init__(self, host, interface, auth_file, project, branch):
        self.request_timeout = 60
        self.host = host
        self.project = project
        self.branch = branch

        with open(auth_file) as a_file:
            auth = json.load(a_file)
            username = auth[host][interface]['username']
            password = auth[host][interface]['password']

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

def config_get_project_by_name(json_projects, project_name):

    for project in json_projects:
        if project['name'] == project_name:
            return project

    return None

def main(args):
    """_"""
    logging.basicConfig(format='%(asctime)s %(message)s', level=logging.INFO)
    #logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)

    parser = argparse.ArgumentParser(description="release issues tracing")

    # Add the arguments
    parser.add_argument('-p', '--prj_path', metavar='PPATH', help='project local path')
    parser.add_argument('-b', '--gerrit_branch', metavar='BRANCH', help='gerrit branch name')
    parser.add_argument('-s', '--start', metavar='START', help='start sha1,tag', required=True)
    parser.add_argument('-e', '--end', metavar='END', help='stop sha1,tag', required=True)
    parser.add_argument('--ex_start', metavar='EX_START', help='exclude start sha1,tag')
    parser.add_argument('--ex_stop', metavar='EX_STOP', help='exclude stop sha1,tag')
    parser.add_argument('-c', '--cgi_link', help='view cgi bugzilla list', action='store_true')
    parser.add_argument('--env', help='tools env file (json)', type=argparse.FileType('r'))

    args = parser.parse_args(args)

    try:
        if args.env is None:
            args.env = open(TFM_STM_TOOLS_ENV, 'r')

        config = json.load(args.env)

        cfg_gerrit = config['gerrit']
        cfg_bzilla = config['bugzilla']

        gerrit_prj = config_get_project_by_name(cfg_gerrit['projects'], "tfm")
        bzilla_prj = config_get_project_by_name(cfg_bzilla['projects'], "tfm")

        auth_file = os.path.expanduser(config['auth']['file'])

    except Exception as exc:
        print("bzlist fail: {}".format(str(exc)))
        return

    if args.prj_path is None:
        args.prj_path = cwd_path

    if args.gerrit_branch is None:
        args.gerrit_branch = gerrit_prj['branches'][0]

    logging.debug("use {} git repositorie".format(args.prj_path))

    git_log="git log " + args.start + ".." + args.end
    log = check_output([git_log], shell=True, cwd=args.prj_path).decode('UTF-8').strip()
    ch_ids = parse_ch_id(log)

    logging.info("waiting gerrit id request...")
    logging.info("gerrit project: {} branch: {}".format(gerrit_prj['path'], args.gerrit_branch))

    gerrit = Gerrit(cfg_gerrit['host'], cfg_gerrit['interface'], auth_file, gerrit_prj['path'], args.gerrit_branch)
    gerrit_id = gerrit.get_gerrit_id(ch_ids)
    logging.debug("gerrit_id:{}".format(gerrit_id))

    logging.info("waiting bugzilla request...")
    bugtool = BugZilla(cfg_bzilla['host'], cfg_bzilla['interface'], auth_file)
    blist = bugtool.get_buglist(gerrit_id, bzilla_prj['sub_sys'], args.cgi_link)
    logging.debug("nb bz found:{}".format(len(blist.bugs)))

    return blist

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])
