#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# SPDX-License-Identifier: BSD-3-Clause
#
# Author: Ludovic.barre@st.com
# -----------------------------------------------------------------------------
import os
import re
import argparse
import urllib
import json
import pwinput
import click
import smtplib
import base64
from dataclasses import dataclass, field, asdict
from typing import Dict
from typing import List
from jinja2 import Environment, FileSystemLoader
from git import Repo
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart

cwd_path = os.path.abspath(os.getcwd())

AUTHOR = os.getenv('AUTHOR', 'Ludovic')

TFM_USER_GUIDE_PATH = os.getenv('TFM_USER_GUIDE_PATH', 'http://lmecxd0472.lme.st.com/trustedfirmware-m')
TFM_USER_GUIDE_EXTENSION = os.getenv('TFM_USER_GUIDE_EXTENSION', 'html')

RELEASES_DIR = os.getenv('RELEASES_DIR', cwd_path + '/releases')

TFM_STM_TOOLS_ENV = os.getenv('TFM_STM_TOOLS_ENV', cwd_path + '/tools_env.json')

@dataclass
class test:
    name: str
    desc: str
    res: str

@dataclass
class bz_info:
    name: str
    desc: str
    url: str

@dataclass
class tfm_link:
    name: str
    url: str

@dataclass
class tfm_release_data:
    name: str
    doc_path: str
    week: str = field(init = False)
    l_changelog: tfm_link = field(init = False)
    l_index: tfm_link = field(init = False)
    l_get_started: tfm_link = field(init = False)
    l_soc_readme: tfm_link = field(init = False)
    l_board_readme: tfm_link = field(init = False)
    l_tests: Dict[str, tfm_link] = field(default_factory=dict)
    list_bz: Dict[str, bz_info] = field(default_factory=dict)
    com_ver: str = field(init = False)
    repo: str = field(init = False)
    l_branch_dev: tfm_link = field(init = False)
    l_branch_int: tfm_link = field(init = False)
    l_tag: tfm_link = field(init = False)
    l_sha1: tfm_link = field(init = False)
    info: str = field(init = False)

def get_branches_matching_pattern(g_repo, commit, pattern):

    # Définir le motif de recherche pour les branches
    regex = re.compile(pattern)
    branches = []

    # Trouver toutes les branches contenant le commit et correspondant au motif
    for branch in g_repo.branches:
        if commit == branch.commit:
            if regex.search(branch.name):
                branches += [branch]

    return branches

def gerrit_web_link(gerrit_cfg, link_type, link):

    gerrit_prj = config_get_project_by_name(gerrit_cfg['projects'], "tfm")

    return ('https://%s/gitweb?p=%s.git;a=%s;h=%s' %
            (urllib.parse.quote_plus(gerrit_cfg['host']),
             urllib.parse.quote_plus(gerrit_prj['path']),
             urllib.parse.quote_plus(link_type),
             urllib.parse.quote_plus(link)))

def tfm_doc_link(path, section, extension):

    return ("{}{}.{}".format(path, section, extension))

def is_matching_tagged(g_repo, commit, pattern):
    tag_pattern = rf"tags/({pattern}[^\s]+)"
    matches = re.findall(tag_pattern, commit.name_rev)

    return matches

def fill_with_json(tfm_data, config, args):

    conf_release = config['release']
    conf_tag = conf_release['tag']
    conf_sha1 = conf_release['sha1']
    conf_b_dev = conf_release['branch_dev']
    conf_b_int = conf_release['branch_int']

    tfm_data.repo = conf_release['repo']

    tfm_data.l_tag = tfm_link(conf_tag['name'], conf_tag['url'])
    tfm_data.l_sha1 = tfm_link(conf_sha1['name'], conf_sha1['url'])
    tfm_data.l_branch_int = tfm_link(conf_b_int['name'], conf_b_int['url'])
    tfm_data.l_branch_dev = tfm_link(conf_b_dev['name'], conf_b_dev['url'])

    tfm_data.com_ver = conf_release['v_com']
    tfm_data.week = conf_release['week']

    return True

def release_is_available(tfm_data, config, args):
    #week pattern 'w' follow of 4 digits
    week_pattern = r'w\d{4}.*$'
    com_tag_pattern = r'TF-Mv'

    cfg_gerrit = config['gerrit']
    gerrit_tfm_prj = config_get_project_by_name(cfg_gerrit['projects'], "tfm")

    try:
        g_repo = Repo(os.getcwd(), search_parent_directories=True)

        tag = g_repo.tags[args.tag]
        commit = tag.commit
        branch_dev = g_repo.branches[args.dev]

        try:
            tfm_data.repo = g_repo.remotes.origin.url
        except:
            tfm_data.repo = "https://{}/{}".format(cfg_gerrit['host'], gerrit_tfm_prj['path'])

        branches = get_branches_matching_pattern(g_repo, commit, week_pattern)
        if len(branches) == 0:
            raise Exception('No such LOCAL week branch on commit tag')
        if len(branches) > 1:
            raise Exception('Too much LOCAL week branch on commit tag')

        branch = branches[0]
        week = (re.search(week_pattern, branch.name)).group()

        for item in commit.iter_parents():
            tags_name = is_matching_tagged(g_repo, item, com_tag_pattern)
            if tags_name:
                break

        if not tags_name:
            raise Exception('No such community tag version')

        tag_name = tags_name[0]

        tfm_data.l_tag = tfm_link(tag.name, gerrit_web_link(cfg_gerrit, 'tag', tag.path))
        tfm_data.l_sha1 = tfm_link(commit.hexsha, gerrit_web_link(cfg_gerrit, 'commit', commit.hexsha))
        tfm_data.l_branch_int = tfm_link(branch.name, gerrit_web_link(cfg_gerrit, 'shortlog', branch.path))
        tfm_data.l_branch_dev = tfm_link(branch_dev.name, gerrit_web_link(cfg_gerrit, 'shortlog', branch_dev.path))

        tfm_data.com_ver = tag_name
        tfm_data.week = week

    except Exception as exc:
        raise Exception(str(exc))

    return True

def get_user_guide_info(tfm_data, args):

    remote_doc_path = TFM_USER_GUIDE_PATH + '/' + args.tag + '/'
    doc_ext = TFM_USER_GUIDE_EXTENSION
    doc_stm = remote_doc_path + 'stm/'
    doc_stm_releases = doc_stm + 'releases/'
    doc_stm_release = doc_stm + tfm_data.doc_path + '/'

    tfm_data.l_changelog = tfm_link('changelog', tfm_doc_link(doc_stm_release, 'changelog', doc_ext))
    tfm_data.l_index = tfm_link('user_guide', tfm_doc_link(remote_doc_path, 'index', doc_ext))
    tfm_data.l_get_started = tfm_link('getting_started', tfm_doc_link(doc_stm_releases, 'getting_started', doc_ext))
    tfm_data.l_soc_readme = tfm_link('soc_readme', tfm_doc_link(remote_doc_path, 'platform/stm/common/stm32mp2xx/readme', doc_ext))
    tfm_data.l_board_readme = tfm_link('board_readme', tfm_doc_link(remote_doc_path, 'platform/stm/stm32mp257f_ev1/readme', doc_ext))

def extract_tables_from_rst(file_path):
    # extract description, name and url of rst file list-table
    # * - description
    #   - `name <link>`_

    with open(file_path, 'r', encoding='utf-8') as file:
        rst_content = file.read()

    bz_list = []
    pattern = re.compile(r'\* - (?P<DESC>.*?)\n\s+\ - `(?P<NAME>\d+) <(?P<URL>.*?)>`_')
    matches = pattern.findall(rst_content)

    for m in pattern.finditer(rst_content):
        bz_list += [bz_info(m.group('NAME'), m.group('DESC'), m.group('URL'))]

    return bz_list

def get_tests_info(tfm_data, args):
    remote_doc_path = TFM_USER_GUIDE_PATH + '/' + args.tag + '/'
    doc_ext = TFM_USER_GUIDE_EXTENSION
    doc_stm = remote_doc_path + 'stm/'
    doc_stm_releases = doc_stm + 'releases/'
    doc_stm_release = doc_stm + tfm_data.doc_path + '/'
    pattern = "_test.rst"

    for f in os.listdir(args.release):
        if f.endswith(pattern):
            filename = os.path.splitext(f)[0]
            name = f.replace(pattern, '')
            tfm_data.l_tests[filename] = tfm_link(name, tfm_doc_link(doc_stm_release, filename, doc_ext))

def get_bug_info(tfm_data, args):
    tfm_data.list_bz['fixed'] = extract_tables_from_rst(os.path.join(args.release, 'fixed.rst'))
    tfm_data.list_bz['issues'] = extract_tables_from_rst(os.path.join(args.release, 'issues.rst'))

def create_html(tfm_data, config):

    conf_msg = config['message']

    jinja_env = Environment(loader=FileSystemLoader(cwd_path + '/template/mail'))
    template = jinja_env.get_template('mail.jinja.html')

    release_data = {}
    release_data['tfm'] = tfm_data

    # We assume that the image file is in the same directory that you run your Python script from
    encoded = base64.b64encode(open(conf_msg['banner_img'], "rb").read()).decode()

    html_render = template.render(release_data, banner_img=encoded, sender=AUTHOR)

    # write the parsed template
    with open(conf_msg['html_file'], "w") as f:
        f.write(html_render)

    return html_render

def send_email(tfm_data, config, args):

    conf_mail = config['mail']
    conf_msg = config['message']

    with open(conf_msg['html_file'], 'r') as f:
        html_render = f.read()

    recipients = conf_mail['cc']

    message = MIMEMultipart("alternative")
    message["Subject"] = "[TF-M delivery] release mail " + tfm_data.name
    message["From"] = conf_mail['from']
    if args.mailing_list:
        message['To'] = ", ".join(conf_mail['to'])
        recipients += conf_mail['to']

    message['Cc'] = ", ".join(conf_mail['cc'])

    part = MIMEText(html_render, "html")
    message.attach(part)

    msg =  "The html mail is ready:\n"
    msg += " Subject: {}\n".format(message["Subject"])
    msg += " From: {}\n".format(message["From"])
    msg += " To {}\n".format(recipients)
    msg += " html file: {}\n".format(conf_msg['html_file'])
    msg += "Do you want to sent? "

    if not click.confirm(msg, default=True):
        print("mail aborted")
        return

    prompt = "{} password : ".format(conf_mail['from'])
    password = pwinput.pwinput(prompt=prompt)

    # send your email
    with smtplib.SMTP(conf_mail['smtp_server'], conf_mail['port']) as server:
        server.starttls()
        server.login(conf_mail['login'], password)
        server.sendmail(conf_mail['from'], recipients, message.as_string())

    print('mail sent')

# TODO:
# create python file with class to manage json env file:
# - open
# - get
# - get projet by name
def config_get_project_by_name(json_projects, project_name):

    for project in json_projects:
        if project['name'] == project_name:
            return project

    return None

def main():
    # arguments
    parser = argparse.ArgumentParser(description="Mail generator for stm tfm")
    parser.add_argument('-t', '--tag', metavar='tag NAME', help='tag name', required=True)
    parser.add_argument('-r', '--release', metavar='PATH', help='path to doc release', required=True)
    parser.add_argument('-d', '--dev', metavar='dev branch NAME', help='dev branch name')
    parser.add_argument('-s', '--send', help='send mail', action='store_true')
    parser.add_argument('-m', '--mailing-list', help='send at mailing list', action='store_true')
    parser.add_argument('-f', '--fill-with-json', help='use json to fill gerrit info', action='store_true')
    parser.add_argument('--env', help='tools env file', type=argparse.FileType('r'))

    args = parser.parse_args()

    try:

        if os.path.isdir(args.release) is False:
            raise Exception("No such path:{}".format(args.release))

        if args.env is None:
            args.env = open(TFM_STM_TOOLS_ENV, 'r')

        config = json.load(args.env)

        cfg_gerrit = config['gerrit']
        gerrit_prj = config_get_project_by_name(cfg_gerrit['projects'], "tfm")

        if not args.dev:
            args.dev = gerrit_prj['branches'][0]
            print("set default dev branch: {}".format(args.dev))

        tfm_data = tfm_release_data(name = args.tag, doc_path = args.release)

        if args.fill_with_json:
            fill_with_json(tfm_data, config, args)
        else:
            release_is_available(tfm_data, config, args)

        get_user_guide_info(tfm_data, args)
        get_tests_info(tfm_data, args)
        get_bug_info(tfm_data, args)

        html_render = create_html(tfm_data, config)

        if args.send:
            send_email(tfm_data, config, args)

    except Exception as exc:
        print("release mail fail: {}".format(str(exc)))

if __name__ == "__main__":
    # execute only if run as a script
    main()

